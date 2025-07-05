#ifndef MAILBOX_H
#define MAILBOX_H

#include <stdint.h>
#include <ewoksys/mmio.h>

#define MAILBOX_BASE (_mmio_base+0xB880)
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

uint32_t bcm283x_mailbox_init(void);
void     bcm283x_mailbox_read(mail_message_t* msg);
void     bcm283x_mailbox_send(mail_message_t* msg);
void     bcm283x_mailbox_call(mail_message_t* msg);

#endif
