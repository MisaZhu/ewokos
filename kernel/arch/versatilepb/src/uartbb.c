#include <dev/uartbb.h>
#include <dev/gpio.h>
#include <timer.h>
/*----------------------------------------------------------------------------*/
/**
 * - implementing bit banging UART
 *   = fixed baud-rate at 9600 (8N1)
 *   = needs the gpio and timer module (initialized in main!)
**/
/*----------------------------------------------------------------------------*/
#define B9600_TX_DELAY 104
#define B9600_RX_DELAY 104

int32_t uartbb_rx,uartbb_tx;
void uartbb_init(int32_t rx_gpio,int32_t tx_gpio) {
	/* set the requested */
	uartbb_rx = rx_gpio;
	uartbb_tx = tx_gpio;
	/* setup pins! */
	gpio_config(uartbb_rx,GPIO_INPUT);
	gpio_config(uartbb_tx,GPIO_OUTPUT);
	/* disable pulls on tx/rx pins? */
	gpio_pull(uartbb_rx,GPIO_PULL_NONE);
	gpio_pull(uartbb_tx,GPIO_PULL_NONE);
	/* mark - logic hi */
	gpio_set(uartbb_tx);
	timer_wait(B9600_TX_DELAY);
}

void uartbb_send(uint32_t data) {
	int32_t loop;
	/* start bit */
	gpio_clr(uartbb_tx);
	timer_wait(B9600_TX_DELAY);
	for (loop=0;loop<8;loop++) {
		if (data&0x01) gpio_set(uartbb_tx);
		else gpio_clr(uartbb_tx);
		timer_wait(B9600_TX_DELAY);
		data >>= 1;
	}
	/* stop bit */
	gpio_set(uartbb_tx);
	timer_wait(B9600_TX_DELAY);
}

void uartbb_find_stop(void) {
	uint32_t init;
	int32_t not_done = 1;
	do {
		/* in case we're low, wait for stop bit (high) */
		while(!gpio_read(uartbb_rx));
		/* mark start-of-high time */
		init = timer_read();
		while(gpio_read(uartbb_rx)) {
			if(timer_read()-init>B9600_RX_DELAY) {
				not_done = 0; break;
			}
		}
	} while(not_done);
}

uint32_t uartbb_read(void) {
	int32_t loop;
	uint32_t data = 0x00;
	/* wait start bit */
	while(gpio_read(uartbb_rx));
	timer_wait(B9600_RX_DELAY);
	/* we read at half period */
	timer_wait(B9600_RX_DELAY/2);
	for (loop=0;loop<8;loop++) {
		data >>= 1;
		if(gpio_read(uartbb_rx)) data |= 0x80;
		timer_wait(B9600_RX_DELAY);
	}
	/* ignore stop bit? */
	if(!gpio_read(uartbb_rx))
		data = 0x00;
	timer_wait(B9600_RX_DELAY/2);
	return data;
}

void uartbb_print_hex_byte(unsigned char byte) {
	unsigned char temp = (byte & 0xf0) >> 4;
	if(temp>9) temp = (temp-10)+0x41;
	else temp += 0x30;
	uartbb_send(temp);
	temp = (byte & 0x0f);
	if(temp>9) temp = (temp-10)+0x41;
	else temp += 0x30;
	uartbb_send(temp);
}

void uartbb_print_hex_uint(uint32_t value) {
	int32_t loop, temp;
	for(loop=3;loop>=0;loop--) {
		temp = (value >> (loop*8)) & 0xff;
		uartbb_print_hex_byte((unsigned char)temp);
	}
}

void uartbb_print(char *message) {
	while(*message) {
		uartbb_send((unsigned int)*message);
		message++;
	}
}
