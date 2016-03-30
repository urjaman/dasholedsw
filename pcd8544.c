
#include "main.h"
#include "pcd8544.h"

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

	// get into the EXTENDED mode!
	command(PCD8544_FUNCTIONSET | PCD8544_EXTENDEDINSTRUCTION );

	// LCD bias select (4 is optimal?)
	command(PCD8544_SETBIAS | bias);

	command( PCD8544_SETVOP | contrast); // Experimentally determined


	// normal mode
	command(PCD8544_FUNCTIONSET);

	// Set display to Normal
	command(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYNORMAL);
	pcd8544_clear();
}


static void pcdspiwrite(uint8_t d)
{
	// Software SPI write with bit banging.
	for(uint8_t bit = 0x80; bit; bit >>= 1) {
		VPORT1_OUT &= ~_BV(5);
		if(d & bit) VPORT1_OUT |= _BV(7);
		else        VPORT1_OUT &= ~_BV(7);;
		_delay_us(1);
		VPORT1_OUT |= _BV(5);
		_delay_us(1);
	}
}

static void command(uint8_t c)
{
	VPORT1_OUT &= ~_BV(6); // D/C
	VPORT1_OUT &= ~_BV(4); // CS
	pcdspiwrite(c);
	VPORT1_OUT |= _BV(4); // CS
}

void pcd8544_clear(void)
{
	for (uint8_t yi=0; yi<6; yi++) {
		command(PCD8544_SETYADDR | yi);
		command(PCD8544_SETXADDR | 0);
		VPORT1_OUT |= _BV(6); // data
		VPORT1_OUT &= ~_BV(4); // CS
		for(uint8_t xi=0; xi < 84; xi++) {
			pcdspiwrite(0);
		}
		VPORT1_OUT |= _BV(4); // CS
	}
	command(PCD8544_SETYADDR );  // no idea why this is necessary but it is to finish the last byte?
}

/* Inverse bit order for our font to work. */
static void pcdspiwrite_rev(uint8_t d)
{
	// Software SPI write with bit banging.
	for(uint8_t bit = 1; bit; bit <<= 1) {
		VPORT1_OUT &= ~_BV(5);
		if(d & bit) VPORT1_OUT |= _BV(7);
		else        VPORT1_OUT &= ~_BV(7);;
		_delay_us(1);
		VPORT1_OUT |= _BV(5);
		_delay_us(1);
	}
}


void pcd8544_write_block_P(const PGM_P buffer, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	uint8_t ye = y+h;
	uint8_t we = x+w;
	for (uint8_t yi=y; yi<ye; yi++) {
		command(PCD8544_SETYADDR | yi);
		command(PCD8544_SETXADDR | x);

		VPORT1_OUT |= _BV(6); // data
		VPORT1_OUT &= ~_BV(4); // CS
		for(uint8_t xi=x; xi < we; xi++) {
			pcdspiwrite_rev(pgm_read_byte(buffer));
			buffer++;
		}
		VPORT1_OUT |= _BV(4); // CS
	}
	command(PCD8544_SETYADDR );  // no idea why this is necessary but it is to finish the last byte?
}

void pcd8544_write_block(const uint8_t *buffer, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	uint8_t ye = y+h;
	uint8_t we = x+w;
	for (uint8_t yi=y; yi<ye; yi++) {
		command(PCD8544_SETYADDR | yi);
		command(PCD8544_SETXADDR | x);

		VPORT1_OUT |= _BV(6); // data
		VPORT1_OUT &= ~_BV(4); // CS
		for(uint8_t xi=x; xi < we; xi++) {
			pcdspiwrite_rev(*buffer);
			buffer++;
		}
		VPORT1_OUT |= _BV(4); // CS
	}
	command(PCD8544_SETYADDR );  // no idea why this is necessary but it is to finish the last byte?
}


