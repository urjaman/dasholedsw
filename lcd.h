/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joerg@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.        Joerg Wunsch
 * ----------------------------------------------------------------------------
 *
 * Stdio demo, upper layer of LCD driver.
 *
 * $Id: lcd.h,v 1.1.2.1 2005/12/28 22:35:08 joerg_wunsch Exp $
 */
#include <avr/pgmspace.h>

#include "pcd8544.h"

#define LCD_DW_MAXX LCDWIDTH

#define LCD_MAXX 10
#define LCD_MAXY 6
void	lcd_init(void);
void	lcd_putchar(unsigned char c);

void lcd_clear(void);
void lcd_gotoxy(uint8_t x, uint8_t y);
void lcd_gotoxy_dw(uint8_t x, uint8_t y);

/* put a fixed-width font string */
void lcd_puts(const unsigned char* str);
void lcd_puts_P(PGM_P str);

/* put a dynamic-width font string */
void lcd_puts_dw(const unsigned char * str);
void lcd_puts_dw_P(PGM_P str);

uint8_t lcd_strwidth(const unsigned char*str);
uint8_t lcd_strwidth_P(PGM_P str);

/* big is always dw, for now */
void lcd_puts_big(const unsigned char * str);
void lcd_puts_big_P(PGM_P str);

uint8_t lcd_strwidth_big(const unsigned char*str);
uint8_t lcd_strwidth_big_P(PGM_P str);
