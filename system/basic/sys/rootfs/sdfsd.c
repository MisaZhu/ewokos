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
	int32_t i; 
	char c, *cp;
	DIR_T  *dp;
	char buf[EXT2_BLOCK_SIZE+1];

	fsinfo_t* kids = NULL;
	uint32_t kid_num = 0;

	//pass 1: collect all entries of this directory
	for (i=0; i<12; i++){
		if (ip->i_block[i] != 0){
			ext2->read_block(ip->i_block[i], buf);
			dp = (DIR_T *)buf;
			cp = buf;

			if(dp->inode == 0)
				continue;

			while (cp < (buf + EXT2_BLOCK_SIZE)){
				if(dp->name_len == 0)
					break;
				//guard against garbage/torn entries: a rec_len
				//below the minimal entry size would loop forever
				//or walk out of the block.
				if(dp->rec_len < 12)
					break;

				c = dp->name[dp->name_len];  // save last byte
				dp->name[dp->name_len] = 0;   

				if(strcmp(dp->name, ".") != 0 && strcmp(dp->name, "..") != 0 && dp->inode != 0 &&
						(dp->file_type == 1 || dp->file_type == 2)) {
					int32_t ino = dp->inode;
					INODE ip_node;
					if(ext2_node_by_ino(ext2, ino, &ip_node) == 0) {
						kids = realloc(kids, sizeof(fsinfo_t) * (kid_num + 1));
						fsinfo_t* f = &kids[kid_num];
						memset(f, 0, sizeof(fsinfo_t));
						strcpy(f->name, dp->name);
						f->type = (dp->file_type == 2) ? FS_TYPE_DIR : FS_TYPE_FILE;
						f->data = (uint32_t)ino;
						set_fsinfo_stat(&f->stat, &ip_node);
						kid_num++;
					}
				}
				//add node
				dp->name[dp->name_len] = c; // restore that last byte
				cp += dp->rec_len;
				dp = (DIR_T *)cp;
			}
		}
	}

	if(kid_num == 0)
		return 0;

	//pass 2: register all kids in vfsd, batched to cut IPC round trips
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

	//pass 3: recurse into sub directories
	for(uint32_t j = 0; j < kid_num; j++) {
		if(kids[j].type == FS_TYPE_DIR) {
			INODE ip_node;
			if(ext2_node_by_ino(ext2, (int32_t)kids[j].data, &ip_node) == 0)
				add_nodes(ext2, &ip_node, &kids[j]);
		}
	}
	free(kids);
	return 0;
}

/*
 * Warm the SD sector cache with a few large multi-block reads before the
 * tree walk: add_nodes fetches one inode-table block per directory entry,
 * and issuing those as scattered single-block SD commands dominates mount
 * time. Only the used prefix of each group's inode table is prefetched
 * (mkfs allocates inodes densely from the front).
 */
#define PREFETCH_CHUNK 128
static void prefetch_inode_tables(ext2_t* ext2) {
	char* scratch = (char*)malloc(PREFETCH_CHUNK * EXT2_BLOCK_SIZE);
	if(scratch == NULL)
		return;

	char bitmap[EXT2_BLOCK_SIZE];
	for(int32_t g = 0; g < ext2->group_num; g++) {
		GD* gd = &ext2->gds[g];
		uint32_t total = ext2->super.s_inodes_per_group;
		if(gd->bg_free_inodes_count >= total) //group has no used inode
			continue;
		if(ext2->read_block(gd->bg_inode_bitmap, bitmap) != 0)
			continue;

		int32_t last = -1;
		for(int32_t bit = (int32_t)total - 1; bit >= 0; bit--) {
			if(bitmap[bit/8] & (1 << (bit%8))) {
				last = bit;
				break;
			}
		}
		if(last < 0)
			continue;

		uint32_t blocks = (uint32_t)(last / 8) + 1; //8 inodes(128B) per 1KB block
		uint32_t blk = gd->bg_inode_table;
		while(blocks > 0) {
			uint32_t n = (blocks > PREFETCH_CHUNK) ? PREFETCH_CHUNK : blocks;
			if(ext2_sd_read_blocks((int32_t)blk, scratch, n) != 0)
				break;
			blk += n;
			blocks -= n;
		}
	}
	free(scratch);
}

