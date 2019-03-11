#include <vfs/vfs.h>
#include <syscallcode.h>
#include <syscall.h>


inline uint32_t vfsAdd(uint32_t nodeTo, const char* name, uint32_t size) {
	return (uint32_t)syscall3(SYSCALL_VFS_ADD, (int32_t)nodeTo, (int32_t)name, (int32_t)size);
}

inline int32_t vfsDel(uint32_t node) {
	return syscall1(SYSCALL_VFS_DEL, (int32_t)node);
}

inline int32_t vfsNodeInfo(uint32_t node, FSInfoT* info) {
	return syscall2(SYSCALL_VFS_INFO, (int32_t)node, (int32_t)info);
}

inline uint32_t vfsNodeByFD(int32_t pid, int32_t fd) {
	return (uint32_t)syscall2(SYSCALL_VFS_NODE_FD, pid, fd);
}

inline uint32_t vfsNodeByName(const char* fname) {
	return (uint32_t)syscall1(SYSCALL_VFS_NODE_NAME, (int32_t)fname);
}

inline uint32_t vfsMount(const char* fname, const char* devName, int32_t devIndex) {
	return (uint32_t)syscall3(SYSCALL_VFS_MOUNT, (int32_t)fname, (int32_t)devName, (int32_t)devIndex);
}

inline int32_t vfsUnmount(uint32_t node) {
	return syscall1(SYSCALL_VFS_UNMOUNT, (int32_t)node);
}


