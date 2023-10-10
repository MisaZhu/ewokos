#ifndef SYSCALLS_H
#define SYSCALLS_H

#define DMA_MAGIC   0xCAFECAFF

enum {
	SYS_NONE = 0,
	SYS_KPRINT,           //1

	//proccess memory manage
	SYS_MALLOC,           //2
	SYS_MSIZE,            //3
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

	SYS_IPC_PING,        //20
	SYS_IPC_READY,  //21

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
	SYS_V2P,              //34
	SYS_P2V,              //35

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

	SYS_SAFE_SET,         //50
	SYS_SOFT_INT,         //51
	SYS_PROC_UUID,        //52
	SYS_CLOSE_KCONSOLE,             //53
	SYS_SCHD_CORE_LOCK,             //54
	SYS_SCHD_CORE_UNLOCK,             //55
	SYS_CALL_NUM          //56
};

#endif
