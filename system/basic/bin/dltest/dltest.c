/*
 * dltest - the host side of the dlopen() self-test.
 *
 * This is the only program in EwokOS that loads a shared object, so it is
 * written to be evidence rather than a demo: every step asserts something
 * specific about the loader, prints what it got, and the run ends with a
 * machine-greppable "DLTEST: PASS" or "DLTEST: FAIL" plus a count. A boot log
 * containing that line is the proof that the kernel's cache-sync syscall, the
 * ELF mapper, the relocation engine, the generated host ABI table and the
 * object lifecycle all work on real hardware-emulated aarch64.
 *
 * What it covers, in order:
 *   1. the host ABI surface (generated table + dl_host_publish registry)
 *   2. dlopen() by absolute path, dlsym() of code AND data
 *   3. a plugin calling back into the host through the registry
 *   4. DT_NEEDED: dlopen() of a bare soname that depends on object #2
 *   5. soname dedup - two objects sharing one instance of A
 *   6. dlopen(NULL), RTLD_NOLOAD, dladdr()
 *   7. the error paths: unresolvable symbol, missing file, missing symbol
 *   8. dlclose() refcounting and ctor/dtor ordering
 *
 * Run it as: dltest
 * The plugins must be installed in /lib (see this directory's Makefile).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include <dl/dl.h>

/* plugin entry points, named as dlsym() sees them */
typedef const char*  (*a_hello_fn)(const char*);
typedef int          (*a_add_fn)(int, int);
typedef int          (*a_rt_fn)(const char*);
typedef int          (*a_look_fn)(const char*);
typedef unsigned int (*abi_fn)(void);
typedef int          (*b_call_fn)(int);
typedef int          (*b_void_fn)(void);

static int g_checks   = 0;
static int g_failures = 0;

/* How many independent libdlplug_m<N>.so objects section 14 holds resident at
   once. Eight is enough to prove the anonymous window's slot bitmap handles
   many concurrent allocations and their release, without being so many that a
   single serial log becomes unreadable. Must match MANY_IDS in the Makefile. */
#define MANY_N 8

/*
 * A function rather than a macro with ##__VA_ARGS__: the test builds under
 * -std=c99 where the GNU comma-swallowing extension is not available, and a
 * plain macro would need every message to be a single string literal anyway.
 */
static void check(int cond, const char* what) {
	g_checks++;
	if(cond) {
		printf("  ok   %s\n", what);
	} else {
		g_failures++;
		printf("  FAIL %s\n", what);
	}
}

static void say_err(const char* prefix) {
	char* e = dlerror();
	printf("  %s: %s\n", prefix, (e != NULL) ? e : "(dlerror returned NULL)");
}

/*
 * Published into the loader's registry before any dlopen(). A plugin referring
 * to this gets it resolved through the runtime path rather than the generated
 * table, which is how an application exports its own API.
 */
static int g_callback_calls = 0;

int host_callback(const char* who) {
	g_callback_calls++;
	printf("  host_callback('%s') -> call #%d\n",
	       (who != NULL) ? who : "(null)", g_callback_calls);
	return 1000 + g_callback_calls;
}

static void section(const char* title) {
	printf("\n--- %s ---\n", title);
}

