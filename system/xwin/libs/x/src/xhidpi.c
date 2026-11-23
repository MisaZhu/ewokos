#include <ewoksys/ewokdef.h>

/*hidpi scale factor cache: clients refresh it from x_update_theme,
  xserverd keeps it in sync with its config via x_set_hidpi. Lives in its
  own TU so xserverd, which links libx for x_hidpi_size, does not pull in
  x.o whose symbols collide with its own xwin/xwin_cmd implementations*/
static bool _x_hidpi = false;
static float _x_hidpi_scale = 2.0;

float x_hidpi_scale(void) {
    return _x_hidpi_scale;
}

/* Effective device-pixel ratio: 1.0 when hidpi is off, the configured scale
   otherwise. Unlike x_hidpi_size() this keeps fractional scales (e.g. 1.6)
   intact - integer truncation there would turn 1.6 into 1 for size==1. */
float x_hidpi_ratio(void) {
    return _x_hidpi ? _x_hidpi_scale : 1.0f;
}

int x_hidpi_size(int size) {
    return _x_hidpi ? size*_x_hidpi_scale : size;
}

void x_set_hidpi(bool hidpi, float scale) {
    _x_hidpi = hidpi;

    _x_hidpi_scale = scale;
    if(_x_hidpi_scale < 1)
        _x_hidpi_scale = 1.0;
    if(_x_hidpi_scale > 4)
        _x_hidpi_scale = 4.0;
}
