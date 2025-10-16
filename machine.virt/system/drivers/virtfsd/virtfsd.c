#include <stdlib.h>
#include <unistd.h>
#include <ewoksys/wait.h>
#include <string.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/klog.h>
#include <ewoksys/mmio.h>
#include <sysinfo.h>
#include <fcntl.h>
#include <stdio.h>
#include <arch/virt/virtfs.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define VIRTFS_DEBUG 0
#if VIRTFS_DEBUG
#define FS_DBG(fmt, ...) klog(fmt, ##__VA_ARGS__)
#else
#define FS_DBG(fmt, ...) \
	do                   \
	{                    \
	} while (0)
#endif

typedef struct{
	uint32_t fid;
	uint32_t pfid;
	uint8_t flag;
}virtfs_node_t;

static virtfs_node_t *nodes = NULL;
static uint32_t virtfs_node_count = 0;
static uint32_t virtfs_node_used = 0;

static int alloc_node(uint32_t pfid)
{
	if(virtfs_node_count - virtfs_node_used < 1024){
		virtfs_node_count += 1024;
		nodes = (virtfs_node_t *)realloc(nodes, sizeof(virtfs_node_t) * virtfs_node_count);
	}
	nodes[virtfs_node_used].fid = virtfs_node_used;
	nodes[virtfs_node_used].pfid = pfid;
	nodes[virtfs_node_used].flag = 0;
	return virtfs_node_used++;
}

static void add_file(fsinfo_t *node_to, struct virtfs_dir_entry *entry, uint32_t fid, virtfs_t fs)
{
	fsinfo_t f;
	memset(&f, 0, sizeof(fsinfo_t));
	memcpy(f.name, entry->name, entry->nlen);
	f.type = FS_TYPE_FILE;
	f.data = fid;
	virtfs_stat_t stat;
	virtfs_stat(fs, 1, fid, &stat);
	f.stat.size = stat.length;
	f.stat.mode = stat.mode | 0444;
	f.stat.atime = stat.atime;
	f.stat.mtime = stat.mtime;
	f.stat.gid = 100;
	f.stat.uid = 100;
	f.stat.mtime = stat.mtime;
	vfs_new_node(&f, node_to->node, false);
}

static int add_dir(fsinfo_t *info_to, fsinfo_t *ret, struct virtfs_dir_entry *entry, uint32_t fid, virtfs_t fs)
{
	memset(ret, 0, sizeof(fsinfo_t));
	memcpy(ret->name, entry->name, entry->nlen);
	virtfs_stat_t stat;
	virtfs_stat(fs, 1, fid, &stat);
	ret->type = FS_TYPE_DIR;
	ret->data = fid;
	ret->stat.size = stat.length;
	ret->stat.mode = stat.mode | 0444;
	ret->stat.atime = stat.atime;
	ret->stat.mtime = stat.mtime;
	ret->stat.gid = 100;
	ret->stat.uid = 100;
	ret->stat.mtime = stat.mtime;
	if (vfs_new_node(ret, info_to->node, false) != 0)
		return -1;
	return 0;
}

static void debug_entry(struct virtfs_dir_entry *entry)
{
	FS_DBG("type:%d name:", entry->type, entry->nlen);
	for (int i = 0; i < entry->nlen; i++)
	{
		FS_DBG("%c", entry->name[i]);
	}
	FS_DBG("\n");
}

static void virtfsd_read_directory(fsinfo_t *info_to, virtfs_t fs, int fid)
{
	char dir[1024];
	char name[256];
	memset(dir, 0, 1024);
	if (virtfs_open(fs, 1, fid, O_RDONLY) == 0)
	{
		uint64_t offset = 0;
		while (true)
		{
			int ret = virtfs_readdir(fs, 1, fid, dir, offset, 1024);
			for (int i = 0; i < ret && ret > 0;)
			{
				struct virtfs_dir_entry *entry = (struct virtfs_dir_entry *)&dir[i];
				if (entry->nlen == 0)
					break;

				i += sizeof(struct virtfs_dir_entry) + entry->nlen;
				offset = entry->offset;
				if (entry->name[0] == '.')
					continue;

				debug_entry(entry);
				if (entry->type == VIRTFS_TYPE_DIR)
				{
					uint32_t new_fid = alloc_node(fid);
					fsinfo_t new_dir;
					int nlen = MIN(entry->nlen, sizeof(name) - 1);
					memcpy(name, entry->name, nlen);
					name[nlen] = '\0';
					if (virtfs_walk(fs, 1, fid, new_fid, name) == 0)
					{
						add_dir(info_to, &new_dir, entry, new_fid, fs);
						virtfsd_read_directory(&new_dir, fs, new_fid);
					}
				}
				else
				{
					fsinfo_t new_file;
					uint32_t new_fid = alloc_node(fid);
					int nlen = MIN(entry->nlen, sizeof(name) - 1);
					memcpy(name, entry->name, nlen);
					name[nlen] = '\0';
					if(virtfs_walk(fs, 1, fid, new_fid, name) == 0){
						add_file(info_to, entry, new_fid, fs);
					}
				}
			}
			if (ret < 1024)
				break;
		}
		//virtfs_close(fs, 1, fid);
	}
}

