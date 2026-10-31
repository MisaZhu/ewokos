/*
 * Test plugin A - the base of the dltest dependency chain.
 *
 * Built as an ET_DYN shared object and loaded at runtime by /bin/dltest. It is
 * deliberately written to exercise as many distinct loader paths as one small
 * file can, so that a single successful run of dltest is evidence about the
 * loader rather than about one happy case:
 *
 *   - calls printf/snprintf, which are NOT in this object: they resolve against
 *     the host's generated ABI export table (libs/dl/gen_export.sh). That is the
 *     replacement for the .dynsym a static ET_EXEC host does not have.
 *   - references host_callback, which the host publishes with
 *     dl_host_publish() before dlopen(): the runtime registry path.
 *   - calls dl_host_lookup()/dl_host_abi_version(), which come from libdl's own
 *     hand-written core table rather than the generated one.
 *   - exports a data symbol (plug_a_counter) as well as functions, so the
 *     R_AARCH64_GLOB_DAT/ABS64 paths are covered, not just JUMP_SLOT.
 *   - calls its own exported function plug_a_add() from plug_a_hello(). Under
 *     -fPIC with default visibility that goes through the PLT, which is what
 *     produces a JUMP_SLOT relocation for an intra-object call.
 *   - has .init_array/.fini_array entries, whose pointers arrive as
 *     R_AARCH64_RELATIVE relocations the loader must fix up before calling.
 *
 * Plugin B links against this object, so it also serves as the DT_NEEDED target
 * that proves dependency loading and cross-object symbol resolution.
 */

#include <stdio.h>
#include <dl/dl.h>

/*
 * Published by the host with dl_host_publish() before it calls dlopen().
 * Resolved through the runtime registry, so if the host forgets to publish it
 * the load fails with "undefined symbol 'host_callback'" - which is the point:
 * an application-owned symbol is opt-in, exactly as on Linux.
 */
extern int host_callback(const char* who);

/* Exported data. A plugin reading a host variable would need a copy
   relocation, which libdl rejects; the other direction - the host reading this
   through dlsym() - is what is exercised here. */
int plug_a_counter = 41;

static int plug_a_ctor_ran = 0;

int plug_a_add(int a, int b) {
	return a + b;
}

const char* plug_a_hello(const char* who) {
	static char buf[128];
	int sum;

	plug_a_counter++;
	/* goes through the PLT: intra-object call to an exported symbol */
	sum = plug_a_add(plug_a_counter, 1);
	snprintf(buf, sizeof(buf), "A: hello %s, counter=%d, add=%d",
	         who, plug_a_counter, sum);
	return buf;
}

/* Call the host back. Proves a plugin can invoke a symbol that exists only in
   the host's runtime registry. */
int plug_a_host_roundtrip(const char* who) {
	return host_callback(who);
}

unsigned int plug_a_abi_version(void) {
	/* dl_host_abi_version() comes from libdl's hand-written core table, not
	   from the generated one - libdl cannot nm itself */
	return dl_host_abi_version();
}

/* Look a host symbol up at runtime instead of binding it at load time. */
int plug_a_runtime_lookup(const char* name) {
	return (dl_host_lookup(name) != NULL) ? 1 : 0;
}

/*
 * constructor/destructor attributes land these in .init_array/.fini_array.
 * EwokOS's own _start never runs .init_array for the host image, so libdl's
 * dl_run_init() is the only thing that will ever call these - if they print,
 * the loader both applied the RELATIVE relocations and walked the array.
 */
__attribute__((constructor))
static void plug_a_ctor(void) {
	plug_a_ctor_ran = 1;
	printf("A: ctor ran (.init_array)\n");
}

__attribute__((destructor))
static void plug_a_dtor(void) {
	printf("A: dtor ran (.fini_array), ctor_ran=%d\n", plug_a_ctor_ran);
}
