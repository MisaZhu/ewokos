/* Support files for GNU libc.  Files in the system namespace go here.
   Files in the C namespace (ie those that do not start with an
   underscore) go in .c.  */

#include <_ansi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <reent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <syscalls.h>
#include <ewoksys/syscall.h>
#include <ewoksys/devcmd.h>
#include <ewoksys/vfs.h>
#include <ewoksys/core.h>

#define EBADF 9
#define ENOSYS 88
#define S_IFCHR 0020000
#define	S_IREAD		0000400
#define	S_IFREG 0100000
#define	ENOMEM 12
#define	EMFILE		24
#define	EINVAL 22
#define	EOVERFLOW	75

# define    SEEK_SET    0
# define    SEEK_CUR    1
# define    SEEK_END    2

#define IO_DEBUG 0


static int _strlen(const char* str){
	int  i = 0;
	while(*str++ != 0){
		i++;
	}
	return i;
}

static void hex2str(int val, char* buf){
	for(int i = 7; i >= 0; i--){
		char a = (val >> i*4)&0xf;	
		if(a < 10){
			*buf++=a+'0';
		}else{
			*buf++=a - 10 +'A';
		}
	}
}

static void int2str(int val, char* buf){
	int mask = 10000000;
	int start = 0;

	if(val < 0){
		*buf++ = '-';
		val = -val;
	}

	for(int i = 0; i < 8; i++){
		char a = (val/mask)%10;	
		if(a > 0)
			start = 1;
		if(start){
			*buf++=a+'0';
		}
		mask/=10;
	}
	if(!start){
		*buf = '0';
	}
}

static void kout(const char *str) {
#if IO_DEBUG
	int len = _strlen(str);
    syscall2(SYS_KPRINT, (ewokos_addr_t)str, (ewokos_addr_t)len);
	if(str[len-1]!='\n')
		syscall2(SYS_KPRINT, (ewokos_addr_t)"\n", 1);	
#endif
}

static void kout_str(const char *str) {
#if IO_DEBUG
	int len = _strlen(str);
    syscall2(SYS_KPRINT, (ewokos_addr_t)str, (ewokos_addr_t)len);
#endif
}

void kout_str_str(const char *a, const char* b) {
	kout_str(a);
	kout_str(" ");
	kout_str(b);
	kout_str("\n");
}

void kout_int(const char *lable, int value) {
	char buf[9] = {0};
	kout_str(lable);
	kout_str(":");
	int2str(value, buf);
	kout_str(buf);
	kout_str("\n");
}

void kout_hex(const char *lable, int value) {
	char buf[9] = {0};
	kout_str(lable);
	kout_str(":");
	hex2str(value, buf);
	kout_str(buf);
	kout_str("\n");
}

// int malloc_test(const char* func, int line){
// 	void* tbuf = malloc(16);
// 	kout_str("mtest: ");
// 	kout_str(func);
// 	kout_int(" ", line);
// 	kout_hex(" ", tbuf);
// 	free(tbuf);
// 	return !!tbuf;
// }

/* Forward prototypes.  */
int	_system		(const char *);
int	_rename		(const char *, const char *);
int	_isatty		(int);
clock_t _times		(struct tms *);
int	_gettimeofday	(struct timeval *, void *);
int	_unlink		(const char *);
int	_link		(const char *, const char *);
int	_stat		(const char *, struct stat *);
int	_fstat		(int, struct stat *);
void *	_sbrk		(ptrdiff_t);
pid_t	_getpid		(void);
int	_close		(int);
clock_t	_clock		(void);
int	_open		(const char *, int, ...);
int	_write		(int, const void *, size_t);
_off_t	_lseek		(int, _off_t, int);
int	_read		(int, void *, size_t);

static int	checkerror	(int);
static int	error		(int);
static int	get_errno	(void);

/* Semihosting utilities.  */
static void initialise_semihosting_exts (void);

/* Struct used to keep track of the file position, just so we
   can implement fseek(fh,x,SEEK_CUR).  */
struct fdent
{
  int handle;
  int pos;
};

