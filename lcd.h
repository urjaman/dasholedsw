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
#define LCD_MAXX 10
#define LCD_MAXY 6
/*
 * Initialize LCD controller.  Performs a software reset.
 */
void	lcd_init(void);

/*
 * Send one character to the LCD.
 */
void	lcd_putchar(unsigned char c);

void lcd_puts(const unsigned char* str);
void lcd_puts_P(PGM_P str);
void lcd_clear(void);
void lcd_gotoxy(uint8_t x, uint8_t y);

void lcd_puts_dw(const unsigned char * str);
void lcd_puts_dw_P(PGM_P str);

void lcd_puts_big(const unsigned char * str);
void lcd_puts_big_P(PGM_P str);
