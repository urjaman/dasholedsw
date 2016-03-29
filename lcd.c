#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>

#include "lcd.h"
#include "pcd8544.h"

static uint8_t lcdx, lcdy;

void
lcd_init(void)
{
	pcd8544_init();
}

void lcd_gotoxy(uint8_t x, uint8_t y) {
	if (y>=LCD_MAXY) y=0;
	if (x>=LCD_MAXX) x=0;
	lcdy = y;
	lcdx = x*8;
}


void lcd_clear(void) {
	pcd8544_clear();
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
        pcd8544_write_block_P(block,lcdx, lcdy, w,1);
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
        for (int i=0;i<w;i++) {
        	uint8_t d = pgm_read_byte(block);
        	block++;
        	uint8_t hi = 0;
        	uint8_t lo = 0;
        	for (int i=0;i<4;i++) {
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
        pcd8544_write_block(buf,lcdx, lcdy, w*2,2);
        lcdx += w*2;
        
}


void lcd_putchar(unsigned char c) {
  lcd_putchar_(c,0);
}



void lcd_puts(const unsigned char * str) {
start:
	if (*str) lcd_putchar_(*str, 0);
	else return;
	str++;
	goto start;
}

void lcd_puts_P(PGM_P str) {
	unsigned char c;
start:
	c = pgm_read_byte(str);
	if (c) lcd_putchar_(c, 0);
	else return;
	str++;
	goto start;
}

void lcd_puts_dw(const unsigned char * str) {
start:
	if (*str) lcd_putchar_(*str, 1);
	else return;
	str++;
	goto start;
}

void lcd_puts_dw_P(PGM_P str) {
	unsigned char c;
start:
	c = pgm_read_byte(str);
	if (c) lcd_putchar_(c, 1);
	else return;
	str++;
	goto start;
}


void lcd_puts_big_P(PGM_P str) {
	unsigned char c;
start:
	c = pgm_read_byte(str);
	if (c) lcd_putchar_big(c);
	else return;
	str++;
	goto start;
}



void lcd_puts_big(const unsigned char * str) {
start:
	if (*str) lcd_putchar_big(*str);
	else return;
	str++;
	goto start;
}
