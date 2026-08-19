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
#include <stdlib.h>
#include <sys/wait.h>
#include <stdint.h>
#include <syscalls.h>
#include <sysinfo.h>
#include <ewoksys/syscall.h>
#include <ewoksys/devcmd.h>
#include <ewoksys/vfs.h>
#include <ewoksys/kernel_tic.h>
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

static void dbg_kout(const char *str) {
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
    dbg_kout(__func__);
   return -1;
}

int
_has_ext_stdout_stderr (void)
{
    dbg_kout(__func__);
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

    errno = 0;
    int flags = vfs_get_flags(fd);
    if(flags == -1)
            return -1;

    bool block = true;
    if(flags & O_NONBLOCK)
        block = false;

    int res = -1;
    if(FS_IS_TYPE(info.type, FS_TYPE_PIPE)) {
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
        /*
         * For shared device nodes, RD visibility is fd-local. Sleeping via the
         * fd-aware helper avoids clearing a node-global sticky bit that may
         * belong to a sibling descriptor.
         */
        vfs_block_by_fd(fd, VFS_EVT_RD);
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

    errno = 0;
    int flags = vfs_get_flags(fd);
    if(flags == -1)
            return -1;
    bool block = true;
    if(flags & O_NONBLOCK)
        block = false;

    int res = -1;
    size_t total_written = 0;
    if(FS_IS_TYPE(info.type, FS_TYPE_PIPE)) {
        while(1) {
            res = vfs_write_pipe(fd, info.node,
                    ((const char *)buf) + total_written,
                    size - total_written, block);
            if(res > 0) {
                total_written += (size_t)res;
                if(total_written >= size) {
                    res = (int)total_written;
                    break;
                }
                if(!block) {
                    res = (int)total_written;
                    break;
                }
                continue;
            }
            if(res == 0 || errno != EAGAIN)
                break;
            if(!block)
                break;
        }
        if(total_written > 0) {
            res = (int)total_written;
        }
        return res;
    }

    while(1) {
        res = vfs_write(fd, &info,
                ((const char *)buf) + total_written,
                size - total_written);
        if(res > 0) {
            total_written += (size_t)res;
            if(total_written >= size) {
                res = (int)total_written;
                break;
            }
            if(!block) {
                res = (int)total_written;
                break;
            }
            continue;
        }
        if(res == 0)
            break;

        if(errno != EAGAIN || !block)
            break;
        /*
         * Mirror the read-side fix: on shared device nodes, wait on this fd's
         * live WR visibility instead of clearing the node-global sticky bit.
         */
        vfs_block_by_fd(fd, VFS_EVT_WR);
    }
    if(total_written > 0) {
        res = (int)total_written;
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
                dbg_kout(" create error");
                return -1;
            }
            created = true;
        }
        else  {
            dbg_kout(" fsinfo err");
            return -1;
        }
    }

    fd = vfs_open(&info, oflag);
    if(fd < 0) {
        if(created)
            vfs_del_node(info.node);
        dbg_kout(" error");
        return -1;
    }

    uint32_t type = FS_BASE_TYPE(info.type);
    bool needs_dev_open = true;
    if((type == FS_TYPE_FILE || type == FS_TYPE_DIR || type == FS_TYPE_LINK) &&
            (oflag & O_TRUNC) == 0) {
        needs_dev_open = false;
    }

    if(needs_dev_open && dev_open(info.mount_pid, fd, &info, oflag) != 0) {
        vfs_close_info(fd);
        if(created)
            vfs_del_node(info.node);
        dbg_kout(" dev_err");
        fd = -1;
    }
    else if(vfs_set_by_fd(fd, &info) != 0) {
        vfs_close(fd);
        if(created)
            vfs_del_node(info.node);
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
  void *result;

  result = proc_malloc_expand(incr);
  if(incr > 0 && result == NULL) {
    errno = ENOMEM;
    return (void *)-1;
  }

  __heap_size += incr;
  __heap_ptr = result;
  if(incr > 0)
    memset(__heap_end, 0, incr);

  prev_heap_end = __heap_end;
  __heap_end += incr;

  return (void *) prev_heap_end;
}

void _libc_init(void){
    dbg_kout(__func__);	
    __heap_ptr = proc_malloc_expand(16384);
    __heap_end = __heap_ptr;
    __heap_size = 16384;	
    memset(__heap_ptr, 0, __heap_size);
}

