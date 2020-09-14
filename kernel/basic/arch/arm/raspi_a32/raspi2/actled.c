#include <mm/mmu.h>
#include <dev/actled.h>
#include "bcm283x/gpio_actled.h"

void actled(bool on) {
	bcm283x_gpio_actled(on);
}

