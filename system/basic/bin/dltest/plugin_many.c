/*
 * plugin_many.c - ONE source, compiled N times with -DPLUG_ID / -DPLUG_TAG to
 * produce N independent shared objects (libdlplug_m1.so .. libdlplug_mN.so).
 *
 * The A/B chain proves dependency loading, but it never holds more than two
 * distinct objects at once, so it cannot show how the loader and the kernel's
 * anonymous DL window behave when MANY objects are resident together. This file
 * exists to give dltest that case:
 *
 *   - each object takes its own slots in the window's bitmap, so opening eight
 *     at once exercises concurrent slot allocation rather than a single reuse;
 *   - many_id() returns PLUG_ID and many_read_id() returns the exported data,
 *     so an object that was mis-mapped, aliased onto another, or relocated with
 *     the wrong bias shows up immediately instead of silently sharing state;
 *   - once all eight are closed and a fresh set is opened, success is the proof
 *     that the window slots AND their backing physical pages were really
 *     released and are reusable - i.e. dlclose does not leak.
 *
 * It deliberately references only printf (resolved from the host's generated ABI
 * export table), NOT host_callback, so every object loads standalone with no
 * dl_host_publish() setup. That also makes these the objects the "leak" mode can
 * leave mapped at exit without any registry dependency.
 */

#include <stdio.h>

#ifndef PLUG_ID
#define PLUG_ID 0
#endif
#ifndef PLUG_TAG
#define PLUG_TAG "M?"
#endif

/* exported data: one private, correctly-relocated copy per object */
int many_id_value = PLUG_ID;

static int ctor_ran = 0;

int many_id(void) {
	return PLUG_ID;
}

/* read the exported data back so the host can confirm each object has its own
   copy rather than aliasing another object's */
int many_read_id(void) {
	return many_id_value;
}

int many_add(int x) {
	return x + PLUG_ID;
}

__attribute__((constructor))
static void many_ctor(void) {
	ctor_ran = 1;
	printf("%s: ctor ran (id=%d)\n", PLUG_TAG, PLUG_ID);
}

__attribute__((destructor))
static void many_dtor(void) {
	printf("%s: dtor ran (ctor_ran=%d)\n", PLUG_TAG, ctor_ran);
}
