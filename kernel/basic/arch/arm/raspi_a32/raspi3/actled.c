#include <mm/mmu.h>
#include <dev/actled.h>
#include "bcm283x/mbox_actled.h"

void actled(bool on) {
	bcm283x_mbox_actled(on);
}

