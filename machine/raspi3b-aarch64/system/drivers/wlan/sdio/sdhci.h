#ifndef __SDHCI_H__
#define __SDHCI_H__

#include <types.h>
#include "mmc.h"


void sdhci_init(void);
int sdhci_set_ios(struct mmc *mmc);
int  sdhci_send_command(struct mmc_cmd *cmd, struct mmc_data *data);
void sdhci_enable_irq(int enable);
#endif