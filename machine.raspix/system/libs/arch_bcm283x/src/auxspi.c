/*----------------------------------------------------------------------------*/
/**
 * - for accessing spi
 *   = this is using spi0 (spi-specific interface)
 *   = bcm2835 has 2 more mini-spi interfaces (spi1/spi2)
 *     $ part of auxiliary peripheral (along with mini-uart)
 *     $ NOT using these
 **/
#include <arch/bcm283x/gpio.h>
#include <arch/bcm283x/spi.h>
#include <ewoksys/mmio.h>

#define AUX_OFFSET  0x00215000
#define SPI1_OFFSET 0x00215080
#define SPI2_OFFSET 0x002150C0
#define AUX_BASE    (_mmio_base | AUX_OFFSET)

static uint32_t  SPI_BASE =  0;

/*AUX register offset*/
#define BCM2835_AUX_IRQ			(AUX_BASE + 0x0000)  /*!< xxx */
#define BCM2835_AUX_ENABLE		(AUX_BASE + 0x0004)  /*!< */

#define BCM2835_AUX_ENABLE_UART1	0x01    /*!<  */
#define BCM2835_AUX_ENABLE_SPI0		0x02	/*!< SPI0 (SPI1 in the device) */
#define BCM2835_AUX_ENABLE_SPI1		0x04	/*!< SPI1 (SPI2 in the device) */


/* SPI register offsets */
#define BCM2835_AUX_SPI_CNTL0	(SPI_BASE + 0x00)
#define BCM2835_AUX_SPI_CNTL1	(SPI_BASE + 0x04)
#define BCM2835_AUX_SPI_STAT	(SPI_BASE + 0x08)
#define BCM2835_AUX_SPI_PEEK	(SPI_BASE + 0x0C)
#define BCM2835_AUX_SPI_IO	    (SPI_BASE + 0x20)
#define BCM2835_AUX_SPI_TXHOLD	(SPI_BASE + 0x30)

/* Bitfields in CNTL0 */
#define BCM2835_AUX_SPI_CNTL0_SPEED	0xFFF00000
#define BCM2835_AUX_SPI_CNTL0_SPEED_MAX	0xFFF
#define BCM2835_AUX_SPI_CNTL0_SPEED_SHIFT	20
#define BCM2835_AUX_SPI_CNTL0_CS	0x000E0000
#define BCM2835_AUX_SPI_CNTL0_POSTINPUT	0x00010000
#define BCM2835_AUX_SPI_CNTL0_VAR_CS	0x00008000
#define BCM2835_AUX_SPI_CNTL0_VAR_WIDTH	0x00004000
#define BCM2835_AUX_SPI_CNTL0_DOUTHOLD	0x00003000
#define BCM2835_AUX_SPI_CNTL0_ENABLE	0x00000800
#define BCM2835_AUX_SPI_CNTL0_IN_RISING	0x00000400
#define BCM2835_AUX_SPI_CNTL0_CLEARFIFO	0x00000200
#define BCM2835_AUX_SPI_CNTL0_OUT_RISING	0x00000100
#define BCM2835_AUX_SPI_CNTL0_CPOL	0x00000080
#define BCM2835_AUX_SPI_CNTL0_MSBF_OUT	0x00000040
#define BCM2835_AUX_SPI_CNTL0_SHIFTLEN	0x0000003F

/* Bitfields in CNTL1 */
#define BCM2835_AUX_SPI_CNTL1_CSHIGH	0x00000700
#define BCM2835_AUX_SPI_CNTL1_TXEMPTY	0x00000080
#define BCM2835_AUX_SPI_CNTL1_IDLE	0x00000040
#define BCM2835_AUX_SPI_CNTL1_MSBF_IN	0x00000002
#define BCM2835_AUX_SPI_CNTL1_KEEP_IN	0x00000001

