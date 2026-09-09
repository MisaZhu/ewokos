#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Global clipboard stored at /tmp/.clipboard (ramfs, mode 0666):
 * every user can read and write it.
 *
 * Write contract: whole-content overwrite ONLY. Each set call truncates
 * the backing file and writes the complete new payload, so a shorter
 * write never leaves a stale tail of the previous content. There is
 * intentionally no append or partial-update API.
 */

/* Overwrite the whole clipboard with `size` bytes from `data`
 * (data may be NULL when size is 0). Returns 0 on success, -1 on error. */
int clipboard_set(const void* data, uint32_t size);

/* Overwrite the whole clipboard with a NUL-terminated string
 * (the NUL itself is not stored). Returns 0 on success, -1 on error. */
int clipboard_set_text(const char* text);

/* Read up to `size` bytes of the current content into `buf`.
 * Returns the number of bytes read (0 when empty), -1 on error. */
int clipboard_get(void* buf, uint32_t size);

/* Size in bytes of the current content, 0 when empty or not created yet. */
uint32_t clipboard_size(void);

/* Return a malloc'ed NUL-terminated copy of the current content
 * ("" when empty), or NULL on error. Caller must free(). */
char* clipboard_get_text(void);

/* Clear the clipboard (overwrite with empty content).
 * Returns 0 on success, -1 on error. */
int clipboard_clear(void);

#ifdef __cplusplus
}
#endif

#endif
