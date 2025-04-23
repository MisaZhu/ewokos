#include <ewoksys/vdevice.h>
#include <ewoksys/vfs.h>
#include <ewoksys/ipc.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/shm.h>
#include <ewoksys/proc.h>
#include <ewoksys/syscall.h>
#include <ewoksys/signal.h>
#include <ewoksys/hashmap.h>

#ifdef __cplusplus
extern "C" {
#endif

static map_t  _files_hash = NULL;

static void device_init(vdevice_t* dev) {
	_files_hash = hashmap_new();
}

static inline const char* file_hash_key(int fd, int pid, uint32_t node) {
	static char key[32];
	snprintf(key, 31, "%x:%x:%x", fd, pid, node);
	return key;
}

static fsinfo_t* file_get_cache(int fd, int pid, uint32_t node) {
	fsinfo_t* info = NULL;
	hashmap_get(_files_hash, file_hash_key(fd, pid, node), (void**)&info);
	return info;
}

static fsinfo_t* file_add(int fd, int pid, fsinfo_t* info) {
	pid = proc_getpid(pid);
	fsinfo_t* ret = (fsinfo_t*)malloc(sizeof(fsinfo_t));
	hashmap_put(_files_hash, file_hash_key(fd, pid, info->node), ret);
	memcpy(ret, info, sizeof(fsinfo_t));
	return ret;
}

static void file_del(int fd, int pid, uint32_t node) {
	pid = proc_getpid(pid);
	fsinfo_t* info = NULL;
	const char* key = file_hash_key(fd, pid, node);
	hashmap_get(_files_hash, key, (void**)&info);
	if(info == NULL)
		return;

	hashmap_remove(_files_hash, key);
	free(info);
}

fsinfo_t* dev_get_file(int fd, int pid, uint32_t node) {
	pid = proc_getpid(pid);
	fsinfo_t* info = file_get_cache(fd, pid, node);
	if(info == NULL) {
		fsinfo_t i;
		if(vfs_get_by_node(node, &i) != 0)
			return NULL;
		info = file_add(fd, pid, &i);
	}
	return info;
}

int dev_update_file(int fd, int from_pid, fsinfo_t* finfo) {
	fsinfo_t* info = dev_get_file(fd, from_pid, finfo->node);
	if(info == NULL)
		return -1;
	memcpy(info, finfo, sizeof(fsinfo_t));
	return 0;
}

static void do_open(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int oflag;
	int fd = proto_read_int(in);
	uint32_t node = proto_read_int(in);
	oflag = proto_read_int(in);

	fsinfo_t info;
	if(vfs_get_by_node(node, &info) != 0) {
		PF->addi(out, -1)->addi(out, ENOENT);
		return;
	}

	if((oflag & O_WRONLY) != 0 && vfs_check_access(from_pid, &info, W_OK) != 0) {
		PF->addi(out, -1)->addi(out, EPERM);
		return;
	}

	if(vfs_check_access(from_pid, &info, R_OK) != 0) {
		PF->addi(out, -1)->addi(out, EPERM);
		return;
	}
	
	int res = 0;
	if(fd >= 0 && dev != NULL && dev->open != NULL) {
		if(dev->open(fd, from_pid, &info, oflag, p) != 0)
			res = -1;
	}
	PF->addi(out, res);
	if(res == 0) {
		file_add(fd, from_pid, &info);
		PF->add(out, &info, sizeof(fsinfo_t));
	}
	else
		PF->addi(out, errno);
}

static void do_close(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	//all close ipc are from vfsd proc, so read owner pid for real owner.
	(void)out;
	int fd = proto_read_int(in);
	uint32_t node = (uint32_t)proto_read_int(in);
	fsinfo_t* fsinfo = proto_read(in, NULL);

	if(dev != NULL && dev->close != NULL) {
		dev->close(fd, from_pid, node, fsinfo, p);
	}
	file_del(fd, from_pid, node);
}

#define READ_BUF_SIZE 32
static void do_read(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int size, offset;
	int fd = proto_read_int(in);
	uint32_t node = proto_read_int(in);
	size = proto_read_int(in);
	offset = proto_read_int(in);
	int32_t shm_id = proto_read_int(in);
	char buffer[READ_BUF_SIZE];

	fsinfo_t* info = dev_get_file(fd, from_pid, node);
	if(info == NULL) {
		PF->addi(out, -1);
		return;
	}
	/*if(vfs_check_access(from_pid, info, R_OK) != 0) {
		PF->addi(out, -1)->addi(out, EPERM);
		return;
	}
	*/

	if(dev != NULL && dev->read != NULL) {
		void* buf;
		if(shm_id == -1) {
			if(size > READ_BUF_SIZE)
				buf = malloc(size);
			else
				buf = buffer;
		}
		else {
			buf = shmat(shm_id, 0, 0);
		}

		if(buf == NULL) {
			PF->addi(out, -1);
		}
		else {
			int32_t rd = dev->read(fd, from_pid, info, buf, size, offset, p);
			PF->addi(out, rd);
			if(rd > 0) {
				if(rd > size)
					rd = size;
				if(shm_id == -1) {
					PF->add(out, buf, rd);
				}
			}

			if(shm_id != -1 && buf != NULL)
				shmdt(buf);
			else if(buf != buffer)
				free(buf);
		}
	}
	else {
		PF->addi(out, -1);
	}
}

static void do_write(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int32_t size, offset;
	int fd = proto_read_int(in);
	uint32_t node = proto_read_int(in);
	offset = proto_read_int(in);
	int32_t shm_id = proto_read_int(in);
	
	fsinfo_t* info = dev_get_file(fd, from_pid, node);
	if(info == NULL) {
		PF->addi(out, -1);
		return;
	}
	/*if(vfs_check_access(from_pid, info, W_OK) != 0) {
		PF->addi(out, -1)->addi(out, EPERM);
		return;
	}
	*/
	
	if(dev != NULL && dev->write != NULL) {
		void* data;
		if(shm_id == -1)
			data = proto_read(in, &size);
		else {
			size = proto_read_int(in);
			data = shmat(shm_id, 0, 0);
		}

		if(data == NULL) {
			PF->addi(out, -1);
		}
		else {
			size = dev->write(fd, from_pid, info, data, size, offset, p);
			info->state |= FS_STATE_CHANGED;
			dev_update_file(fd, from_pid, info);
			PF->addi(out, size);
			PF->add(out, info, sizeof(fsinfo_t));
		}
		if(shm_id != -1 && data != NULL)
			shmdt(data);
	}
	else {
		PF->addi(out, -1);
	}
}

static void do_read_block(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int size, index;
	size = proto_read_int(in);
	index = proto_read_int(in);
	int32_t shm_id = proto_read_int(in);

	if(dev != NULL && dev->read_block != NULL) {
		void* buf;
		if(shm_id == -1)
			buf = malloc(size);
		else
			buf = shmat(shm_id, 0, 0);
		if(buf == NULL) {
			PF->addi(out, -1);
		}
		else {
			size = dev->read_block(from_pid, buf, size, index, p);
			PF->addi(out, size);
			if(size > 0) {
				if(shm_id == -1) {
					PF->add(out, buf, size);
				}
			}

			if(shm_id != -1 && buf != NULL)
				shmdt(buf);
			else
				free(buf);
		}
	}
	else {
		PF->addi(out, -1);
	}
}

static void do_write_block(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int32_t size, index;
	void* data = proto_read(in, &size);
	index = proto_read_int(in);

	if(dev != NULL && dev->write_block != NULL) {
		size = dev->write_block(from_pid, data, size, index, p);
		PF->addi(out, size);
	}
	else {
		PF->addi(out, -1);
	}
}

static void do_dma(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int fd = proto_read_int(in);
	uint32_t node = (uint32_t)proto_read_int(in);

	int shm_id = -1;	
	int size = 0;
	if(dev != NULL && dev->dma != NULL) {
		fsinfo_t* info = dev_get_file(fd, from_pid, node);
		if(info != NULL)
			shm_id = dev->dma(fd, from_pid, info, &size, p);
	}
	PF->addi(out, shm_id)->addi(out, size);
}

static void do_fcntl(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int fd = proto_read_int(in);
	uint32_t node = proto_read_int(in);
	int32_t cmd = proto_read_int(in);

	fsinfo_t* info = dev_get_file(fd, from_pid, node);
	if(info == NULL) {
		PF->addi(out, -1);
		return;
	}

	proto_t arg_in, arg_out;
	PF->init(&arg_out);

	int32_t arg_size;
	void* arg_data = proto_read(in, &arg_size);
	PF->init_data(&arg_in, arg_data, arg_size);

	int res = -1;
	if(dev != NULL && dev->fcntl != NULL) {
		res = dev->fcntl(fd, from_pid, info, cmd, &arg_in, &arg_out, p);
		if(res == 0)
			dev_update_file(fd, from_pid, info);
	}
	PF->clear(&arg_in);

	PF->addi(out, res)->
			add(out, info, sizeof(fsinfo_t))->
			add(out, arg_out.data, arg_out.size);
	PF->clear(&arg_out);
}

static void do_flush(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	(void)from_pid;
	int fd = proto_read_int(in);
	uint32_t node = (uint32_t)proto_read_int(in);
	fsinfo_t* info = dev_get_file(fd, from_pid, node);

	if(info == NULL) {
		PF->addi(out, -1)->addi(out, ENOENT);
		return;
	}
	/*if(vfs_check_access(from_pid, info, W_OK) != 0) {
		PF->addi(out, -1)->addi(out, EPERM);
		return;
	}
	*/

	int ret = 0;
	if(dev != NULL && dev->flush != NULL) {
		ret = dev->flush(fd, from_pid, info, p);
	}
	PF->addi(out, ret);
}

static void do_create(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	(void)from_pid;
	uint32_t node_to = proto_read_int(in);
	uint32_t node = proto_read_int(in);
	fsinfo_t info_to, info;

	if(vfs_get_by_node(node_to, &info_to) != 0) {
		PF->addi(out, -1)->addi(out, ENOENT);
		return;
	}

	if(vfs_get_by_node(node, &info) != 0) {
		PF->addi(out, -1)->addi(out, ENOENT);
		return;
	}

	if(vfs_check_access(from_pid, &info_to, W_OK) != 0) {
		PF->addi(out, -1)->addi(out, EPERM);
		return;
	}

	int res = 0;
	if(dev != NULL && dev->create != NULL)
		res = dev->create(from_pid, &info_to, &info, p);

	if(res == 0) {
		PF->addi(out, res)->add(out, &info, sizeof(fsinfo_t));
		return;
	}		
	PF->addi(out, -1);
}

static void do_unlink(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	(void)from_pid;
	uint32_t node = proto_read_int(in);
	const char* fname = proto_read_str(in);

	fsinfo_t info;
	if(vfs_get_by_node(node, &info) != 0) {
		PF->addi(out, -1)->addi(out, ENOENT);
		return;
	}
	
	if(vfs_check_access(from_pid, &info, W_OK) != 0) {
		PF->addi(out, -1)->addi(out, EPERM);
		return;
	}

	int res = 0;
	if(dev != NULL && dev->unlink != NULL)
		res = dev->unlink(&info, fname, p);
	else if(info.type != FS_TYPE_FILE && info.type != FS_TYPE_DIR) {
		PF->addi(out, -1)->addi(out, EPERM);
		return;
	}
	else
		res = vfs_del_node(info.node);
	PF->addi(out, res);
}

static void do_set(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	fsinfo_t info_old, info;
	proto_read_to(in, &info, sizeof(fsinfo_t));
	if(vfs_get_by_node(info.node, &info_old) != 0) {
		PF->addi(out, -1)->addi(out, ENOENT);
		return;
	}
	
	if(vfs_check_access(from_pid, &info_old, W_OK) != 0) {
		PF->addi(out, -1)->addi(out, EPERM);
		return;
	}

	int res = 0;
	if(dev != NULL && dev->set != NULL)
		res = dev->set(from_pid, &info, p);
	PF->addi(out, res);
}

static char* read_cmd_arg(char* cmd, int* offset) {
	char* p = NULL;
	uint8_t quotes = 0;

	while(cmd[*offset] != 0) {
		char c = cmd[*offset];
		(*offset)++;
		if(quotes) { //read whole quotes content.
			if(c == '"') {
				cmd[*offset-1] = 0;
				return p;
			}
			continue;
		}
		if(c == ' ') { //read next arg.
			if(p == NULL) //skip begin spaces.
				continue;
			cmd[*offset-1] = 0;
			break;
		}
		else if(p == NULL) {
			if(c == '"') { //if start of quotes.
				quotes = 1;
				(*offset)++;
			}
			p = cmd + *offset - 1;
		}
	}
	return p;
}

#define ARG_MAX 16

static char* gen_str(const char* s) {
	char* res = (char*)malloc(strlen(s)+1);
	strcpy(res, s);
	return res;
}

static char* do_basic_cmd(vdevice_t* dev, int argc, char** argv) {
	if(strcmp(argv[0], "dev.echo") == 0) {
		if(argc > 1)
			return gen_str(argv[1]);
		else
			return gen_str("");
	}
	return NULL;
}

static void do_cmd(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	const char* cmd = proto_read_str(in);
	if(cmd == NULL || cmd[0] == 0)
		return;

	char* argv[ARG_MAX] = {0};
	int argc = 0;
	int offset = 0;

	while(argc < ARG_MAX) {
		char* arg = read_cmd_arg((char*)cmd, &offset); 
		if(arg == NULL || arg[0] == 0)
			break;
		argv[argc] = (char*)malloc(strlen(arg)+1);
		strcpy(argv[argc], arg);
		argc++;
	}

	if(argc == 0)
		return;

	char* res = do_basic_cmd(dev, argc, argv);
	if(res == NULL && dev != NULL && dev->cmd != NULL)
		res = dev->cmd(from_pid, argc, argv, p);

	argc = 0;
	while(argc < ARG_MAX) {
		if(argv[argc] != NULL)
			free(argv[argc]);
		argc++;
	}

	if(res != NULL) {
		PF->adds(out, res);
		free(res);
	}
}

static void do_clear_buffer(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	(void)from_pid;
	uint32_t node = proto_read_int(in);

	int res = -1;
	if(dev != NULL && dev->clear_buffer != NULL)
		res = dev->clear_buffer(node, p);
	PF->addi(out, res);
}

static void do_dev_cntl(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	PF->addi(out, -1);
	if(dev == NULL || dev->dev_cntl == NULL)
		return;

	int cmd = proto_read_int(in);
	int32_t  sz;
	void* data = proto_read(in, &sz);
	
	proto_t in_arg, ret;
	PF->init(&in_arg);
	PF->init(&ret);
	if(data != NULL && sz > 0) 
		PF->copy(&in_arg, data, sz);

	if(dev->dev_cntl(from_pid, cmd, &in_arg, &ret, p) == 0) {
		PF->clear(out)->addi(out, 0)->add(out, ret.data, ret.size);
	}
	PF->clear(&in_arg);
	PF->clear(&ret);
}

static void do_interrupt(vdevice_t* dev, proto_t *in, void* p) {
	if(dev != NULL && dev->interrupt != NULL) {
		dev->interrupt(in, p);
	}
}

static void handle(int from_pid, int cmd, proto_t* in, proto_t* out, void* p) {
	vdevice_t* dev = (vdevice_t*)p;
	if(dev == NULL)
		return;
	PF->clear(out);

	p = dev->extra_data;
	switch(cmd) {
	case FS_CMD_OPEN:
		do_open(dev, from_pid, in, out, p);
		break;
	case FS_CMD_CLOSE:
		do_close(dev, from_pid, in, out, p);
		break;
	case FS_CMD_READ:
		do_read(dev, from_pid, in, out, p);
		break;
	case FS_CMD_WRITE:
		do_write(dev, from_pid, in, out, p);
		break;
	case FS_CMD_READ_BLOCK:
		do_read_block(dev, from_pid, in, out, p);
		break;
	case FS_CMD_WRITE_BLOCK:
		do_write_block(dev, from_pid, in, out, p);
		break;
	case FS_CMD_DMA:
		do_dma(dev, from_pid, in, out, p);
		break;
	case FS_CMD_FLUSH:
		do_flush(dev, from_pid, in, out, p);
		break;
	case FS_CMD_CNTL:
		do_fcntl(dev, from_pid, in, out, p);
		break;
	case FS_CMD_CREATE:
		do_create(dev, from_pid, in, out, p);
		break;
	case FS_CMD_UNLINK:
		do_unlink(dev, from_pid, in, out, p);
		break;
	case FS_CMD_SET:
		do_set(dev, from_pid, in, out, p);
		break;
	case FS_CMD_CMD:
		do_cmd(dev, from_pid, in, out, p);
		break;
	case FS_CMD_CLEAR_BUFFER:
		do_clear_buffer(dev, from_pid, in, out, p);
		break;
	case FS_CMD_INTERRUPT:
		do_interrupt(dev, in, p);
		break;
	case FS_CMD_DEV_CNTL:
		do_dev_cntl(dev, from_pid, in, out, p);
		break;
	}
}

static int do_mount(vdevice_t* dev, fsinfo_t* mnt_point, int type, int mode) {
	fsinfo_t info;
	memset(&info, 0, sizeof(fsinfo_t));

	//create a non-father node 
	strcpy(info.name, mnt_point->name);
	info.type = type;
	if(type == FS_TYPE_DIR)
		info.stat.size = 1024;
	info.stat.uid = getuid();
	info.stat.gid = getgid();
	info.stat.mode = mode;
	vfs_new_node(&info, 0, true); // 0 means no father node

	if(dev->mount != NULL) { //do device mount precess
		if(dev->mount(&info, dev->extra_data) != 0) {
			vfs_del_node(info.node);
			return -1;
		}
	}

	//mount the new node to mnt_point, previous node will be saved as well
	if(vfs_mount(mnt_point->node, info.node) != 0) {
		vfs_del_node(info.node);
		return -1;
	}
	memcpy(mnt_point, &info, sizeof(fsinfo_t));
	return 0;
}

static void sig_stop(int sig_no, void* p) {
  (void)sig_no;
  vdevice_t* dev = (vdevice_t*)p;
  dev->terminated = true;
}

void device_stop(vdevice_t* dev) {
	if(dev == NULL)
		return;

	dev->terminated = true;
	if(dev->loop_step == NULL)
		proc_wakeup((uint32_t)dev);
}

int device_run(vdevice_t* dev, const char* mnt_point, int mnt_type, int mode) {
	if(dev == NULL)
		return -1;
	device_init(dev);

	sys_signal(SYS_SIG_STOP, sig_stop, dev);
	
	fsinfo_t mnt_point_info;
	if(mnt_point != NULL) {
		if(vfs_get_by_name(mnt_point, &mnt_point_info) != 0) {
			if(vfs_create(mnt_point, &mnt_point_info, mnt_type & FS_TYPE_MASK, mode, true, true) != 0)
				return -1;
		}

		if(do_mount(dev, &mnt_point_info, mnt_type, mode) != 0)
			return -1;
	}

	int ipc_flags = 0;

	//if(dev->loop_step != NULL) 
	ipc_flags |= IPC_NON_BLOCK;
	ipc_serv_run(handle, dev->handled, dev, ipc_flags);

	while(!dev->terminated) {
		if(dev->loop_step != NULL) {
			dev->loop_step(dev->extra_data);
		}
		else {
			proc_block_by(getpid(), (uint32_t)dev);
		}
	}

	if(mnt_point != NULL && dev->umount != NULL) {
		dev->umount(mnt_point_info.node, dev->extra_data);
	}
	vfs_umount(mnt_point_info.node);
	hashmap_free(_files_hash);
	return 0;
}

int dev_cntl_by_pid(int pid, int cmd, proto_t* in, proto_t* out) {
	proto_t in_arg;
	PF->init(&in_arg)->addi(&in_arg, cmd);

	if(in != NULL)
		PF->add(&in_arg, in->data, in->size);

	int res = -1;
	if(out != NULL) {
		proto_t ret;
		PF->init(&ret);
		res = ipc_call(pid, FS_CMD_DEV_CNTL, &in_arg, &ret);
		PF->clear(&in_arg);
		if(res != 0) {
			PF->clear(&ret);
			return -1;
		}

		res = proto_read_int(&ret);
		if(res != 0) {
			res = -1;
		}
		else {
			int32_t sz;
			void *data = proto_read(&ret, &sz);
			PF->copy(out, data, sz);
		}
		PF->clear(&ret);
	}
	else {
		res = ipc_call(pid, FS_CMD_DEV_CNTL, &in_arg, NULL);
		PF->clear(&in_arg);
	}
	return res;
}

char* dev_cmd_by_pid(int pid, const char* cmd) {
	proto_t in_arg;
	PF->init(&in_arg)->adds(&in_arg, cmd);

	int res = -1;
	proto_t out;
	PF->init(&out);
	res = ipc_call(pid, FS_CMD_CMD, &in_arg, &out);
	PF->clear(&in_arg);
	if(res != 0) {
		PF->clear(&out);
		return NULL;
	}

	const char* s = proto_read_str(&out);
	char* ret = NULL;
	if(s != NULL) {
		ret = (char*)calloc(1, strlen(s)+1);
		strcpy(ret, s);
	}
	PF->clear(&out);
	return ret;
}

int dev_get_pid(const char* fname) {
	fsinfo_t info;
	if(vfs_get_by_name(fname, &info) != 0)
		return -1;
	return info.mount_pid;
}

int dev_cntl(const char* fname, int cmd, proto_t* in, proto_t* out) {
	int pid = dev_get_pid(fname);
	if(pid < 0)
		return -1;
	return dev_cntl_by_pid(pid, cmd, in, out);
}

char* dev_cmd(const char* fname, const char* cmd) {
	int pid = dev_get_pid(fname);
	if(pid < 0)
		return NULL;
	return dev_cmd_by_pid(pid, cmd);
}

#ifdef __cplusplus
}
#endif

