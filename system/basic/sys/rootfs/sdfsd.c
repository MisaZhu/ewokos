#include <stdlib.h>
#include <unistd.h>
#include <ewoksys/wait.h>
#include <string.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/klog.h>
#include <sysinfo.h>
#include <fcntl.h>
#include <sd/sd.h>
#include <ext2/ext2fs.h>
#include <stdio.h>
#include <bsp/bsp_sd.h>

#define SD_BUFFER_SIZE (1024*1024*64) //64M buffer size

static int32_t ext2_sd_read_blocks(int32_t block, void* buf, uint32_t count) {
	return sd_read_blocks(block, buf, count);
}

static uint32_t dir_block_count(ext2_t* ext2, const INODE* inode) {
	uint32_t block_size = ext2_block_size(ext2);
	if(inode->i_size == 0)
		return 0;
	return (inode->i_size + block_size - 1) / block_size;
}

static int32_t dirent_name_equals(const DIR_T* dp, const char* name) {
	size_t len = strlen(name);
	return dp->name_len == len && memcmp(dp->name, name, len) == 0;
}

static int32_t dirent_type_to_fs(const DIR_T* dp, const INODE* inode) {
	if(dp->file_type == EXT2_FT_DIR || (inode->i_mode & 0xF000) == EXT2_S_IFDIR)
		return FS_TYPE_DIR;
	if(dp->file_type == EXT2_FT_FILE || (inode->i_mode & 0xF000) == EXT2_S_IFREG)
		return FS_TYPE_FILE;
	return FS_TYPE_UNKNOWN;
}

static void set_fsinfo_stat(node_stat_t* stat, INODE* inode) {
	stat->atime = inode->i_atime;
	stat->ctime = inode->i_ctime;
	stat->mtime = inode->i_mtime;
	stat->gid = inode->i_gid;
	stat->uid = inode->i_uid;
	stat->links_count = inode->i_links_count;
	stat->mode = inode->i_mode;
	stat->size = inode->i_size;
}

static void set_inode_stat(node_stat_t* stat, INODE* inode) {
	inode->i_atime = stat->atime;
	inode->i_ctime = stat->ctime;
	inode->i_mtime = stat->mtime;
	inode->i_gid = stat->gid;
	inode->i_uid = stat->uid;
	inode->i_links_count = stat->links_count;
	//keep the on-disk file-type bits (S_IFDIR/S_IFREG): the VFS stat
	//mode only carries permissions, a plain chmod must not wipe them.
	inode->i_mode = (inode->i_mode & 0xF000) | (stat->mode & 0x0FFF);
	inode->i_size = stat->size;
}

#define NEW_NODES_BATCH 64

static int32_t add_nodes(ext2_t* ext2, INODE *ip, fsinfo_t* dinfo) {
	char *cp;
	DIR_T  *dp;
	uint32_t block_size = ext2_block_size(ext2);
	uint32_t blocks = dir_block_count(ext2, ip);
	char buf[EXT2_MAX_BLOCK_SIZE + 1];

	fsinfo_t* kids = NULL;
	uint32_t kid_num = 0;
	uint32_t kid_cap = 0;

	for(uint32_t lbk = 0; lbk < blocks; lbk++) {
		memset(buf, 0, sizeof(buf));
		int32_t rd = ext2_read_block(ext2, ip, buf, (int32_t)block_size, (int32_t)(lbk * block_size));
		if(rd <= 0)
			continue;
		dp = (DIR_T *)buf;
		cp = buf;

		while (cp < (buf + block_size)){
			if(dp->name_len == 0 || dp->rec_len < 12 ||
					dp->rec_len < (uint16_t)(4 * ((8 + dp->name_len + 3) / 4)) ||
					(cp + dp->rec_len) > (buf + block_size))
				break;

			if(dp->inode != 0 && !dirent_name_equals(dp, ".") && !dirent_name_equals(dp, "..")) {
				int32_t ino = dp->inode;
				INODE ip_node;
				if(ext2_node_by_ino(ext2, ino, &ip_node) == 0) {
					int32_t type = dirent_type_to_fs(dp, &ip_node);
					if(type == FS_TYPE_DIR || type == FS_TYPE_FILE) {
						if(kid_num >= kid_cap) { //grow geometrically, one realloc per entry is O(n^2)
							kid_cap = (kid_cap == 0) ? 16 : (kid_cap * 2);
							kids = realloc(kids, sizeof(fsinfo_t) * kid_cap);
						}
						fsinfo_t* f = &kids[kid_num];
						memset(f, 0, sizeof(fsinfo_t));
						memcpy(f->name, dp->name, dp->name_len);
						f->name[dp->name_len] = 0;
						f->type = type;
						f->data = (uint32_t)ino;
						set_fsinfo_stat(&f->stat, &ip_node);
						kid_num++;
					}
				}
			}
			cp += dp->rec_len;
			dp = (DIR_T *)cp;
		}
	}

	if(kid_num == 0)
		return 0;

	//only preload the first level under the mount root; deeper directories
	//are expanded lazily by vfsd through FS_CMD_KIDS.
	for(uint32_t off = 0; off < kid_num; off += NEW_NODES_BATCH) {
		uint32_t n = kid_num - off;
		if(n > NEW_NODES_BATCH)
			n = NEW_NODES_BATCH;
		if(vfs_new_nodes(&kids[off], n, dinfo->node) != 0) {
			//fall back to the one-by-one path (old vfsd)
			for(uint32_t j = 0; j < n; j++)
				vfs_new_node(&kids[off+j], dinfo->node, false, false);
		}
	}
	free(kids);
	return 0;
}

