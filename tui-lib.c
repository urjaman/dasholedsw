#include "main.h"
#include "lcd.h"
#include "buttons.h"
#include "lib.h"
#include "tui-lib.h"
#include "timer.h"
#include "adc.h"


uint8_t tui_pollkey(void) {
	uint8_t v = buttons_get();
	mini_mainloop();
	return v;
}

uint8_t tui_waitforkey(void) {
	uint8_t v;
	for(;;) {
		v = tui_pollkey();
		if (v) return v;
	}
}

void tui_gen_menuheader(PGM_P header) {
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

	for (uint8_t i=0;i<LCDWIDTH/2;i++) banner[i] = i&1?0x89:0x91;
	lcd_clear();
	lcd_gotoxy(0,0);
	lcd_write_dwb(banner, banw1);
	lcd_clear_dw(clrw1);
	lcd_puts_dw_P(header);
	lcd_clear_dw(clrw2);
	lcd_write_dwb(banner, banw2);
}

int32_t tui_gen_adjmenu(PGM_P header, printval_func_t *printer, int32_t min, int32_t max, int32_t start, int32_t step) {
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
			lcd_gotoxy_dw(0,2);
			lcd_puts_dw_P(lbm==2?PSTR("DIR: NEXT"):PSTR("DIR: PREV"));
			lcd_clear_eol();
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

//Generic Exit Menu Item
const unsigned char tui_exit_menu[] PROGMEM = "EXIT MENU";

uint8_t tui_gen_listmenu(PGM_P header, PGM_P const menu_table[], const uint8_t entries, uint8_t start) {
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
		for (uint8_t bp=0;bp<brackl;bp++) {
			lcd_gotoxy_dw(0, bp+1);
			if (bp==vbp) lcd_puts_dw_P(idstr);
			else lcd_clear_dw(idw);
			lcd_puts_dw_P((PGM_P)pgm_read_word(&(menu_table[vi])));
			lcd_clear_eol();
			vi++;
		}
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
			case BUTTON_CANCEL: {
					PGM_P last_entry = (PGM_P)pgm_read_word(&menu_table[entries-1]);
					if (last_entry == (PGM_P)tui_exit_menu) return entries-1;
					else return start;
				}
				break;
		}
	}
}

static uint8_t tui_voltmenu_printer(unsigned char* buf, int32_t val) {
	adc_print_dV(buf,(uint16_t)val);
	return strlen((char*)buf);
}

uint16_t tui_gen_voltmenu(PGM_P header, uint16_t start) {
	return adc_from_dV(
		tui_gen_adjmenu(header,tui_voltmenu_printer,0,1600,adc_to_dV(start),1)
		);
}

static uint8_t tui_nummenu_printer(unsigned char* buf, int32_t val) {
	return uint2str(buf,(uint16_t)val);
}

uint16_t tui_gen_nummenu(PGM_P header, uint16_t min, uint16_t max, uint16_t start) {
	return (uint16_t)tui_gen_adjmenu(header,tui_nummenu_printer,min,max,start,1);
}


static void tui_gen_message_start(PGM_P l1) {
	lcd_clear();
	lcd_gotoxy_dw((LCDWIDTH - lcd_strwidth_P(l1))/2,0);
	lcd_puts_dw_P(l1);
}

static void tui_gen_message_end(void) {
	timer_delay_ms(100);
	tui_waitforkey();
}

void tui_gen_message(PGM_P l1, PGM_P l2) {
	tui_gen_message_start(l1);
	lcd_gotoxy((LCDWIDTH - lcd_strwidth_P(l2))/2,1);
	lcd_puts_dw_P(l2);
	tui_gen_message_end();
}

void tui_gen_message_m(PGM_P l1, const unsigned char* l2m) {
	tui_gen_message_start(l1);
	lcd_gotoxy((LCDWIDTH - lcd_strwidth(l2m))/2,1);
	lcd_puts_dw(l2m);
	tui_gen_message_end();
}

const unsigned char tui_q_s1[] PROGMEM = "NO";
const unsigned char tui_q_s2[] PROGMEM = "YES";

PGM_P const tui_q_table[] PROGMEM = {
    (PGM_P)tui_q_s1,
    (PGM_P)tui_q_s2,
};

uint8_t tui_are_you_sure(void) {
	return tui_gen_listmenu(PSTR("Are you sure?"), tui_q_table, 2, 0);
}


