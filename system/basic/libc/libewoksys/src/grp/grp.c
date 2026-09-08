#include <grp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define GR_LINE_MAX 512
#define GR_MEM_MAX 32

static FILE *gr_fp = NULL;
static struct group gr_ent;
static char gr_line[GR_LINE_MAX];
static char *gr_mem[GR_MEM_MAX + 1];

/* Parse one "/etc/group" line (name:passwd:gid:mem1,mem2,...).
 * Pointers reference the static gr_line buffer, so the returned struct is
 * only valid until the next call. */
static struct group *gr_parse(char *line)
{
	char *s = line;
	char *nl;
	char *c;
	int n = 0;

	nl = strchr(line, '\n');
	if (nl) *nl = 0;

	gr_ent.gr_name = s;
	c = strchr(s, ':'); if (!c) return NULL; *c = 0; s = c + 1;
	gr_ent.gr_passwd = s;
	c = strchr(s, ':'); if (!c) return NULL; *c = 0; s = c + 1;
	gr_ent.gr_gid = (gid_t)atoi(s);
	c = strchr(s, ':');
	if (!c) { gr_mem[0] = NULL; gr_ent.gr_mem = gr_mem; return &gr_ent; }
	*c = 0; s = c + 1;

	while (n < GR_MEM_MAX && *s) {
		gr_mem[n++] = s;
		c = strchr(s, ',');
		if (c) { *c = 0; s = c + 1; }
		else break;
	}
	gr_mem[n] = NULL;
	gr_ent.gr_mem = gr_mem;
	return &gr_ent;
}

struct group *fgetgrent(FILE *stream)
{
	if (!stream) return NULL;
	while (fgets(gr_line, sizeof(gr_line), stream)) {
		if (gr_line[0] == '#' || gr_line[0] == '\n') continue;
		return gr_parse(gr_line);
	}
	return NULL;
}

void setgrent(void)
{
	if (gr_fp) fseek(gr_fp, 0, SEEK_SET);
	else gr_fp = fopen("/etc/group", "r");
}

void endgrent(void)
{
	if (gr_fp) { fclose(gr_fp); gr_fp = NULL; }
}

struct group *getgrent(void)
{
	if (!gr_fp) gr_fp = fopen("/etc/group", "r");
	if (!gr_fp) return NULL;
	return fgetgrent(gr_fp);
}

struct group *getgrnam(const char *name)
{
	struct group *g;
	if (!name) return NULL;
	setgrent();
	while ((g = getgrent()) != NULL) {
		if (g->gr_name && strcmp(g->gr_name, name) == 0) return g;
	}
	return NULL;
}

struct group *getgrgid(gid_t gid)
{
	struct group *g;
	setgrent();
	while ((g = getgrent()) != NULL) {
		if (g->gr_gid == gid) return g;
	}
	return NULL;
}
