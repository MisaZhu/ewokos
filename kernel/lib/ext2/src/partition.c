#include <stdint.h>
#include <kstring.h>
#include <mm/kmalloc.h>
#include "gpt.h"
#include "partition.h"

#define GPT_MAX_ENTRY_ARRAY_SIZE (1024U * 1024U)
#define EXT_SUPER_MAGIC 0xef53U
#define EXT_FEATURE_INCOMPAT_FILETYPE 0x2U
#define EXT_FEATURE_INCOMPAT_RECOVER 0x4U
#define EXT_FEATURE_RO_COMPAT_SUPPORTED 0x3U

static int32_t (*_read_sector)(int32_t sector, void* buf);

/* calculate size of entries array in bytes for specified number of entries */
static inline int gpt_calculate_sizeof_entries(
                struct gpt_header *hdr,
                uint32_t nents, uint32_t *sz)
{
    uint32_t esz = hdr ? le32_to_cpu(hdr->sizeof_partition_entry) :
                 sizeof(struct gpt_entry);

    if (nents == 0 || esz < sizeof(struct gpt_entry) ||
            UINT32_MAX/esz < nents) {
        printf("entries array size check failed");
        return -1;
    }

    *sz = (uint32_t) nents * esz;
    return 0;
}

static inline int gpt_sizeof_entries(struct gpt_header *hdr, uint32_t *sz)
{
    return gpt_calculate_sizeof_entries(hdr, le32_to_cpu(hdr->npartition_entries), sz);
}

static uint64_t gpt_partition_size(const struct gpt_entry *e)
{
    uint64_t start = gpt_partition_start(e);
    uint64_t end = gpt_partition_end(e);

    return start > end ? 0 : end - start + 1ULL;
}

static int valid_pmbr(uint8_t *firstsector)
{
    int i, ret = -1; /* invalid by default */
    struct gpt_legacy_mbr *pmbr = 0;

    if (!firstsector)
        goto done;

    pmbr = (struct gpt_legacy_mbr *) firstsector;

    if (le16_to_cpu(pmbr->signature) != MSDOS_MBR_SIGNATURE)
        goto done;

    ret = 0;
    /* seems like a valid MBR was found, check DOS primary partitions */
    for (i = 0; i < 4; i++) {
        if (pmbr->partition_record[i].os_type == EFI_PMBR_OSTYPE) {
            /*
             * Ok, we at least know that there's a protective MBR,
             * now check if there are other partition types for
             * hybrid MBR.
             */
            ret = GPT_MBR_PROTECTIVE;
            break;
        }
    }

    if (ret != GPT_MBR_PROTECTIVE)
        goto done;


    for (i = 0 ; i < 4; i++) {
        if ((pmbr->partition_record[i].os_type != EFI_PMBR_OSTYPE) &&
            (pmbr->partition_record[i].os_type != 0x00)) {
            ret = GPT_MBR_HYBRID;
            goto done;
        }
    }
done:
    return ret;
}

static int32_t read_lba(uint64_t lba,
            void *buffer, const uint32_t bytes)
{
    uint32_t sectors = (bytes + SECTOR_SIZE - 1U) / SECTOR_SIZE;

    if (lba > 0x7fffffffULL || (sectors != 0 &&
                sectors - 1U > 0x7fffffffU - (uint32_t)lba))
        return -1;
    for (uint32_t i = 0; i < sectors; ++i) {
        if (_read_sector((int32_t)(lba + i),
                    (uint8_t*)buffer + i * SECTOR_SIZE) != 0)
            return -1;
    }
    return 0;
}

/* Check if there is a valid header signature */
static int gpt_check_signature(struct gpt_header *header)
{
    return header->signature == cpu_to_le64(GPT_HEADER_SIGNATURE);
}

static int gpt_check_lba_sanity(struct gpt_header *header)
{
    int ret = 0;
    uint64_t lu, fu;

    fu = le64_to_cpu(header->first_usable_lba);
    lu = le64_to_cpu(header->last_usable_lba);

    /* check if first and last usable LBA make sense */
    if (lu < fu) {
        printf("error: header last LBA is before first LBA\n");
        goto done;
    }

    /* the header has to be outside usable range */
    if (GPT_PRIMARY_PARTITION_TABLE_LBA >= fu &&
        GPT_PRIMARY_PARTITION_TABLE_LBA <= lu) {
        printf("error: header outside of usable range\n");
        goto done;
    }

    ret = 1; /* sane */
done:
    return ret;
}

/* Returns the GPT entry array */
static unsigned char *gpt_read_entries(struct gpt_header *header)
{
    uint32_t sz = 0;
    unsigned char *ret = 0;
    uint32_t alloc_sz;
    uint64_t lba;

    if (gpt_sizeof_entries(header, &sz))
        return 0;
    if (sz > GPT_MAX_ENTRY_ARRAY_SIZE)
        return 0;

    alloc_sz = (sz + SECTOR_SIZE - 1U) & ~(SECTOR_SIZE - 1U);
    if (alloc_sz < sz)
        return 0;

    ret = kcalloc(1, alloc_sz);
    if (!ret)
        return 0;

    lba = le64_to_cpu(header->partition_entry_lba);
    if (read_lba(lba, ret, alloc_sz) != 0)
        goto fail;
    return ret;

fail:
    kfree(ret);
    return 0;
}

static inline uint32_t gpt_get_nentries(struct gpt_label *gpt)
{
    return (uint32_t) le32_to_cpu(gpt->pheader->npartition_entries);
}

