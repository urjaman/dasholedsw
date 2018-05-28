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

#if 0

#define bld(to,bit_to) bld2(to,bit_to)
#define bst(from,bit_from) bst2(from,bit_from)
#define bld2(to,bit_to) asm("bld %0, " #bit_to "\n\t" : "+r" (to) :)
#define bst2(from,bit_from) asm("bst %0, " #bit_from "\n\t" :: "r" (from))
#define nop asm volatile("nop")

#define SPI_PORT VPORT1_OUT
#define SCLK 1
#define SID 3

// Provides F = F_CPU/6, 50/50 duty cycle
// Note: if you have an ISR that touches this port, cli() this sequence.
static void spiwrite(uint8_t c)
{
	uint8_t p = SPI_PORT & ~_BV(SCLK);
	bst(c,7);
	bld(p,SID);
	SPI_PORT = p;
	nop;
	bst(c,6);
	SPI_PORT |= _BV(SCLK);
	nop;
	bld(p,SID);
	SPI_PORT = p;
	nop;
	bst(c,5);
	SPI_PORT |= _BV(SCLK);
	nop;
	bld(p,SID);
	SPI_PORT = p;
	nop;
	bst(c,4);
	SPI_PORT |= _BV(SCLK);
	nop;
	bld(p,SID);
	SPI_PORT = p;
	nop;
	bst(c,3);
	SPI_PORT |= _BV(SCLK);
	nop;
	bld(p,SID);
	SPI_PORT = p;
	nop;
	bst(c,2);
	SPI_PORT |= _BV(SCLK);
	nop;
	bld(p,SID);
	SPI_PORT = p;
	nop;
	bst(c,1);
	SPI_PORT |= _BV(SCLK);
	nop;
	bld(p,SID);
	SPI_PORT = p;
	nop;
	bst(c,0);
	SPI_PORT |= _BV(SCLK);
	nop;
	bld(p,SID);
	SPI_PORT = p;
	nop;
	nop;
	SPI_PORT |= _BV(SCLK);
}

static void command(uint8_t c)
{
	// command
	VPORT0_OUT &= ~_BV(5); // D/C
	VPORT0_OUT &= ~_BV(6); // ~CS
	spiwrite(c);
	VPORT0_OUT |= _BV(6); // CS
}

static void pixel(uint16_t color)
{
	// data
	VPORT0_OUT |=  _BV(5); // D/C
	VPORT0_OUT &= ~_BV(6); // ~CS
	spiwrite(color >> 8);
	spiwrite(color);
	VPORT0_OUT |= _BV(6); // CS
}

static void spi_init(void) {
}

#else

ISR(USARTC0_TXC_vect) {
	VPORT0_OUT |= _BV(6); // CS
}

static void spi_init(void) {
	USARTC0_CTRLC = USART_CMODE_MSPI_gc | _BV(1); /* UCPHA=1 */
	USARTC0_BAUDCTRLA = 2;
	PORTC.PIN1CTRL |= _BV(6); // INVEN
	VPORT1_OUT &= ~_BV(1);
	USARTC0_CTRLA = USART_TXCINTLVL_LO_gc;
	USARTC0_CTRLB = USART_TXEN_bm;
}

static void spiwrite(uint8_t c) {
	while (!(USARTC0_STATUS & USART_DREIF_bm));
	cli();
	VPORT0_OUT &= ~_BV(6); // ~CS
	USARTC0_DATA = c;
	sei();
	USARTC0_STATUS = USART_TXCIF_bm;
}

static void command(uint8_t c)
{
	// command
	if (VPORT0_IN & _BV(5)) {
		while (!(VPORT0_IN & _BV(6))); /* Wait for CS up by ISR. */
		VPORT0_OUT &= ~_BV(5); // D/C
	}
	spiwrite(c);
}

static void pixel(uint16_t color)
{
	// data
	if (!(VPORT0_IN & _BV(5))) {
		while (!(VPORT0_IN & _BV(6))); /* Wait for CS up by ISR. */
		VPORT0_OUT |=  _BV(5); // D/C
	}
	spiwrite(color >> 8);
	spiwrite(color);
}
#endif


static void set_drawbox(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	command(SSD1331_CMD_SETCOLUMN);
	command(x);
	command(x+w-1);

	command(SSD1331_CMD_SETROW);
	command(y);
	command(y+h-1);
}

#if 0
uint16_t dp_get_color(uint8_t r, uint8_t g, uint8_t b)
{
	uint16_t c;
	c = r >> 3;
	c <<= 6;
	c |= g >> 2;
	c <<= 5;
	c |= b >> 3;
	return c;
}
#endif

static uint8_t dp_last_bl = 0x07;

void dp_init(void)
{
	// set pin directions
	VPORT0_OUT |= _BV(6) | _BV(5);
	VPORT0_DIR |= _BV(6) | _BV(5);
	VPORT1_OUT |= _BV(3) | _BV(2) | _BV(1);
	VPORT1_DIR |= _BV(3) | _BV(2) | _BV(1);

	spi_init();

	// Toggle RST low to reset; CS low so it'll listen to us
	VPORT0_OUT &= ~_BV(6);

	_delay_us(500);
	VPORT1_OUT &= ~_BV(2);
	_delay_us(500);
	VPORT1_OUT |= _BV(2);
	_delay_us(500);

	// Initialization Sequence
	command(SSD1331_CMD_DISPLAYOFF);  	// 0xAE
	command(SSD1331_CMD_SETREMAP); 	// 0xA0
#if defined SSD1331_COLORORDER_RGB
	command(0x72);				// RGB Color
#else
	command(0x76);				// BGR Color
#endif
	command(SSD1331_CMD_STARTLINE); 	// 0xA1
	command(0x0);
	command(SSD1331_CMD_DISPLAYOFFSET); 	// 0xA2
	command(0x0);
	command(SSD1331_CMD_NORMALDISPLAY);  	// 0xA4
	command(SSD1331_CMD_SETMULTIPLEX); 	// 0xA8
	command(0x3F);  			// 0x3F 1/64 duty
	command(SSD1331_CMD_SETMASTER);  	// 0xAD
	command(0x8E);
	command(SSD1331_CMD_POWERMODE);  	// 0xB0
	command(0x0B);
	command(SSD1331_CMD_PRECHARGE);  	// 0xB1
	command(0x31);
	command(SSD1331_CMD_CLOCKDIV);  	// 0xB3
	command(0xF0);  // 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
	command(SSD1331_CMD_PRECHARGEA);  	// 0x8A
	command(0x64);
	command(SSD1331_CMD_PRECHARGEB);  	// 0x8B
	command(0x78);
	command(SSD1331_CMD_PRECHARGEA);  	// 0x8C
	command(0x64);
	command(SSD1331_CMD_PRECHARGELEVEL);  	// 0xBB
	command(0x3A);
	command(SSD1331_CMD_VCOMH);  		// 0xBE
	command(0x3E);
	command(SSD1331_CMD_MASTERCURRENT);  	// 0x87
	command(0x06);
	command(SSD1331_CMD_CONTRASTA);  	// 0x81
	command(0x91);
	command(SSD1331_CMD_CONTRASTB);  	// 0x82
	command(0x50);
	command(SSD1331_CMD_CONTRASTC);  	// 0x83
	command(0x7D);
	command(SSD1331_CMD_DISPLAYON);	//--turn on oled panel
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

