#ifndef VFS_TREE_H
#define VFS_TREE_H

#include <fsinfo.h>
#include <kernel/proc.h>

typedef struct vfs_node {
	struct vfs_node* father; 
	struct vfs_node* first_kid; /*first child*/
	struct vfs_node* last_kid; /*last child*/
	struct vfs_node* next; /*next brother*/
	struct vfs_node* prev; /*prev brother*/
	uint32_t kids_num;

	fsinfo_t fsinfo;

	uint32_t refs;
	uint32_t refs_w;
} vfs_node_t;

vfs_node_t* vfs_new_node(void);

int32_t vfs_add(vfs_node_t* node_to, vfs_node_t* node);

int32_t vfs_set(vfs_node_t* node, fsinfo_t* info);

vfs_node_t* vfs_get(vfs_node_t* father, const char* name);

vfs_node_t* vfs_node_by_fd(int32_t fd);

int32_t vfs_get_mount(vfs_node_t* node, mount_t* mount);

int32_t vfs_get_mount_by_id(int32_t id, mount_t* mount);

int32_t vfs_open(int32_t pid, vfs_node_t* node, int32_t wr);

void vfs_close(proc_t* proc, int32_t fd);

int32_t vfs_seek(int32_t fd, int32_t offset);

int32_t vfs_tell(int32_t fd);

int32_t vfs_mount(vfs_node_t* org, vfs_node_t* node);

void vfs_umount(vfs_node_t* node);

int32_t vfs_del(vfs_node_t* node);

int32_t vfs_dup2(int32_t from, int32_t to);

int32_t vfs_dup(int32_t from);

void vfs_init(void);

vfs_node_t* vfs_root(void);

#endif
