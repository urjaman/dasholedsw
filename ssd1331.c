/***************************************************
  This is a library for the 0.96" 16-bit Color OLED with SSD1331 driver chip

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/684

  These displays use SPI to communicate, 4 or 5 pins are required to
  interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/
/* This has been modified a lot by Urja Rannikko <urjaman@gmail.com> */

#include "main.h"
#include "ssd1331.h"

static void spiwrite(uint8_t c) {
	uint8_t s = SREG;
	while (!(USARTC0_STATUS & USART_DREIF_bm));
	cli();
	VPORT0_OUT &= ~_BV(6); // ~CS
	USARTC0_DATA = c;
	USARTC0_STATUS = USART_TXCIF_bm;
	SREG = s;
}

static void end_prev(void)
{
	if (!(VPORT0_OUT & _BV(6))) {
		while (!(USARTC0_STATUS & USART_TXCIF_bm));
		VPORT0_OUT |= _BV(6); // CS
	}
}

static void command(uint8_t c)
{
	if (VPORT0_OUT & _BV(5)) end_prev();
	// command
	VPORT0_OUT &= ~_BV(5); // D/C
	spiwrite(c);
}

static void pixel(uint16_t color)
{
	if (!(VPORT0_OUT & _BV(5))) end_prev();
	// data
	VPORT0_OUT |=  _BV(5); // D/C
	spiwrite(color >> 8);
	spiwrite(color);
}


static void set_drawbox(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	command(SSD1331_CMD_SETCOLUMN);
	command(x);
	command(x+w-1);

	command(SSD1331_CMD_SETROW);
	command(y);
	command(y+h-1);
}

static uint8_t dp_last_bl = 0x07;

static const uint8_t initseq[] PROGMEM = {
	SSD1331_CMD_DISPLAYOFF,  	// 0xAE
	SSD1331_CMD_SETREMAP, 	// 0xA0
	0x72,				// RGB Color (0x76 for BGR that we dont use)
	SSD1331_CMD_STARTLINE, 	// 0xA1
	0x0,
	SSD1331_CMD_DISPLAYOFFSET, 	// 0xA2
	0x0,
	SSD1331_CMD_NORMALDISPLAY,  	// 0xA4
	SSD1331_CMD_SETMULTIPLEX, 	// 0xA8
	0x3F,  			// 0x3F 1/64 duty
	SSD1331_CMD_SETMASTER,  	// 0xAD
	0x8E,
	SSD1331_CMD_POWERMODE,  	// 0xB0
	0x0B,
	SSD1331_CMD_PRECHARGE,  	// 0xB1
	0x31,
	SSD1331_CMD_CLOCKDIV,  	// 0xB3
	0xF0,  // 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
	SSD1331_CMD_PRECHARGEA,  	// 0x8A
	0x64,
	SSD1331_CMD_PRECHARGEB,  	// 0x8B
	0x78,
	SSD1331_CMD_PRECHARGEA,  	// 0x8C
	0x64,
	SSD1331_CMD_PRECHARGELEVEL,  	// 0xBB
	0x3A,
	SSD1331_CMD_VCOMH,  		// 0xBE
	0x3E,
	SSD1331_CMD_MASTERCURRENT,  	// 0x87
	0x06,
	SSD1331_CMD_CONTRASTA,  	// 0x81
	0x91,
	SSD1331_CMD_CONTRASTB,  	// 0x82
	0x50,
	SSD1331_CMD_CONTRASTC,  	// 0x83
	0x7D,
	SSD1331_CMD_DISPLAYON	//--turn on oled panel
};