static int sdext2_mount(vdevice_t* dev, fsinfo_t* info, void* p) {
	(void)dev;
	ext2_t* ext2 = (ext2_t*)p;
	INODE root_node;
	info->state |= FS_STATE_KIDS_LOADED;
	if(ext2_node_by_fname(ext2, "/", &root_node) != 0)
		return -1;
	add_nodes(ext2, &root_node, info);
	return 0;
}

static int sdext2_create(vdevice_t* dev, int pid, fsinfo_t* info_to, fsinfo_t* info, void* p) {
	(void)dev;
	ext2_t* ext2 = (ext2_t*)p;
	int32_t ino_to = (int32_t)info_to->data;
	if(ino_to == 0) ino_to = 2;

	INODE inode_to;
	if(ext2_node_by_ino(ext2, ino_to, &inode_to) != 0)
		return -1;

	int ino = -1;
	if(FS_IS_TYPE(info->type, FS_TYPE_DIR))  {
		info->stat.size = ext2_block_size(ext2);
		ino = ext2_create_dir(ext2, ino_to, &inode_to, info->name, info->stat.uid, info->stat.gid, info->stat.mode);
	}
	else {
		info->stat.size = 0;
		ino = ext2_create_file(ext2, ino_to, &inode_to, info->name, info->stat.uid, info->stat.gid, info->stat.mode);
	}

	if(ino == -1)
		return -1;
	info->data = ino;
	if(FS_IS_TYPE(info->type, FS_TYPE_DIR))
		info->state |= FS_STATE_KIDS_LOADED;
	return 0;
}

static int sdext2_open(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, int oflag, void* p) {
	(void)dev;
	(void)fd;
	(void)from_pid;

	/*
	 * Path lookup already validated inode existence before VFS asks the
	 * mounted filesystem to open it. For regular non-truncating opens,
	 * there is no per-fd state to initialize in ext2, so avoid the extra
	 * synchronous inode fetch on every exec/script read.
	 */
	if((oflag & O_TRUNC) == 0) {
		return 0;
	}

	ext2_t* ext2 = (ext2_t*)p;
	int32_t ino = (int32_t)info->data;
	if(ino == 0)
		return -1;

	INODE inode;
	if(ext2_node_by_ino(ext2, ino, &inode) != 0) {
		return -1;
	}

	if((oflag & O_TRUNC) != 0) {
		if(ext2_truncate(ext2, (uint32_t)ino, &inode) != 0)
			return -1;
		set_fsinfo_stat(&info->stat, &inode);
	}
	return 0;	
}

static int sdext2_set(vdevice_t* dev, int from_pid, fsinfo_t* info, void* p) {
	(void)dev;
	ext2_t* ext2 = (ext2_t*)p;
	int32_t ino = (int32_t)info->data;
	if(ino == 0)
		return -1;

	INODE inode;
	if(ext2_node_by_ino(ext2, ino, &inode) != 0) {
		return -1;
	}

	set_inode_stat(&info->stat, &inode);
	put_node(ext2, ino, &inode);
	return 0;
}

static int sdext2_get(vdevice_t* dev, int from_pid, const char* fname, fsinfo_t* info, void* p) {
	(void)dev;
	ext2_t* ext2 = (ext2_t*)p;
	DIR_T dirp;
	uint32_t ino = ext2_ino_by_fname(ext2, fname, &dirp);
	if(ino <= 0)
		return -1;

	INODE inode;
	if(ext2_node_by_ino(ext2, ino, &inode) != 0)
		return -1;

	memset(info, 0, sizeof(fsinfo_t));
	strcpy(info->name, dirp.name);
	info->type = dirent_type_to_fs(&dirp, &inode);
	info->data = (uint32_t)ino;
	set_fsinfo_stat(&info->stat, &inode);
	return 0;
}

