/*
 * Test plugin B - depends on plugin A through DT_NEEDED.
 *
 * Linked with -ldlplug_a, so the linker records DT_NEEDED "libdlplug_a.so" and
 * leaves plug_a_add/plug_a_counter undefined for the loader to resolve. That
 * makes this object the test for three things at once:
 *
 *   - dl_load_from_path() following DT_NEEDED and searching for a bare soname
 *     (there is no rpath mechanism in the EwokOS rootfs, so the search has to
 *     find libdlplug_a.so in /lib by name);
 *   - dependency ordering: A must be mapped, relocated and initialised before
 *     B is relocated, otherwise B's references to A resolve to nothing;
 *   - cross-object relocation, both a function (JUMP_SLOT through B's PLT) and
 *     a data symbol (GLOB_DAT for plug_a_counter).
 *
 * The ctor/dtor ordering is the visible proof of the second point: A's ctor must
 * print before B's, and B's dtor before A's.
 */

#include <stdio.h>
#include <dl/dl.h>

/* resolved from libdlplug_a.so at load time */
extern int plug_a_add(int a, int b);
extern int plug_a_counter;

int plug_b_call_a(int x) {
	return plug_a_add(x, 100);
}

/* Reading another object's variable goes through a GOT entry filled in by a
   GLOB_DAT relocation, so a wrong bias shows up here immediately. */
int plug_b_read_a_counter(void) {
	return plug_a_counter;
}

/* Bump A's variable through its GOT slot and report the result, so the host can
   verify the two objects really share one instance rather than each having a
   private copy. */
int plug_b_bump_a_counter(void) {
	plug_a_counter += 1000;
	return plug_a_counter;
}

unsigned int plug_b_abi_version(void) {
	return dl_host_abi_version();
}

__attribute__((constructor))
static void plug_b_ctor(void) {
	printf("B: ctor ran (.init_array), sees plug_a_counter=%d\n",
	       plug_a_counter);
}

__attribute__((destructor))
static void plug_b_dtor(void) {
	printf("B: dtor ran (.fini_array)\n");
}
