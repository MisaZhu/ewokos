#include <hardware.h>
#include <irq.h>
#include <string.h>
#include <mm/mmu.h>
#include <dev/uart.h>

/* receive buffer */
#define RECEIVE_BUFFER_SIZE 16
static char receiveBuffer[RECEIVE_BUFFER_SIZE];
static int receiveBufferHead = 0;
static int receiveBufferTail = 0;

/* forward declarations for local functions */
static void uartInterruptHandler(void);
static int circularInc(int operand, int circleSize);

/*
 * uart_init intializes the receive_buffer, then setups up the interrupt
 * handler for the serial port.
 */
void uartInit(void)
{
	receiveBufferHead = 0;
	receiveBufferTail = 0;

	uartDevInit();
	registerInterruptHandler(UART_IRQ, uartInterruptHandler);
}

void uartPuts(const char* str) {
	int i = 0;
	while(true) {
		int c = (int)str[i++];
		if(c == 0)
			break;
		uartPutch(c);
	}
}

void uartPutch(int c) {
	if(c == '\r')
		c = '\n';
	uartTransmit(c);
}

/*
 * uartgetch reads the next available character from receive_buffer. If the buffer
 * is empty, this function returns 0.
 */
int uartGetch(void)
{
	int keycode = 0;

	if(receiveBufferHead != receiveBufferTail) {
		keycode = receiveBuffer[receiveBufferTail];
		receiveBufferTail = circularInc(receiveBufferTail, RECEIVE_BUFFER_SIZE);
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
static void uartInterruptHandler(void)
{
	while (uartReadyToRecv()) {
		int newHead = 0;
		int data = uartReceive();

		newHead = circularInc(receiveBufferHead,
						       RECEIVE_BUFFER_SIZE);
		if (newHead != receiveBufferTail) {
			/* buffer not full */
			receiveBuffer[receiveBufferHead] = (char) data;
			receiveBufferHead = newHead;
		}
	}
}

