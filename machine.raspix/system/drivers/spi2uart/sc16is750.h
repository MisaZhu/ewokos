#ifndef MAIN_SC16IS750_H_
#define MAIN_SC16IS750_H_

#include <stdint.h>

//Device Address

//A:VDD
//B:GND
//C:SCL
//D:SDA
#define  SC16IS750_ADDRESS_AA  (0X90)
#define  SC16IS750_ADDRESS_AB  (0X92)
#define  SC16IS750_ADDRESS_AC  (0X94)
#define  SC16IS750_ADDRESS_AD  (0X96)
#define  SC16IS750_ADDRESS_BA  (0X98)
#define  SC16IS750_ADDRESS_BB  (0X9A)
#define  SC16IS750_ADDRESS_BC  (0X9C)
#define  SC16IS750_ADDRESS_BD  (0X9E)
#define  SC16IS750_ADDRESS_CA  (0XA0)
#define  SC16IS750_ADDRESS_CB  (0XA2)
#define  SC16IS750_ADDRESS_CC  (0XA4)
#define  SC16IS750_ADDRESS_CD  (0XA6)
#define  SC16IS750_ADDRESS_DA  (0XA8)
#define  SC16IS750_ADDRESS_DB  (0XAA)
#define  SC16IS750_ADDRESS_DC  (0XAC)
#define  SC16IS750_ADDRESS_DD  (0XAE)


//General Registers
#define   SC16IS750_REG_RHR       (0x00)
#define   SC16IS750_REG_THR       (0X00)
#define   SC16IS750_REG_IER       (0X01)
#define   SC16IS750_REG_FCR       (0X02)
#define   SC16IS750_REG_IIR       (0X02)
#define   SC16IS750_REG_LCR       (0X03)
#define   SC16IS750_REG_MCR       (0X04)
#define   SC16IS750_REG_LSR       (0X05)
#define   SC16IS750_REG_MSR       (0X06)
#define   SC16IS750_REG_SPR       (0X07)
#define   SC16IS750_REG_TCR       (0X06)
#define   SC16IS750_REG_TLR       (0X07)
#define   SC16IS750_REG_TXLVL     (0X08)
#define   SC16IS750_REG_RXLVL     (0X09)
#define   SC16IS750_REG_IODIR     (0X0A)
#define   SC16IS750_REG_IOSTATE   (0X0B)
#define   SC16IS750_REG_IOINTENA  (0X0C)
#define   SC16IS750_REG_IOCONTROL (0X0E)
#define   SC16IS750_REG_EFCR      (0X0F)

//Special Registers
#define   SC16IS750_REG_DLL (0x00)
#define   SC16IS750_REG_DLH (0X01)

//Enhanced Registers
#define   SC16IS750_REG_EFR   (0X02)
#define   SC16IS750_REG_XON1  (0X04)
#define   SC16IS750_REG_XON2  (0X05)
#define   SC16IS750_REG_XOFF1 (0X06)
#define   SC16IS750_REG_XOFF2 (0X07)

//Interrupt Enable Register
#define   SC16IS750_INT_CTS   (0X80)
#define   SC16IS750_INT_RTS   (0X40)
#define   SC16IS750_INT_XOFF  (0X20)
#define   SC16IS750_INT_SLEEP (0X10)
#define   SC16IS750_INT_MODEM (0X08)
#define   SC16IS750_INT_LINE  (0X04)
#define   SC16IS750_INT_THR   (0X02)
#define   SC16IS750_INT_RHR   (0X01)

//Interrupt Identification Register
enum SC16IS750_IIR {
  SC16IS750_RECEIVE_LINE_STATUS_ERROR,
  SC16IS750_RECEIVE_TIMEOUT_INTERRUPT,
  SC16IS750_RHR_INTERRUPT,
  SC16IS750_THR_INTERRUPT,
  SC16IS750_MODEM_INTERRUPT,
  SC16IS750_INPUT_PIN_CHANGE_STATE,
  SC16IS750_RECEIVE_XOFF,
  SC16IS750_CTS_RTS_CHANGE
};

//Application Related 

//#define SC16IS750_CRYSTCAL_FREQ (14745600UL) 
//#define SC16IS750_CRYSTCAL_FREQ (1843200UL)   
//#define SC16IS750_CRYSTCAL_FREQ (16000000UL)    
//#define SC16IS750_DEBUG_PRINT (0)
#define   SC16IS750_PROTOCOL_I2C  (0)
#define   SC16IS750_PROTOCOL_SPI  (1)

#define   SC16IS750_CHANNEL      0x00
#define   SC16IS750_CHANNEL_A    0x00
#define   SC16IS750_CHANNEL_B    0x01
#define   SC16IS750_CHANNEL_BOTH 0x00
#define   SC16IS750_CHANNEL_NONE 0xFF

#define   SC16IS750_SINGLE_CHANNEL 0x01
#define   SC16IS750_DUAL_CHANNEL   0x02