#define MAX_OPEN_FILES 20

/* User file descriptors (fd) are integer indexes into 
   the openfiles[] array. Error checking is done by using
   findslot(). 

   This openfiles array is manipulated directly by only 
   these 5 functions:

	findslot() - Translate entry.
	newslot() - Find empty entry.
	initilise_monitor_handles() - Initialize entries.
	_swiopen() - Initialize entry.
	_close() - Handle stdout == stderr case.

   Every other function must use findslot().  */


static struct fdent* 	findslot	(int);
static int		newslot		(void);

/* Register name faking - works in collusion with the linker.  */
//register char * stack_ptr asm ("sp");


/* following is copied from libc/stdio/local.h to check std streams */
extern void   __sinit (struct _reent *);
#define CHECK_INIT(ptr) \
  do						\
    {						\
      if ((ptr) && !(ptr)->__cleanup)		\
	__sinit (ptr);				\
    }						\
  while (0)

static int monitor_stdin;
static int monitor_stdout;
static int monitor_stderr;

static int supports_ext_exit_extended = -1;
static int supports_ext_stdout_stderr = -1;

int
_has_ext_exit_extended (void)
{
    kout(__func__);
   return -1;
}

int
_has_ext_stdout_stderr (void)
{
	kout(__func__);
	return -1;
}

static int
get_errno (void)
{
#ifdef ARM_RDI_MONITOR
  return do_AngelSWI (AngelSWI_Reason_Errno, NULL);
#else
//  register int r0 asm("r0");
//  asm ("swi %a1" : "=r"(r0) : "i" (SWI_GetErrno));
//  return r0;
	return -1;
#endif
}

/* Set errno and return result. */
static int
error (int result)
{
  errno = get_errno ();
  return result;
}

/* Check the return and set errno appropriately. */
static int
checkerror (int result)
{
  if (result == -1)
    return error (-1);
  return result;
}

/* fd, is a valid user file handle.
   Translates the return of _swiread into
   bytes read. */
int __attribute__((weak))
_read (int fd, void * buf, size_t size)
{
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;

	int flags = vfs_get_flags(fd);
	if(flags == -1)
	 		return -1;

	bool block = true;
	if(flags & O_NONBLOCK)
		block = false;

	int res = -1;
	if(info.type == FS_TYPE_PIPE) {
		while(1) {
			res = vfs_read_pipe(fd, info.node, buf, size, block);
			if(res >= 0 || errno != EAGAIN)
				break;
			if(!block)
				break;
		}
		return res;
	}

	while(1) {
		res = vfs_read(fd, &info, buf, size);
		if(res >= 0)
			break;

		if(errno != EAGAIN || !block)
			break;
		proc_block_by(info.mount_pid, RW_BLOCK_EVT);
	}
	return res;
}

off_t
_lseek (int fd, off_t offset, int whence)
{
  	if(whence == SEEK_CUR) {
		int cur = vfs_tell(fd);
		if(cur < 0)
			cur = 0;
		offset += cur;
	}
	else if(whence == SEEK_END) {
		fsinfo_t info;
		int cur = 0;
		if(vfs_get_by_fd(fd, &info) == 0)
			cur = info.stat.size;
		offset += cur;
	}
	vfs_seek(fd, offset);
	return offset;
}

int __attribute__((weak))
_write (int fd, const void * buf, size_t size)
{
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;

	int flags = vfs_get_flags(fd);
	if(flags == -1)
	 		return -1;
	bool block = true;
	if(flags & O_NONBLOCK)
		block = false;

	int res = -1;
	if(info.type == FS_TYPE_PIPE) {
		while(1) {
			res = vfs_write_pipe(fd, info.node, buf, size, block);
			if(res >= 0 || errno != EAGAIN)
				break;
			if(!block)
				break;
		}
		return res;
	}

	while(1) {
		res = vfs_write(fd, &info, buf, size);
		if(res >= 0)
			break;

		if(errno != EAGAIN || !block)
			break;
		proc_block_by(info.mount_pid, RW_BLOCK_EVT);
	}
	return res;
}

