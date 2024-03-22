#include <types.h>
#include <stdlib.h>
#include <stdio.h>
#include <utils/log.h>

#include "brcm.h"


static struct brcmf_console *_console = NULL;

void brcm_console_init(uint32_t addr){
    if(!_console){
        _console = calloc(1, sizeof(struct brcmf_console));
    }
    _console->console_addr = addr;
}


int brcmf_sdio_readconsole(void)
{
    u8 line[CONSOLE_LINE_MAX], ch;
    u32 n, idx, addr;
    int rv;

    /* Don't do anything until FWREADY updates console address */
    if (!_console || !_console->console_addr)
        return 0;

    /* Read console log struct */
    addr = _console->console_addr + offsetof(struct rte_console, log_le);
    rv = brcmf_sdiod_ramrw(false, addr, (u8 *)&_console->log_le,
                   sizeof(_console->log_le));
    if (rv < 0)
        return rv;

    /* Allocate console buffer (one time only) */
    if (_console->buf == NULL) {
        _console->bufsize = le32_to_cpu(_console->log_le.buf_size);
        _console->buf = malloc(_console->bufsize);
        if (_console->buf == NULL)
            return -ENOMEM;
    }

    idx = le32_to_cpu(_console->log_le.idx);

    /* Protect against corrupt value */
    if (idx > _console->bufsize)
        return -EBADE;

    /* Skip reading the console buffer if the index pointer
     has not moved */
    if (idx == _console->last)
        return 0;

    /* Read the console buffer */
    addr = le32_to_cpu(_console->log_le.buf);
    rv = brcmf_sdiod_ramrw(false, addr, _console->buf, _console->bufsize);
    if (rv < 0)
        return rv;

    while (_console->last != idx) {
        for (n = 0; n < CONSOLE_LINE_MAX - 2; n++) {
            if (_console->last == idx) {
                /* This would output a partial line.
                 * Instead, back up
                 * the buffer pointer and output this
                 * line next time around.
                 */
                if (_console->last >= n)
                    _console->last -= n;
                else
                    _console->last = _console->bufsize - n;
                goto break2;
            }
            ch = _console->buf[_console->last];
            _console->last = (_console->last + 1) % _console->bufsize;
            if (ch == '\n')
                break;
            line[n] = ch;
        }

        if (n > 0) {
            if (line[n - 1] == '\r')
                n--;
            line[n] = 0;
            brcm_log("CONSOLE: %s\n", line);
        }
    }
break2:

    return 0;
}

void brcm_console_free(void){
    free(_console);
}