void _libc_exit(void){
  //dbg_kout(__func__);	
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
  dbg_kout(__func__);
  errno = ENOSYS;
  return -1;
}

int
_unlink (const char *path)
{
  dbg_kout(__func__);
  fsinfo_t info;
  if(vfs_get_by_name(path, &info) != 0)
    return -1;
  if(dev_unlink(info.mount_pid, info.node, path) != 0)	
    return -1;
  return 0;
}

uint64_t get_kernel_usec(void) {
    vsyscall_info_t *vsys = (vsyscall_info_t *)syscall0(SYS_GET_VSYSCALL_INFO);
    if (vsys != NULL)
            return  vsys->kernel_usec;
    return 0;
}

int
_gettimeofday (struct timeval * tp, void * tzvp)
{
    dbg_kout(__func__);
    static uint32_t init_sec = 0;
    static uint32_t init_sec_tic = 0;
    static uint32_t last_try_tic = 0;

    struct timezone *tzp = tzvp;
    if (tp)
    {
        uint64_t now_usec = get_kernel_usec();
        uint32_t now_sec = now_usec / 1000000;

        int need_query = 0;
        if(init_sec == 0) {
            // Not yet synced: retry at most once every 5 seconds to avoid flooding timed.
            if(last_try_tic == 0 || (now_sec - last_try_tic) >= 5)
                need_query = 1;
        }
        else if((now_sec - init_sec_tic) > 600) {
            need_query = 1;
        }

        if(need_query) {
            last_try_tic = now_sec;
            proto_t out;
            PF->init(&out);
            int res = -1;
            if(dev_cmd_cntl("/dev/time", 0, NULL, &out) == 0) {
                res = proto_read_int(&out);
                if(res == 0) {
                    init_sec = proto_read_int(&out);
                    init_sec_tic = now_sec;
                }
            }
            PF->clear(&out);
        }

        tp->tv_usec = now_usec % 1000000;
        tp->tv_sec = now_sec - init_sec_tic + init_sec;
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
  dbg_kout(__func__);
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
  dbg_kout(__func__);
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
  fsinfo_t info;

  dbg_kout(__func__);
  if (vfs_get_by_fd(fd, &info) != 0) {
    errno = EBADF;
    return 0;
  }
  if (FS_IS_TYPE(info.type, FS_TYPE_CHAR)) {
    return 1;
  }
  errno = ENOTTY;
  return 0;
}

int
_system (const char *s)
{
      dbg_kout(__func__);
      return 0;
}

int
_rename (const char * oldpath, const char * newpath)
{
  dbg_kout(__func__);
#ifdef ARM_RDI_MONITOR
  ewokos_addr_t block[4];
  block[0] = (ewokos_addr_t)oldpath;
  block[1] = (ewokos_addr_t)strlen(oldpath);
  block[2] = (ewokos_addr_t)newpath;
  block[3] = (ewokos_addr_t)strlen(newpath);
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
  errno = ENOSYS;
  return -1;
#endif
}

void _exit(int err){
  dbg_kout(__func__);
}

void __malloc_init(void){
  //dbg_kout(__func__);
}

void __malloc_close(void){
  //dbg_kout(__func__);
}

void _kill(int pid, int sig){
    syscall2(SYS_SIGNAL, (ewokos_addr_t)pid, (ewokos_addr_t)sig);
}

void _fini(void){
  dbg_kout(__func__);
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
    dbg_kout(__func__);
    return proc_fork();
}

int _wait(int *status)
{
  dbg_kout(__func__);
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

#endif

void __sync_synchronize(void) {
#ifdef __aarch64__
    __asm__ __volatile__("isb" ::: "memory");
#elif defined(__arm__) && (__ARM_ARCH > 6)
    __asm__ __volatile__("isb" ::: "memory");
#else
    __asm__ __volatile__("" ::: "memory");
#endif
}

#if defined(__arm__)
/*
 * Bare-metal ARM toolchains in this tree do not ship libatomic, but the shm
 * pipe fast path and vfsd refcounting need 32-bit atomic ops. Provide the
 * helper symbols GCC lowers __atomic_* builtins (4 bytes) to.
 */
uint32_t __atomic_exchange_4(volatile void* ptr, uint32_t val, int memmodel) {
    volatile uint32_t* p = (volatile uint32_t*)ptr;
    uint32_t old;

    (void)memmodel;
#if (__ARM_ARCH >= 6)
    uint32_t tmp;
    __asm__ __volatile__(
            "dmb ish\n"
            "1: ldrex %0, [%2]\n"
            "strex %1, %3, [%2]\n"
            "cmp %1, #0\n"
            "bne 1b\n"
            "dmb ish\n"
            : "=&r"(old), "=&r"(tmp)
            : "r"(p), "r"(val)
            : "cc", "memory");
#else
    __asm__ __volatile__(
            "swp %0, %2, [%1]\n"
            : "=&r"(old)
            : "r"(p), "r"(val)
            : "memory");
#endif
    return old;
}

uint32_t __atomic_load_4(const volatile void* ptr, int memmodel) {
    const volatile uint32_t* p = (const volatile uint32_t*)ptr;
    uint32_t val;

    (void)memmodel;
    val = *p;
    __sync_synchronize();
    return val;
}

void __atomic_store_4(volatile void* ptr, uint32_t val, int memmodel) {
    volatile uint32_t* p = (volatile uint32_t*)ptr;

    (void)memmodel;
    __sync_synchronize();
    *p = val;
}

#if (__ARM_ARCH >= 6)
#define ATOMIC_FETCH_OP_4(name, op) \
    uint32_t __atomic_fetch_##name##_4(volatile void* ptr, uint32_t val, \
            int memmodel) { \
        volatile uint32_t* p = (volatile uint32_t*)ptr; \
        uint32_t old, tmp, res; \
        (void)memmodel; \
        __asm__ __volatile__( \
                "dmb ish\n" \
                "1: ldrex %0, [%3]\n" \
                op " %1, %0, %4\n" \
                "strex %2, %1, [%3]\n" \
                "cmp %2, #0\n" \
                "bne 1b\n" \
                "dmb ish\n" \
                : "=&r"(old), "=&r"(tmp), "=&r"(res) \
                : "r"(p), "r"(val) \
                : "cc", "memory"); \
        return old; \
    }

ATOMIC_FETCH_OP_4(add, "add")
ATOMIC_FETCH_OP_4(sub, "sub")
ATOMIC_FETCH_OP_4(and, "and")
ATOMIC_FETCH_OP_4(or, "orr")
ATOMIC_FETCH_OP_4(xor, "eor")
#undef ATOMIC_FETCH_OP_4

_Bool __atomic_compare_exchange_4(volatile void* ptr, void* expected,
        uint32_t desired, _Bool weak, int success_memmodel,
        int failure_memmodel) {
    volatile uint32_t* p = (volatile uint32_t*)ptr;
    uint32_t exp = *(volatile uint32_t*)expected;
    uint32_t old, res;

    (void)weak;
    (void)success_memmodel;
    (void)failure_memmodel;
    __asm__ __volatile__(
            "dmb ish\n"
            "1: ldrex %0, [%2]\n"
            "cmp %0, %3\n"
            "bne 2f\n"
            "strex %1, %4, [%2]\n"
            "cmp %1, #0\n"
            "bne 1b\n"
            "mov %1, #1\n"
            "b 3f\n"
            "2: clrex\n"
            "mov %1, #0\n"
            "3: dmb ish\n"
            : "=&r"(old), "=&r"(res)
            : "r"(p), "r"(exp), "r"(desired)
            : "cc", "memory");
    if(!res)
        *(volatile uint32_t*)expected = old;
    return res;
}
#else /* __ARM_ARCH < 6: no ldrex/strex, emulate with swp */
#define ATOMIC_FETCH_OP_4(name, expr) \
    uint32_t __atomic_fetch_##name##_4(volatile void* ptr, uint32_t val, \
            int memmodel) { \
        volatile uint32_t* p = (volatile uint32_t*)ptr; \
        uint32_t old, want; \
        (void)memmodel; \
        do { \
            old = *p; \
            want = (expr); \
            __asm__ __volatile__("swp %0, %2, [%1]\n" \
                    : "=&r"(want) : "r"(p), "r"(want) : "memory"); \
        } while(want != old); \
        return old; \
    }