int
_open (const char * fname, int oflag, ...)
{
  	int fd = -1;
	bool created = false;
	fsinfo_t info;
	if(vfs_get_by_name(fname, &info) != 0) {
		if((oflag & O_CREAT) != 0) {
			if(vfs_create(fname, &info, FS_TYPE_FILE, 0644, false, false) != 0){
				kout(" create error");
				return -1;
			}
			created = true;
		}
		else  {
			kout(" fsinfo err");
			return -1;
		}
	}

	fd = vfs_open(&info, oflag);
	if(fd < 0) {
		if(created)
			vfs_del_node(info.node);
		kout(" error");
		return -1;
	}

	if(dev_open(info.mount_pid, fd, &info, oflag) != 0) {
		vfs_close_info(fd);
		if(created)
			vfs_del_node(info.node);
		kout(" dev_err");
		fd = -1;
	}
	return fd;
}

/* fd, is a user file descriptor. */
int
_close (int fd)
{
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;

	int ret = vfs_close(fd);
	return ret;
}

pid_t __attribute__((weak))
_getpid (void)
{
  pid_t pid =  proc_getpid(-1);
  return pid;
}

void *__heap_ptr = NULL;
void *__heap_end = NULL;
unsigned __heap_size = 0;
void * __attribute__((weak))
_sbrk (ptrdiff_t incr)
{
  char *prev_heap_end;

  __heap_size += incr;
  __heap_ptr = proc_malloc_expand(incr);
  if(incr > 0)
  	memset(__heap_end, 0, incr);

  prev_heap_end = __heap_end;
  __heap_end += incr;
//   kout_int("sbrk", incr);
//   kout_int("pid", _getpid());
//   kout_int("current", __heap_size);

  return (void *) prev_heap_end;
}

void _libc_init(void){
	kout(__func__);	
	__heap_ptr = proc_malloc_expand(16384);
	__heap_end = __heap_ptr;
	__heap_size = 16384;	
	memset(__heap_ptr, 0, __heap_size);
}

void _libc_exit(void){
  //kout(__func__);	
}


int __attribute__((weak))
_fstat (int fd, struct stat * st)
{
  memset (st, 0, sizeof (* st));
  fsinfo_t info;
  if(vfs_get_by_fd(fd, &info) != 0)
    return -1;
  /* Return the file size. */
  st->st_uid = info.stat.uid;
  st->st_gid = info.stat.gid;
  st->st_size = info.stat.size;
  st->st_mode = info.stat.mode;
  st->st_atime = info.stat.atime;
  st->st_ctime = info.stat.ctime;
  st->st_mtime = info.stat.mtime;
  st->st_nlink = info.stat.links_count;
  return 0;
}

int __attribute__((weak))
_stat (const char *fname, struct stat *st)
{
  memset (st, 0, sizeof (* st));
  /* The best we can do is try to open the file readonly.  If it exists,
     then we can guess a few things about it.  */
	fsinfo_t info;
	if(vfs_get_by_name(fname, &info) != 0) {
    return -1;
	}
  st->st_uid = info.stat.uid;
  st->st_gid = info.stat.gid;
  st->st_size = info.stat.size;
  st->st_mode = info.stat.mode;
  st->st_atime = info.stat.atime;
  st->st_ctime = info.stat.ctime;
  st->st_mtime = info.stat.mtime;
  st->st_nlink = info.stat.links_count;
  /* Not interested in the error.  */
  return 0;
}

int __attribute__((weak))
_link (const char *__path1 __attribute__ ((unused)), const char *__path2 __attribute__ ((unused)))
{
  kout(__func__);
  errno = ENOSYS;
  return -1;
}

int
_unlink (const char *path)
{
  kout(__func__);
  fsinfo_t info;
  if(vfs_get_by_name(path, &info) != 0)
	return -1;
  if(dev_unlink(info.mount_pid, info.node, path) != 0)	
	return -1;
  return 0;
}

