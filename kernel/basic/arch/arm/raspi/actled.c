#include <dev/actled.h>
#include "bcm283x/actled.h"

void actled(bool on) {
	bcm283x_actled(on);
}
