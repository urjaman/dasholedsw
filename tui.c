#include "main.h"
#include "relay.h"
#include "adc.h"
#include "buttons.h"
#include "timer.h"
#include "lcd.h"
#include "lib.h"
#include "backlight.h"
#include "tui.h"
#include "tui-lib.h"
#include "pcd8544.h"

#define TUI_DEFAULT_REFRESH_INTERVAL 5

static uint8_t tui_force_draw;
static uint8_t tui_next_refresh;
static uint8_t tui_refresh_interval=TUI_DEFAULT_REFRESH_INTERVAL; // by default 1s

extern uint16_t adc_avg_cnt;
static uint8_t prev_k = 0;


static void tui_draw_mainpage(uint8_t forced) {
	tui_force_draw = 0;
	if (!forced) {
//		tui_refresh_interval = tui_update_refresh_interval();
		tui_next_refresh = timer_get_5hz_cnt()+tui_refresh_interval;
	}
	lcd_clear();
	uint8_t buf[10];
	adc_print_v(buf, adc_read_mb());
	lcd_gotoxy_dw(0,0);
	lcd_puts_big(buf);

	static uint8_t ct = 40;
	pcd8544_contrast(++ct);
	luint2str(buf, ct);
	if (ct == 50) ct = 29;
	lcd_gotoxy_dw(0, 5);
	lcd_puts(buf);

	luint2xstr(buf, adc_avg_cnt);
	lcd_gotoxy_dw(0, 4);
	lcd_puts(buf);

	buf[0] = prev_k + '0'; buf[1] = 'K'; buf[2] = 0;
	lcd_gotoxy_dw(0,2);
	lcd_puts(buf);

	adc_print_v(buf, adc_read_sb());
	lcd_gotoxy_dw(0, 3);
	lcd_puts(buf);

}

void tui_init(void) {
	tui_draw_mainpage(0);
}


void tui_run(void) {
	uint8_t k = buttons_get();
	if ((prev_k != k)&&(k)) {
		prev_k = k;
		tui_force_draw = 1;
	}
	if (k==BUTTON_OK) {
		tui_mainmenu();
		tui_force_draw = 1;
	}
	if (tui_force_draw) {
		tui_draw_mainpage(tui_force_draw);
		return;
	} else {
		if (timer_get_5hzp()) {
			signed char update = (signed char)(timer_get_5hz_cnt() - tui_next_refresh);
			if (update>=0) {
				tui_draw_mainpage(0);
				return;
			}
		}
	}
}


void tui_activate(void) {
	tui_force_draw=1;
}

const unsigned char tui_rm_s1[] PROGMEM = "OFF";
const unsigned char tui_rm_s2[] PROGMEM = "ON";
const unsigned char tui_rm_s3[] PROGMEM = "AUTO";
PGM_P const tui_rm_table[] PROGMEM = {
    (PGM_P)tui_rm_s1,
    (PGM_P)tui_rm_s2,
    (PGM_P)tui_rm_s3,
};

static void tui_relaymenu(void) {
	uint8_t sel;
	sel = tui_gen_listmenu(PSTR("RELAY MODE"), tui_rm_table, 3, relay_get_mode());
	relay_set(sel);
}


const unsigned char tui_blsm_name[] PROGMEM = "Disp. cfg";
const unsigned char tui_blsm_s1[] PROGMEM = "Brightness";
const unsigned char tui_blsm_s2[] PROGMEM = "Drv Bright";
const unsigned char tui_blsm_s3[] PROGMEM = "Timeout";
PGM_P const tui_blsm_table[] PROGMEM = { // BL Settings Menu
	(PGM_P)tui_blsm_s1,
	(PGM_P)tui_blsm_s2,
	(PGM_P)tui_blsm_s3,
	(PGM_P)tui_exit_menu
};

static void tui_blsettingmenu(void) {
	uint8_t sel = 0;
	for(;;) {
		sel = tui_gen_listmenu((PGM_P)tui_blsm_name, tui_blsm_table, 4, sel);
		switch (sel) {
			case 0: {
			uint8_t v = tui_gen_nummenu((PGM_P)tui_blsm_s1, 0, 16, backlight_get());
			backlight_set(v);
			}
			break;

			case 1: {
			uint8_t v = tui_gen_nummenu((PGM_P)tui_blsm_s2, 0, backlight_get(), backlight_get_dv());
			backlight_set_dv(v);
			}
			break;

			case 2: {
			uint8_t v = tui_gen_nummenu((PGM_P)tui_blsm_s3, 1, 255, backlight_get_to());
			backlight_set_to(v);
			}
			break;

			default:
			return;
		}
	}
}



const unsigned char tui_sm_name[] PROGMEM = "SETTINGS";
const unsigned char tui_sm_s1[] PROGMEM = "Rly V.Thrs";
const unsigned char tui_sm_s2[] PROGMEM = "Rly KeepOn";
// BL Settings menu (3)

// Exit Menu (4)


PGM_P const tui_sm_table[] PROGMEM = { // Settings Menu
    (PGM_P)tui_sm_s1,
    (PGM_P)tui_sm_s2,
    (PGM_P)tui_blsm_name,
    (PGM_P)tui_exit_menu
};

static void tui_settingsmenu(void) {
	uint8_t sel = 0;
	for(;;) {
		sel = tui_gen_listmenu((PGM_P)tui_sm_name, tui_sm_table, 4, sel);
		switch (sel) {
			case 0: {
			uint16_t v = tui_gen_voltmenu((PGM_P)tui_sm_s1, relay_get_autovoltage());
			relay_set_autovoltage(v);
			}
			break;

			case 1: {
			uint8_t v = tui_gen_nummenu((PGM_P)tui_sm_s2, 1, 255, relay_get_keepon());
			relay_set_keepon(v);
			}
			break;

			case 2:
			tui_blsettingmenu();
			break;

			default:
			return;
		}
	}
}

const unsigned char tui_mm_s1[] PROGMEM = "RELAY MODE";
// Settings Menu (2)
// Exit Menu (4)

PGM_P const tui_mm_table[] PROGMEM = {
    (PGM_P)tui_mm_s1,
    (PGM_P)tui_sm_name,
    (PGM_P)tui_exit_menu
};

void tui_mainmenu(void) {
	uint8_t sel=0;
	for (;;) {
		sel = tui_gen_listmenu(PSTR("MAIN MENU"), tui_mm_table, 3, sel);
		switch (sel) {
			case 0:
				tui_relaymenu();
				break;
			case 1:
				tui_settingsmenu();
				break;
			default:
				return;
		}
	}
}
