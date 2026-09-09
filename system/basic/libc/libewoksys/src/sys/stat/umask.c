#include <sys/stat.h>

/* Per-process file mode creation mask. 0022 is the classic Unix default and
 * keeps the historical EwokOS behavior for existing callers
 * (0666 -> 0644, 0777 -> 0755, 0600 -> 0600).
 * fork() inherits it through the data segment copy; exec() resets it to the
 * default because the new image re-initializes .data/.bss. */
static mode_t _umask = 0022;

mode_t umask(mode_t mask) {
	mode_t old = _umask;
	_umask = mask & 0777;
	return old;
}
