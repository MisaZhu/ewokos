#ifndef FAT32_FS_HEAD_H
#define FAT32_FS_HEAD_H

#include <stdint.h>

#define FAT32_SECTOR_SIZE     512
#define FAT32_DIRENT_SIZE     32
#define FAT32_NAME_MAX        256

/* directory entry attributes */
#define FAT32_ATTR_READ_ONLY  0x01
#define FAT32_ATTR_HIDDEN     0x02
#define FAT32_ATTR_SYSTEM     0x04
#define FAT32_ATTR_VOLUME_ID  0x08
#define FAT32_ATTR_DIRECTORY  0x10
#define FAT32_ATTR_ARCHIVE    0x20
#define FAT32_ATTR_LONG_NAME  0x0F
#define FAT32_ATTR_LONG_NAME_MASK 0x3F

/* FAT entry values (28 valid bits) */
#define FAT32_ENTRY_MASK      0x0FFFFFFF
#define FAT32_ENTRY_FREE      0x00000000
#define FAT32_ENTRY_BAD       0x0FFFFFF7
#define FAT32_ENTRY_EOC       0x0FFFFFFF
#define FAT32_ENTRY_EOC_MIN   0x0FFFFFF8

/* dirent name[0] markers */
#define FAT32_DIRENT_END      0x00
#define FAT32_DIRENT_FREE     0xE5

#define FAT32_LFN_LAST_FLAG   0x40
#define FAT32_LFN_CHARS       13

/* boot sector / BIOS parameter block (FAT32 layout) */
typedef struct {
	uint8_t   jmp[3];
	char      oem[8];
	uint16_t  bytes_per_sector;
	uint8_t   sectors_per_cluster;
	uint16_t  reserved_sector_count;
	uint8_t   num_fats;
	uint16_t  root_entry_count;   /* 0 for FAT32 */
	uint16_t  total_sectors16;    /* 0 for FAT32 */
	uint8_t   media;
	uint16_t  fat_size16;         /* 0 for FAT32 */
	uint16_t  sectors_per_track;
	uint16_t  num_heads;
	uint32_t  hidden_sectors;
	uint32_t  total_sectors32;
	uint32_t  fat_size32;         /* sectors per FAT */
	uint16_t  ext_flags;
	uint16_t  fs_version;
	uint32_t  root_cluster;
	uint16_t  fs_info;
	uint16_t  backup_boot_sector;
	uint8_t   reserved[12];
	uint8_t   drive_number;
	uint8_t   reserved1;
	uint8_t   boot_signature;
	uint32_t  volume_id;
	char      volume_label[11];
	char      fs_type[8];
} __attribute__((packed)) fat32_bpb_t;

/* short (8.3) directory entry */
typedef struct {
	uint8_t   name[11];
	uint8_t   attr;
	uint8_t   nt_res;             /* 0x08:base lowercase, 0x10:ext lowercase */
	uint8_t   crt_time_tenth;
	uint16_t  crt_time;
	uint16_t  crt_date;
	uint16_t  lst_acc_date;
	uint16_t  fst_clus_hi;
	uint16_t  wrt_time;
	uint16_t  wrt_date;
	uint16_t  fst_clus_lo;
	uint32_t  file_size;
} __attribute__((packed)) fat32_dirent_t;

/* long file name directory entry */
typedef struct {
	uint8_t   order;
	uint16_t  name1[5];
	uint8_t   attr;               /* always FAT32_ATTR_LONG_NAME */
	uint8_t   type;
	uint8_t   checksum;
	uint16_t  name2[6];
	uint16_t  fst_clus_lo;        /* always 0 */
	uint16_t  name3[2];
} __attribute__((packed)) fat32_lfn_t;

typedef int32_t (*fat32_read_sector_func_t)(int32_t sector, void* buf);
typedef int32_t (*fat32_read_sectors_func_t)(int32_t sector, void* buf, uint32_t count);
typedef int32_t (*fat32_write_sector_func_t)(int32_t sector, const void* buf);

typedef struct {
	fat32_bpb_t bpb;
	uint32_t fat_start_sector;    /* partition-relative */
	uint32_t data_start_sector;   /* partition-relative */
	uint32_t bytes_per_cluster;
	uint32_t total_clusters;
	uint32_t free_hint;           /* next-free-cluster scan hint */

	fat32_read_sector_func_t read_sector;
	fat32_read_sectors_func_t read_sectors;
	fat32_write_sector_func_t write_sector;

	/* one-sector FAT cache */
	uint8_t* fat_cache;
	int32_t  fat_cache_sector;    /* FAT-relative sector index, -1: invalid */
	int32_t  fat_cache_dirty;
} fat32_t;

/* the in-memory node, plays the role ext2's INODE does.
 * FAT has no inode table so the location of the short directory
 * entry is kept here for metadata write-back. */
typedef struct {
	uint32_t start_cluster;       /* 0 for empty file */
	uint32_t size;
	uint8_t  attr;
	uint16_t crt_date;
	uint16_t crt_time;
	uint16_t wrt_date;
	uint16_t wrt_time;

	uint32_t dirent_dir_cluster;  /* first cluster of parent dir, 0 for root */
	uint32_t dirent_index;        /* linear index of the short entry in parent */
	uint32_t dirent_lfn_num;      /* LFN entries right before the short entry */

	char     name[FAT32_NAME_MAX];/* utf-8 */
} fat32_node_t;

/* directory iterator */
typedef struct {
	uint32_t first_cluster;
	uint32_t cur_cluster;
	uint32_t cluster_seq;
	uint32_t entry_in_cluster;
	uint8_t* cbuf;
	int32_t  cbuf_valid;
} fat32_dir_t;

static inline uint32_t fat32_cluster_to_sector(const fat32_t* fat, uint32_t cluster) {
	return fat->data_start_sector + (cluster - 2) * fat->bpb.sectors_per_cluster;
}

static inline uint32_t fat32_dirents_per_cluster(const fat32_t* fat) {
	return fat->bytes_per_cluster / FAT32_DIRENT_SIZE;
}

static inline int32_t fat32_is_eoc(uint32_t entry) {
	return (entry & FAT32_ENTRY_MASK) >= FAT32_ENTRY_EOC_MIN;
}

static inline uint32_t fat32_dirent_cluster(const fat32_dirent_t* e) {
	return ((uint32_t)e->fst_clus_hi << 16) | e->fst_clus_lo;
}

#endif
