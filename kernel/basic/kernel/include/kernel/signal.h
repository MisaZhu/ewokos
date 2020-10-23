#ifndef PROC_SIGNAL_H
#define PROC_SIGNAL_H

#include <kernel/proc.h>

extern int32_t  proc_signal_setup(uint32_t entry);
extern void     proc_signal_send(context_t* ctx, proc_t* proc, int32_t sig_no);

#endif
