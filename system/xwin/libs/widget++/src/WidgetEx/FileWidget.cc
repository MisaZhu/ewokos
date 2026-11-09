#include <Widget/Label.h>
#include <Widget/Grid.h>
#include <Widget/Scroller.h>
#include <WidgetEx/FileWidget.h>
#include <WidgetEx/Menu.h>
#include <WidgetEx/ConfirmDialog.h>
#include <Widget/WidgetWin.h>
#include <Widget/RootWidget.h>
#include <ewoksys/proc.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/keydef.h>
#include <tinyjson/tinyjson.h>
#include <graph/graph_image.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ewoksys/hashmap.h>
#include <iconbuf/iconbuf.h>
#include <clipboard/clipboard.h>

#include <x++/X.h>

using namespace Ewok;

#define MAX_FILES 1024
#define LOAD_BUDGET_MS 50 /*time budget per timer tick for progressive loading*/

/* clipboard format: "xfinder:copy\npath1\npath2\n..." or "xfinder:cut\n..." */
#define CLIP_PREFIX_COPY "xfinder:copy\n"
#define CLIP_PREFIX_CUT  "xfinder:cut\n"
#define CLIP_PREFIX_LEN  13

/* context menu item IDs */
enum {
	MENU_OPEN = 1,
	MENU_NEW_WINDOW,
	MENU_COPY,
	MENU_CUT,
	MENU_PASTE,
	MENU_DELETE,
	MENU_SELECT_ALL
};

#define LONG_PRESS_MS  500   /* long-press threshold in milliseconds */
#define LONG_PRESS_SLOP 10   /* max pixel movement to still count as hold */

class CWDLabel: public Label {
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.titleBGColor, true);
		font_t* font = theme->getFont();
		int y = r.y + (r.h-font_get_height(font, theme->basic.fontSize))/2;
		graph_draw_text_font(g, r.x+X::hidpiSize(4), y, label.c_str(), font, theme->basic.fontSize, theme->basic.titleColor);
	}
public:
	CWDLabel(const char* label) : Label(label) {}
};

static void freeIcon(void*p) {
	graph_t* g = (graph_t*)p;
	graph_free(g);
}

static int free_cache(map_t map, const char* key, any_t data, any_t arg) {
	graph_t* g = (graph_t*)data;
	if(g == NULL)
		return MAP_OK;

	hashmap_remove(map, key);
	graph_free(g);
	return MAP_OK;
}

class FileGrid: public Grid {
	FileWidget* fileWidget;
	struct dirent* files;
	uint32_t fileCap;
	char cwd[FS_FULL_NAME_MAX+1];
	json_var_t* fileTypes;
	graph_t* dirIcon;
	graph_t* fileIcon;
	graph_t* devIcon;
	map_t iconCache;
	DIR* dirp; /*directory handle for progressive loading*/
	uint32_t fileNum; /*number of entries loaded so far*/
	bool loading;

	CWDLabel *cwdLabel;

	/* long-press state */
	bool longPressPending;
	bool longPressTriggered;
	uint64_t longPressTime;
	int32_t longPressX, longPressY;

	/* multi-select state */
	bool* multiSel;     /* per-item selection bitmap */
	uint32_t multiSelCap;
	int32_t selAnchor;  /* anchor for Shift+click range select */
	Menu* ctxMenu;      /* right-click / long-press context menu */
	int32_t ctxIndex;   /* item index under the context menu */

	bool ensureFileCapacity(uint32_t need) {
		uint32_t newCap;
		struct dirent* newFiles;

		if(need <= fileCap)
			return true;

		newCap = fileCap == 0 ? 32 : fileCap;
		while(newCap < need && newCap < MAX_FILES)
			newCap *= 2;
		if(newCap < need)
			newCap = need;
		if(newCap > MAX_FILES)
			newCap = MAX_FILES;
		if(newCap < need)
			return false;

		newFiles = (struct dirent*)realloc(files, sizeof(struct dirent) * newCap);
		if(newFiles == NULL)
			return false;
		files = newFiles;

		/* grow multiSel in lockstep */
		bool* newSel = (bool*)realloc(multiSel, sizeof(bool) * newCap);
		if(newSel == NULL)
			return false;
		for(uint32_t i = multiSelCap; i < newCap; i++)
			newSel[i] = false;
		multiSel = newSel;
		multiSelCap = newCap;

		fileCap = newCap;
		return true;
	}

	void clearMultiSel() {
		for(uint32_t i = 0; i < fileNum; i++)
			multiSel[i] = false;
	}

