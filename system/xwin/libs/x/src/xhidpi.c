#include <ewoksys/ewokdef.h>

/*hidpi scale factor cache: clients refresh it from x_update_theme,
  xserverd keeps it in sync with its config via x_set_hidpi. Lives in its
  own TU so xserverd, which links libx for x_hidpi_size, does not pull in
  x.o whose symbols collide with its own xwin/xwin_cmd implementations*/
static bool _x_hidpi = false;

int x_hidpi_size(int size) {
    return _x_hidpi ? size*2 : size;
}

void x_set_hidpi(bool hidpi) {
    _x_hidpi = hidpi;
}