static fsinfo_t* sdext2_kids(vdevice_t* dev, fsinfo_t* info_dir, uint32_t* num, void* p) {
	(void)dev;
	fsinfo_t* ret = NULL;
	*num = 0;
	if(!FS_IS_TYPE(info_dir->type, FS_TYPE_DIR))
		return NULL;

	ext2_t* ext2 = (ext2_t*)p;
	int32_t ino_dir = (int32_t)info_dir->data;
	if(ino_dir == 0) ino_dir = 2;

	INODE inode_dir;
	if(ext2_node_by_ino(ext2, ino_dir, &inode_dir) != 0)
		return NULL;

	char *cp;
	DIR_T  *dp;
	uint32_t block_size = ext2_block_size(ext2);
	uint32_t blocks = dir_block_count(ext2, &inode_dir);
	char buf[EXT2_MAX_BLOCK_SIZE + 1];

	for(uint32_t lbk = 0; lbk < blocks; lbk++) {
		memset(buf, 0, sizeof(buf));
		if(ext2_read_block(ext2, &inode_dir, buf, (int32_t)block_size, (int32_t)(lbk * block_size)) <= 0)
			continue;
		dp = (DIR_T *)buf;
		cp = buf;

		while (cp < (buf + block_size)){
			if(dp->name_len == 0 || dp->rec_len < 12 ||
					dp->rec_len < (uint16_t)(4 * ((8 + dp->name_len + 3) / 4)) ||
					(cp + dp->rec_len) > (buf + block_size))
				break;

			if(dp->inode != 0 && !dirent_name_equals(dp, ".") && !dirent_name_equals(dp, "..")) {
				int32_t ino = dp->inode;
				INODE ip_node;
				if(ext2_node_by_ino(ext2, ino, &ip_node) == 0) {
					int32_t type = dirent_type_to_fs(dp, &ip_node);
					if(type == FS_TYPE_DIR || type == FS_TYPE_FILE) {
						fsinfo_t f;
						memset(&f, 0, sizeof(fsinfo_t));
						memcpy(f.name, dp->name, dp->name_len);
						f.name[dp->name_len] = 0;
						f.data = (uint32_t)ino;
						f.type = type;
						set_fsinfo_stat(&f.stat, &ip_node);

						ret = realloc(ret, sizeof(fsinfo_t) * (*num + 1));
						memcpy(&ret[*num], &f, sizeof(fsinfo_t));
						(*num)++;
					}
				}
			}
			cp += dp->rec_len;
			dp = (DIR_T *)cp;
		}
	}
	return ret;
}

static int sdext2_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
		void* buf, int size, int offset, void* p) {
	(void)dev;
	(void)fd;

	ext2_t* ext2 = (ext2_t*)p;
	int32_t ino = (int32_t)info->data;
	if(ino == 0)
		ino = 2;
	INODE inode;
	if(ext2_node_by_ino(ext2, ino, &inode) != 0) {
		return -1;
	}

	int rsize = info->stat.size - offset;
	if(rsize < size)
		size = rsize;
	if(size < 0)
		size = -1;

	if(size > 0) {
		size = ext2_read(ext2, &inode, buf, size, offset);
	}
	return size;	
}

static int sdext2_write(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
		const void* buf, int size, int offset, void* p) {
	(void)dev;
	(void)fd;
	(void)from_pid;

	ext2_t* ext2 = (ext2_t*)p;
	int32_t ino = (int32_t)info->data;
	if(ino == 0)
		return -1;

	INODE inode;
	if(ext2_node_by_ino(ext2, ino, &inode) != 0) {
		return -1;
	}
	size = ext2_write(ext2, &inode, buf, size, offset);
	if(size >= 0) {
		set_fsinfo_stat(&info->stat, &inode);
		put_node(ext2, ino, &inode);
	}
	return size;	
}

static int sdext2_unlink(vdevice_t* dev, fsinfo_t* info, const char* fname, void* p) {
	(void)dev;
	ext2_t* ext2 = (ext2_t*)p;
	int ret = FS_IS_TYPE(info->type, FS_TYPE_DIR) ? ext2_rmdir(ext2, fname) : ext2_unlink(ext2, fname);
	if(ret != 0)
		return -1;
	return vfs_del_node(info->node);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	if((int16_t)getuid() >= 0) {
		klog("this process can only loaded by kernel!\n");
		return -1;
	}

	if(bsp_sd_init() != 0) {
		return -1;
	}

	ext2_t ext2;
	if(ext2_init_ex(&ext2, sd_read, ext2_sd_read_blocks, sd_write, SD_BUFFER_SIZE) != 0) { //max buffer size 16MB
		sd_quit();
		return -1;
	}

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "rootfs(ext2)");
	dev.mount = sdext2_mount;
	dev.read = sdext2_read;
	dev.write = sdext2_write;
	dev.create = sdext2_create;
	dev.open = sdext2_open;
	dev.set = sdext2_set;
	dev.get = sdext2_get;
	dev.kids = sdext2_kids;
	dev.unlink = sdext2_unlink;
	
	dev.extra_data = &ext2;
	device_run(&dev, "/", FS_TYPE_DIR, 0777);
	ext2_quit(&ext2);
	sd_quit();
	return 0;
}
