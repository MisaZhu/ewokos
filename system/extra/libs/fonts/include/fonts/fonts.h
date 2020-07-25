#ifndef FONTS_H
#define FONTS_H

#include <graph/font.h>

#ifdef __cplusplus
extern "C" {
#endif

font_t* font_by_name(const char* name);
font_item_t* font_by_index(uint32_t index);

#ifdef __cplusplus
}
#endif

#endif