int
_gettimeofday (struct timeval * tp, void * tzvp)
{
  kout(__func__);
  struct timezone *tzp = tzvp;
  if (tp)
    {
    /* Ask the host for the seconds since the Unix epoch.  */
#ifdef ARM_RDI_MONITOR
      tp->tv_sec = do_AngelSWI (AngelSWI_Reason_Time,NULL);
#else
      {
        int value;
        //asm ("swi %a1; mov %0, r0" : "=r" (value): "i" (SWI_Time) : "r0");
        tp->tv_sec = value;
      }
#endif
      tp->tv_usec = 0;
    }

  /* Return fixed data for the timezone.  */
  if (tzp)
    {
      tzp->tz_minuteswest = 0;
      tzp->tz_dsttime = 0;
    }

  return 0;
}

/* Return a clock that ticks at 100Hz.  */
clock_t 
_clock (void)
{
  kout(__func__);
  clock_t timeval;

#ifdef ARM_RDI_MONITOR
  timeval = do_AngelSWI (AngelSWI_Reason_Clock,NULL);
#else
  //asm ("swi %a1; mov %0, r0" : "=r" (timeval): "i" (SWI_Clock) : "r0");
#endif
  return timeval;
}

/* Return a clock that ticks at 100Hz.  */
clock_t
_times (struct tms * tp)
{
  kout(__func__);
  clock_t timeval = _clock();

  if (tp)
    {
      tp->tms_utime  = timeval;	/* user time */
      tp->tms_stime  = 0;	/* system time */
      tp->tms_cutime = 0;	/* user time, children */
      tp->tms_cstime = 0;	/* system time, children */
    }

  return timeval;
};


int
_isatty (int fd)
{
  kout(__func__);
  errno = get_errno ();
  return 0;
}

int
_system (const char *s)
{
	  kout(__func__);
	  return 0;
}

int
_rename (const char * oldpath, const char * newpath)
{
  kout(__func__);
#ifdef ARM_RDI_MONITOR
  int block[4];
  block[0] = (int)oldpath;
  block[1] = strlen(oldpath);
  block[2] = (int)newpath;
  block[3] = strlen(newpath);
  return checkerror (do_AngelSWI (AngelSWI_Reason_Rename, block)) ? -1 : 0;
#else
 // register int r0 asm("r0");
 // register int r1 asm("r1");
 // r0 = (int)oldpath;
 // r1 = (int)newpath;
 // asm ("swi %a3" 
 //      : "=r" (r0)
 //      : "0" (r0), "r" (r1), "i" (SWI_Rename));
 // return checkerror (r0);
#endif
}

void _exit(int err){
  kout(__func__);
}

void __malloc_init(void){
  //kout(__func__);
}

void __malloc_close(void){
  //kout(__func__);
}

void _kill(int pid, int sig){
	syscall2(SYS_SIGNAL, (ewokos_addr_t)pid, (ewokos_addr_t)sig);
}

void _fini(void){
  kout(__func__);
}

int
_execve(const char *name, char *const argv[], char *const env[])
{
	return proc_exec(name);
}

int execl(const char *name, const char* arg0, ...) {
	return proc_exec(name);
}

int _fork()
{
  	kout(__func__);
	return proc_fork();
}

int _wait(int *status)
{
  kout(__func__);
  errno = EAGAIN;
  return -1;
}

void __aeabi_unwind_cpp_pr0(void) {
}

void __aeabi_unwind_cpp_pr1(void) {
}

#if __aarch64__
double __trunctfdf2(_Float128 x) {
    return (double)x;
}

float __trunctfsf2(_Float128 a) {
	return (float)a;
}

_Float128 __extenddftf2(double a) {
    return (_Float128)a;
}

_Float128 __extendsftf2(float a) {
    return (_Float128)a;
}

_Float128 __addtf3(_Float128 a, _Float128 b) {
	return a + b;
}


int access(const char *path, int mode){
	fsinfo_t info;
	if(vfs_get_by_name(path, &info) != 0)
		return -1;
	return vfs_check_access(getpid(), &info, mode);
}

#endif