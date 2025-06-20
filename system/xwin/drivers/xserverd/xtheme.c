#include <ewoksys/vfs.h>
#include <tinyjson/tinyjson.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include "xtheme.h"

int x_load_theme(const char* name, x_theme_t* theme) {
	if(theme == NULL || name == NULL || name[0] == 0) 
        return -1;

	strncpy(theme->name, name, THEME_NAME_MAX-1);

	char fname[FS_FULL_NAME_MAX];
	snprintf(fname, FS_FULL_NAME_MAX-1, "%s/%s/x/theme.json", X_THEME_ROOT, name);
	json_var_t *conf_var = json_parse_file(fname);	

	theme->fontSize = json_get_int_def(conf_var, "font_size", DEFAULT_SYSTEM_FONT_SIZE);

	const char* v = json_get_str_def(conf_var, "font", DEFAULT_SYSTEM_FONT);
	memset(theme->fontName, 0, FONT_NAME_MAX);
	strncpy(theme->fontName, v, FONT_NAME_MAX-1);

	theme->fgColor = json_get_int_def(conf_var, "fg_color", 0xffffffff);
	theme->bgColor = json_get_int_def(conf_var, "bg_color", 0xff000000);
	theme->docFGColor = json_get_int_def(conf_var, "doc_fg_color", 0xff000000);
	theme->docBGColor = json_get_int_def(conf_var, "doc_bg_color", 0xffffffff);

	theme->fgUnfocusColor = json_get_int_def(conf_var, "fg_unfocus_color", 0);
	theme->bgUnfocusColor = json_get_int_def(conf_var, "bg_unfocus_color", 0);

	theme->fgDisableColor = json_get_int_def(conf_var, "fg_disable_color", 0xff444444);
	theme->bgDisableColor = json_get_int_def(conf_var, "bg_disable_color", 0xff888888);

	theme->hideColor = json_get_int_def(conf_var, "hide_color", 0);
	theme->selectColor = json_get_int_def(conf_var, "select_color", 0);
	theme->selectBGColor = json_get_int_def(conf_var, "select_bg_color", 0);

	theme->titleColor = json_get_int_def(conf_var, "title_color", 0);
	theme->titleBGColor = json_get_int_def(conf_var, "title_bg_color", 0);

	theme->widgetFGColor = json_get_int_def(conf_var, "widget_color", 0xff000000);
	theme->widgetBGColor = json_get_int_def(conf_var, "widget_bg_color", 0xffffffff);

	if(conf_var != NULL)
		json_var_unref(conf_var);
	return 0;
}

int x_load_xwm_theme(const char* name, xwm_theme_t* theme) {
	if(theme == NULL || name == NULL || name[0] == 0) 
        return -1;

	char fname[FS_FULL_NAME_MAX];
	snprintf(fname, FS_FULL_NAME_MAX-1, "%s/%s/xwm/theme.json", X_THEME_ROOT, name);
	json_var_t* conf_var = json_parse_file(fname);

	theme->fgColor = json_get_int_def(conf_var, "fg_color", 0xff888888);
	theme->bgColor = json_get_int_def(conf_var, "bg_color", 0xff666666);
	theme->fgTopColor = json_get_int_def(conf_var, "fg_top_color", 0xff222222);
	theme->bgTopColor = json_get_int_def(conf_var, "bg_top_color", 0xffaaaaaa);
	theme->desktopFGColor = json_get_int_def(conf_var, "desktop_fg_color", 0xff555588);
	theme->desktopBGColor = json_get_int_def(conf_var, "desktop_bg_color", 0xff8888aa);
	theme->frameW = json_get_int_def(conf_var, "frame_width", 2);
	theme->shadow = json_get_int_def(conf_var, "shadow", 0);
	theme->titleH = json_get_int_def(conf_var, "title_h", 24);
	theme->fontSize = json_get_int_def(conf_var, "font_size", 14);

	const char* v = json_get_str_def(conf_var, "font", DEFAULT_SYSTEM_FONT);
	memset(theme->fontName, 0, FONT_NAME_MAX);
	strncpy(theme->fontName, v, FONT_NAME_MAX-1);

	v = json_get_str_def(conf_var, "pattern", "");
	memset(theme->patternName, 0, THEME_NAME_MAX);
	strncpy(theme->patternName, v, THEME_NAME_MAX-1);

	theme->bgEffect = json_get_int_def(conf_var, "bg_effect", 0);
	theme->alpha = json_get_int_def(conf_var, "alpha", 0);

	if(theme->shadow > 0)
		theme->alpha = true;

	if(conf_var != NULL)
		json_var_unref(conf_var);
	return 0;
}