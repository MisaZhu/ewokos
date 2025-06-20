#ifndef XTHEME_H
#define XTHEME_H

#include <ewoksys/ewokdef.h>
#include <font/font.h>

#ifdef __cplusplus
extern "C" {
#endif

#define X_THEME_ROOT        "/usr/x/themes"
#define THEME_NAME_MAX 64 
#define X_DEFAULT_XTHEME    "opencde"

typedef struct {
	char     name[THEME_NAME_MAX];

	char     fontName[FONT_NAME_MAX];
	uint32_t fontSize;
	int32_t  charSpace;
	int32_t  lineSpace;

	uint32_t bgColor;
	uint32_t fgColor;

	uint32_t docBGColor;
	uint32_t docFGColor;

	uint32_t bgUnfocusColor;
	uint32_t fgUnfocusColor;

	uint32_t bgDisableColor;
	uint32_t fgDisableColor;

	uint32_t hideColor;

	uint32_t selectColor;
	uint32_t selectBGColor;

	uint32_t titleColor;
	uint32_t titleBGColor;

	uint32_t widgetFGColor;
	uint32_t widgetBGColor;
} x_theme_t;

typedef struct {
	char     fontName[FONT_NAME_MAX];
	char     patternName[THEME_NAME_MAX];
	uint32_t titleH;
	uint32_t frameW;
	uint32_t shadow;
	uint32_t fontSize;
	uint32_t bgColor;
	uint32_t fgColor;
	uint32_t bgTopColor;
	uint32_t fgTopColor;
	uint32_t desktopFGColor;
	uint32_t desktopBGColor;
	uint32_t bgEffect;
	bool     alpha;
} xwm_theme_t;

#ifdef __cplusplus
}
#endif

#endif
