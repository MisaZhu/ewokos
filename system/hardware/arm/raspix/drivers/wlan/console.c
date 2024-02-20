#include "types.h"
#include "include/brcm.h"

static struct brcmf_console *c;

void brcm_console_init(uint32_t addr){
    c = malloc(sizeof(struct brcmf_console));
    memset(c, 0, sizeof(struct brcmf_console));
    c->console_addr = addr;
    klog("%s %x\n", __func__, addr);
}


int brcmf_sdio_readconsole(void)
{
    u8 line[CONSOLE_LINE_MAX], ch;
    u32 n, idx, addr;
    int rv;

    /* Don't do anything until FWREADY updates console address */
    if (!c && c->console_addr == 0)
        return 0;

    /* Read console log struct */
    addr = c->console_addr + offsetof(struct rte_console, log_le);
    rv = brcmf_sdiod_ramrw(false, addr, (u8 *)&c->log_le,
                   sizeof(c->log_le));
    if (rv < 0)
        return rv;

    /* Allocate console buffer (one time only) */
    if (c->buf == NULL) {
        c->bufsize = le32_to_cpu(c->log_le.buf_size);
        klog("%d\n", c->bufsize);
        c->buf = malloc(c->bufsize);
        if (c->buf == NULL)
            return -ENOMEM;
    }

    idx = le32_to_cpu(c->log_le.idx);

    /* Protect against corrupt value */
    if (idx > c->bufsize)
        return -EBADE;

    /* Skip reading the console buffer if the index pointer
     has not moved */
    if (idx == c->last)
        return 0;

    /* Read the console buffer */
    addr = le32_to_cpu(c->log_le.buf);
    rv = brcmf_sdiod_ramrw(false, addr, c->buf, c->bufsize);
    if (rv < 0)
        return rv;

    while (c->last != idx) {
        for (n = 0; n < CONSOLE_LINE_MAX - 2; n++) {
            if (c->last == idx) {
                /* This would output a partial line.
                 * Instead, back up
                 * the buffer pointer and output this
                 * line next time around.
                 */
                if (c->last >= n)
                    c->last -= n;
                else
                    c->last = c->bufsize - n;
                goto break2;
            }
            ch = c->buf[c->last];
            c->last = (c->last + 1) % c->bufsize;
            if (ch == '\n')
                break;
            line[n] = ch;
        }

        if (n > 0) {
            if (line[n - 1] == '\r')
                n--;
            line[n] = 0;
            klog("CONSOLE: %s\n", line);
        }
    }
break2:

    return 0;
}