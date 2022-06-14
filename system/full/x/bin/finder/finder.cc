#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sconf/sconf.h>
#include <upng/upng.h>
#include <fonts/fonts.h>
#include <x++/X.h>
#include <sys/keydef.h>
#include <dirent.h>


using namespace Ewok;

class Finder: public XWin {
	font_t* font;
	int     selected;
	int     start;
	static const int MAX_FILES = 256;

	char cwd[FS_FULL_NAME_MAX+1];
	struct dirent files[MAX_FILES];
	int nums;

	void upBack(void) {
		if(strcmp(cwd, "/") == 0)
			return;

		int len = strlen(cwd)  - 1;
		for(int i=len; i>=0; i--) {
			if(cwd[i] == '/') {
				cwd[i] = 0;
				break;
			}
		}
		if(cwd[0] == 0)
			strcpy(cwd, "/");

		readDir(cwd);
		repaint(true);
	}

	bool check(const char* fname, const char* ext) {
		int i = strlen(fname) - strlen(ext);
		if(strcmp((fname+i), ext) == 0)
			return true;
		return false;
	}

	void load(const char* fname) {
		char cmd[FS_FULL_NAME_MAX+1] = "";
		if(check(fname, ".nes")) {
			snprintf(cmd, FS_FULL_NAME_MAX, "/bin/x/emu %s", fname);
		}
		else if(check(fname, ".png")) {
			snprintf(cmd, FS_FULL_NAME_MAX, "/bin/x/png %s", fname);
		}
		if(cmd[0] == 0)
			return;

		int pid = fork();
		if(pid == 0)  {
			exec(cmd);
			exit(0);
		}
	}

	void runProc(int i) {
		struct dirent* it = &files[i];
		char fname[FS_FULL_NAME_MAX+1];
		if(strcmp(cwd, "/") == 0)
			snprintf(fname, FS_FULL_NAME_MAX, "/%s", it->d_name);
		else
			snprintf(fname, FS_FULL_NAME_MAX, "%s/%s", cwd, it->d_name);

		if(it->d_type == DT_DIR) {
			readDir(fname);
			repaint(true);
			return;
		}
		else if(strncmp(fname, "/bin/x/", 7) == 0) {
			int pid = fork();
			if(pid == 0)  {
				exec(fname);
				exit(0);
			}
		}
		else 
			load(fname);
	}

	void readDir(const char* r) {
		DIR* dirp = opendir(r);
		if(dirp == NULL)
			return;
		if(r != cwd)	
			strcpy(cwd, r);
		selected = 0;
		nums = 0;

		int i;
		for(i=0; i<MAX_FILES; i++) {
			struct dirent* it = readdir(dirp);
			if(it == NULL)
				break;
			memcpy(&files[i], it, sizeof(struct dirent));
		}
		closedir(dirp);
		nums = i;
	}

protected:
	void onRepaint(graph_t* g) {
		char name[FS_FULL_NAME_MAX+1];
		graph_clear(g, 0xff000000);
		int h = font->h + 4;
		snprintf(name, FS_FULL_NAME_MAX, "[%s]", cwd);
		graph_draw_text(g, 8, 4, name, font, 0xffffff00);

		for(int i=start; i<nums; i++) {
			struct dirent* it = &files[i];
			if(i == selected)
				graph_fill(g, 2, (i+1-start)*h, g->w-2, h, 0xff444444);

			uint32_t color = 0xffffffff;
			if(it->d_name[0] == '.')
				color = 0xff888888;

			if(it->d_type == DT_DIR)
				snprintf(name, FS_FULL_NAME_MAX, "[%s]", it->d_name);
			else
				strcpy(name, it->d_name);
			graph_draw_text(g, 8, (i+1-start)*h+4, name, font, color);
		}
	}

	void onEvent(xevent_t* ev) {
		xinfo_t xinfo;
		getInfo(xinfo);
		int h = font->h + 4;
		int lines = xinfo.wsr.h/h - 1;
		
		if(ev->type == XEVT_IM) {
			int key = ev->value.im.value;
			if(ev->state == XIM_STATE_PRESS) {
				if(key == KEY_UP)
					selected--;
				else if(key == KEY_DOWN)
					selected++;
				else
					return;
			}
			else {//RELEASE
				if(key == KEY_LEFT || key == KEY_BUTTON_B) {
					upBack();
					return;
				}
				else if(key == KEY_RIGHT || key == KEY_ENTER) {
					runProc(selected);
					return;
				}
			}

			if(selected >= nums)
				selected = nums-1;
			if(selected < 0)
				selected = 0;

			if(selected < start) {
				start -= lines;
				if(start < 0)
					start = 0;
			}
			else if((selected - start) >= lines) 
				start += lines - 1;
			repaint(true);
		}
	}
public:
	inline Finder() {
		selected = 0;
		start = 0;
		font = font_by_name("8x16");
		readDir("/");
	}

	inline ~Finder() {
	}

	bool readConfig(const char* fname) {
		sconf_t *conf = sconf_load(fname);	
		if(conf == NULL)
			return false;
		font = font_by_name(sconf_get(conf, "font"));
		sconf_free(conf);
		return true;
	}
};

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	Finder xwin;
	xwin.readConfig("/etc/x/finder.conf");

	X x;
	x.open(&xwin, 0,
			0,
			300,
			200,
			"finder",
			X_STYLE_NORMAL);

	xwin.setVisible(true);
	x.run(NULL);
	return 0;
}