	int32_t multiSelCount() {
		int32_t n = 0;
		for(uint32_t i = 0; i < fileNum; i++) {
			if(multiSel[i])
				n++;
		}
		return n;
	}

	void selectAll() {
		/* skip index 0 which is always ".." */
		for(uint32_t i = 1; i < fileNum; i++)
			multiSel[i] = true;
		update();
	}

	void selectRange(int32_t from, int32_t to) {
		if(from > to) {
			int32_t t = from;
			from = to;
			to = t;
		}
		if(from < 1) from = 1; /* skip ".." */
		if(to >= (int32_t)fileNum) to = (int32_t)fileNum - 1;
		for(int32_t i = from; i <= to; i++)
			multiSel[i] = true;
	}

	/* build newline-separated list of selected full paths (malloc'd) */
	char* buildSelectedPaths(int32_t* count) {
		int32_t n = multiSelCount();
		if(n == 0) {
			/* fallback: use itemSelected if no multi-selection */
			if(itemSelected > 0 && itemSelected < (int32_t)fileNum) {
				multiSel[itemSelected] = true;
				n = 1;
			} else {
				*count = 0;
				return NULL;
			}
		}
		uint32_t sz = 0;
		for(uint32_t i = 0; i < fileNum; i++) {
			if(multiSel[i])
				sz += strlen(cwd) + strlen(files[i].d_name) + 2;
		}
		sz += 1;
		char* buf = (char*)malloc(sz);
		if(buf == NULL) {
			*count = 0;
			return NULL;
		}
		buf[0] = 0;
		uint32_t off = 0;
		int32_t cnt = 0;
		for(uint32_t i = 0; i < fileNum; i++) {
			if(!multiSel[i])
				continue;
			if(cnt > 0)
				buf[off++] = '\n';
			const char* fp = getFullname(files[i].d_name);
			uint32_t len = strlen(fp);
			memcpy(buf + off, fp, len);
			off += len;
			cnt++;
		}
		buf[off] = 0;
		*count = cnt;
		return buf;
	}

	void doCopy() {
		int32_t count = 0;
		char* paths = buildSelectedPaths(&count);
		if(paths == NULL || count == 0) {
			if(paths) free(paths);
			return;
		}
		uint32_t total = CLIP_PREFIX_LEN + strlen(paths) + 1;
		char* buf = (char*)malloc(total);
		if(buf) {
			strcpy(buf, CLIP_PREFIX_COPY);
			strcat(buf, paths);
			clipboard_set_text(buf);
			free(buf);
		}
		free(paths);
		clearMultiSel();
		update();
	}

	void doCut() {
		int32_t count = 0;
		char* paths = buildSelectedPaths(&count);
		if(paths == NULL || count == 0) {
			if(paths) free(paths);
			return;
		}
		uint32_t total = CLIP_PREFIX_LEN + strlen(paths) + 1;
		char* buf = (char*)malloc(total);
		if(buf) {
			strcpy(buf, CLIP_PREFIX_CUT);
			strcat(buf, paths);
			clipboard_set_text(buf);
			free(buf);
		}
		free(paths);
		clearMultiSel();
		update();
	}

	void showError(const char* msg) {
		ConfirmDialog* dlg = new ConfirmDialog(msg);
		XTheme* theme = getTheme();
		uint32_t tw = 0, th = 0;
		if(theme && theme->getFont())
			font_text_size(msg, theme->getFont(), theme->basic.fontSize, &tw, &th);
		int32_t w = (int32_t)tw + X::hidpiSize(40);
		int32_t h = (int32_t)th + X::hidpiSize(60);
		if(w < X::hidpiSize(200)) w = X::hidpiSize(200);
		if(h < X::hidpiSize(80)) h = X::hidpiSize(80);
		dlg->popup(getWin(), w, h, "Error", XWIN_STYLE_NO_RESIZE);
		dlg->pop();
	}

