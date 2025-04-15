#ifndef __GPT_TABLE_H__
#define __GPT_TABLE_H__

#define GPT_HEADER_SIGNATURE 0x5452415020494645LL /* EFI PART */
#define GPT_HEADER_REVISION_V1_02 0x00010200
#define GPT_HEADER_REVISION_V1_00 0x00010000
#define GPT_HEADER_REVISION_V0_99 0x00009900
#define GPT_HEADER_MINSZ          92 /* bytes */

#define GPT_PMBR_LBA        0
#define GPT_MBR_PROTECTIVE  1
#define GPT_MBR_HYBRID      2

#define GPT_PRIMARY_PARTITION_TABLE_LBA 0x00000001ULL

#define MSDOS_MBR_SIGNATURE 0xAA55
#define EFI_PMBR_OSTYPE     0xEE
#define GPT_PART_NAME_LEN   (72 / sizeof(uint16_t))
#define GPT_NPARTITIONS     ((size_t) FDISK_GPT_NPARTITIONS_DEFAULT)

#define le16_to_cpu(x)		(x)
#define le32_to_cpu(x)		(x)
#define le64_to_cpu(x)		(x)
#define cpu_to_le16(x)		(x)
#define cpu_to_le32(x)		(x)
#define cpu_to_le64(x)		(x)

#define SECTOR_SIZE	512

/* only checking that the GUID is 0 is enough to verify an empty partition. */
#define GPT_UNUSED_ENTRY_GUID	\
    ((struct gpt_guid) {0x00000000, 0x0000, 0x0000, 0x00, 0x00, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}})

/* Linux native partition type */
#define GPT_ROOTFS_ENTRY_TYPE   \
    ((struct gpt_guid) {0x0FC63DAF, 0x8483, 0x4772, 0x8E, 0x79, {0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4}})

enum {
    /* UEFI specific */
    GPT_ATTRBIT_REQ      = 0,
    GPT_ATTRBIT_NOBLOCK  = 1,
    GPT_ATTRBIT_LEGACY   = 2,

    /* GUID specific (range 48..64)*/
    GPT_ATTRBIT_GUID_FIRST  = 48,
    GPT_ATTRBIT_GUID_COUNT  = 16
};

/* Globally unique identifier */
struct gpt_guid {
    uint32_t   time_low;
    uint16_t   time_mid;
    uint16_t   time_hi_and_version;
    uint8_t    clock_seq_hi;
    uint8_t    clock_seq_low;
    uint8_t    node[6];
};

struct gpt_entry {
    struct gpt_guid     type; /* purpose and type of the partition */
    struct gpt_guid     partition_guid;
    uint64_t            lba_start;
    uint64_t            lba_end;
    uint64_t            attrs;
    uint16_t            name[GPT_PART_NAME_LEN];
}  __attribute__ ((packed));

/* GPT header */
struct gpt_header {
    uint64_t            signature; /* header identification */
    uint32_t            revision; /* header version */
    uint32_t            size; /* in bytes */
    uint32_t            crc32; /* header CRC checksum */
    uint32_t            reserved1; /* must be 0 */
    uint64_t            my_lba; /* LBA of block that contains this struct (LBA 1) */
    uint64_t            alternative_lba; /* backup GPT header */
    uint64_t            first_usable_lba; /* first usable logical block for partitions */
    uint64_t            last_usable_lba; /* last usable logical block for partitions */
    struct gpt_guid     disk_guid; /* unique disk identifier */
    uint64_t            partition_entry_lba; /* LBA of start of partition entries array */
    uint32_t            npartition_entries; /* total partition entries - normally 128 */
    uint32_t            sizeof_partition_entry; /* bytes for each GUID pt */
    uint32_t            partition_entry_array_crc32; /* partition CRC checksum */
    uint8_t             reserved2[512 - 92]; /* must all be 0 */
} __attribute__ ((packed));

struct gpt_record {
    uint8_t             boot_indicator; /* unused by EFI, set to 0x80 for bootable */
    uint8_t             start_head; /* unused by EFI, pt start in CHS */
    uint8_t             start_sector; /* unused by EFI, pt start in CHS */
    uint8_t             start_track;
    uint8_t             os_type; /* EFI and legacy non-EFI OS types */
    uint8_t             end_head; /* unused by EFI, pt end in CHS */
    uint8_t             end_sector; /* unused by EFI, pt end in CHS */
    uint8_t             end_track; /* unused by EFI, pt end in CHS */
    uint32_t            starting_lba; /* used by EFI - start addr of the on disk pt */
    uint32_t            size_in_lba; /* used by EFI - size of pt in LBA */
} __attribute__ ((packed));

/* Protected MBR and legacy MBR share same structure */
struct gpt_legacy_mbr {
    uint8_t             boot_code[440];
    uint32_t            unique_mbr_signature;
    uint16_t            unknown;
    struct gpt_record   partition_record[4];
    uint16_t            signature;
} __attribute__ ((packed));


struct gpt_label {
    struct gpt_header   *pheader;   /* primary header */
    struct gpt_header   *bheader;   /* backup header */
    unsigned char *ents;            /* entries (partitions) */
};

/* gpt_entry macros */
#define gpt_partition_start(_e)     le64_to_cpu((_e)->lba_start)
#define gpt_partition_end(_e)       le64_to_cpu((_e)->lba_end)

#endif