void dp_init(void)
{
	// set pin directions, etc.
	PORTC.PIN1CTRL |= _BV(6); // Invert XCK
	VPORT0_OUT |= _BV(6) | _BV(5);
	VPORT0_DIR |= _BV(6) | _BV(5);
	VPORT1_OUT |= _BV(3) | _BV(2);
	VPORT1_OUT &= ~_BV(1);
	VPORT1_DIR |= _BV(3) | _BV(2) | _BV(1);

	// Init USART MSPIM
	USARTC0_CTRLC = USART_CMODE_MSPI_gc | _BV(1); /* UCPHA=1 */
	USARTC0_BAUDCTRLA = 2; /* F_CPU / (2*( _2_ +1)) = F_CPU/6 = 5.33 Mhz */
	USARTC0_CTRLB = USART_TXEN_bm;

	// Toggle RST low to reset; CS low so it'll listen to us
	VPORT0_OUT &= ~_BV(6);

	_delay_us(500);
	VPORT1_OUT &= ~_BV(2);
	_delay_us(500);
	VPORT1_OUT |= _BV(2);
	_delay_us(500);

	VPORT0_OUT |= _BV(6); // CS

	for (uint8_t n=0;n<sizeof(initseq);n++) {
		command(pgm_read_byte(initseq+n));
	}
}

static uint16_t dp_fg=dp_get_color(0,255,255), dp_bg=dp_get_color(0,0,0);

void dp_set_fg_bg(uint16_t fg, uint16_t bg)
{
	dp_fg = fg;
	dp_bg = bg;
}

void dp_set_bl(uint8_t bl)
{
	if (!dp_last_bl) {
		command(SSD1331_CMD_DISPLAYON);
	}
	if (bl>0x10) bl=0x10;
	if (bl) {
		command(SSD1331_CMD_MASTERCURRENT);
		command(bl-1);
	} else {
		command(SSD1331_CMD_DISPLAYOFF);
	}
	dp_last_bl = bl;
}

void dp_write_block(const uint8_t *buffer, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	set_drawbox(x,y*8,w,h*8);
	for (uint8_t z=0; z<h; z++) {
		for (uint8_t n=0x80; n; n = n >> 1) {
			for(uint8_t v=0; v < w; v++) {
				if (buffer[v] & n) pixel(dp_fg);
				else pixel(dp_bg);
			}
		}
		buffer += w;
	}
}

void dp_write_block_P(const PGM_P buffer, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	set_drawbox(x,y*8,w,h*8);
	for (uint8_t z=0; z<h; z++) {
		for (uint8_t n=0x80; n; n = n >> 1) {
			for(uint8_t v=0; v < w; v++) {
				if (pgm_read_byte(buffer+v) & n) pixel(dp_fg);
				else pixel(dp_bg);
			}
		}
		buffer += w;
	}
}

static void dp_clear_area(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	set_drawbox(x,y,w,h);
	const uint16_t pixels = w*h;
	for (uint16_t i=0; i<pixels; i++) pixel(dp_bg);
}

void dp_clear_block(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	dp_clear_area(x,y*8,w,h*8);
}

/* TinyGif support */

#include "tinygif/tdgif_lib.h"

static TGifInfo* TGInfo;
static void dp_tgif_cb(uint8_t idx) {
	pixel(pgm_read_word(&(TGInfo->Colors[idx])));
}

uint8_t dp_draw_tgif(const void *tgif, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	/* Center a TGIF on a given xywh box, clear the overflow with bg color. */
	TGifInfo Info;
	if (TDGifGetInfo(tgif, &Info, w, h, 0xFFFF) == TGIF_ERROR) {
		return Info.Error;
	}
	/* Overflows */
	uint8_t t,b,l,r; /* top, bottom, left, right */
	t = (h - Info.Height) / 2;
	b = (h - Info.Height) - t;
	l = (w - Info.Width) / 2;
	r = (w - Info.Width) - l;
	if (t) dp_clear_area(x, y, w, t);
	if (b) dp_clear_area(x, y+t+Info.Height, w, b);
	if (l) dp_clear_area(x, y+t, l, Info.Height);
	if (r) dp_clear_area(x+l+Info.Width, y+t, r, Info.Height);

	set_drawbox(x+l, y+t, Info.Width, Info.Height);

	TGInfo = &Info;
	TDGifDecompress(TGInfo, dp_tgif_cb);
	TGInfo = NULL;
	return Info.Error;
}

