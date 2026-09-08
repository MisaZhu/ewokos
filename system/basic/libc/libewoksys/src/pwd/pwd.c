#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PW_LINE_MAX 512

static FILE *pw_fp = NULL;
static struct passwd pw_ent;
static char pw_line[PW_LINE_MAX];

/* Parse one "/etc/passwd" line (name:passwd:uid:gid:gecos:dir:shell).
 * Field pointers reference the static pw_line buffer, so the returned
 * struct is only valid until the next call. */
static struct passwd *pw_parse(char *line)
{
	char *f[7];
	char *s = line;
	char *nl;
	int i, n = 0;

	nl = strchr(line, '\n');
	if (nl) *nl = 0;

	for (i = 0; i < 7; i++) {
		char *c = strchr(s, ':');
		f[i] = s;
		if (c) { *c = 0; s = c + 1; n = i + 1; }
		else { n = i + 1; break; }
	}
	for (i = n; i < 7; i++) f[i] = (char *)"";

	pw_ent.pw_name = f[0];
	pw_ent.pw_passwd = f[1];
	pw_ent.pw_uid = (uid_t)atoi(f[2]);
	pw_ent.pw_gid = (gid_t)atoi(f[3]);
	pw_ent.pw_gecos = f[4];
	pw_ent.pw_dir = f[5];
	pw_ent.pw_shell = f[6];
	return &pw_ent;
}

struct passwd *fgetpwent(FILE *stream)
{
	if (!stream) return NULL;
	while (fgets(pw_line, sizeof(pw_line), stream)) {
		if (pw_line[0] == '#' || pw_line[0] == '\n') continue;
		return pw_parse(pw_line);
	}
	return NULL;
}

void setpwent(void)
{
	if (pw_fp) fseek(pw_fp, 0, SEEK_SET);
	else pw_fp = fopen("/etc/passwd", "r");
}

void endpwent(void)
{
	if (pw_fp) { fclose(pw_fp); pw_fp = NULL; }
}

struct passwd *getpwent(void)
{
	if (!pw_fp) pw_fp = fopen("/etc/passwd", "r");
	if (!pw_fp) return NULL;
	return fgetpwent(pw_fp);
}

struct passwd *getpwnam(const char *name)
{
	struct passwd *p;
	if (!name) return NULL;
	setpwent();
	while ((p = getpwent()) != NULL) {
		if (p->pw_name && strcmp(p->pw_name, name) == 0) return p;
	}
	return NULL;
}

struct passwd *getpwuid(uid_t uid)
{
	struct passwd *p;
	setpwent();
	while ((p = getpwent()) != NULL) {
		if (p->pw_uid == uid) return p;
	}
	return NULL;
}
