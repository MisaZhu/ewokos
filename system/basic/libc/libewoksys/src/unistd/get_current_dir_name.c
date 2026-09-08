#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* GNU extension: return a malloc()'d copy of the current directory. */
char *get_current_dir_name(void)
{
	char buf[512];
	char *p;
	if (getcwd(buf, sizeof(buf)) == NULL) return NULL;
	p = (char *)malloc(strlen(buf) + 1);
	if (p) strcpy(p, buf);
	return p;
}