ATOMIC_FETCH_OP_4(add, old + val)
ATOMIC_FETCH_OP_4(sub, old - val)
ATOMIC_FETCH_OP_4(and, old & val)
ATOMIC_FETCH_OP_4(or, old | val)
ATOMIC_FETCH_OP_4(xor, old ^ val)
#undef ATOMIC_FETCH_OP_4

_Bool __atomic_compare_exchange_4(volatile void* ptr, void* expected,
        uint32_t desired, _Bool weak, int success_memmodel,
        int failure_memmodel) {
    volatile uint32_t* p = (volatile uint32_t*)ptr;
    uint32_t exp = *(volatile uint32_t*)expected;
    uint32_t old;

    (void)weak;
    (void)success_memmodel;
    (void)failure_memmodel;
    if(*p != exp) {
        *(volatile uint32_t*)expected = *p;
        return 0;
    }
    __asm__ __volatile__("swp %0, %2, [%1]\n"
            : "=&r"(old)
            : "r"(p), "r"(desired)
            : "memory");
    if(old != exp) {
        *(volatile uint32_t*)expected = old;
        return 0;
    }
    return 1;
}
#endif /* __ARM_ARCH >= 6 */

/*
 * Legacy __sync_* builtins used by pthread: GCC lowers them to these
 * out-of-line libgcc helpers when the target arch has no suitable inline
 * sequence (no -march given, the tree default is armv4t). Modern GCC
 * dropped these helpers from libgcc, so provide them here on top of the
 * __atomic_* implementations above.
 */
