#ifndef MAILBOX_H
#define MAILBOX_H

#include <stdint.h>
#include <kernel/hw_info.h>

#define MAILBOX_BASE (MMIO_BASE+0xB880)
#define MAIL0_READ (((volatile mail_message_t *)(0x00 + MAILBOX_BASE)))
#define MAIL0_STATUS (((volatile mail_status_t *)(0x18 + MAILBOX_BASE)))
#define MAIL0_WRITE (((volatile mail_message_t *)(0x20 + MAILBOX_BASE)))
#define PROPERTY_CHANNEL 8
#define FRAMEBUFFER_CHANNEL 1

typedef struct {
    uint8_t channel: 4;
    uint32_t data: 28;
} mail_message_t;

typedef struct {
    uint32_t reserved: 30;
    uint8_t empty: 1;
    uint8_t full:1;
} mail_status_t;

void mailbox_read(uint8_t channel, mail_message_t* msg);
void mailbox_send(uint8_t channel, mail_message_t* msg);

#endif