#define   SC16IS750_DEFAULT_SPEED 19200 

typedef struct {
  int i2c_fd;
  int spi_channel;
  uint8_t device_address_i2c;
  uint8_t device_address_sspin;
  uint8_t protocol;
  int peek_buf;
  uint8_t peek_flag;
  long crystal_freq;
  uint32_t timeout;
  int channels;
} SC16IS750_t;

  void    SC16IS750_init(SC16IS750_t * dev, uint8_t protocol, uint8_t address, int channels);
  void    SC16IS750_begin(SC16IS750_t * dev, uint32_t baud_A, uint32_t baud_B, long crystal_freq);     
  int     SC16IS750_read(SC16IS750_t * dev, uint8_t channel);
  void    SC16IS750_write(SC16IS750_t * dev, uint8_t channel, uint8_t val);
  int     SC16IS750_available(SC16IS750_t * dev, uint8_t channel);
  void    SC16IS750_pinMode(SC16IS750_t * dev, uint8_t pin, uint8_t io);
  void    SC16IS750_digitalWrite(SC16IS750_t * dev, uint8_t pin, uint8_t value);
  uint8_t SC16IS750_digitalRead(SC16IS750_t * dev, uint8_t pin);
  uint8_t SC16IS750_ping(SC16IS750_t * dev);
  void    SC16IS750_setTimeout(SC16IS750_t * dev, uint32_t);
  size_t  SC16IS750_readBytes(SC16IS750_t * dev, uint8_t channel, char *buffer, size_t length);
  int16_t SC16IS750_readwithtimeout(SC16IS750_t * dev, uint8_t * channel);
  int     SC16IS750_peek(SC16IS750_t * dev, uint8_t channel);
  void    SC16IS750_flush(SC16IS750_t * dev, uint8_t channel);
  uint8_t SC16IS750_GPIOGetPortState(SC16IS750_t * dev);
  uint8_t SC16IS750_InterruptPendingTest(SC16IS750_t * dev, uint8_t channel);
	int16_t SC16IS750_InterruptEventTest(SC16IS750_t * dev, uint8_t channel);
  void    SC16IS750_SetPinInterrupt(SC16IS750_t * dev, uint8_t io_int_ena);
  void    SC16IS750_InterruptControl(SC16IS750_t * dev, uint8_t channel, uint8_t int_ena);
  void    SC16IS750_ModemPin(SC16IS750_t * dev, uint8_t gpio); //gpio == 0, gpio[7:4] are modem pins, gpio == 1 gpio[7:4] are gpios
  void    SC16IS750_GPIOLatch(SC16IS750_t * dev, uint8_t latch);
  
  int16_t SC16IS750_SetBaudrate(SC16IS750_t * dev, uint8_t channel, uint32_t baudrate);
  uint8_t SC16IS750_ReadRegister(SC16IS750_t * dev, uint8_t channel, uint8_t reg_addr);
  void    SC16IS750_WriteRegister(SC16IS750_t * dev, uint8_t channel, uint8_t reg_addr, uint8_t val);
  void    SC16IS750_SetLine(SC16IS750_t * dev, uint8_t channel, uint8_t data_length, uint8_t parity_select, uint8_t stop_length );
  void    SC16IS750_GPIOSetPinMode(SC16IS750_t * dev, uint8_t pin_number, uint8_t i_o);
  void    SC16IS750_GPIOSetPinState(SC16IS750_t * dev, uint8_t pin_number, uint8_t pin_state);
  
  uint8_t SC16IS750_GPIOGetPinState(SC16IS750_t * dev, uint8_t pin_number);
  void    SC16IS750_GPIOSetPortMode(SC16IS750_t * dev, uint8_t port_io);
  void    SC16IS750_GPIOSetPortState(SC16IS750_t * dev, uint8_t port_state);
  void    SC16IS750_ResetDevice(SC16IS750_t * dev);
  
  void    SC16IS750_FIFOEnable(SC16IS750_t * dev, uint8_t channel, uint8_t fifo_enable);
  void    SC16IS750_FIFOReset(SC16IS750_t * dev, uint8_t channel, uint8_t rx_fifo);
  void    SC16IS750_FIFOSetTriggerLevel(SC16IS750_t * dev, uint8_t channel, uint8_t rx_fifo, uint8_t length);
  uint8_t SC16IS750_FIFOAvailableData(SC16IS750_t * dev, uint8_t channel);
  uint8_t SC16IS750_FIFOAvailableSpace(SC16IS750_t * dev, uint8_t channel);
  void    SC16IS750_WriteByte(SC16IS750_t * dev, uint8_t channel, uint8_t val);
  int     SC16IS750_ReadByte(SC16IS750_t * dev, uint8_t channel);
  void    SC16IS750_EnableTransmit(SC16IS750_t * dev, uint8_t channel, uint8_t tx_enable);

#endif