	void doPaste() {
		char* clip = clipboard_get_text();
		if(clip == NULL || clip[0] == 0) {
			if(clip) free(clip);
			return;
		}

		bool isCut = false;
		const char* pathStart = NULL;
		if(strncmp(clip, CLIP_PREFIX_COPY, CLIP_PREFIX_LEN) == 0)
			pathStart = clip + CLIP_PREFIX_LEN;
		else if(strncmp(clip, CLIP_PREFIX_CUT, CLIP_PREFIX_LEN) == 0) {
			pathStart = clip + CLIP_PREFIX_LEN;
			isCut = true;
		}

		if(pathStart == NULL || pathStart[0] == 0) {
			free(clip);
			return;
		}

		/* parse paths and perform operations */
		char* pathsCopy = strdup(pathStart);
		free(clip);
		if(pathsCopy == NULL)
			return;

		char dstDir[FS_FULL_NAME_MAX+1];
		if(strcmp(cwd, "/") == 0)
			strcpy(dstDir, "/");
		else
			snprintf(dstDir, FS_FULL_NAME_MAX, "%s/", cwd);

		bool hasError = false;
		char* saveptr = NULL;
		char* tok = strtok_r(pathsCopy, "\n", &saveptr);
		while(tok != NULL) {
			if(isCut) {
				/* move: rename src to cwd/basename */
				const char* bname = strrchr(tok, '/');
				bname = bname ? bname + 1 : tok;
				char dst[FS_FULL_NAME_MAX+1];
				if(strcmp(cwd, "/") == 0)
					snprintf(dst, FS_FULL_NAME_MAX, "/%s", bname);
				else
					snprintf(dst, FS_FULL_NAME_MAX, "%s/%s", cwd, bname);
				if(rename(tok, dst) != 0)
					hasError = true;
			} else {
				/* copy: fork+exec /bin/cp -r <src> <dstdir> and wait */
				int pid = fork();
				if(pid == 0) {
					const char* args[5];
					args[0] = "/bin/cp";
					args[1] = "-r";
					args[2] = tok;
					args[3] = dstDir;
					args[4] = NULL;
					execv("/bin/cp", (char* const*)args);
					_exit(1);
				} else if(pid > 0) {
					int status = 0;
					waitpid(pid, &status, 0);
					if(WEXITSTATUS(status) != 0) {
						hasError = true;
					} else {
						/* verify destination was actually created */
						const char* bname = strrchr(tok, '/');
						bname = bname ? bname + 1 : tok;
						char dst[FS_FULL_NAME_MAX+1];
						snprintf(dst, FS_FULL_NAME_MAX, "%s%s", dstDir, bname);
						if(access(dst, F_OK) != 0)
							hasError = true;
					}
				} else {
					hasError = true;
				}
			}
			tok = strtok_r(NULL, "\n", &saveptr);
		}

		free(pathsCopy);
		if(isCut)
			clipboard_clear();

		if(hasError)
			showError("Paste failed: permission denied or target error");

		/* refresh directory listing */
		readDir(cwd);
		update();
	}

	void doDelete() {
		int32_t count = 0;
		char* paths = buildSelectedPaths(&count);
		if(paths == NULL || count == 0) {
			if(paths) free(paths);
			return;
		}

		/* fork+exec /bin/rm -rf for each selected path, wait for completion */
		bool hasError = false;
		char* saveptr = NULL;
		char* tok = strtok_r(paths, "\n", &saveptr);
		while(tok != NULL) {
			int pid = fork();
			if(pid == 0) {
				const char* args[4];
				args[0] = "/bin/rm";
				args[1] = "-rf";
				args[2] = tok;
				args[3] = NULL;
				execv("/bin/rm", (char* const*)args);
				_exit(1);
			} else if(pid > 0) {
				int status = 0;
				waitpid(pid, &status, 0);
				if(WEXITSTATUS(status) != 0)
					hasError = true;
				/* belt-and-suspenders: verify file is actually gone */
				else if(access(tok, F_OK) == 0)
					hasError = true;
			} else {
				hasError = true;
			}
			tok = strtok_r(NULL, "\n", &saveptr);
		}

		free(paths);
		clearMultiSel();

		if(hasError)
			showError("Delete failed: permission denied");

		readDir(cwd);
		update();
	}

	const char* getFullname(const char* dname) {
		static char fname[FS_FULL_NAME_MAX+1] = {0};
		if(strcmp(cwd, "/") == 0)
			snprintf(fname, FS_FULL_NAME_MAX, "/%s", dname);
		else
			snprintf(fname, FS_FULL_NAME_MAX, "%s/%s", cwd, dname);
		return fname;
	}
 
	void drawIcon(graph_t* g, int at, XTheme* theme, int x, int y, int w, int h) {
		graph_t* img = NULL;
		if(files[at].d_type == DT_DIR)
			img = dirIcon;
		else if(files[at].d_type == DT_CHR || files[at].d_type == DT_BLK)
			img = devIcon;
		else {
			img = getIcon(files[at].d_name);
			if(img == NULL)
				img = fileIcon;
		}

		int32_t sz = h - theme->basic.fontSize;
		int dx = (w - img->w)/2;
		int dy = (sz - img->h) / 2;
		graph_blt_alpha(img, 0, 0, img->w, img->h,
				g, x+dx, y+dy, img->w, img->h, 0xff);
	}

