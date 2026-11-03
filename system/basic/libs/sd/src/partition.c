#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sd/gpt.h>
#include <sd/partition.h>

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

static uint8_t _sector_buf[SECTOR_SIZE];

static int32_t (*_read_sector)(int32_t sector, void* buf);

static inline int hw_read_data(uint8_t* dest, uint64_t offset, size_t size){
    uint64_t sector = offset/SECTOR_SIZE;
    uint32_t unalign = (uint32_t)(offset%SECTOR_SIZE);

    if(unalign){
        size_t read_len = SECTOR_SIZE - unalign;
        if(read_len > size)
            read_len = size;

        if(sector > 0x7fffffffULL ||
                _read_sector((int32_t)sector, _sector_buf) != SECTOR_SIZE)
            return -1;

        memcpy(dest, _sector_buf + unalign, read_len);
        dest += read_len;
        size -= read_len;
        sector++;
    }

    while(size/SECTOR_SIZE > 0){
        if(sector > 0x7fffffffULL ||
                _read_sector((int32_t)sector, dest) != SECTOR_SIZE)
            return -1;
        dest += SECTOR_SIZE;
        size -= SECTOR_SIZE;
        sector++;
    }

    uint32_t left = size%SECTOR_SIZE;

    if(left){
        if(sector > 0x7fffffffULL ||
                _read_sector((int32_t)sector, _sector_buf) != SECTOR_SIZE)
            return -1;
        memcpy(dest, _sector_buf, left);
    }

    return 0;
}

/* calculate size of entries array in bytes for specified number of entries */
static inline int gpt_calculate_sizeof_entries(
                struct gpt_header *hdr,
                uint32_t nents, size_t *sz)
{
    uint32_t esz = hdr ? le32_to_cpu(hdr->sizeof_partition_entry) :
                 sizeof(struct gpt_entry);

    if (nents == 0 || esz == 0 || SIZE_MAX/esz < nents) {
        printf("entries array size check failed");
        return -1;
    }

    *sz = (size_t) nents * esz;
    return 0;
}

static inline int gpt_sizeof_entries(struct gpt_header *hdr, size_t *sz)
{
    return gpt_calculate_sizeof_entries(hdr, le32_to_cpu(hdr->npartition_entries), sz);
}

