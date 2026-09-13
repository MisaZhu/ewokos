/* newlib ABI compatibility: the `_ctype_` character-class table.
 *
 * EwokOS's own <ctype.h> (libewoksys/include/ctype.h) implements the ctype
 * helpers as `static inline`, so EwokOS-native code never references this
 * symbol. The cross toolchain, however, still ships newlib's <ctype.h>, whose
 * macros expand to `(_ctype_+1)[(int)c]` and therefore need the 257-byte
 * `_ctype_` table that newlib's libc.a normally supplies. EwokOS replaces
 * newlib's libc with its own -lewoksys/-lc/-lgloss, so any third-party object
 * that happened to compile against newlib's <ctype.h> would fail to link with
 * "undefined reference to `_ctype_'".
 *
 * That happens in practice for the mario_js natives: if native_RegExp.c is
 * compiled before EwokOS's ctype.h has been installed into the SDK include dir
 * (or a stale pre-install object is served from ccache, whose direct-mode
 * manifest does not notice a newly added -I header), it binds to newlib's
 * table-based ctype.h and references `_ctype_`. Providing the table here makes
 * such objects link and behave exactly as they would under newlib, independent
 * of build order and ccache state.
 *
 * The bytes below are newlib's C-locale table verbatim (extracted from the
 * toolchain's libc_a-ctype_.o, .rodata._ctype_, 257 bytes). Layout: index 0 is
 * EOF (-1); indices 1..256 map to chars 0..255. Flag bits (newlib <ctype.h>):
 *   _U 0x01 upper  _L 0x02 lower  _N 0x04 digit  _S 0x08 space
 *   _P 0x10 punct  _C 0x20 control  _X 0x40 hex  _B 0x80 blank
 * Note newlib's quirks reproduced here: tab/LF/VT/FF/CR are _S|_C (0x28) and
 * NOT _B, while space is _S|_B (0x88); '0'-'9' carry only _N (0x04) because
 * isxdigit tests (_X|_N), so _X is set only on a-f/A-F. Keep in sync with the
 * toolchain table.
 */
const char _ctype_[257] = {
	/* 0x00 (EOF) */  0,
	/* 0x00-0x0f */  32, 32, 32, 32, 32, 32, 32, 32, 40, 40, 40, 40, 40, 32, 32, 32,
	/* 0x10-0x1f */  32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
	/* 0x20-0x2f */  136, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	/* 0x30-0x3f */  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 16, 16, 16, 16, 16, 16,
	/* 0x40-0x4f */  16, 65, 65, 65, 65, 65, 65, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	/* 0x50-0x5f */  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 16, 16, 16, 16, 16,
	/* 0x60-0x6f */  16, 66, 66, 66, 66, 66, 66, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	/* 0x70-0x7f */  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 16, 16, 16, 16, 32,
	/* 0x80-0xff: C locale leaves the extended range unclassified */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,
};
