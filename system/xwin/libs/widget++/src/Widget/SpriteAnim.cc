
#include <Widget/SpriteWin.h>
#include <Widget/WidgetX.h>
#include <x++/X.h>
#include <unistd.h>
#include <graph/graph_png.h>
#include <openlibm.h>
#include <ewoksys/vdevice.h>
#include <time.h>
#include "Widget/SpriteAnim.h"
#include <tinyjson/tinyjson.h>
#include <string.h>
#include <stdlib.h>

using namespace Ewok;

void SpriteAnim::drawSprite(graph_t* g, const grect_t& r) {
    graph_t* img = spriteImg;
    if(img == NULL || steps == 0)
        return;
    uint32_t spriteWidth = img->w/steps;
    graph_blt(img, step*spriteWidth, 0, spriteWidth, img->h,
            g, r.x + r.w/2 - spriteWidth/2, r.y + r.h/2 - img->h/2,
            spriteWidth, img->h);
}

SpriteAnim::SpriteAnim() {
    step = 0;
    steps = 1;
    spriteImg = NULL;
}
	
SpriteAnim::~SpriteAnim() {
    if(spriteImg != NULL)
        graph_free(spriteImg);
}

gsize_t SpriteAnim::getSpriteSize(void) {
    gsize_t size = {0, 0};
    if(spriteImg == NULL)
        return size;
    size.w = spriteImg->w / steps;
    size.h = spriteImg->h;
    return size;
}

void SpriteAnim::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
    drawSprite(g, r);
}

void SpriteAnim::onTimer(uint32_t timerFPS, uint32_t timerStep) {
    step++;
    if(step >= steps)
        step = 0;   
    update();
};

bool SpriteAnim::setSprite(const std::string& fname, uint32_t steps, float zoom, uint32_t fps) {
    if(spriteImg != NULL)
        graph_free(spriteImg);
    spriteImg = png_image_new(fname.c_str());
    if(spriteImg == NULL)
        return false;

    graph_t* tmp = graph_scalef(spriteImg, zoom);
    if(tmp == NULL)
        return false;
    graph_free(spriteImg);
    spriteImg = tmp;

    this->steps = steps;
    this->fps = fps;
    return true;
}

bool SpriteAnim::setSpriteByScript(const std::string& fname) {
    json_var_t* json_var = json_parse_file(fname.c_str());
    if(json_var == NULL)
        return false;

    const char* img_file = json_get_str_def(json_var, "image", "");
    uint32_t steps = json_get_int_def(json_var, "steps", 1);
    uint32_t fps = json_get_int_def(json_var, "fps", 1);
    float zoom = json_get_float_def(json_var, "zoom", 1.0f);
    if(img_file[0] == '\0' || steps == 0)
        return false;
    
    // 如果 img_file 不是绝对路径，则拼接 fname 所在目录
    char* full_path = NULL;
    bool res = false;
    if(img_file[0] != '/') {
        // 查找 fname 中最后一个 '/' 的位置
        const char* fname_cstr = fname.c_str();
        const char* last_slash = strrchr(fname_cstr, '/');
        if(last_slash != NULL) {
            // 计算目录长度（包含 '/'）
            size_t dir_len = last_slash - fname_cstr + 1;
            size_t img_len = strlen(img_file);
            full_path = (char*)malloc(dir_len + img_len + 1);
            if(full_path != NULL) {
                memcpy(full_path, fname_cstr, dir_len);
                strcpy(full_path + dir_len, img_file);
                res = setSprite(full_path, steps, zoom, fps);
                free(full_path);
            } else {
                res = setSprite(img_file, steps, zoom, fps);
            }
        } else {
            res = setSprite(img_file, steps, zoom, fps);
        }
    } else {
        res = setSprite(img_file, steps, zoom, fps);
    }
    
    json_var_unref(json_var);    
    return res;
}