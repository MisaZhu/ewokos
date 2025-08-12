#include <Widget/Label.h>
#include <Widget/Grid.h>
#include <Widget/Scroller.h>
#include <WidgetEx/FileWidget.h>
#include <Widget/RootWidget.h>
#include <ewoksys/proc.h>
#include <tinyjson/tinyjson.h>
#include <graph/graph_png.h>
#include <dirent.h>
#include <unistd.h>

#include <x++/X.h>

using namespace Ewok;

#define MAX_FILES 1024

class CWDLabel: public Label {
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.titleBGColor, true);
		font_t* font = theme->getFont();
		int y = r.y + (r.h-font_get_height(font, theme->basic.fontSize))/2;
		graph_draw_text_font(g, r.x+4, y, label.c_str(), font, theme->basic.fontSize, theme->basic.titleColor);
	}
public:
	CWDLabel(const char* label) : Label(label) {}
};

static void freeIcon(void*p) {
	graph_t* g = (graph_t*)p;
	graph_free(g);
}

class FileGrid: public Grid {
	FileWidget* fileWidget;
	struct dirent files[MAX_FILES];
	char cwd[FS_FULL_NAME_MAX+1];
	json_var_t* fileTypes;
	graph_t* dirIcon;
	graph_t* fileIcon;
	graph_t* devIcon;

	CWDLabel *cwdLabel;
 
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

		uint32_t sz = h - theme->basic.fontSize;
		int dx = (w - img->w)/2;
		int dy = (sz-img->h) / 2;
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
		DIR* dirp = opendir(r);
		if(dirp == NULL)
			return;
		if(r != cwd) {
			setCWD(r);
		}

		strcpy(files[0].d_name, "..");
		files[0].d_type = DT_DIR;

		int i;
		int num = 1;
		for(i=1; i<MAX_FILES; i++) {
			struct dirent* it = readdir(dirp);
			if(it == NULL)
				break;
			if(it->d_name[0] == '.')
				continue;
			memcpy(&files[num], it, sizeof(struct dirent));
			num++;
		}
		closedir(dirp);
		setItemNum(num);
		itemStart = 0;
		itemSelected = 0;
		updateScroller();
	}

	void loadIcons() {
		fileIcon = png_image_new(x_get_theme_fname(X_THEME_ROOT, "xfinder", "icons/file.png"));
		if(fileIcon == NULL)
			fileIcon = png_image_new(X::getResName("icons/file.png"));

		dirIcon = png_image_new(x_get_theme_fname(X_THEME_ROOT, "xfinder", "icons/folder.png"));
		if(dirIcon == NULL)
			dirIcon = png_image_new(X::getResName("icons/folder.png"));

		devIcon = png_image_new(x_get_theme_fname(X_THEME_ROOT, "xfinder", "icons/device.png"));
		if(devIcon == NULL)
			devIcon = png_image_new(X::getResName("icons/device.png"));
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

	graph_t*  getIcon(const char* fname) {
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
				graph_t* img = (graph_t*)json_get_raw(it, "icon_image");
				if(img != NULL)
					return img;

				img = png_image_new(x_get_theme_fname(X_THEME_ROOT, "xfinder", icon));
				if(img == NULL)
					return NULL;
				json_var_add(it, "icon_image", json_var_new_obj(img, freeIcon));
				return img;
			}
		}
		return NULL;
	}

protected:
	void drawBG(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);
	}

	void drawItem(graph_t* g, XTheme* theme, int32_t index, const grect_t& r) {
		if(index >= itemNum)
			return;

		if(index == itemSelected)
			graph_fill_round(g, r.x, r.y, r.w, r.h, 6, theme->basic.selectBGColor);

		drawIcon(g, index, theme, r.x , r.y, r.w, r.h);
		drawTitle(g, index, theme, r.x , r.y, r.w, r.h);
	}

	void onEnter(int i) {
		struct dirent* it = &files[i];
		char fname[FS_FULL_NAME_MAX+1];
		if(strcmp(it->d_name, "..") == 0) {
			upBack();
			return;
		}
		else if(strcmp(cwd, "/") == 0)
			snprintf(fname, FS_FULL_NAME_MAX, "/%s", it->d_name);
		else
			snprintf(fname, FS_FULL_NAME_MAX, "%s/%s", cwd, it->d_name);

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
		char fname[FS_FULL_NAME_MAX+1];
		if(strcmp(cwd, "/") == 0)
			snprintf(fname, FS_FULL_NAME_MAX, "/%s", it->d_name);
		else
			snprintf(fname, FS_FULL_NAME_MAX, "%s/%s", cwd, it->d_name);
		fileWidget->select(fname);
	}

public:
	friend FileWidget;
	FileGrid(FileWidget* fileWidget) {
		this->fileWidget = fileWidget;
		cwdLabel = NULL;
		scrollerV = NULL;
		loadIcons();
		fileTypes = loadFileTypes("/usr/system/filetypes.json");
	}

	void setCWDLabel(CWDLabel* l) {
		cwdLabel = l;
	}
};

void FileWidget::onFocus() {
	RootWidget* root = getRoot();
	root->focus(fileGrid);		
}

void FileWidget::build() {
	setType(Container::VERTICLE);
	setAlpha(false);

	CWDLabel* cwdLabel = new CWDLabel("/");
	cwdLabel->fix(0, 20);
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
	scrollerV->fix(8, 0);
	c->add(scrollerV);
	fgrid->setScrollerV(scrollerV);
}

FileWidget::FileWidget() {
	itemSize = 72;
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