#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/Sprite.h>
#include <x++/X.h>
#include <unistd.h>
#include <ewoksys/kernel_tic.h>
#include <openlibm.h>
#include <ewoksys/vdevice.h>
#include <time.h>

using namespace Ewok;

void Sprite::onEvent(xevent_t* ev) {
    if (ev->type == XEVT_MOUSE) {
        static int lastX = -1, lastY = -1;
        if (ev->state == MOUSE_STATE_CLICK) {
            gpos_t pos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
            if(pos.x > 0 && pos.x < 24 && pos.y > 0 && pos.y < 24)
                this->close();
        }
        else if (ev->state == MOUSE_STATE_DOWN) {
            lastX = ev->value.mouse.x;
            lastY = ev->value.mouse.y;
        }
        else if (ev->state == MOUSE_STATE_DRAG) {
            if(ev->value.mouse.relative) {
                this->move(ev->value.mouse.rx, ev->value.mouse.ry);
            }
            else {
                if(lastX != -1 || lastY != -1)
                    this->move(ev->value.mouse.x - lastX, ev->value.mouse.y - lastY);
                lastX = ev->value.mouse.x;
                lastY = ev->value.mouse.y;
            }
        }
    }
}

void Sprite::onRepaint(graph_t* g) {
    WidgetWin::onRepaint(g);
    if(focused()) {
        graph_fill_circle(g, 12, 12, 8, 0xffcccccc);
        graph_fill_circle(g, 12, 12, 4, 0xff222222);
        graph_circle(g, 12, 12, 8, 2, 0xff222222);
    }
}
