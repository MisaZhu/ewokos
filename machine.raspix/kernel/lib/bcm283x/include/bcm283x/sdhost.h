#ifndef _SDHOST_H_
#define _SDHOST_H_

#include <bcm283x/sdmmc.h>

struct bcm2835_plat {
    struct mmc_config cfg;
    struct mmc mmc;
};

struct bcm2835_host {
    uint8_t *ioaddr;
    int         clock;      /* Current clock speed */
    unsigned int        max_clk;    /* Max possible freq */
    unsigned int        blocks;     /* remaining PIO blocks */

    u32         ns_per_fifo_word;

    /* cached registers */
    u32         hcfg;
    u32         cdiv;

    struct mmc_cmd  *cmd;       /* Current command */
    struct mmc_data     *data;      /* Current data request */
    bool            use_busy:1; /* Wait for busy interrupt */

    struct mmc      *mmc;
    struct bcm2835_plat *plat;
    unsigned int        firmware_sets_cdiv:1;
};

#endif
