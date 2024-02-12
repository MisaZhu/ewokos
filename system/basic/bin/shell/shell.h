#ifndef SHELL_H
#define SHELL_H

#include <ewoksys/mstr.h>

extern bool _script_mode;
extern bool _terminated;

typedef struct st_old_cmd {
	str_t* cmd;
	struct st_old_cmd* next;
	struct st_old_cmd* prev;
} old_cmd_t;
extern old_cmd_t* _history;
extern old_cmd_t* _history_tail;

int32_t cmd_gets(int fd, str_t* buf);
int32_t handle_shell_cmd(const char* cmd);
void    add_history(const char* cmd);
void    free_history(void);

#endif