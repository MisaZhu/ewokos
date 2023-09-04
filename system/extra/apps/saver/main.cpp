/*
 *   This file is part of nes_emu.
 *   Copyright (c) 2019 Franz Flasch.
 *
 *   nes_emu is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   nes_emu is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with nes_emu.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/kernel_tic.h>
#include <sys/keydef.h>
#include <sys/klog.h>
#include <x++/X.h>

#include <ferox.h>
#include "pic.h"

using namespace Ewok;
   /* NES part */

#define KEY_TIMEOUT 2	

#define MAX_WALL_COUNT  4
#define BALL_RADIUS     3

xscreen_t scr;

class ScreenSaver : public XWin {
private:
	frWorld *world;
	frBody *walls[MAX_WALL_COUNT];
	uint32_t lastSec;
    uint64_t lastUsec;
 
	
	const frMaterial MATERIAL_BRICK = {
	    1.0f,		//density        
	    0.9f,		//restitution    
	    0.05f,		//staticFriction 
	    0.035f		//dynamicFriction
	};
	
	const frMaterial MATERIAL_WALL = {
	    1.25f,
	    0.9f,
	    0.075f,
	    0.05f
	};

public:
	inline ScreenSaver() {
		Init();
 	}
	
	inline ~ScreenSaver() {
	}

protected:
	void Init(void) {
		 const Rectangle bounds = {
   		     .x = -0.05f * frNumberPixelsToMeters(scr.size.w),
   		     .y = -0.05f * frNumberPixelsToMeters(scr.size.h),
   		     .width = 1.1f * frNumberPixelsToMeters(scr.size.w),
   		     .height = 1.1f * frNumberPixelsToMeters(scr.size.h)
   		 };

   		 world = frCreateWorld(frVec2ScalarMultiply(FR_WORLD_DEFAULT_GRAVITY, 0.000005f), bounds);
		//left
   		 walls[0] = frCreateBodyFromShape(
   		     FR_BODY_STATIC,
   		     FR_FLAG_NONE,
   		     frVec2PixelsToMeters((Vector2) { -0.05f * scr.size.w, 0.5f * scr.size.h }),
   		     frCreateRectangle(
   		         MATERIAL_WALL,
   		         frNumberPixelsToMeters(0.1f * scr.size.w),
   		         frNumberPixelsToMeters(1.1f * scr.size.h)
   		     )
   		 );
		//down
   		 walls[1] = frCreateBodyFromShape(
   		     FR_BODY_STATIC,
   		     FR_FLAG_NONE,
   		     frVec2PixelsToMeters((Vector2) { 0.5f * scr.size.w, 1.05f * scr.size.h }),
   		     frCreateRectangle(
   		         MATERIAL_WALL,
   		         frNumberPixelsToMeters(1.1f * scr.size.w),
   		         frNumberPixelsToMeters(0.1f * scr.size.h)
   		     )
   		 );
		//right
   		 walls[2] = frCreateBodyFromShape(
   		     FR_BODY_STATIC,
   		     FR_FLAG_NONE,
   		     frVec2PixelsToMeters((Vector2) { 1.05f * scr.size.w, 0.5f * scr.size.h }),
   		     frCreateRectangle(
   		         MATERIAL_WALL,
   		         frNumberPixelsToMeters(0.1f * scr.size.w),
   		         frNumberPixelsToMeters(1.1f * scr.size.h)
   		     )
   		 );
		//up
   		 walls[3] = frCreateBodyFromShape(
   		     FR_BODY_STATIC,
   		     FR_FLAG_NONE,
   		     frVec2PixelsToMeters((Vector2) { 0.5f * scr.size.w, -0.05f * scr.size.h }),
   		     frCreateRectangle(
   		         MATERIAL_WALL,
   		         frNumberPixelsToMeters(1.1f * scr.size.w),
   		         frNumberPixelsToMeters(0.1f * scr.size.h)
   		     )
   		 );

   		 for (int i = 0; i < MAX_WALL_COUNT; i++)
   		     frAddToWorld(world, walls[i]);
		
		for(int i = 0; i < 6; i++){
              frBody *body = frCreateBodyFromShape(FR_BODY_DYNAMIC,FR_FLAG_NONE,
                 frVec2PixelsToMeters((Vector2) { random()%scr.size.w, 0 }),
                 frCreateCircle(MATERIAL_BRICK, frNumberPixelsToMeters(BALL_RADIUS))
             );
             frSetBodyVelocity(body, (Vector2){random()%10/200.0f, 0});
             frSetBodyUserData(body, (void*)(random()%1800 + 2000));
             frAddToWorld(world, body);
		}
	}
	
	void DrawCircle(graph_t* g, int centreX, int centreY, int radius, uint32_t color){
		uint8_t *buf = (uint8_t*)g->buffer;
    	uint32_t *s = (uint32_t*)object;
    	uint32_t *d = (uint32_t*)buf;
    	uint32_t startX = centreX - radius;
    	uint32_t startY = centreY - radius;

    	for(int line = 0; line < 32; line++){
    	    if(line + startY < 0)
    	        continue;
    	    if(line + startY >= scr.size.h)
    	        break;

    	    for(int pix = 0; pix <32; pix++){
    	        if(pix + startX < 0)
    	            continue;
    	        if(pix + startX >= scr.size.w)
    	            break;
    	        uint8_t alpha =  ((uint8_t*)(s + line * 32 + pix))[3];
    	        if(alpha != 0){
    	            d[(line + startY) * scr.size.w + pix + startX] = s[line * 32 + pix];
    	        }
    	    }
    	}
	}

    void waitForNextFrame(){
        uint32_t sec;
        uint64_t usec;
        int wait;

        kernel_tic(&sec, &usec);
        wait = 1000000/60 - ((sec - lastSec) * 1000000 + (usec - lastUsec));
        if(wait > 0)
            usleep(wait);


        kernel_tic(&lastSec, &lastUsec);
    }

	void onRepaint(graph_t* g) {
	    frSimulateWorld(world, (1.0f / 60.0f) * 100.0f);
		graph_clear(g, 0);
        const int bodyCount = frGetWorldBodyCount(world);
        for (int i = MAX_WALL_COUNT; i < bodyCount; i++) {
            frBody *body = frGetWorldBody(world, i);
            const Vector2 bodyPos = frGetBodyPosition(body);
            int x = (int)frNumberMetersToPixels(bodyPos.x);
            int y = (int)frNumberMetersToPixels(bodyPos.y);
            int l = (int)frGetBodyUserData(body) - 1;
            if(l < 0 || y > scr.size.h + 640){
                frSetBodyPosition(body, frVec2PixelsToMeters((Vector2){ random()%scr.size.w, 0}));
                frSetBodyVelocity(body, (Vector2){random()%10/1000.0f, 0});
                l = random()%1800 + 2000;
            }
            frSetBodyUserData(body, (void*)l);

            DrawCircle(g, x, y, BALL_RADIUS, 0);
        }	
		waitForNextFrame();
	}
};

static void loop(void* p) {
	XWin* xwin = (XWin*)p;
	xwin->repaint();
}

int main(int argc, char *argv[])
{
	const char* path;

    /*init window*/

	X x;
	x.screenInfo(scr, 0);
	ScreenSaver saver;
	x.open(&saver, 0, 0, scr.size.w, scr.size.h, "ScreenSaver", X_STYLE_NO_FRAME);
	saver.setVisible(true);

	x.run(loop, &saver);

    return 0;
}
