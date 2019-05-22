#ifndef PIPE_H
#define PIPE_H

#include <unistd.h>
#include <ipc.h>
#include <proto.h>

void pipes_init(void);
void pipes_clear(void);
void do_pipe_close(uint32_t node_addr, bool force);
int32_t do_pipe_open(int32_t pid, proto_t* out);
int32_t do_pipe_write(int32_t pid, proto_t* in, proto_t* out);
int32_t do_pipe_read(int32_t pid, proto_t* in, proto_t* out);

#endif