	void drawTitle(graph_t* g, int at, XTheme* theme, int x, int y, int w, int h) {
		const char* title = files[at].d_name;
		uint32_t tw, th;
		font_text_size(title, theme->getFont(), theme->basic.fontSize, &tw, &th);
		int xto = x + (w - (int32_t)tw)/2;
		if(xto > x)
			x = xto;
		
		font_t* font = theme->getFont();
		if(font == NULL)
			return;

		y += h - th;
		graph_draw_text_font(g, x, y, title, font, theme->basic.fontSize, theme->basic.fgColor);
	}

	void setCWD(const char* r) {
		strcpy(cwd, r);
		if(cwdLabel != NULL)
			cwdLabel->setLabel(cwd);
	}

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
			setCWD("/");

		fileWidget->enter(cwd);
		readDir(cwd);
		update();
	}

	bool check(const char* fname, const char* ext) {
		int i = strlen(fname) - strlen(ext);
		if(strcmp((fname+i), ext) == 0)
			return true;
		return false;
	}

	const char* fileType(const char* fname) {
		if(fileTypes == NULL)
			return "";
		int num = json_var_array_size(fileTypes);
		for(int i=0; i<num; i++) {
			json_var_t* it = json_var_array_get_var(fileTypes, i);
			if(it == NULL)
				break;

			const char* ext = json_get_str(it, "ext");
			const char* open_with = json_get_str(it, "open_with");
			if(ext[0] == 0 || open_with[0] == 0)
				break;
			if(check(fname, ext))
				return open_with;
		}
		return "";
	}

	void readDir(const char* r) {
		if(dirp != NULL) {
			closedir(dirp);
			dirp = NULL;
		}
		loading = false;
		dirp = opendir(r);
		if(dirp == NULL) {
			/*loading cannot start: clear a busy cursor that may have been
			  left on by a previous, still in-progress load*/
			getWin()->busy(false);
			return;
		}
		if(!ensureFileCapacity(1)) {
			closedir(dirp);
			dirp = NULL;
			getWin()->busy(false);
			return;
		}
		if(r != cwd) {
			setCWD(r);
		}
		getWin()->busy(true);

		strcpy(files[0].d_name, "..");
		files[0].d_type = DT_DIR;
		fileNum = 1;
		itemStart = 0;
		pixelOffset = 0;
		itemSelected = 0;
		selAnchor = -1;
		clearMultiSel();
		setItemNum(fileNum);
		updateScroller();
		loading = true;
	}

	/*progressive loading: read entries step by step inside the event
	  loop so each entry is painted as soon as it is read*/
	void readDirStep(void) {
		if(!loading || dirp == NULL)
			return;

		uint64_t t0 = kernel_tic_ms(0);
		bool done = false;
		while(fileNum < MAX_FILES) {
			struct dirent* it = readdir(dirp);
			if(it == NULL) {
				done = true; /*end of directory reached*/
				break;
			}
			if(it->d_name[0] == '.')
				continue;
			if(!ensureFileCapacity(fileNum + 1)) {
				done = true; /*cannot grow further, stop loading*/
				break;
			}
			memcpy(&files[fileNum], it, sizeof(struct dirent));

			if(it->d_type != DT_DIR && check(it->d_name, ".png")) {
				const char* fname = getFullname(it->d_name);
				getImgIconAndCache(fname);
			}
			fileNum++;
			setItemNum(fileNum);
			update();

			if((kernel_tic_ms(0) - t0) >= LOAD_BUDGET_MS)
				break; /*time budget used up, resume on the next tick*/
		}

		if(fileNum >= MAX_FILES)
			done = true;

		/*only close the handle once reading is truly finished; a break caused
		  by the time budget leaves dirp open so loading continues next tick.
		  Closing it here is what lets the completion block below run and clear
		  the busy cursor.*/
		if(done && dirp != NULL) {
			closedir(dirp);
			dirp = NULL;
		}

		if(dirp == NULL) {
			loading = false;
			itemSelected = 0;
			updateScroller();
			update();
			getWin()->busy(false);
		}
	}

	void loadBasicIcons() {
		char fname[FS_FULL_NAME_MAX+1] = {0};
		if(fileIcon == NULL) {
			x_get_theme_fname(X_THEME_ROOT, "xfinder", "icons/file.png", fname, FS_FULL_NAME_MAX);
			fileIcon = getImgIconAndCache(fname);
			if(fileIcon == NULL)
				fileIcon = getImgIconAndCache(X::getResFullName("icons/file.png").c_str());
		}

		if(dirIcon == NULL) {
			x_get_theme_fname(X_THEME_ROOT, "xfinder", "icons/folder.png", fname, FS_FULL_NAME_MAX);
			graph_free(dirIcon);
			dirIcon = getImgIconAndCache(fname);
			if(dirIcon == NULL)
				dirIcon = getImgIconAndCache(X::getResFullName("icons/folder.png").c_str());
		}

		if(devIcon == NULL) {
			x_get_theme_fname(X_THEME_ROOT, "xfinder", "icons/device.png", fname, FS_FULL_NAME_MAX);
			devIcon = getImgIconAndCache(fname);
			if(devIcon == NULL)
				devIcon = getImgIconAndCache(X::getResFullName("icons/device.png").c_str());
		}
	}

	json_var_t* loadFileTypes(const char* confFile) {
		int sz;
		char* str = (char*)vfs_readfile(confFile, &sz);
		if(str == NULL) 
			return NULL;
		str[sz] = 0;
		json_var_t* ret = json_parse_str(str);
		free(str);
		return ret;
	}

	graph_t*  getImgIconAndCache(const char* fname) {
		graph_t* icon =  NULL;
		hashmap_get(iconCache, fname, (void**)&icon);
		if(icon != NULL)
			return icon;

		int32_t size = itemW > itemH ? itemW : itemH;
		icon = get_icon(fname, size*2/3);
		if(icon == NULL)
			return NULL;
		hashmap_put(iconCache, fname, icon);
		return icon;
	}

	graph_t*  getIcon(const char* dname) {
		const char* fname = getFullname(dname);
		graph_t* img = NULL;
		if(check(fname, ".png") || check(fname, ".jpg") || 
				check(fname, ".gif") || check(fname, ".tga") || check(fname, ".svg")) {
			img = getImgIconAndCache(fname);
			if(img != NULL)
				return img;
		}

		if(fileTypes == NULL)
			return NULL;
		int num = json_var_array_size(fileTypes);
		for(int i=0; i<num; i++) {
			json_var_t* it = json_var_array_get_var(fileTypes, i);
			if(it == NULL)
				break;

			const char* ext = json_get_str(it, "ext");
			const char* icon = json_get_str(it, "icon");
			if(ext[0] == 0 || icon[0] == 0)
				continue;

			if(check(fname, ext)) {
				img = (graph_t*)json_get_raw(it, "icon_image");
				if(img != NULL)
					return img;

				char fname[FS_FULL_NAME_MAX+1] = {0};
				x_get_theme_fname(X_THEME_ROOT, "xfinder", icon, fname, FS_FULL_NAME_MAX);
				img = getImgIconAndCache(fname);
				if(img == NULL)
					return NULL;
				json_var_add(it, "icon_image", json_var_new_obj(img, freeIcon));
				return img;
			}
		}
		return NULL;
	}

