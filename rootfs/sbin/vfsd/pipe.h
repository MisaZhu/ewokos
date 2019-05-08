#ifndef PIPE_H
#define PIPE_H

#include <unistd.h>
#include <ipc.h>
#include <proto.h>

void pipes_init(void);
void pipes_clear(void);
void do_pipe_close(uint32_t node_addr, bool force);
void do_pipe_open(package_t* pkg);
void do_pipe_write(package_t* pkg);
void do_pipe_read(package_t* pkg);

#endif