static inline unsigned char *gpt_get_entry_ptr(struct gpt_label *gpt, uint32_t i)
{
    return gpt->ents + le32_to_cpu(gpt->pheader->sizeof_partition_entry) * i;
}

static inline struct gpt_entry *gpt_get_entry(struct gpt_label *gpt, uint32_t i)
{
    return (struct gpt_entry *) gpt_get_entry_ptr(gpt, i);
}

static inline int gpt_entry_is_used(const struct gpt_entry *e)
{
    return memcmp((void*)&e->type, &GPT_UNUSED_ENTRY_GUID,
            sizeof(struct gpt_guid)) != 0;
}

static uint32_t get_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
            ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int gpt_entry_is_supported_ext(const struct gpt_entry *e)
{
    uint8_t sector[SECTOR_SIZE];
    uint64_t start = gpt_partition_start(e);
    uint64_t end = gpt_partition_end(e);
    uint32_t log_block_size;
    uint32_t feature_incompat;
    uint32_t feature_ro_compat;
    uint16_t inode_size;
    uint16_t magic;

    if (start > end || end - start < 2U || start > 0x7ffffffdULL)
        return 0;
    if (_read_sector((int32_t)(start + 2U), sector) != 0)
        return 0;

    log_block_size = get_le32(sector + 24);
    magic = (uint16_t)sector[56] | ((uint16_t)sector[57] << 8);
    inode_size = (uint16_t)sector[88] | ((uint16_t)sector[89] << 8);
    feature_incompat = get_le32(sector + 96);
    feature_ro_compat = get_le32(sector + 100);

    return magic == EXT_SUPER_MAGIC && log_block_size <= 2U &&
            inode_size >= 128U &&
            /* The kernel ext3 reader accepts RECOVER read-only.  Keep the
             * partition selector consistent so an uncleanly unmounted SD
             * rootfs is still discoverable at the next boot. */
            (feature_incompat &
                ~(EXT_FEATURE_INCOMPAT_FILETYPE |
                  EXT_FEATURE_INCOMPAT_RECOVER)) == 0 &&
            (feature_ro_compat & ~EXT_FEATURE_RO_COMPAT_SUPPORTED) == 0;
}

static struct gpt_header *gpt_read_header(uint64_t lba, unsigned char **_ents)
{
    struct gpt_header *header = 0;
    unsigned char *ents = 0;
    uint32_t hsz;

    header = kcalloc(1, SECTOR_SIZE);
    if (!header)
        return 0;

    /* read and verify header */
    if (read_lba(lba, header, SECTOR_SIZE) != 0)
        goto invalid;

    if (!gpt_check_signature(header))
        goto invalid;

    /* make sure header size is between 92 and sector size bytes */
    hsz = le32_to_cpu(header->size);
    if (hsz < GPT_HEADER_MINSZ || hsz > SECTOR_SIZE)
        goto invalid;

    //if (!gpt_check_header_crc(header, 0))
    //    goto invalid;

    /* read and verify entries */
    ents = gpt_read_entries(header);
    if (!ents)
        goto invalid;

    //if (!gpt_check_entryarr_crc(header, ents))
    //    goto invalid;

    if (!gpt_check_lba_sanity(header))
        goto invalid;

    /* valid header must be at MyLBA */
    if (le64_to_cpu(header->my_lba) != lba)
        goto invalid;

    if (_ents)
        *_ents = ents;
    else
        kfree(ents);

    return header;
invalid:
    kfree(header);
    kfree(ents);

    printf("read header on LBA %llu failed\n", (unsigned long long)lba);
    return 0;
}

uint32_t get_rootfs_entry(int32_t (*read_sector)(int32_t sector, void* buf)){
    uint32_t rootfs_start = 0;
    uint8_t *mbr;
    if (read_sector == 0)
        return 0;
    mbr = (uint8_t*)kmalloc(SECTOR_SIZE);
    if (mbr == 0)
        return 0;
    _read_sector = read_sector;
    if (_read_sector(0, mbr) != 0)
        goto done;

    int mbr_type = valid_pmbr(mbr);
    if(mbr_type == 0){
        partition_t* p = (partition_t*)(mbr + 0x1BE);
        for(int32_t i=0; i < 4; i++) {
            if(p->start_sector > 0)
                rootfs_start = p->start_sector;
            p++;
        }
    }else if(mbr_type == GPT_MBR_PROTECTIVE){
        struct gpt_label gpt = {0};
        gpt.pheader = gpt_read_header(GPT_PRIMARY_PARTITION_TABLE_LBA, &gpt.ents);
        if (gpt.pheader == 0 || gpt.ents == 0)
            goto done;

        uint32_t n = gpt_get_nentries(&gpt);
        for(uint32_t i = 0; i < n; i++){
            struct gpt_entry *e = gpt_get_entry(&gpt, i);
            uint64_t start = gpt_partition_start(e);
            if(gpt_entry_is_used(e) &&
                    memcmp(e, &GPT_ROOTFS_ENTRY_TYPE,
                        sizeof(struct gpt_guid)) == 0 &&
                    gpt_partition_size(e) != 0 &&
                    start <= 0x7fffffffULL &&
                    gpt_entry_is_supported_ext(e)){
                rootfs_start = (uint32_t)start;
                break;
            }
        }
        kfree(gpt.ents);
        kfree(gpt.pheader);
    }
done:
    kfree(mbr);
    return rootfs_start;
}