static int valid_pmbr(uint8_t *firstsector)
{
    int i, ret = -1; /* invalid by default */
    struct gpt_legacy_mbr *pmbr = NULL;

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

static ssize_t read_lba(uint64_t lba,
            void *buffer, const size_t bytes)
{
    size_t sectors = (bytes + SECTOR_SIZE - 1) / SECTOR_SIZE;
    for(size_t i = 0; i < sectors; i++) {
        if(lba + i > 0x7fffffffULL ||
                _read_sector((int32_t)(lba + i),
                    (uint8_t*)buffer + i * SECTOR_SIZE) != SECTOR_SIZE)
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
    if (fu < GPT_PRIMARY_PARTITION_TABLE_LBA &&
        GPT_PRIMARY_PARTITION_TABLE_LBA < lu) {
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
    size_t sz = 0;

    unsigned char *ret = NULL;
    off_t offset;

    if (gpt_sizeof_entries(header, &sz))
        return NULL;

    ret = calloc(1, sz);
    if (!ret)
        return NULL;

    offset = (off_t) le64_to_cpu(header->partition_entry_lba) *
               SECTOR_SIZE;

    if (hw_read_data(ret, offset, sz) != 0) {
        free(ret);
        return NULL;
    }
    return ret;
}

static inline unsigned char *gpt_get_entry_ptr(struct gpt_label *gpt, size_t i)
{
    return gpt->ents + le32_to_cpu(gpt->pheader->sizeof_partition_entry) * i;
}

static inline struct gpt_entry *gpt_get_entry(struct gpt_label *gpt, size_t i)
{
    return (struct gpt_entry *) gpt_get_entry_ptr(gpt, i);
}

static struct gpt_header *gpt_read_header(uint64_t lba, unsigned char **_ents)
{
    struct gpt_header *header = NULL;
    unsigned char *ents = NULL;
    uint32_t hsz;

    header = calloc(1, SECTOR_SIZE);
    if (!header)
        return NULL;

    /* read and verify header */
    if (read_lba(lba, header, SECTOR_SIZE) != 0)
        goto invalid;

    if (!gpt_check_signature(header))
        goto invalid;

    /* make sure header size is between 92 and sector size bytes */
    hsz = le32_to_cpu(header->size);
    if (hsz < GPT_HEADER_MINSZ || hsz > SECTOR_SIZE)
        goto invalid;

    if (le64_to_cpu(header->my_lba) != lba ||
            le64_to_cpu(header->first_usable_lba) >
                le64_to_cpu(header->last_usable_lba) ||
            le64_to_cpu(header->last_usable_lba) >=
                le64_to_cpu(header->alternative_lba) ||
            le32_to_cpu(header->npartition_entries) == 0 ||
            le32_to_cpu(header->npartition_entries) > 4096 ||
            le32_to_cpu(header->sizeof_partition_entry) < sizeof(struct gpt_entry) ||
            le32_to_cpu(header->sizeof_partition_entry) > 4096)
        goto invalid;

    //if (!gpt_check_header_crc(header, NULL))
    //    goto invalid;

    /* read and verify entries */
    ents = gpt_read_entries(header);
    if (!ents)
        goto invalid;

    //if (!gpt_check_entryarr_crc(header, ents))
    //    goto invalid;

    if (!gpt_check_lba_sanity(header))
        goto invalid;

    if (_ents)
        *_ents = ents;
    else
        free(ents);

    return header;
invalid:
    free(header);
    free(ents);

    printf("read header on LBA %llu failed\n",
            (unsigned long long)lba);
    return NULL;
}

static int guid_is_rootfs(const struct gpt_guid* guid) {
    static const struct gpt_guid linux_root_x86_64 =
        {0x4f68bce3, 0xe8cd, 0x4db1, 0x96, 0xe7,
            {0xfb, 0xca, 0xf9, 0x84, 0xb7, 0x09}};
    static const struct gpt_guid linux_root_arm64 =
        {0xb921b045, 0x1df0, 0x41c3, 0xaf, 0x44,
            {0x4c, 0x6f, 0x28, 0x0d, 0x3f, 0xae}};
    static const struct gpt_guid linux_root_generic =
        {0x44479540, 0xf297, 0x41b2, 0x9a, 0xf7,
            {0xd1, 0x31, 0xd5, 0xf0, 0x45, 0x8a}};
    static const struct gpt_guid linux_filesystem = GPT_ROOTFS_ENTRY_TYPE;
    return memcmp(guid, &linux_filesystem, sizeof(*guid)) == 0 ||
        memcmp(guid, &linux_root_x86_64, sizeof(*guid)) == 0 ||
        memcmp(guid, &linux_root_arm64, sizeof(*guid)) == 0 ||
        memcmp(guid, &linux_root_generic, sizeof(*guid)) == 0;
}

static uint32_t mbr_rootfs_start(const uint8_t* mbr) {
    const struct gpt_record* records =
        (const struct gpt_record*)(mbr + 0x1be);
    uint32_t rootfs_start = 0;
    for(int i = 0; i < 4; i++) {
        /* same selection as the kernel-side MBR fallback
         * (kernel/lib/ext2/src/partition.c): take the last partition
         * with a nonzero start sector, without requiring os_type 0x83.
         * On cards prepared by third-party tools the rootfs partition
         * often carries a non-0x83 type (e.g. 0x0C/0x0E/0x07), which the
         * old 0x83-only check rejected, leaving start_sector = 0 and
         * making the ext3 superblock read land at the raw disk offset. */
        if(records[i].starting_lba > 0)
            rootfs_start = records[i].starting_lba;
    }
    return rootfs_start;
}

uint32_t get_rootfs_entry(int32_t (*read_sector)(int32_t sector, void* buf)){
    uint32_t rootfs_start = 0;
    uint8_t *mbr = (uint8_t*)calloc(1, SECTOR_SIZE);
    if(mbr == NULL || read_sector == NULL)
        goto out;
    _read_sector = read_sector;
    if(_read_sector(0, mbr) != SECTOR_SIZE)
        goto out;

    int mbr_type = valid_pmbr(mbr);
    if(mbr_type == GPT_MBR_PROTECTIVE || mbr_type == GPT_MBR_HYBRID) {
        struct gpt_label gpt;
        gpt.pheader = gpt_read_header(GPT_PRIMARY_PARTITION_TABLE_LBA, &gpt.ents);
        if(gpt.pheader) {
            uint32_t n = le32_to_cpu(gpt.pheader->npartition_entries);
            for(uint32_t i = 0; i < n; i++) {
                struct gpt_entry *e = gpt_get_entry(&gpt, i);
                struct gpt_guid type;
                memcpy(&type, &e->type, sizeof(type));
                if(guid_is_rootfs(&type) &&
                        gpt_partition_start(e) <= gpt_partition_end(e) &&
                        gpt_partition_start(e) <= 0xffffffffULL) {
                    rootfs_start = (uint32_t)gpt_partition_start(e);
                    break;
                }
            }
            free(gpt.ents);
            free(gpt.pheader);
        }
    }
    if(rootfs_start == 0 && mbr_type == 0)
        rootfs_start = mbr_rootfs_start(mbr);
out:
    free(mbr);
    return rootfs_start;
}
