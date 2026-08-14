#ifndef FAT32_FS_H
#define FAT32_FS_H

#include <fat32/fat32head.h>

int32_t fat32_init(fat32_t* fat, fat32_read_sector_func_t read_sector, fat32_write_sector_func_t write_sector);
int32_t fat32_init_ex(fat32_t* fat, fat32_read_sector_func_t read_sector, fat32_read_sectors_func_t read_sectors, fat32_write_sector_func_t write_sector);

void    fat32_quit(fat32_t* fat);

int32_t fat32_flush(fat32_t* fat);

void    fat32_root_node(fat32_t* fat, fat32_node_t* node);

int32_t fat32_node_by_fname(fat32_t* fat, const char* fname, fat32_node_t* node);

int32_t fat32_read(fat32_t* fat, fat32_node_t* node, char* buf, int32_t nbytes, int32_t offset);

int32_t fat32_write(fat32_t* fat, fat32_node_t* node, const char* data, int32_t nbytes, int32_t offset);

int32_t fat32_truncate(fat32_t* fat, fat32_node_t* node);

/* write node metadata (size/cluster/time/attr) back to its directory entry,
 * the counterpart of ext2's put_node */
int32_t fat32_update_node(fat32_t* fat, fat32_node_t* node);

int32_t fat32_create_dir(fat32_t* fat, fat32_node_t* dir_node, const char* name, fat32_node_t* out);

int32_t fat32_create_file(fat32_t* fat, fat32_node_t* dir_node, const char* name, fat32_node_t* out);

int32_t fat32_unlink(fat32_t* fat, const char* fname);

int32_t fat32_rmdir(fat32_t* fat, const char* fname);

/* directory iteration ("."/".." and volume labels are skipped) */
int32_t fat32_diropen(fat32_t* fat, fat32_node_t* dir_node, fat32_dir_t* it);
int32_t fat32_dirnext(fat32_t* fat, fat32_dir_t* it, fat32_node_t* out); /* 1:got, 0:end, -1:error */
void    fat32_dirclose(fat32_dir_t* it);

void*   fat32_readfile(fat32_t* fat, const char* fname, int32_t* size);

/* FAT date/time <-> unix time */
uint32_t fat32_dt2unix(uint16_t fdate, uint16_t ftime);
void     fat32_unix2dt(uint32_t utime, uint16_t* fdate, uint16_t* ftime);

#endif
