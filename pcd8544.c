
#include "main.h"
#include "pcd8544.h"
#include <string.h>

#define PCD8544_POWERDOWN 0x04
#define PCD8544_ENTRYMODE 0x02
#define PCD8544_EXTENDEDINSTRUCTION 0x01

#define PCD8544_DISPLAYBLANK 0x0
#define PCD8544_DISPLAYNORMAL 0x4
#define PCD8544_DISPLAYALLON 0x1
#define PCD8544_DISPLAYINVERTED 0x5

// H = 0
#define PCD8544_FUNCTIONSET 0x20
#define PCD8544_DISPLAYCONTROL 0x08
#define PCD8544_SETYADDR 0x40
#define PCD8544_SETXADDR 0x80

// H = 1
#define PCD8544_SETTEMP 0x04
#define PCD8544_SETBIAS 0x10
#define PCD8544_SETVOP 0x80

/* Master, Enable, CLK2X, DIV16 => CLKper/8 */
#define SPI_CTRL_DEFAULT 0xD1

static void command(uint8_t c);

void pcd8544_init(void)
{
	const uint8_t contrast = 40;
	const uint8_t bias = 4;
	VPORT1_OUT |= _BV(5);
	// toggle RST low to reset
	VPORT1_OUT &= ~_BV(3);
	_delay_us(500);
	VPORT1_OUT |= _BV(3);

	SPIC_INTCTRL = 0;
	SPIC_CTRL = SPI_CTRL_DEFAULT;
	SPIC_CTRLB = 0x04; /* SSD */

	// get into the EXTENDED mode!
	command(PCD8544_FUNCTIONSET | PCD8544_EXTENDEDINSTRUCTION );

	// LCD bias select (4 is optimal?)
	command(PCD8544_SETBIAS | bias);

	command( PCD8544_SETVOP | contrast); // Experimentally determined


	// normal mode
	command(PCD8544_FUNCTIONSET);

	// Set display to Normal
	command(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYNORMAL);

	pcd8544_clear_block(0, 0, LCDWIDTH, LCDHEIGHT/8);
}

static void spi_command_start(void)
{
	VPORT1_OUT &= ~_BV(6); // D/C
	VPORT1_OUT &= ~_BV(4); // CS
}

static void spi_write_put(uint8_t d)
{
	uint8_t dummy = SPIC_STATUS;
	SPIC_DATA = d;
	if (SPIC_STATUS & SPI_WRCOL_bm) {
		while (!(SPIC_STATUS & SPI_IF_bm));
		SPIC_DATA = d;
	}
	(void)dummy;
}

static void spi_write_wait(void)
{
	while (!(SPIC_STATUS & SPI_IF_bm));

}

static void spi_command_end(void)
{
	spi_write_wait();
	VPORT1_OUT |= _BV(4); // CS
}

static void command(uint8_t c)
{
	spi_command_start();
	spi_write_put(c);
	spi_command_end();
}

static void spi_data_start(void)
{
	/* Flip Data Order (for our font) */
	SPIC_CTRL = SPI_CTRL_DEFAULT | 0x20;
	VPORT1_OUT |= _BV(6); // data
	VPORT1_OUT &= ~_BV(4); // CS
}

static void spi_data_end(void)
{
	spi_write_wait();
	VPORT1_OUT |= _BV(4); // CS
	SPIC_CTRL = SPI_CTRL_DEFAULT;
}

static void spi_data_finish(void)
{
	command(PCD8544_SETYADDR);
}



void pcd8544_write_block_P(const PGM_P buffer, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	uint8_t ye = y+h;
	uint8_t we = x+w;
	for (uint8_t yi=y; yi<ye; yi++) {
		command(PCD8544_SETYADDR | yi);
		command(PCD8544_SETXADDR | x);

		spi_data_start();
		for(uint8_t xi=x; xi < we; xi++) {
			spi_write_put(pgm_read_byte(buffer));
			buffer++;
		}
		spi_data_end();

	}
	spi_data_finish();
}

void pcd8544_write_block(const uint8_t *buffer, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	uint8_t ye = y+h;
	uint8_t we = x+w;
	for (uint8_t yi=y; yi<ye; yi++) {
		command(PCD8544_SETYADDR | yi);
		command(PCD8544_SETXADDR | x);

		spi_data_start();
		for(uint8_t xi=x; xi < we; xi++) {
			spi_write_put(*buffer);
			buffer++;
		}
		spi_data_end();
	}
	spi_data_finish();
}

void pcd8544_clear_block(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	uint8_t buf[w];
	memset(buf, 0, w);
	for (uint8_t yi=y;yi<(y+h);yi++) {
		pcd8544_write_block(buf, x, yi, w, 1);
	}
}
