#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sd/gpt.h>
#include <sd/partition.h>

static uint8_t _sector_buf[SECTOR_SIZE];

static int32_t (*_read_sector)(int32_t sector, void* buf);

static inline int hw_read_data(uint8_t* dest, uint32_t offset,  int size){
	uint32_t sector = offset/SECTOR_SIZE;
	uint32_t unalign = offset%SECTOR_SIZE;

	if(unalign){
		uint32_t read_len = SECTOR_SIZE - unalign;
		if(read_len > size)
			read_len = size;

		_read_sector(sector, _sector_buf);

		memcpy(dest, _sector_buf + unalign, read_len);
		dest += read_len;
		size -= read_len;
		sector++;
	}

	while(size/SECTOR_SIZE > 0){
		_read_sector(sector, dest);
		dest += SECTOR_SIZE;
		size -= SECTOR_SIZE;
		sector++;
	}

	uint32_t left = size%SECTOR_SIZE;

	if(left){
		_read_sector(sector, _sector_buf);
		memcpy(dest, _sector_buf, left);
	}

	return size;
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

static uint64_t gpt_partition_size(const struct gpt_entry *e)
{
    uint64_t start = gpt_partition_start(e);
    uint64_t end = gpt_partition_end(e);

    return start > end ? 0 : end - start + 1ULL;
}

static int valid_pmbr(uint8_t *firstsector)
{
    int i, part = 0, ret = -1; /* invalid by default */
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
            part = i;
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
	for(int i = 0 ; i < (bytes + SECTOR_SIZE - 1)/SECTOR_SIZE; i++)
	_read_sector(lba+i, (uint8_t*)buffer + i * SECTOR_SIZE);
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
    ssize_t ssz;

    unsigned char *ret = NULL;
    off_t offset;

    if (gpt_sizeof_entries(header, &sz))
        return NULL;

    ret = calloc(1, sz);
    if (!ret)
        return NULL;

    offset = (off_t) le64_to_cpu(header->partition_entry_lba) *
               SECTOR_SIZE;

	hw_read_data(ret, offset, sz);
    return ret;
}

static inline size_t gpt_get_nentries(struct gpt_label *gpt)
{
    return (size_t) le32_to_cpu(gpt->pheader->npartition_entries);
}

static inline unsigned char *gpt_get_entry_ptr(struct gpt_label *gpt, size_t i)
{
    return gpt->ents + le32_to_cpu(gpt->pheader->sizeof_partition_entry) * i;
}

static inline struct gpt_entry *gpt_get_entry(struct gpt_label *gpt, size_t i)
{
    return (struct gpt_entry *) gpt_get_entry_ptr(gpt, i);
}

static inline int gpt_entry_is_used(const struct gpt_entry *e)
{
    return memcmp(&e->type, &GPT_UNUSED_ENTRY_GUID,
            sizeof(struct gpt_guid)) != 0;
}

static size_t partitions_in_use(struct gpt_label *gpt)
{
    size_t i, used = 0;

    for (i = 0; i < gpt_get_nentries(gpt); i++) {
        struct gpt_entry *e = gpt_get_entry(gpt, i);

        if (gpt_entry_is_used(e))
            used++;
    }
    return used;
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

    /* valid header must be at MyLBA */
    if (le64_to_cpu(header->my_lba) != lba)
        goto invalid;

    if (_ents)
        *_ents = ents;
    else
        free(ents);

    return header;
invalid:
    free(header);
    free(ents);

    printf("read header on LBA %d failed\n", lba);
    return NULL;
}

uint32_t get_rootfs_entry(int32_t (*read_sector)(int32_t sector, void* buf)){
	uint32_t rootfs_start = 0;
	uint8_t *mbr = (uint8_t*)malloc(SECTOR_SIZE);
	_read_sector = read_sector;
	_read_sector(0, mbr);

	int mbr_type = valid_pmbr(mbr);
	if(mbr_type == 0){
    	partition_t* p = (partition_t*)(mbr + 0x1BE);
    	for(int32_t i=0; i < 4; i++) {
			if(p->start_sector > 0)
				rootfs_start = p->start_sector;
			p++;
    	}
	}else if(mbr_type == GPT_MBR_PROTECTIVE){
		struct gpt_label gpt;
		gpt.pheader = gpt_read_header(GPT_PRIMARY_PARTITION_TABLE_LBA, &gpt.ents);
        if(gpt.pheader){
            int n = partitions_in_use(&gpt);
            for(int i = 0; i < n; i++){
                struct gpt_entry *e = gpt_get_entry(&gpt, i);
                if(memcmp(e, &GPT_ROOTFS_ENTRY_TYPE, sizeof(struct gpt_guid)) == 0){
                    rootfs_start = gpt_partition_start(e);
                }
            }
            free(gpt.ents);
            free(gpt.pheader);
        }
	}
	free(mbr);
	return rootfs_start;
}