protected:
	/*progressive loading driven by the event loop timer*/
	void onTimer(uint32_t timerFPS, uint32_t timerSteps) {
		readDirStep();

		/* long-press detection */
		if(longPressPending && !longPressTriggered) {
			uint64_t now = kernel_tic_ms(0);
			if((now - longPressTime) >= LONG_PRESS_MS) {
				longPressTriggered = true;
				longPressPending = false;
				/* select item under the press point */
				int sel = hitTest(longPressX, longPressY);
				if(sel > 0 && sel < (int32_t)fileNum) {
					if(!multiSel[sel]) {
						clearMultiSel();
						multiSel[sel] = true;
						selAnchor = sel;
						Grid::select(sel);
					}
					ctxIndex = sel;
				} else {
					ctxIndex = -1;
				}
				showContextMenu(longPressX, longPressY);
				update();
			}
		}
	}

	void drawBG(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill_rect(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);
	}

	void drawItem(graph_t* g, XTheme* theme, int32_t index, const grect_t& r) {
		if(index >= itemNum)
			return;

		/* highlight multi-selected items or the single focused item */
		if(index < (int32_t)fileNum && multiSel[index])
			graph_fill_round(g, r.x, r.y, r.w, r.h, X::hidpiSize(12), theme->basic.selectBGColor);
		else if(index == itemSelected && multiSelCount() == 0)
			graph_fill_round(g, r.x, r.y, r.w, r.h, X::hidpiSize(12), theme->basic.selectBGColor);

		drawIcon(g, index, theme, r.x , r.y, r.w, r.h);
		drawTitle(g, index, theme, r.x , r.y, r.w, r.h);
	}

	void onEnter(int i) {
		struct dirent* it = &files[i];
		if(strcmp(it->d_name, "..") == 0) {
			upBack();
			return;
		}
		const char *fname = getFullname(it->d_name);
		if(it->d_type == DT_DIR) {
			fileWidget->enter(fname);
			readDir(fname);
			update();
			return;
		}
		else {
			const char* prog = fileType(fname);
			fileWidget->load(fname, prog);
		}
	}

	void onSelect(int i) {
		struct dirent* it = &files[i];
		const char *fname = getFullname(it->d_name);
		fileWidget->select(fname);
	}

	/* hit-test: convert screen coords to item index */
	int hitTest(int32_t screenX, int32_t screenY) {
		gpos_t ipos = getInsidePos(screenX, screenY);
		uint32_t iw = (uint32_t)(area.w / (float)cols);
		int x = ipos.x / iw;
		int y = (ipos.y + pixelOffset) / itemH;
		return itemStart + y*cols + x;
	}

	/* override selectByMouse for Ctrl/Shift multi-select */
	void selectByMouse(xevent_t* ev) {
		if(ev->state != MOUSE_STATE_DOWN)
			return;
		if(ev->value.mouse.button == MOUSE_BUTTON_RIGHT)
			return; /* handled in onMouse for context menu */

		int sel = hitTest(ev->value.mouse.x, ev->value.mouse.y);
		if(sel < 0 || sel >= (int32_t)fileNum || sel == 0) {
			/* clicked empty area or "..": deselect all */
			clearMultiSel();
			selAnchor = -1;
			Grid::select(-1);
			return;
		}

		if(shiftHeld) {
			/* range select from anchor to current */
			if(selAnchor < 1)
				selAnchor = sel;
			clearMultiSel();
			selectRange(selAnchor, sel);
		} else if(ctrlHeld) {
			/* toggle individual item */
			multiSel[sel] = !multiSel[sel];
			selAnchor = sel;
		} else {
			/* normal click: clear multi-select, set single */
			clearMultiSel();
			selAnchor = sel;
		}
		Grid::select(sel);
	}

	/* override onMouse: right-click menu + long-press tracking */
	bool onMouse(xevent_t* ev) {
		if(ev->type != XEVT_MOUSE)
			return ListBase::onMouse(ev);

		/* right-click DOWN: show context menu immediately */
		if(ev->state == MOUSE_STATE_DOWN &&
				ev->value.mouse.button == MOUSE_BUTTON_RIGHT) {
			longPressPending = false;
			int sel = hitTest(ev->value.mouse.x, ev->value.mouse.y);
			if(sel > 0 && sel < (int32_t)fileNum) {
				if(!multiSel[sel]) {
					clearMultiSel();
					multiSel[sel] = true;
					selAnchor = sel;
					Grid::select(sel);
				}
				ctxIndex = sel;
			} else {
				ctxIndex = -1;
			}
			showContextMenu(ev->value.mouse.x, ev->value.mouse.y);
			update();
			return true;
		}

		/* block right-click CLICK / UP from triggering enter */
		if(ev->value.mouse.button == MOUSE_BUTTON_RIGHT) {
			if(ev->state == MOUSE_STATE_CLICK || ev->state == MOUSE_STATE_UP)
				return true; /* swallow */
		}

		/* left button DOWN: start long-press timer */
		if(ev->state == MOUSE_STATE_DOWN) {
			longPressPending = true;
			longPressTriggered = false;
			longPressTime = kernel_tic_ms(0);
			longPressX = ev->value.mouse.x;
			longPressY = ev->value.mouse.y;
		}

		/* DRAG/MOVE: cancel long-press if moved too far */
		if(ev->state == MOUSE_STATE_DRAG || ev->state == MOUSE_STATE_MOVE) {
			if(longPressPending) {
				int32_t dx = ev->value.mouse.x - longPressX;
				int32_t dy = ev->value.mouse.y - longPressY;
				if(dx*dx + dy*dy > LONG_PRESS_SLOP*LONG_PRESS_SLOP)
					longPressPending = false;
			}
		}

		/* UP: cancel pending long-press */
		if(ev->state == MOUSE_STATE_UP) {
			longPressPending = false;
		}

		/* CLICK: suppress enter if long-press was triggered or shift/ctrl held */
		if(ev->state == MOUSE_STATE_CLICK) {
			if(longPressTriggered) {
				longPressTriggered = false;
				return true; /* swallow the click, don't enter */
			}
			if(shiftHeld || ctrlHeld)
				return true; /* multi-select mode, don't enter */
		}

		return ListBase::onMouse(ev);
	}

	/* override onIM to add keyboard shortcuts */
	bool onIM(xevent_t* ev) {
		int32_t key = ev->value.im.value;

		/* track modifier state on both press and release */
		if(key == KEY_CTRL) {
			ctrlHeld = (ev->state == XIM_STATE_PRESS);
			return false;
		}
		if(key == KEY_LSHIFT || key == KEY_RSHIFT) {
			shiftHeld = (ev->state == XIM_STATE_PRESS);
			return false;
		}

		if(ev->state != XIM_STATE_PRESS)
			return false;

		bool ctrl = (ev->value.im.ctrl != 0) || ctrlHeld;
		bool shift = (ev->value.im.shift != 0) || shiftHeld;

		if(ctrl) {
			switch(key) {
			case 'c': case 'C':
				doCopy();
				return true;
			case 'x': case 'X':
				doCut();
				return true;
			case 'v': case 'V':
				doPaste();
				return true;
			case 'a': case 'A':
				selectAll();
				return true;
			}
		}

		/* Delete / Backspace key deletes selected files */
		if(key == KEY_BACKSPACE || key == 0x7F) {
			if(multiSelCount() > 0 || itemSelected > 0) {
				doDelete();
				return true;
			}
		}

		/* Shift+arrow for range selection */
		if(shift && (key == KEY_UP || key == KEY_DOWN ||
				key == KEY_LEFT || key == KEY_RIGHT)) {
			int32_t sel = itemSelected;
			if(key == KEY_UP) sel -= cols;
			else if(key == KEY_DOWN) sel += cols;
			else if(key == KEY_LEFT) sel--;
			else if(key == KEY_RIGHT) sel++;
			if(sel < 0) sel = 0;
			if(sel >= (int32_t)fileNum) sel = (int32_t)fileNum - 1;
			if(selAnchor < 0)
				selAnchor = itemSelected;
			clearMultiSel();
			selectRange(selAnchor, sel);
			Grid::select(sel);
			return true;
		}

		/* clear multi-select on normal arrow navigation */
		if(key == KEY_UP || key == KEY_DOWN ||
				key == KEY_LEFT || key == KEY_RIGHT) {
			clearMultiSel();
		}

		return Grid::onIM(ev);
	}

	void showContextMenu(int32_t screenX, int32_t screenY) {
		if(ctxMenu != NULL) {
			delete ctxMenu;
			ctxMenu = NULL;
		}
		ctxMenu = new Menu();
		ctxMenu->setItemSize(X::hidpiSize(24));

		bool hasSelection = (multiSelCount() > 0);
		bool hasClip = hasClipboardContent();

		if(hasSelection && ctxIndex > 0)
			ctxMenu->add(MENU_OPEN, "Open", NULL, NULL, menuCallback, this);
		ctxMenu->add(MENU_NEW_WINDOW, "New Window", NULL, NULL, menuCallback, this);
		if(hasSelection)
			ctxMenu->add(MENU_COPY, "Copy", NULL, NULL, menuCallback, this);
		if(hasSelection && ctxIndex > 0)
			ctxMenu->add(MENU_CUT, "Cut", NULL, NULL, menuCallback, this);
		if(hasClip)
			ctxMenu->add(MENU_PASTE, "Paste", NULL, NULL, menuCallback, this);
		if(hasSelection && ctxIndex > 0)
			ctxMenu->add(MENU_DELETE, "Delete", NULL, NULL, menuCallback, this);
		ctxMenu->add(MENU_SELECT_ALL, "Select All", NULL, NULL, menuCallback, this);

		uint32_t n = ctxMenu->getItemNum();
		if(n == 0) {
			delete ctxMenu;
			ctxMenu = NULL;
			return;
		}
		uint32_t h = n * ctxMenu->getItemSize();
		uint32_t w = X::hidpiSize(120);
		ctxMenu->popup(getWin(), screenX, screenY, w, h, "menu", XWIN_STYLE_NO_TITLE);
		ctxMenu->pop();
	}

	bool hasClipboardContent() {
		char* clip = clipboard_get_text();
		if(clip == NULL)
			return false;
		bool has = (strncmp(clip, CLIP_PREFIX_COPY, CLIP_PREFIX_LEN) == 0 ||
				strncmp(clip, CLIP_PREFIX_CUT, CLIP_PREFIX_LEN) == 0);
		free(clip);
		return has;
	}

	static void menuCallback(MenuItem* item, void* arg) {
		FileGrid* self = (FileGrid*)arg;
		switch(item->id) {
		case MENU_OPEN:
			if(self->ctxIndex >= 0 && self->ctxIndex < (int32_t)self->fileNum)
				self->onEnter(self->ctxIndex);
			break;
		case MENU_NEW_WINDOW: {
			int pid = fork();
			if(pid == 0) {
				proc_detach();
				proc_exec("/apps/xfinder/xfinder");
				_exit(0);
			}
			break;
		}
		case MENU_COPY:
			self->doCopy();
			break;
		case MENU_CUT:
			self->doCut();
			break;
		case MENU_PASTE:
			self->doPaste();
			break;
		case MENU_DELETE:
			self->doDelete();
			break;
		case MENU_SELECT_ALL:
			self->selectAll();
			break;
		}
		if(self->ctxMenu != NULL) {
			self->ctxMenu->hide();
		}
	}

	bool ctrlHeld;
	bool shiftHeld;