/* Bitfields in STAT */
#define BCM2835_AUX_SPI_STAT_TX_LVL	0xFF000000
#define BCM2835_AUX_SPI_STAT_RX_LVL	0x00FF0000
#define BCM2835_AUX_SPI_STAT_TX_FULL	0x00000400
#define BCM2835_AUX_SPI_STAT_TX_EMPTY	0x00000200
#define BCM2835_AUX_SPI_STAT_RX_FULL	0x00000100
#define BCM2835_AUX_SPI_STAT_RX_EMPTY	0x00000080
#define BCM2835_AUX_SPI_STAT_BUSY	0x00000040
#define BCM2835_AUX_SPI_STAT_BITCOUNT	0x0000003F

#define BCM2835_AUX_SPI_CLOCK_MIN	30500		/*!< 30,5kHz */
#define BCM2835_AUX_SPI_CLOCK_MAX	125000000 	/*!< 125Mhz */
#define BCM2835_CORE_CLK_HZ		250000000	/*!< 250 MHz */

struct bcm2835aux_spi{
    uint32_t ctrl0;
    uint32_t ctrl1;
    uint32_t stats;
    uint32_t peek[4];
    uint32_t io[4];
    uint32_t txhold[4]; 
};

#define SPI1_SCLK 21
#define SPI1_MOSI 20
#define SPI1_MISO 19

#define MIN(a, b)   (((a)>(b))?(b):(a))
#define READ32(addr) (*((volatile uint32_t *)(addr)))
#define WRITE32(addr, val) (*((volatile uint32_t *)(addr)) = (uint32_t)(val))

#define DIV_ROUND_UP(n,d)	(((n) + (d) - 1) / (d))

uint16_t bcm283x_aux_spi_CalcClockDivider(int speed_hz)
{
    uint16_t divider;

    if (speed_hz < (uint32_t) BCM2835_AUX_SPI_CLOCK_MIN) {
	    speed_hz = (uint32_t) BCM2835_AUX_SPI_CLOCK_MIN;
    } else if (speed_hz > (uint32_t) BCM2835_AUX_SPI_CLOCK_MAX) {
	    speed_hz = (uint32_t) BCM2835_AUX_SPI_CLOCK_MAX;
    }

    divider = (uint16_t) DIV_ROUND_UP(BCM2835_CORE_CLK_HZ, 2 * speed_hz) - 1;

    if (divider > (uint16_t) BCM2835_AUX_SPI_CNTL0_SPEED_MAX) {
	    return (uint16_t) BCM2835_AUX_SPI_CNTL0_SPEED_MAX;
    }

    return divider;
}

static uint16_t _spi_clock_div = 0;

void bcm283x_auxspi_set_clock(int clock){
    _spi_clock_div = bcm283x_aux_spi_CalcClockDivider(clock); 
}

int bcm283x_auxspi_init(int ch)
{
    (void)ch;

    if(ch == 0){
        klog("bcm283x_auxspi_init\n");
        bcm283x_gpio_config(SPI1_SCLK, GPIO_ALTF4);
	    bcm283x_gpio_config(SPI1_MOSI, GPIO_ALTF4);
	    bcm283x_gpio_config(SPI1_MISO, GPIO_ALTF4);
        SPI_BASE = _mmio_base + SPI1_OFFSET;
        WRITE32(BCM2835_AUX_ENABLE, BCM2835_AUX_ENABLE_SPI0|READ32(BCM2835_AUX_ENABLE));
    }else{
        SPI_BASE = _mmio_base + SPI2_OFFSET;
        WRITE32(BCM2835_AUX_ENABLE, BCM2835_AUX_ENABLE_SPI1|READ32(BCM2835_AUX_ENABLE));

    }
    WRITE32(BCM2835_AUX_SPI_CNTL1, 0);
    WRITE32(BCM2835_AUX_SPI_CNTL0, BCM2835_AUX_SPI_CNTL0_CLEARFIFO);  
    /*Default 1Mhz*/
    bcm283x_auxspi_set_clock(1000000);
    
    return 1; // OK
}

