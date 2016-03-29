#include "main.h"
#include "relay.h"
#include "adc.h"
#include "buttons.h"
#include "timer.h"
#include "lcd.h"
#include "lib.h"
#include "backlight.h"
#include "tui.h"

#define TUI_DEFAULT_REFRESH_INTERVAL 4

static uint8_t tui_force_draw;
static uint8_t tui_next_refresh;
static uint8_t tui_refresh_interval=TUI_DEFAULT_REFRESH_INTERVAL; // by default 1s


static void tui_draw_mainpage(uint8_t forced) {
	tui_force_draw = 0;
	if (!forced) {
//		tui_refresh_interval = tui_update_refresh_interval();
		tui_next_refresh = timer_get_5hz_cnt()+tui_refresh_interval;
	}
	lcd_gotoxy(0,0);
	lcd_puts_dw_P(PSTR("Hello World!"));
	lcd_gotoxy(0,2);
	lcd_puts_big_P(PSTR("12.34V"));

}

void tui_init(void) {
	tui_draw_mainpage(0);
}


void tui_run(void) {
	uint8_t k = buttons_get();
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