public:
	friend FileWidget;
	FileGrid(FileWidget* fileWidget) {
		this->fileWidget = fileWidget;
		files = NULL;
		fileCap = 0;
		cwd[0] = 0;
		cwdLabel = NULL;
		scrollerV = NULL;
		dirp = NULL;
		fileNum = 0;
		loading = false;
		iconCache = hashmap_new(0);
		fileIcon = NULL;
		dirIcon = NULL;
		devIcon = NULL;
		fileTypes = loadFileTypes("/usr/system/filetypes.json");
		multiSel = NULL;
		multiSelCap = 0;
		selAnchor = -1;
		ctxMenu = NULL;
		ctxIndex = -1;
		ctrlHeld = false;
		shiftHeld = false;
		longPressPending = false;
		longPressTriggered = false;
		longPressTime = 0;
		longPressX = 0;
		longPressY = 0;
	}

	~FileGrid() {
		if(dirp != NULL)
			closedir(dirp);
		if(files != NULL)
			free(files);
		if(multiSel != NULL)
			free(multiSel);
		if(ctxMenu != NULL)
			delete ctxMenu;
		if(iconCache != NULL) {
			hashmap_iterate(iconCache, free_cache, NULL);	
			hashmap_free(iconCache);
		}
	}

	void setCWDLabel(CWDLabel* l) {
		cwdLabel = l;
	}

	void setItemSize(int w, int h) {
		Grid::setItemSize(w, h);
		loadBasicIcons();
	}
};

