#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>

#include "lcd.h"

static uint8_t lcdx, lcdy;

void
lcd_init(void)
{
	dp_init();
}

void lcd_gotoxy(uint8_t x, uint8_t y)
{
	if (y>=LCD_MAXY) y=0;
	if (x>=LCD_MAXX) x=0;
	lcdy = y;
	lcdx = (x*8);
}

void lcd_gotoxy_dw(uint8_t x, uint8_t y)
{
	if (y>=LCD_MAXY) y=0;
	if (x>=LCD_DW_MAXX) x=0;
	lcdy = y;
	lcdx = x;
}


void lcd_clear(void)
{
	dp_clear_block(0, 0, LCDWIDTH, LCDHEIGHT/8);
}

void lcd_clear_dw(uint8_t w)
{
	if ((lcdx+w)>LCDWIDTH) {
		w = LCDWIDTH - lcdx;
	}
	dp_clear_block(lcdx, lcdy, w, 1);
	lcdx += w;
}

void lcd_clear_eol(void)
{
	lcd_clear_dw(LCDWIDTH - lcdx);
	lcdx = 0;
	lcdy++;
}

void lcd_clear_big_eol(void)
{
	dp_clear_block(lcdx, lcdy, LCDWIDTH - lcdx, 2);
	lcdx = 0;
	lcdy += 2;
}

void lcd_clear_eos(void)
{
	while (lcdy < LCD_MAXY) lcd_clear_eol();
	lcdy = 0;
}

void lcd_write_dwb(uint8_t *buf, uint8_t w)
{
	if ((lcdx+w)>LCDWIDTH) {
		w = LCDWIDTH - lcdx;
	}
	dp_write_block(buf, lcdx, lcdy, w, 1);
	lcdx += w;
}

#include "mfont8x8.c"

// Some data for dynamic width mode with this font.
#include "font-dyn-meta.c"

static void lcd_putchar_(unsigned char c, uint8_t dw)
{

	PGM_P block;
	if (c < 0x20) c = 0x20;
	block = (const char*)&(my_font[c-0x20][0]);
	uint8_t w = 8;
	if (dw) {
		uint8_t font_meta_b = pgm_read_byte(&(font_metadata[c-0x20]));
		block += XOFF(font_meta_b);
		w = DW(font_meta_b);
	}
	if ((lcdx+w)>LCDWIDTH) {
		w = LCDWIDTH - lcdx;
	}
	dp_write_block_P(block,lcdx, lcdy, w,1);
	lcdx += w;
}

static void lcd_putchar_big(unsigned char c)
{
	uint8_t buf[32];

	PGM_P block;
	if (c < 0x20) c = 0x20;
	block = (const char*)&(my_font[c-0x20][0]);
	uint8_t font_meta_b = pgm_read_byte(&(font_metadata[c-0x20]));
	uint8_t w = DW(font_meta_b);
	block += XOFF(font_meta_b);
	if ((lcdx+(w*2))>LCDWIDTH) {
		w = (LCDWIDTH - lcdx)/2;
	}
	for (int i=0; i<w; i++) {
		uint8_t d = pgm_read_byte(block);
		block++;
		uint8_t hi = 0;
		uint8_t lo = 0;
		for (int i=0; i<4; i++) {
			hi = hi >> 2;
			lo = lo >> 2;
			if (d & _BV(4+i)) hi |= 0xC0;
			if (d & _BV(i)) lo |= 0xC0;
		}
		buf[i*2] = hi;
		buf[i*2+1] = hi;
		buf[(w+i)*2] = lo;
		buf[(w+i)*2+1] = lo;
	}
	dp_write_block(buf,lcdx, lcdy, w*2,2);
	lcdx += w*2;
}

static uint8_t lcd_dw_charw(uint8_t c)
{
	if (c < 0x20) c = 0x20;
	uint8_t font_meta_b = pgm_read_byte(&(font_metadata[c-0x20]));
	return DW(font_meta_b);
}


uint8_t lcd_strwidth(const unsigned char * str)
{
	uint8_t w = 0;
	do {
		if (*str) w += lcd_dw_charw(*str);
		else return w;
		str++;
	} while(1);
}

uint8_t lcd_strwidth_P(PGM_P str)
{
	uint8_t w = 0;
	do {
		uint8_t c = pgm_read_byte(str);
		if (c) w += lcd_dw_charw(c);
		else return w;
		str++;
	} while(1);
}

/* big = 2x small, but dont tell the user ;) */
uint8_t lcd_strwidth_big(const unsigned char * str)
{
	return lcd_strwidth(str)*2;
}

uint8_t lcd_strwidth_big_P(PGM_P str)
{
	return lcd_strwidth_P(str)*2;
}

void lcd_putchar(unsigned char c)
{
	lcd_putchar_(c,0);
}

void lcd_puts(const unsigned char * str)
{
	do {
		if (*str) lcd_putchar_(*str, 0);
		else return;
		str++;
	} while(1);
}

void lcd_puts_P(PGM_P str)
{
	do {
		uint8_t c = pgm_read_byte(str);
		if (c) lcd_putchar_(c, 0);
		else return;
		str++;
	} while (1);
}

void lcd_puts_dw(const unsigned char * str)
{
start:
	if (*str) lcd_putchar_(*str, 1);
	else return;
	str++;
	goto start;
}

void lcd_puts_dw_P(PGM_P str)
{
	unsigned char c;
start:
	c = pgm_read_byte(str);
	if (c) lcd_putchar_(c, 1);
	else return;
	str++;
	goto start;
}


void lcd_puts_big_P(PGM_P str)
{
	unsigned char c;
start:
	c = pgm_read_byte(str);
	if (c) lcd_putchar_big(c);
	else return;
	str++;
	goto start;
}

void lcd_puts_big(const unsigned char * str)
{
start:
	if (*str) lcd_putchar_big(*str);
	else return;
	str++;
	goto start;
}

/* Fill the screen with one character. */
void lcd_huge_char(uint8_t c)
{
	PGM_P block;
	if (c < 0x20) c = 0x20;
	block = (const char*)&(my_font[c-0x20][0]);
	uint8_t buf[64];
	lcd_gotoxy_dw(0,0);
	uint8_t extra = (LCDWIDTH - 64) / 2;
	for (uint8_t b=0x80; b; b = b >> 1) {
		lcd_clear_dw(extra);
		for (uint8_t n=0; n<8; n++) {
			if (pgm_read_byte(block+n) & b) {
				memset(buf+n*8,0xFF,8);
			} else {
				memset(buf+n*8,0,8);
			}
		}
		lcd_write_dwb(buf, 64);
		lcd_clear_eol();
	}
	lcd_clear_eos();
}
