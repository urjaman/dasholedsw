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
#include "tui-mod.h"
#include "saver.h"
#include "speedo.h"

#define TUI_DEFAULT_REFRESH_INTERVAL 5

static uint8_t tui_force_draw;
static uint8_t tui_next_refresh;
static uint8_t tui_refresh_interval=TUI_DEFAULT_REFRESH_INTERVAL; // by default 1s

static uint8_t tui_page = 0;

static void tui_draw_mainpage(uint8_t forced)
{
	tui_force_draw = 0;
	if (!forced) {
		tui_next_refresh = timer_get_5hz_cnt()+tui_refresh_interval;
	}

	tui_draw_mods(tui_page);
}

void tui_init(void)
{
	tui_init_themes();
	lcd_clear();
	tui_draw_mainpage(0);
}


void tui_run(void)
{
	uint8_t k = buttons_get();
	if (k==BUTTON_OK) {
		tui_mainmenu();
		lcd_clear();
		tui_force_draw = 1;
	} else if (k==BUTTON_NEXT) {
		const uint8_t tui_pages = tui_mods_pages();
		tui_page = (tui_page + 1) % tui_pages;
		lcd_huge_char('1' + tui_page);
		timer_delay_ms(150);
		timer_delay_ms(150);
		lcd_clear();
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


void tui_activate(void)
{
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

static void tui_relaymenu(void)
{
	uint8_t sel;
	sel = tui_gen_listmenu(PSTR("RELAY MODE"), tui_rm_table, 3, relay_get_mode());
	relay_set(sel);
}


const unsigned char tui_blsm_name[] PROGMEM = "Disp. cfg";
const unsigned char tui_blsm_s1[] PROGMEM = "Brightness";
const unsigned char tui_blsm_s2[] PROGMEM = "Drv Bright";
const unsigned char tui_blsm_s3[] PROGMEM = "Always Drv";
const unsigned char tui_blsm_s4[] PROGMEM = "Timeout";
const unsigned char tui_blsm_s5[] PROGMEM = "Theme";

PGM_P const tui_blsm_table[] PROGMEM = { // BL Settings Menu
	(PGM_P)tui_blsm_s1,
	(PGM_P)tui_blsm_s2,
	(PGM_P)tui_blsm_s3,
	(PGM_P)tui_blsm_s4,
	(PGM_P)tui_blsm_s5,
	(PGM_P)tui_exit_menu
};

static void tui_blsettingmenu(void)
{
	uint8_t sel = 0;
	for(;;) {
		sel = tui_gen_listmenu((PGM_P)tui_blsm_name, tui_blsm_table, 6, sel);
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
				uint8_t v = tui_yes_no((PGM_P)tui_blsm_s3, backlight_get_dv_always());
				backlight_set_dv_always(v);
			}
			break;

			case 3: {
				uint8_t v = tui_gen_nummenu((PGM_P)tui_blsm_s4, 1, 255, backlight_get_to());
				backlight_set_to(v);
			}
			break;

			case 4:
				tui_pick_theme();
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
const unsigned char tui_sm_s3[] PROGMEM = "TUI Modules";
const unsigned char tui_sm_s4[] PROGMEM = "Speedo Calib";
const unsigned char tui_sm_s5[] PROGMEM = "Load Settings";
const unsigned char tui_sm_s6[] PROGMEM = "Save Settings";
// Exit Menu (4)


PGM_P const tui_sm_table[] PROGMEM = { // Settings Menu
	(PGM_P)tui_sm_s1,
	(PGM_P)tui_sm_s2,
	(PGM_P)tui_blsm_name,
	(PGM_P)tui_sm_s3,
	(PGM_P)tui_sm_s4,
	(PGM_P)tui_sm_s5,
	(PGM_P)tui_sm_s6,
	(PGM_P)tui_exit_menu
};

static void tui_settingsmenu(void)
{
	uint8_t sel = 0;
	for(;;) {
		sel = tui_gen_listmenu((PGM_P)tui_sm_name, tui_sm_table, 8, sel);
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

			case 3:
				tui_modules_editor();
				break;

			case 4:
				speedo_calibrate();
				break;

			case 5: {
				PGM_P msg = saver_load_settings();
				if (msg) tui_gen_message(PSTR("ERROR:"), msg);
				else tui_gen_message(PSTR("SETTINGS"), PSTR("LOADED"));
			}
			break;

			case 6: {
				uint8_t buf[32];
				uint16_t sz = saver_save_settings();
				uint2str(buf, sz);
				strcat_P((char*)buf, PSTR(" bytes"));
				tui_gen_message_m(PSTR("Saved"), buf);
			}
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

void tui_mainmenu(void)
{
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