static void run_all(void) {
	void* ha;
	void* hb;
	void* h;

	section("1. host ABI surface");
	printf("  EWOK_DL_ABI_VERSION (compiled against) = %u\n",
	       (unsigned)EWOK_DL_ABI_VERSION);
	printf("  dl_host_abi_version() (running against) = %u\n",
	       (unsigned)dl_host_abi_version());
	printf("  __ewok_dl_abi_count = %u\n", (unsigned)__ewok_dl_abi_count);
	check(dl_host_abi_version() == EWOK_DL_ABI_VERSION,
	      "run-time ABI version matches dl/dl.h");
	check(__ewok_dl_abi_count > 100,
	      "the nm-generated export table is populated");
	check(dl_object_count() == 0, "no shared objects loaded yet");

	check(dl_host_publish("host_callback", (void*)host_callback) == 0,
	      "dl_host_publish(\"host_callback\")");
	check(dl_host_publish("host_callback", (void*)host_callback) == -1,
	      "re-publishing the same name is refused, not silently shadowed");
	check(dl_host_publish("x", NULL) == -1,
	      "publishing a NULL address is refused");
	check(dl_host_lookup("host_callback") == (void*)host_callback,
	      "dl_host_lookup finds the published symbol");
	check(dl_host_lookup("no_such_host_symbol") == NULL,
	      "dl_host_lookup misses an unknown name");
	check(dl_host_lookup("printf") != NULL,
	      "dl_host_lookup reaches the generated table (printf)");
	check(dl_host_lookup("malloc") != NULL,
	      "dl_host_lookup reaches the generated table (malloc)");

	section("2. dlopen by absolute path");
	ha = dlopen("/lib/libdlplug_a.so", RTLD_NOW);
	if(ha == NULL) {
		say_err("dlopen(/lib/libdlplug_a.so) failed");
		check(0, "dlopen(\"/lib/libdlplug_a.so\", RTLD_NOW)");
		printf("\n  cannot continue without plugin A\n");
		return;
	}
	check(ha != NULL, "dlopen(\"/lib/libdlplug_a.so\", RTLD_NOW)");
	printf("  handle = %p\n", ha);
	check(dl_object_count() == 1, "one object resident");

	{
		a_hello_fn a_hello   = (a_hello_fn)dlsym(ha, "plug_a_hello");
		a_add_fn   a_add     = (a_add_fn)dlsym(ha, "plug_a_add");
		int*       a_counter = (int*)dlsym(ha, "plug_a_counter");
		a_rt_fn    a_rt      = (a_rt_fn)dlsym(ha, "plug_a_host_roundtrip");
		abi_fn     a_abi     = (abi_fn)dlsym(ha, "plug_a_abi_version");
		a_look_fn  a_look    = (a_look_fn)dlsym(ha, "plug_a_runtime_lookup");
		const char* msg;

		check(a_hello != NULL,   "dlsym(plug_a_hello)");
		check(a_add != NULL,     "dlsym(plug_a_add)");
		check(a_counter != NULL, "dlsym(plug_a_counter) - a DATA symbol");
		check(a_rt != NULL,      "dlsym(plug_a_host_roundtrip)");
		check(a_abi != NULL,     "dlsym(plug_a_abi_version)");
		check(a_look != NULL,    "dlsym(plug_a_runtime_lookup)");
		if(a_hello == NULL || a_add == NULL || a_counter == NULL)
			return;

		section("3. calling into the plugin");
		check(*a_counter == 41,
		      "the plugin's initialised data has its expected value (41)");
		check(a_add(2, 3) == 5, "plug_a_add(2, 3) == 5");
		msg = a_hello("dltest");
		printf("  plugin returned: %s\n", (msg != NULL) ? msg : "(null)");
		/*
		 * counter goes 41 -> 42 and the reported sum is 43, which only
		 * happens if the call to A's own exported plug_a_add went through
		 * its PLT and landed on the relocated address.
		 */
		check(msg != NULL && strstr(msg, "counter=42") != NULL &&
		      strstr(msg, "add=43") != NULL,
		      "plug_a_hello ran, and its intra-object PLT call resolved");
		check(*a_counter == 42, "the host sees the plugin's write to its own data");

		section("4. plugin -> host callback through the registry");
		check(a_rt("plugin A") == 1001,
		      "the plugin called host_callback and got 1001 back");
		check(g_callback_calls == 1, "the host saw exactly one callback");

		section("5. plugin -> libdl core table");
		check(a_abi() == EWOK_DL_ABI_VERSION,
		      "the plugin read the ABI version through libdl's core table");
		check(a_look("printf") == 1,
		      "the plugin's dl_host_lookup(\"printf\") succeeds");
		check(a_look("definitely_not_a_symbol") == 0,
		      "the plugin's dl_host_lookup misses an unknown name");
	}

	section("6. DT_NEEDED: dlopen a bare soname that depends on A");
	hb = dlopen("libdlplug_b.so", RTLD_NOW | RTLD_GLOBAL);
	if(hb == NULL) {
		say_err("dlopen(libdlplug_b.so) failed");
		check(0, "dlopen(\"libdlplug_b.so\") resolves a bare soname in /lib");
		return;
	}
	check(hb != NULL, "dlopen(\"libdlplug_b.so\") resolves a bare soname in /lib");
	printf("  handle = %p\n", hb);
	check(dl_object_count() == 2,
	      "two objects resident: B, plus the A its DT_NEEDED resolved to");

	{
		b_call_fn b_call = (b_call_fn)dlsym(hb, "plug_b_call_a");
		b_void_fn b_read = (b_void_fn)dlsym(hb, "plug_b_read_a_counter");
		b_void_fn b_bump = (b_void_fn)dlsym(hb, "plug_b_bump_a_counter");
		abi_fn    b_abi  = (abi_fn)dlsym(hb, "plug_b_abi_version");
		int*      a_counter = (int*)dlsym(ha, "plug_a_counter");

		check(b_call != NULL, "dlsym(plug_b_call_a)");
		check(b_read != NULL, "dlsym(plug_b_read_a_counter)");
		check(b_bump != NULL, "dlsym(plug_b_bump_a_counter)");
		check(b_abi != NULL,  "dlsym(plug_b_abi_version)");
		if(b_call == NULL || b_read == NULL || b_bump == NULL)
			return;

		check(b_call(5) == 105,
		      "B reached A's function through a cross-object JUMP_SLOT");
		check(b_read() == 42,
		      "B reads A's variable through a GLOB_DAT-relocated GOT slot");
		check(b_bump() == 1042,
		      "B bumped A's variable to 1042 through that same slot");
		check(a_counter != NULL && *a_counter == 1042,
		      "the host, A and B all share ONE instance of A");
		check(b_abi() == EWOK_DL_ABI_VERSION,
		      "B also reaches libdl's core table");
	}

	section("7. handle identity, RTLD_NOLOAD, dlopen(NULL)");
	h = dlopen("libdlplug_a.so", RTLD_NOW);
	check(h == ha, "re-opening A by soname returns the same handle");
	check(dlclose(h) == 0, "dlclose of that extra reference");
	check(dl_object_count() == 2, "A survived: B still holds it");

	h = dlopen("libdlplug_b.so", RTLD_NOLOAD | RTLD_NOW);
	check(h == hb, "RTLD_NOLOAD returns the already-loaded object");
	check(dlclose(h) == 0, "dlclose of the NOLOAD reference");

	h = dlopen("libneverloaded.so", RTLD_NOLOAD | RTLD_NOW);
	check(h == NULL, "RTLD_NOLOAD on an absent object returns NULL");
	check(dlerror() == NULL, "...and sets no error, because it is a query");

	h = dlopen(NULL, RTLD_NOW);
	check(h != NULL, "dlopen(NULL) returns a handle for the host scope");
	if(h != NULL) {
		check(dlsym(h, "printf") != NULL,
		      "dlsym on the host handle finds a generated-table symbol");
		check(dlsym(h, "host_callback") == (void*)host_callback,
		      "dlsym on the host handle finds a registry symbol");
		check(dlclose(h) == 0, "dlclose of the host handle is a no-op");
	}

	section("8. dladdr");
	{
		Dl_info info;
		void*   a_add = dlsym(ha, "plug_a_add");

		if(dladdr(a_add, &info) == 0) {
			check(0, "dladdr on a plugin function");
		} else {
			printf("  dli_fname = %s\n",
			       (info.dli_fname != NULL) ? info.dli_fname : "(null)");
			printf("  dli_sname = %s\n",
			       (info.dli_sname != NULL) ? info.dli_sname : "(null)");
			printf("  dli_fbase = %p  dli_saddr = %p\n",
			       info.dli_fbase, info.dli_saddr);
			check(info.dli_sname != NULL &&
			      strcmp(info.dli_sname, "plug_a_add") == 0,
			      "dladdr names the symbol");
			check(info.dli_saddr == a_add,
			      "dladdr's address equals what dlsym returned");
			check(info.dli_fname != NULL &&
			      strstr(info.dli_fname, "libdlplug_a.so") != NULL,
			      "dladdr names the object it lives in");
			check(info.dli_fbase != NULL, "dladdr reports a load base");
		}

		check(dladdr((void*)host_callback, &info) == 0,
		      "dladdr on host code returns 0 (the host is a static link)");
		say_err("dladdr(host code)");
	}

	section("9. error paths");
	h = dlopen("/lib/libdlplug_bad.so", RTLD_NOW);
	check(h == NULL,
	      "dlopen of the plugin with an unresolvable symbol fails");
	if(h == NULL) {
		char* e = dlerror();
		printf("  dlerror: %s\n", (e != NULL) ? e : "(null)");
		check(e != NULL &&
		      strstr(e, "this_symbol_does_not_exist_anywhere") != NULL,
		      "dlerror names the symbol that could not be resolved");
	} else {
		dlclose(h);
	}
	check(dl_object_count() == 2,
	      "the failed load left nothing behind (no leaked mapping)");

	h = dlopen("/lib/libdoes_not_exist.so", RTLD_NOW);
	check(h == NULL, "dlopen of a missing file fails");
	if(h == NULL)
		say_err("dlerror");

	h = dlopen("libdlplug_b.so", RTLD_LAZY);
	check(h == hb, "RTLD_LAZY is accepted (and binds eagerly anyway)");
	if(h != NULL)
		dlclose(h);

	check(dlsym(ha, "plug_a_not_there") == NULL,
	      "dlsym of an absent symbol returns NULL");
	say_err("dlerror");
	check(dlsym(ha, "") == NULL, "dlsym of an empty name is refused");
	check(dlsym(RTLD_NEXT, "printf") == NULL,
	      "RTLD_NEXT is refused rather than silently misbehaving");
	say_err("dlerror(RTLD_NEXT)");

	section("10. dlclose refcounting and dtor ordering");
	/*
	 * Reference counts at this point: A = 2 (this handle + B's DT_NEEDED),
	 * B = 1. Closing B must run B's destructor first, then drop A to 1; A's
	 * destructor runs only when its own handle closes. Watch the serial log
	 * for "B: dtor" before "A: dtor" - that order is the visible proof that
	 * fini runs dependent-first, the reverse of init.
	 */
	check(dlclose(hb) == 0, "dlclose(B)");
	check(dl_object_count() == 1,
	      "B is gone; A survives on B's released dependency reference");
	check(dlclose(ha) == 0, "dlclose(A)");
	check(dl_object_count() == 0, "everything unloaded, nothing leaked");
	check(dlclose(ha) == -1, "dlclose of a handle that is already dead fails");
	say_err("dlerror(dead handle)");
	check(dlsym(ha, "plug_a_add") == NULL, "dlsym through a dead handle fails");

	section("11. unpublish");
	check(dl_host_unpublish("host_callback") == 0, "dl_host_unpublish");
	check(dl_host_lookup("host_callback") == NULL,
	      "...and the symbol is no longer resolvable");
	check(dl_host_unpublish("host_callback") == -1,
	      "unpublishing a name that is not published fails");

	/* and a reload after everything was unloaded: proves the mapping was
	   really released and that a second load is not affected by the first */
	section("12. reload after full unload");
	/*
	 * Section 11 withdrew host_callback, which plugin A references, so the
	 * reload would fail on it for a reason that says nothing about unloading.
	 * Re-publish first - which is itself worth asserting, because a registry
	 * that could not reuse a withdrawn name would leak a slot every time an
	 * application reloaded a plugin.
	 */
	check(dl_host_publish("host_callback", (void*)host_callback) == 0,
	      "the registry accepts a name again after it was unpublished");
	ha = dlopen("/lib/libdlplug_a.so", RTLD_NOW);
	if(ha == NULL)
		say_err("dlopen(reload)");
	check(ha != NULL, "A loads again after having been fully unloaded");
	if(ha != NULL) {
		int* c = (int*)dlsym(ha, "plug_a_counter");
		check(c != NULL && *c == 41,
		      "the reloaded A starts from its initial data again (41)");
		check(dlclose(ha) == 0, "dlclose of the reloaded A");
		check(dl_object_count() == 0, "and nothing is left behind");
	}

	section("13. anonymous window, page size and W^X");
	{
		int ps = getpagesize();
		void* m;
		printf("  getpagesize() = %d\n", ps);
		check(ps >= 4096 && (ps & (ps - 1)) == 0,
		      "getpagesize() reports a real power-of-two page size");

		/*
		 * mmap() now comes from the kernel's anonymous window
		 * (SYS_MMAP_ANON), which has real per-page permissions, so mprotect()
		 * actually changes them instead of being a no-op. On a target with no
		 * window (32-bit, or an older kernel) libc falls back to the heap
		 * emulation and mprotect() is a no-op that still returns 0, so every
		 * check below passes either way - it just proves more on a target
		 * that has the window.
		 */
		m = mmap(NULL, 65536, PROT_READ | PROT_WRITE,
		         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		check(m != NULL && m != MAP_FAILED, "mmap() of a 64k anonymous region");
		if(m != NULL && m != MAP_FAILED) {
			volatile unsigned char* p = (volatile unsigned char*)m;
			p[0] = 0xA5; p[65535] = 0x5A;
			check(p[0] == 0xA5 && p[65535] == 0x5A,
			      "the mapping is writable at both ends");
			check(mprotect(m, 65536, PROT_READ) == 0,
			      "mprotect() downgrades it to read-only");
			check(p[0] == 0xA5, "and it is still readable afterwards");
			check(mprotect(m, 65536, PROT_READ | PROT_WRITE) == 0,
			      "mprotect() restores write permission");
			p[0] = 0x11;
			check(p[0] == 0x11, "and it is writable again");
			check(munmap(m, 65536) == 0, "munmap() releases it");
		}

		/*
		 * Every section above ran with each plugin's text mapped
		 * read-only-and-execute, because dl_elf_protect()'s W^X pass runs
		 * inside dlopen(). A successful call here is therefore the proof that
		 * dropping the write bit off the text did not break code execution.
		 */
		ha = dlopen("/lib/libdlplug_a.so", RTLD_NOW);
		if(ha == NULL)
			say_err("dlopen(W^X)");
		check(ha != NULL, "A loads with the W^X permission pass active");
		if(ha != NULL) {
			a_add_fn add = (a_add_fn)dlsym(ha, "plug_a_add");
			check(add != NULL && add(20, 22) == 42,
			      "A's code runs from its now read-only text segment");
			dlclose(ha);
		}
	}

	section("14. many objects resident at once, then released and reused");
	{
		/*
		 * The A/B chain never holds more than two distinct objects, so it
		 * cannot show what happens when MANY are mapped together. These eight
		 * (libdlplug_m1.so .. m8.so, one source compiled with a different
		 * PLUG_ID each) each take their own slots in the anonymous window, so
		 * opening them all exercises concurrent slot allocation; closing them
		 * all and opening a fresh set is the proof that dlclose released the
		 * slots AND their backing pages rather than leaking them.
		 */
		void* mh[MANY_N];
		char  path[64];
		int   i;
		int   all_open = 1;

		for(i = 0; i < MANY_N; i++) {
			snprintf(path, sizeof(path), "/lib/libdlplug_m%d.so", i + 1);
			mh[i] = dlopen(path, RTLD_NOW);
			if(mh[i] == NULL) {
				say_err("dlopen(many)");
				all_open = 0;
				break;
			}
		}
		check(all_open, "opened 8 distinct shared objects simultaneously");
		if(!all_open) {
			for(i = 0; i < MANY_N; i++)
				if(mh[i] != NULL) dlclose(mh[i]);
			return;
		}
		check(dl_object_count() == MANY_N,
		      "all 8 are resident, each in its own window slots");

		/* Every object must be separately mapped and relocated: its own
		   many_id()/many_read_id() return its own PLUG_ID, which a shared or
		   mis-biased mapping would break. */
		{
			int distinct = 1;
			for(i = 0; i < MANY_N; i++) {
				int (*idfn)(void)   = (int(*)(void))dlsym(mh[i], "many_id");
				int (*readfn)(void) = (int(*)(void))dlsym(mh[i], "many_read_id");
				if(idfn == NULL || readfn == NULL ||
				   idfn() != i + 1 || readfn() != i + 1) {
					distinct = 0;
					printf("  object m%d misbehaved\n", i + 1);
				}
			}
			check(distinct, "each object runs its own relocated code and data");
		}

		/* Distinct load bases prove eight separate mappings, not aliases of
		   one - a direct check on the window handing out different slots. */
		{
			void* bases[MANY_N];
			int   uniq = 1, j;
			for(i = 0; i < MANY_N; i++) {
				Dl_info inf;
				void*   f = dlsym(mh[i], "many_id");
				bases[i] = NULL;
				if(f != NULL && dladdr(f, &inf) != 0)
					bases[i] = inf.dli_fbase;
			}
			for(i = 0; i < MANY_N && uniq; i++)
				for(j = i + 1; j < MANY_N; j++)
					if(bases[i] == bases[j]) { uniq = 0; break; }
			check(uniq, "the 8 objects occupy 8 distinct load addresses");
		}

		for(i = 0; i < MANY_N; i++)
			dlclose(mh[i]);
		check(dl_object_count() == 0,
		      "closing all 8 released every object and its window slots");

		/* Reopen a fresh set: only succeeds if the slots and pages really were
		   reclaimed, which is the resource-release proof for the multi-object
		   case. */
		all_open = 1;
		for(i = 0; i < MANY_N; i++) {
			snprintf(path, sizeof(path), "/lib/libdlplug_m%d.so", i + 1);
			mh[i] = dlopen(path, RTLD_NOW);
			if(mh[i] == NULL) {
				say_err("dlopen(reopen)");
				all_open = 0;
				break;
			}
		}
		check(all_open, "the released slots were reusable for a fresh set of 8");
		if(all_open) {
			check(dl_object_count() == MANY_N,
			      "8 resident again after the release/reload cycle");
			for(i = 0; i < MANY_N; i++)
				dlclose(mh[i]);
		}
		check(dl_object_count() == 0,
		      "and released cleanly a second time - no slot leak");
	}
}

int main(int argc, char** argv) {
	/*
	 * wxfault mode: deliberately store into a plugin's executable text page.
	 * With W^X enforced by the kernel window that store takes a permission
	 * fault and the kernel kills this process, so the ABSENCE of the "no fault"
	 * line below - together with a data-abort in the boot log - is the proof
	 * that the text really is write-protected. It is a separate invocation
	 * precisely because it is expected to die.
	 */
	if(argc > 1 && strcmp(argv[1], "wxfault") == 0) {
		void* h;
		a_add_fn fn;
		/*
		 * Plugin A references host_callback, which resolves only through the
		 * runtime registry, so it must be published before the load or dlopen
		 * fails with "undefined symbol 'host_callback'" and never reaches the
		 * write that this mode exists to provoke. The full suite publishes it
		 * in section 1; this separate invocation has to do the same.
		 */
		dl_host_publish("host_callback", (void*)host_callback);
		h = dlopen("/lib/libdlplug_a.so", RTLD_NOW);
		if(h == NULL) {
			/* dlerror() clears the message, so read it ONCE into a local;
			   calling it twice (once to test, once to print) yields NULL. */
			char* e = dlerror();
			printf("WXFAULT: dlopen failed: %s\n",
			       (e != NULL) ? e : "(no dlerror message)");
			return 2;
		}
		fn = (a_add_fn)dlsym(h, "plug_a_add");
		if(fn == NULL) {
			printf("WXFAULT: dlsym(plug_a_add) failed\n");
			return 2;
		}
		printf("WXFAULT: storing into executable text at %p ...\n", (void*)fn);
		fflush(stdout);
		*(volatile int*)fn = 0xd65f03c0;   /* RET - a write into R+X text */
		printf("WXFAULT: no fault - W^X is NOT enforced\n");
		return 3;
	}

	/*
	 * leak mode: open several objects and exit WITHOUT dlclose, leaving them
	 * mapped when the process dies. Nothing in user space releases them, so the
	 * kernel alone must reclaim the whole DL window on exit (dl_window_release
	 * in proc_funeral). A dltest run right after this one - which opens its own
	 * objects, including the eight in section 14 - is the evidence that the
	 * teardown returned the slots and pages and did not corrupt the allocator.
	 * Uses the many-plugins, which need no host_callback, so the leak is the
	 * only thing this process does.
	 */
	if(argc > 1 && strcmp(argv[1], "leak") == 0) {
		int i, n = 0;
		char path[64];
		for(i = 1; i <= 4; i++) {
			snprintf(path, sizeof(path), "/lib/libdlplug_m%d.so", i);
			if(dlopen(path, RTLD_NOW) != NULL)
				n++;
		}
		printf("LEAK: left %d objects mapped, did NOT dlclose, exiting now\n", n);
		fflush(stdout);
		return 0;
	}

	(void)argc;
	(void)argv;

	printf("=== dltest: EwokOS dlopen() self-test ===\n");
	run_all();

	printf("\n=== %d checks, %d failures ===\n", g_checks, g_failures);
	if(g_failures == 0) {
		printf("DLTEST: PASS\n");
		return 0;
	}
	printf("DLTEST: FAIL\n");
	return 1;
}