#define __SYNC_SEQ_CST 5

uint32_t __sync_lock_test_and_set_4(volatile void* ptr, uint32_t val) {
    return __atomic_exchange_4(ptr, val, __SYNC_SEQ_CST);
}

void __sync_lock_release_4(volatile void* ptr) {
    __sync_synchronize();
    *(volatile uint32_t*)ptr = 0;
}

uint32_t __sync_fetch_and_add_4(volatile void* ptr, uint32_t val) {
    return __atomic_fetch_add_4(ptr, val, __SYNC_SEQ_CST);
}

uint32_t __sync_fetch_and_sub_4(volatile void* ptr, uint32_t val) {
    return __atomic_fetch_sub_4(ptr, val, __SYNC_SEQ_CST);
}

uint32_t __sync_fetch_and_and_4(volatile void* ptr, uint32_t val) {
    return __atomic_fetch_and_4(ptr, val, __SYNC_SEQ_CST);
}

uint32_t __sync_fetch_and_or_4(volatile void* ptr, uint32_t val) {
    return __atomic_fetch_or_4(ptr, val, __SYNC_SEQ_CST);
}

uint32_t __sync_fetch_and_xor_4(volatile void* ptr, uint32_t val) {
    return __atomic_fetch_xor_4(ptr, val, __SYNC_SEQ_CST);
}

_Bool __sync_bool_compare_and_swap_4(volatile void* ptr, uint32_t oldval,
        uint32_t newval) {
    uint32_t expected = oldval;
    return __atomic_compare_exchange_4(ptr, &expected, newval, 0,
            __SYNC_SEQ_CST, __SYNC_SEQ_CST);
}

uint32_t __sync_val_compare_and_swap_4(volatile void* ptr, uint32_t oldval,
        uint32_t newval) {
    volatile uint32_t* p = (volatile uint32_t*)ptr;
    uint32_t cur;

#if (__ARM_ARCH >= 6)
    uint32_t res;
    __asm__ __volatile__(
            "dmb ish\n"
            "1: ldrex %0, [%2]\n"
            "cmp %0, %3\n"
            "bne 2f\n"
            "strex %1, %4, [%2]\n"
            "cmp %1, #0\n"
            "bne 1b\n"
            "2: clrex\n"
            "dmb ish\n"
            : "=&r"(cur), "=&r"(res)
            : "r"(p), "r"(oldval), "r"(newval)
            : "cc", "memory");
#else
    do {
        cur = *p;
        if(cur != oldval)
            break;
        __asm__ __volatile__("swp %0, %2, [%1]\n"
                : "=&r"(cur) : "r"(p), "r"(newval) : "memory");
    } while(cur != oldval);
#endif
    return cur;
}
#undef __SYNC_SEQ_CST
#endif /* __arm__ */
