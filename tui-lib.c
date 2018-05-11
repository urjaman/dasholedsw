#include "main.h"
#include "lcd.h"
#include "buttons.h"
#include "lib.h"
#include "tui-lib.h"
#include "timer.h"
#include "adc.h"


uint8_t tui_pollkey(void)
{
	uint8_t v = buttons_get();
	mini_mainloop();
	return v;
}

uint8_t tui_waitforkey(void)
{
	uint8_t v;
	for(;;) {
		v = tui_pollkey();
		if (v) return v;
	}
}

void tui_gen_menuheader(PGM_P header)
{
	uint8_t tw = lcd_strwidth_P(header);

	uint8_t banw1 = ((LCDWIDTH - tw) / 2) - 1;
	uint8_t banw2 = LCDWIDTH - tw - banw1 - 2;
	if (tw>=(LCDWIDTH-4)) {
		banw1 = 0;
		banw2 = 0;
		if (tw>=LCDWIDTH) tw = LCDWIDTH;
	}
	uint8_t clrw1 = (LCDWIDTH - tw - banw1 - banw2) / 2;
	uint8_t clrw2 = (LCDWIDTH - tw - banw1 - banw2 - clrw1);
	uint8_t banner[LCDWIDTH/2];

	for (uint8_t i=0; i<LCDWIDTH/2; i++) banner[i] = i&1?0x89:0x91;
	tui_set_color(TTC_HEAD2);
	lcd_gotoxy_dw(0,0);
	lcd_write_dwb(banner, banw1);
	tui_set_color(TTC_HEAD1);
	lcd_clear_dw(clrw1);
	lcd_puts_dw_P(header);
	lcd_clear_dw(clrw2);
	tui_set_color(TTC_HEAD2);
	lcd_write_dwb(banner, banw2);
	tui_default_color();
}

int32_t tui_gen_adjmenu(PGM_P header, printval_func_t *printer, int32_t min, int32_t max, int32_t start,
                        int32_t step)
{
	tui_gen_menuheader(header);
	uint8_t lbm = buttons_hw_count()==1?2:0;
	int32_t idx=start;
	/* No LBM for 16x2s... if i end up using them. */
	if (LCD_MAXY<=2) lbm = 0;
	for (;;) {
		uint8_t buf[LCDWIDTH/2];
		uint8_t sl = printer(buf, idx);
		buf[sl] = 0;
		sl = lcd_strwidth(buf);
		lcd_gotoxy_dw(0,1);
		lcd_clear_dw((LCDWIDTH - sl)/2);
		lcd_puts_dw(buf);
		lcd_clear_eol();
		if (lbm) {
			tui_set_color(TTC_LN2);
			lcd_gotoxy_dw(0,2);
			lcd_puts_dw_P(lbm==2?PSTR("DIR: NEXT"):PSTR("DIR: PREV"));
			lcd_clear_eol();
			tui_unset_color();
		}
		lcd_clear_eos();
		if (lbm) {
			timer_delay_ms(180);
			timer_delay_ms(100);
		}
		uint8_t key = tui_waitforkey();
		if ((lbm==1)&&(key&BUTTON_NEXT)) key = BUTTON_PREV;
		switch (key) {
			case BUTTON_NEXT:
				if ((idx+step) > max) {
					idx = min;
				} else {
					idx += step;
				}
				break;
			case BUTTON_PREV:
				if ((idx-step) < min) {
					idx = max;
				} else {
					idx -= step;
				}
				break;
			case BUTTON_OK:
				if (lbm==2) {
					lbm = 1;
					break;
				}
				return idx;
			case BUTTON_CANCEL:
				return start;
		}
	}
}

