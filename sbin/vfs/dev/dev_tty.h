#ifndef DEV_TTY_H
#define DEV_TTY_H

#include <types.h>
#include <kserv/fs.h>
#include <tree.h>

int readTTY(TreeNodeT* node, int seek, char* buf, uint32_t size);

int writeTTY(TreeNodeT* node, int seek, const char* buf, uint32_t size);

#endif
