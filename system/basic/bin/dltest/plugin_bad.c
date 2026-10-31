/*
 * Test plugin BAD - deliberately unloadable.
 *
 * Everything else in this directory is a success case, and a loader that only
 * ever passes is not much evidence. This object references a symbol that
 * exists nowhere - not in the plugin, not in the host's generated export table,
 * not in the runtime registry - so dlopen() must fail during relocation and
 * dlerror() must say which symbol was missing.
 *
 * The reference is a plain undefined extern (not weak), because a weak symbol
 * is allowed to stay unresolved: dl_relocate() binds it to 0 and the load
 * succeeds, which is the correct POSIX behaviour but the wrong thing to test
 * here.
 *
 * dltest only checks that dlopen() returns NULL and prints the message. It does
 * not match the text, because the wording belongs to the loader and a test that
 * pins it breaks for reasons unrelated to correctness.
 */

/* never defined anywhere */
extern int this_symbol_does_not_exist_anywhere(int x);

int plug_bad_entry(int x) {
	return this_symbol_does_not_exist_anywhere(x) + 1;
}