void bcm283x_auxspi_write16(uint16_t data)
{
    uint32_t reg = (_spi_clock_div << BCM2835_AUX_SPI_CNTL0_SPEED_SHIFT);
    reg |= BCM2835_AUX_SPI_CNTL0_ENABLE;
    reg |= BCM2835_AUX_SPI_CNTL0_MSBF_OUT;
    reg |= 16; // Shift length

    WRITE32(BCM2835_AUX_SPI_CNTL0, reg);
    WRITE32(BCM2835_AUX_SPI_CNTL1, BCM2835_AUX_SPI_CNTL1_MSBF_IN);

    while (READ32(BCM2835_AUX_SPI_STAT) & BCM2835_AUX_SPI_STAT_TX_FULL);

    WRITE32(BCM2835_AUX_SPI_IO, (uint32_t) data << 16);
}


void bcm283x_auxspi_transfer(const char *tbuf, char *rbuf, uint32_t len) {

	char *tx = (char *)tbuf;
	char *rx = (char *)rbuf;
	uint32_t tx_len = len;
	uint32_t rx_len = len;
	uint32_t count;
	uint32_t data;
	uint32_t i;
	uint8_t byte;

	uint32_t reg = (_spi_clock_div << BCM2835_AUX_SPI_CNTL0_SPEED_SHIFT);
	reg |= BCM2835_AUX_SPI_CNTL0_ENABLE;
	reg |= BCM2835_AUX_SPI_CNTL0_MSBF_OUT;
	reg |= BCM2835_AUX_SPI_CNTL0_VAR_WIDTH;

	WRITE32(BCM2835_AUX_SPI_CNTL0, reg);
	WRITE32(BCM2835_AUX_SPI_CNTL1, BCM2835_AUX_SPI_CNTL1_MSBF_IN);

	while ((tx_len > 0) || (rx_len > 0)) {

		while (!(READ32(BCM2835_AUX_SPI_STAT) & BCM2835_AUX_SPI_STAT_TX_FULL) && (tx_len > 0)) {
			count = MIN(tx_len, 3);
			data = 0;

			for (i = 0; i < count; i++) {
				byte = (tx != NULL) ? (uint8_t) *tx++ : (uint8_t) 0;
				data |= byte << (8 * (2 - i));
			}

			data |= (count * 8) << 24;
			tx_len -= count;

			if (tx_len != 0) {
				WRITE32(BCM2835_AUX_SPI_TXHOLD, data);
			} else {
				WRITE32(BCM2835_AUX_SPI_IO, data);
			}

		}

		while (!(READ32(BCM2835_AUX_SPI_STAT) & BCM2835_AUX_SPI_STAT_RX_EMPTY) && (rx_len > 0)) {
			count = MIN(rx_len, 3);
			data = READ32(BCM2835_AUX_SPI_IO);

			if (rbuf != NULL) {
				switch (count) {
				case 3:
					*rx++ = (char)((data >> 16) & 0xFF);
					/*@fallthrough@*/
					/* no break */
				case 2:
					*rx++ = (char)((data >> 8) & 0xFF);
					/*@fallthrough@*/
					/* no break */
				case 1:
					*rx++ = (char)((data >> 0) & 0xFF);
				}
			}

			rx_len -= count;
		}

		while (!(READ32(BCM2835_AUX_SPI_STAT) & BCM2835_AUX_SPI_STAT_BUSY) && (rx_len > 0)) {
			count = MIN(rx_len, 3);
			data = READ32(BCM2835_AUX_SPI_IO);

			if (rbuf != NULL) {
				switch (count) {
				case 3:
					*rx++ = (char)((data >> 16) & 0xFF);
					/*@fallthrough@*/
					/* no break */
				case 2:
					*rx++ = (char)((data >> 8) & 0xFF);
					/*@fallthrough@*/
					/* no break */
				case 1:
					*rx++ = (char)((data >> 0) & 0xFF);
				}
			}

			rx_len -= count;
		}
	}
}