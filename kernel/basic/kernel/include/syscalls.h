#ifndef SYSCALLS_H
#define SYSCALLS_H

enum {
	SYS_NONE = 0,
	SYS_KPRINT,           //1

	//proccess memory manage
	SYS_MALLOC,           //2
	SYS_REALLOC,          //3
	SYS_FREE,             //4

	SYS_EXEC_ELF,         //5
	SYS_FORK,             //6
	SYS_THREAD,           //7
	SYS_YIELD,            //8
	SYS_WAIT_PID,         //9
	SYS_USLEEP,           //10
	SYS_EXIT,             //11
	SYS_DETACH,           //12
	SYS_BLOCK,            //13
	SYS_WAKEUP,           //14
	SYS_SIGNAL_SETUP,     //15
	SYS_SIGNAL,           //16
	SYS_SIGNAL_END,       //17

	SYS_GET_PID,          //18
	SYS_GET_THREAD_ID,    //19

	SYS_PROC_PING,        //20
	SYS_PROC_READY_PING,  //21

	SYS_PROC_GET_CMD,     //22
	SYS_PROC_SET_CMD,     //23
	SYS_PROC_GET_UID,     //24
	SYS_PROC_SET_UID,     //25

	//share memory syscalls
	SYS_PROC_SHM_ALLOC,   //26
	SYS_PROC_SHM_MAP,     //27
	SYS_PROC_SHM_UNMAP,   //28
	SYS_PROC_SHM_REF,     //29

	SYS_GET_SYS_INFO,     //30
	SYS_GET_SYS_STATE,    //31
	SYS_GET_PROCS,        //32

	//map mmio memory for userspace access
	SYS_MEM_MAP,          //33
	//map kernel memory (just one page size) for userspace access
	SYS_KPAGE_MAP,        //34
	SYS_KPAGE_UNMAP,      //35

	//internal proccess communication
	SYS_IPC_SETUP,        //36
	SYS_IPC_CALL,         //37
	SYS_IPC_GET_ARG,      //38
	SYS_IPC_SET_RETURN,   //39
	SYS_IPC_GET_RETURN,   //40
	SYS_IPC_END,          //41
	SYS_IPC_DISABLE,      //42
	SYS_IPC_ENABLE,       //43

	SYS_CORE_READY,       //44
	SYS_CORE_PID,         //45
	SYS_GET_KEVENT,       //46
	SYS_GET_KERNEL_TIC,   //47

	SYS_INTR_SETUP,       //48
	SYS_INTR_END,         //49

	SYS_SAFE_SET          //50
};

#endif