int tui_enh_listmenu(PGM_P header, listprint_func_t *printer, uint8_t entries, uint8_t start)
{
	tui_gen_menuheader(header);
	uint8_t idx=start;
	uint8_t scroll=0;
	PGM_P idstr = PSTR("> ");
	uint8_t idw = lcd_strwidth_P(idstr);
	const uint8_t brackl = entries < (LCD_MAXY-1) ? entries : LCD_MAXY - 1;
	for (;;) {
		int8_t vbp = idx-scroll;
		if (vbp < 0) {
			scroll += vbp;
		} else if (vbp>=brackl) {
			scroll += (vbp-brackl)+1;
		}
		vbp = idx-scroll;
		if ((!vbp)&&(idx)) {
			scroll--;
		} else if ((vbp == brackl-1)&&(idx<(entries-1))) {
			scroll++;
		}
		vbp = idx-scroll;
		uint8_t vi = idx - vbp;
		for (uint8_t bp=0; bp<brackl; bp++) {
			lcd_gotoxy_dw(0, bp+1);
			if (bp==vbp) {
				tui_set_color(vi&1 ? TTC_HL2 : TTC_HL1);
				lcd_puts_dw_P(idstr);
			} else {
				tui_set_color(vi&1 ? TTC_LN2 : TTC_LN1);
				lcd_clear_dw(idw);
			}
			printer(vi);
			vi++;
		}
		tui_default_color();
		lcd_clear_eos();
		timer_delay_ms(50);
		uint8_t key = tui_waitforkey();
		switch (key) {
			case BUTTON_NEXT:
				if ((idx+1) >= entries) {
					idx = 0;
				} else {
					idx++;
				}
				break;
			case BUTTON_PREV:
				if (!idx) {
					idx = entries-1;
				} else {
					idx--;
				}
				break;
			case BUTTON_OK:
				return idx;
			case BUTTON_CANCEL:
				return -1;
		}
	}
}

static PGM_P const * tui_pgm_menu_table;
static void tui_menu_table_printer(uint8_t idx)
{
	lcd_puts_dw_P((PGM_P)pgm_read_word(&(tui_pgm_menu_table[idx])));
	lcd_clear_eol();
}

//Generic Exit Menu Item
const unsigned char tui_exit_menu[] PROGMEM = "EXIT MENU";

uint8_t tui_gen_listmenu(PGM_P header, PGM_P const menu_table[], const uint8_t entries, uint8_t start)
{
	tui_pgm_menu_table = menu_table;
	int r = tui_enh_listmenu(header, tui_menu_table_printer, entries, start);
	if (r<0) { // canceled
		PGM_P last_entry = (PGM_P)pgm_read_word(&menu_table[entries-1]);
		if (last_entry == (PGM_P)tui_exit_menu) return entries-1;
		return start;
	}
	return r;
}

static uint8_t tui_voltmenu_printer(unsigned char* buf, int32_t val)
{
	adc_print_dV(buf,(uint16_t)val);
	return strlen((char*)buf);
}

uint16_t tui_gen_voltmenu(PGM_P header, uint16_t start)
{
	return adc_from_dV(
	               tui_gen_adjmenu(header,tui_voltmenu_printer,0,1600,adc_to_dV(start),1)
	       );
}

static uint8_t tui_nummenu_printer(unsigned char* buf, int32_t val)
{
	return uint2str(buf,(uint16_t)val);
}

uint16_t tui_gen_nummenu(PGM_P header, uint16_t min, uint16_t max, uint16_t start)
{
	return (uint16_t)tui_gen_adjmenu(header,tui_nummenu_printer,min,max,start,1);
}


static void tui_gen_message_start(PGM_P l1)
{
	uint8_t clrw = (LCDWIDTH - lcd_strwidth_P(l1))/2;
	lcd_gotoxy_dw(0,0);
	lcd_clear_dw(clrw);
	lcd_puts_dw_P(l1);
	lcd_clear_eol();
}

static void tui_gen_message_end(void)
{
	lcd_clear_eos();
	timer_delay_ms(100);
	tui_waitforkey();
}

void tui_gen_message(PGM_P l1, PGM_P l2)
{
	tui_gen_message_start(l1);
	lcd_clear_dw((LCDWIDTH - lcd_strwidth_P(l2))/2);
	lcd_puts_dw_P(l2);
	tui_gen_message_end();
}

void tui_gen_message_m(PGM_P l1, const unsigned char* l2m)
{
	tui_gen_message_start(l1);
	lcd_clear_dw((LCDWIDTH - lcd_strwidth(l2m))/2);
	lcd_puts_dw(l2m);
	tui_gen_message_end();
}

const unsigned char tui_q_s1[] PROGMEM = "NO";
const unsigned char tui_q_s2[] PROGMEM = "YES";

PGM_P const tui_q_table[] PROGMEM = {
	(PGM_P)tui_q_s1,
	(PGM_P)tui_q_s2,
};

uint8_t tui_yes_no(PGM_P msg, uint8_t prv)
{
	return tui_gen_listmenu(msg, tui_q_table, 2, prv);
}

uint8_t tui_are_you_sure(void)
{
	return tui_yes_no(PSTR("Are you sure?"), 0);
}