static int sdext2_mount(vdevice_t* dev, fsinfo_t* info, void* p) {
	(void)dev;
	ext2_t* ext2 = (ext2_t*)p;
	INODE root_node;
	ext2_node_by_fname(ext2, "/", &root_node);
	prefetch_inode_tables(ext2);
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
	if(info->type == FS_TYPE_DIR)  {
		info->stat.size = EXT2_BLOCK_SIZE;
		ino = ext2_create_dir(ext2, ino_to, &inode_to, info->name, info->stat.uid, info->stat.gid, info->stat.mode);
	}
	else {
		info->stat.size = 0;
		ino = ext2_create_file(ext2, ino_to, &inode_to, info->name, info->stat.uid, info->stat.gid, info->stat.mode);
	}

	if(ino == -1)
		return -1;
	info->data = ino;
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
		inode.i_size = 0;
		info->stat.size = 0;
		put_node(ext2, ino, &inode);
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
	if(dirp.file_type == 2)
		info->type = FS_TYPE_DIR;
	else if(dirp.file_type == 1)
		info->type = FS_TYPE_FILE;
	info->data = (uint32_t)ino;
	set_fsinfo_stat(&info->stat, &inode);
	return 0;
}

static fsinfo_t* sdext2_kids(vdevice_t* dev, fsinfo_t* info_dir, uint32_t* num, void* p) {
	(void)dev;
	fsinfo_t* ret = NULL;
	*num = 0;
	if(info_dir->type != FS_TYPE_DIR)
		return NULL;

	ext2_t* ext2 = (ext2_t*)p;
	int32_t ino_dir = (int32_t)info_dir->data;
	if(ino_dir == 0) ino_dir = 2;

	INODE inode_dir;
	if(ext2_node_by_ino(ext2, ino_dir, &inode_dir) != 0)
		return NULL;

	int32_t i; 
	char c, *cp;
	DIR_T  *dp;
	char buf[EXT2_BLOCK_SIZE+1];

	for (i=0; i<12; i++){
		if (inode_dir.i_block[i] != 0){
			ext2->read_block(inode_dir.i_block[i], buf);
			dp = (DIR_T *)buf;
			cp = buf;

			if(dp->inode == 0)
				continue;

			while (cp < (buf + EXT2_BLOCK_SIZE)){
				if(dp->name_len == 0)
					break;
				//guard against garbage/torn entries: a rec_len
				//below the minimal entry size would loop forever
				//or walk out of the block.
				if(dp->rec_len < 12)
					break;

				c = dp->name[dp->name_len];  // save last byte
				dp->name[dp->name_len] = 0;   

				if(strcmp(dp->name, ".") != 0 && strcmp(dp->name, "..") != 0 && dp->inode != 0) {
					int32_t ino = dp->inode;
					INODE ip_node;
					if(ext2_node_by_ino(ext2, ino, &ip_node) == 0) {
						fsinfo_t f;
						memset(&f, 0, sizeof(fsinfo_t));
						strcpy(f.name, dp->name);
						f.data = (uint32_t)ino;
						if(dp->file_type == 2) //director
							f.type = FS_TYPE_DIR;
						else if(dp->file_type == 1) //file
							f.type = FS_TYPE_FILE;
						set_fsinfo_stat(&f.stat, &ip_node);

						ret = realloc(ret, sizeof(fsinfo_t) * (*num + 1));
						memcpy(&ret[*num], &f, sizeof(fsinfo_t));
						(*num)++;
						//klog("add file %s\n", dp->name);
					}
				}
				//add node
				dp->name[dp->name_len] = c; // restore that last byte
				cp += dp->rec_len;
				dp = (DIR_T *)cp;
			}
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
		info->stat.size += size;
		inode.i_size = info->stat.size;
		put_node(ext2, ino, &inode);
	}
	return size;	
}

static int sdext2_unlink(vdevice_t* dev, fsinfo_t* info, const char* fname, void* p) {
	(void)dev;
	ext2_t* ext2 = (ext2_t*)p;
	if(ext2_unlink(ext2, fname) != 0)
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