void FileWidget::onFocus() {
	RootWidget* root = getRoot();
	root->focus(fileGrid);		
}

void FileWidget::build() {
	setType(Container::VERTICAL);
	setAlpha(false);

	CWDLabel* cwdLabel = new CWDLabel("/");
	cwdLabel->fix(0, X::hidpiSize(20));
	add(cwdLabel);

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	add(c);

	FileGrid* fgrid = new FileGrid(this);
	fgrid->setCWDLabel(cwdLabel);
	fgrid->setItemSize(itemSize, itemSize);
	c->add(fgrid);
	fileGrid = fgrid;

	Scroller* scrollerV = new Scroller();
	scrollerV->fix(X::hidpiSize(8), 0);
	c->add(scrollerV);
	fgrid->setScrollerV(scrollerV);
}

FileWidget::FileWidget(uint32_t sz) {
	itemSize = sz;
	build();
}


void FileWidget::load(const string& fname, const string& open_with) {
	file = fname;
	onFile(fname, open_with);
}

void FileWidget::loadDir(const string& dname) {
	FileGrid* fgrid = (FileGrid*)fileGrid;
	fgrid->readDir(dname.c_str());
	update();
}

void FileWidget::enter(const string& pathname) {
	path = pathname;
	onPath(pathname);
}

void FileWidget::select(const string& fname) {
	file = fname;
	onSelect(fname);
}

void FileWidget::refreshDir() {
	FileGrid* fgrid = (FileGrid*)fileGrid;
	fgrid->readDir(fgrid->cwd);
	update();
}
