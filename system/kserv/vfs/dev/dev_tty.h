#ifndef DEV_TTY_H
#define DEV_TTY_H

#include <types.h>
#include <kserv/fs.h>
#include <tree.h>

void mountTTY(TreeNodeT* node);

int readTTY(TreeNodeT* node, int seek, char* buf, uint32_t size);

int infoTTY(TreeNodeT* node, FSInfoT* info);

int writeTTY(TreeNodeT* node, int seek, const char* buf, uint32_t size);

#endif
