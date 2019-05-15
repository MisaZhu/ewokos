#ifndef KFILE_H
#define KFILE_H

#include <types.h>
#include <fsinfo.h>

typedef struct kfile {
	fs_info_t node_info;
	uint16_t ref_r;
	uint16_t ref_w;
} kfile_t;

void kf_init(void);
int32_t kf_open(int32_t pid, fs_info_t* info, int32_t wr);
void kf_close(int32_t pid, int32_t fd);
int32_t kf_dup2(int32_t old_fd, int32_t new_fd);
void kf_unref(kfile_t* kf, uint32_t wr);
void kf_ref(kfile_t* kf, uint32_t wr);
int32_t kf_get_ref(uint32_t node_addr, uint32_t wr);
int32_t kf_node_info_by_fd(int32_t pid, int32_t fd, fs_info_t* info);
int32_t kf_node_update(fs_info_t* info);
void kf_close_proc(int32_t pid);

#endif