static int _mount(fsinfo_t *info, void *p)
{
	fsinfo_t ret;

	virtfs_t fs = (virtfs_t)p;
	int root = alloc_node(0);
	FS_DBG("fid:%d\n", root);
	virtfs_attach(fs, 2, root, 0xffffffff, "root", "/");
	virtfsd_read_directory(info, fs, root);
	return 0;
}

static int _create(int pid, fsinfo_t *info_to, fsinfo_t *info, void *p)
{
	FS_DBG("virtfsd create %s\n", info->name);
	return 0;
}

static int _open(int fd, int from_pid, fsinfo_t *info, int oflag, void *p)
{
	FS_DBG("virtfsd open %s %d flag:%08x\n", info->name, info->data, oflag);
	virtfs_t fs = (virtfs_t)p;
	if(nodes[info->data].flag)
		return 0;
	int ret = virtfs_open(fs, 1, info->data, oflag&0xFF);
	if(ret == 0)
		nodes[info->data].flag = 1;
	return ret;
}

static int _stat(int from_pid, fsinfo_t *info, node_stat_t *stat, void *p)
{
	FS_DBG("virtfsd stat %s %d\n", info->name, info->data);
	virtfs_t fs = (virtfs_t)p;
	virtfs_stat_t st;
	if (info->type == FS_TYPE_DIR)
		return 0;
	if (virtfs_stat(fs, 1, info->data, &st) == 0)
	{
		stat->size = st.length;
		stat->mode = st.mode | 0444;
		stat->atime = st.atime;
		stat->mtime = st.mtime;
		stat->gid = 100;
		stat->uid = 100;
		stat->mtime = st.mtime;
	}
	return 0;
}

static int _set(int from_pid, fsinfo_t *info, void *p)
{
	FS_DBG("virtfsd set %s\n", info->name);
	return 0;
}

static int _read(int fd, int from_pid, fsinfo_t *info,
				 void *buf, int size, int offset, void *p)
{
	FS_DBG("virtfsd read %s %d offset:%d size:%d\n", info->name, info->data, offset, size);
	virtfs_t fs = (virtfs_t)p;
	return virtfs_read(fs, 1, info->data, buf, offset, size);
}

static int _write(int fd, int from_pid, fsinfo_t *info,
				  const void *buf, int size, int offset, void *p)
{
	(void)fd;
	(void)from_pid;
	FS_DBG("virtfsd write %s %d\n", info->name, info->data);
	return size;
}

static int _unlink(fsinfo_t *info, const char *fname, void *p)
{
	FS_DBG("virtfsd unlink %s %d\n", fname, info->data);
	return 0;
}

static int _close(int fd, int from_pid, uint32_t node, fsinfo_t* info, void* p)
{
	FS_DBG("virtfsd close %s %d\n", info->name, info->data);
	virtfs_t fs = (virtfs_t)p;
	return virtfs_sync(fs, 1, info->data);
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	_mmio_base = mmio_map();
	virtfs_t fs = virtfs_init();
	if (fs == NULL)
	{
		FS_DBG("virtfs_init failed!\n");
		return -1;
	}
	virtfs_set_version(fs, "9P2000.L");

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "sharedfs(virtfs)");

	dev.mount = _mount;
	dev.read = _read;
	dev.stat = _stat;
	dev.write = _write;
	dev.create = _create;
	dev.open = _open;
	dev.close = _close;
	dev.set = _set;
	dev.unlink = _unlink;

	dev.extra_data = fs;
	device_run(&dev, "/mnt/share", FS_TYPE_DIR, 0777);
	return 0;
}
