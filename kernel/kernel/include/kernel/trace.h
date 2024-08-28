#ifndef TRACE_H
#define TRACE_H

#include <kernel/context.h>

#ifdef SCHD_TRACE

typedef struct {
	int32_t pid;
	context_t ctx;
} trace_t;

extern uint32_t get_trace(trace_t *traces);
extern uint32_t get_trace_fps(void);
extern void update_trace(uint32_t usec_gap);
extern void pause_trace(void);
extern void resume_trace(void);
#endif

#endif
