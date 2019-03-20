#include <hardware.h>
#include <irq.h>
#include <kstring.h>
#include <mm/mmu.h>
#include <dev/uart.h>

/* receive buffer */
#define RECEIVE_BUFFER_SIZE 16
static char recv_buf[RECEIVE_BUFFER_SIZE];
static int recv_buf_head = 0;
static int recv_buf_tail = 0;

/* forward declarations for local functions */
static void uart_interrupt_handler(void);
static int circularInc(int operand, int circleSize);

/*
 * uart_init intializes the receive_buffer, then setups up the interrupt
 * handler for the serial port.
 */
void uart_init(void)
{
	recv_buf_head = 0;
	recv_buf_tail = 0;

	uart_dev_init();
	register_interrupt_handler(get_uart_irq(), uart_interrupt_handler);
}

void uart_puts(const char* str) {
	int i = 0;
	while(true) {
		int c = (int)str[i++];
		if(c == 0)
			break;
		uart_putch(c);
	}
}

void uart_putch(int c) {
	if(c == '\r')
		c = '\n';
	uart_trans(c);
}

/*
 * uartgetch reads the next available character from receive_buffer. If the buffer
 * is empty, this function returns 0.
 */
int uart_getch(void)
{
	int keycode = 0;

	if(recv_buf_head != recv_buf_tail) {
		keycode = recv_buf[recv_buf_tail];
		recv_buf_tail = circularInc(recv_buf_tail, RECEIVE_BUFFER_SIZE);
	}

	return keycode;
}

/* circular_inc increments operand modula circle_size. */
static int circularInc(int operand, int circleSize)
{
	operand++;
	if (operand == circleSize)
		operand = 0;
	return operand;
}

/*
 * uart_interrupt_handler reads a character from the uart0 serial port, and
 * puts it into receive_buffer. If the buffer is full, the character is ignored.
 */
static void uart_interrupt_handler(void)
{
	while (uart_ready_to_recv()) {
		int newHead = 0;
		int data = uart_recv();

		newHead = circularInc(recv_buf_head,
						       RECEIVE_BUFFER_SIZE);
		if (newHead != recv_buf_tail) {
			/* buffer not full */
			recv_buf[recv_buf_head] = (char) data;
			recv_buf_head = newHead;
		}
	}
}

