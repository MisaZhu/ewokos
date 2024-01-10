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

static void add_file(fsinfo_t* node_to, const char* name, INODE* inode, int32_t ino) {
	fsinfo_t f;
	memset(&f, 0, sizeof(fsinfo_t));
	strcpy(f.name, name);
	f.type = FS_TYPE_FILE;
	f.data = (uint32_t)ino;
	set_fsinfo_stat(&f.stat, inode);
	vfs_new_node(&f, node_to->node);
}

static int add_dir(fsinfo_t* info_to, fsinfo_t* ret, const char* dn, INODE* inode, int ino) {
	memset(ret, 0, sizeof(fsinfo_t));
	strcpy(ret->name, dn);
	ret->type = FS_TYPE_DIR;
	ret->data = (uint32_t)ino;
	set_fsinfo_stat(&ret->stat, inode);
	if(vfs_new_node(ret, info_to->node) != 0)
		return -1;
	return 0;
}

static int32_t add_nodes(ext2_t* ext2, INODE *ip, fsinfo_t* dinfo) {
	int32_t i; 
	char c, *cp;
	DIR_T  *dp;
	char buf[EXT2_BLOCK_SIZE+1];

	for (i=0; i<12; i++){
		if (ip->i_block[i] != 0){
			ext2->read_block(ip->i_block[i], buf, 1);
			dp = (DIR_T *)buf;
			cp = buf;

			if(dp->inode == 0)
				continue;

			while (cp < (buf + EXT2_BLOCK_SIZE)){
				if(dp->name_len == 0)
					break;

				c = dp->name[dp->name_len];  // save last byte
				dp->name[dp->name_len] = 0;   

				if(strcmp(dp->name, ".") != 0 && strcmp(dp->name, "..") != 0) {
					int32_t ino = dp->inode;
					INODE ip_node;
					if(ext2_node_by_ino(ext2, ino, &ip_node) == 0) {
						if(dp->file_type == 2) {//director
							fsinfo_t ret;
							add_dir(dinfo, &ret, dp->name, &ip_node, ino);
							add_nodes(ext2, &ip_node, &ret);
						}
						else if(dp->file_type == 1) {//file
							add_file(dinfo, dp->name, &ip_node, ino);
						}
					}
				}
				//add node
				dp->name[dp->name_len] = c; // restore that last byte
				cp += dp->rec_len;
				dp = (DIR_T *)cp;
			}
		}
	}
	return 0;
}

static int sdext2_mount(fsinfo_t* info, void* p) {
	ext2_t* ext2 = (ext2_t*)p;
	INODE root_node;
	ext2_node_by_fname(ext2, "/", &root_node);
	add_nodes(ext2, &root_node, info);
	return 0;
}

static int sdext2_create(int pid, fsinfo_t* info_to, fsinfo_t* info, void* p) {
	ext2_t* ext2 = (ext2_t*)p;
	int32_t ino_to = (int32_t)info_to->data;
	if(ino_to == 0) ino_to = 2;

	if(vfs_check_access(pid, info_to, VFS_ACCESS_W) != 0)
		return -1;

	INODE inode_to;
	if(ext2_node_by_ino(ext2, ino_to, &inode_to) != 0)
		return -1;

	int ino = -1;
	if(info->type == FS_TYPE_DIR)  {
		info->stat.size = EXT2_BLOCK_SIZE;
		ino = ext2_create_dir(ext2, &inode_to, info->name, info->stat.uid, info->stat.gid, info->stat.mode);
	}
	else {
		info->stat.size = 0;
		ino = ext2_create_file(ext2, &inode_to, info->name, info->stat.uid, info->stat.gid, info->stat.mode);
	}

	if(ino == -1)
		return -1;
	info->data = ino;
	vfs_set(info);
	return 0;
}

static int sdext2_open(int fd, int from_pid, uint32_t node, int oflag, void* p) {
	(void)fd;
	(void)from_pid;

	fsinfo_t info;
	if(vfs_get_by_node(node, &info) != 0)
		return -1;
	
	if((oflag & O_WRONLY) != 0 && vfs_check_access(from_pid, &info, VFS_ACCESS_W) != 0)
		return -1;

	if(vfs_check_access(from_pid, &info, VFS_ACCESS_R) != 0)
		return -1;

	ext2_t* ext2 = (ext2_t*)p;
	int32_t ino = (int32_t)info.data;
	if(ino == 0)
		return -1;

	INODE inode;
	if(ext2_node_by_ino(ext2, ino, &inode) != 0) {
		return -1;
	}

	if((oflag & O_TRUNC) != 0) {
		inode.i_size = 0;
		info.stat.size = 0;
		put_node(ext2, ino, &inode);
		vfs_set(&info);
	}
	return 0;	
}

static int sdext2_read(int fd, int from_pid, uint32_t node, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;

	fsinfo_t info;
	if(vfs_get_by_node(node, &info) != 0)
		return -1;
	if(vfs_check_access(from_pid, &info, VFS_ACCESS_R) != 0)
		return -1;

	ext2_t* ext2 = (ext2_t*)p;
	int32_t ino = (int32_t)info.data;
	if(ino == 0)
		ino = 2;
	INODE inode;
	if(ext2_node_by_ino(ext2, ino, &inode) != 0) {
		return -1;
	}

	int rsize = info.stat.size - offset;
	if(rsize < size)
		size = rsize;
	if(size < 0)
		size = -1;

	if(size > 0) 
		size = ext2_read(ext2, &inode, buf, size, offset);
	return size;	
}

static int sdext2_write(int fd, int from_pid, uint32_t node,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;

	fsinfo_t info;
	if(vfs_get_by_node(node, &info) != 0)
		return -1;
	if(vfs_check_access(from_pid, &info, VFS_ACCESS_W) != 0)
		return -1;

	ext2_t* ext2 = (ext2_t*)p;
	int32_t ino = (int32_t)info.data;
	if(ino == 0)
		return -1;

	INODE inode;
	if(ext2_node_by_ino(ext2, ino, &inode) != 0) {
		return -1;
	}
	size = ext2_write(ext2, &inode, buf, size, offset);
	if(size >= 0) {
		info.stat.size += size;
		inode.i_size = info.stat.size;
		put_node(ext2, ino, &inode);
		vfs_set(&info);
	}
	return size;	
}

static int sdext2_unlink(fsinfo_t* info, const char* fname, void* p) {
	(void)info;
	ext2_t* ext2 = (ext2_t*)p;
	return ext2_unlink(ext2, fname);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	if(getuid() >= 0) {
		klog("this process can only loaded by kernel!\n");
		return -1;
	}

	klog("\n    init sdc ... ");
	if(bsp_sd_init() != 0) {
		klog("failed!\n");
		return -1;
	}
	klog("[ok]\n");

	ext2_t ext2;
	klog("    init ext2 fs ... ");
	if(ext2_init(&ext2, sd_read, sd_write) != 0) {
		sd_quit();
		klog("failed!\n");
		return -1;
	}
	klog("[ok]\n");
	sd_set_buffer(ext2.super.s_blocks_count*2);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "rootfs(ext2)");
	dev.mount = sdext2_mount;
	dev.read = sdext2_read;
	dev.write = sdext2_write;
	dev.create = sdext2_create;
	dev.open = sdext2_open;
	dev.unlink = sdext2_unlink;
	
	dev.extra_data = &ext2;
	device_run(&dev, "/", FS_TYPE_DIR, 0771);
	ext2_quit(&ext2);
	sd_quit();
	return 0;
}