#ifndef SPRITE_ANIM_H
#define SPRITE_ANIM_H

#include "Widget/Widget.h"

namespace Ewok {

class SpriteAnim : public Widget {
protected:
	uint32_t step;
    uint32_t steps;
	uint32_t fps;
	graph_t* spriteImg;

	void drawSprite(graph_t* g, const grect_t& r);
    void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);
    void onTimer(uint32_t timerFPS, uint32_t timerStep);

public:
	SpriteAnim();
	~SpriteAnim();

	inline uint32_t getFPS(void) { return fps; }
	gsize_t getSpriteSize(void);

    bool setSprite(const std::string& fname, uint32_t steps, uint32_t fps);
    bool setSpriteByScript(const std::string& fname);
};

}
#endif