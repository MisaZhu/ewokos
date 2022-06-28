#include <sys/syscall.h>
#include <arch/bcm283x/mailbox.h>

uint32_t bcm283x_mailbox_init(void) {
	return mmio_map();
}

void bcm283x_mailbox_read(int channel, mail_message_t *msg) {
	mail_status_t stat;

	// Make sure that the message is from the right channel
	do {
		// Make sure there is mail to recieve
		do {
			stat = *MAIL0_STATUS;
		} while (stat.empty);

		// Get the message
		*msg = *MAIL0_READ;
	} while (msg->channel != channel);
}

void  bcm283x_mailbox_send(int channel, mail_message_t* msg) {
	mail_status_t stat;
	msg->channel = channel;

	// Make sure you can send mail
	do {
		stat = *MAIL0_STATUS;
	} while (stat.full);

	// send the message
	*MAIL0_WRITE = *msg;
}

