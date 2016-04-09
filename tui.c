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
	luint2xstr(buf, timer_get());
	lcd_gotoxy_dw(0, 5);
	lcd_puts(buf);

	luint2xstr(buf, adc_avg_cnt);
	lcd_gotoxy_dw(0, 4);
	lcd_puts(buf);

	buf[0] = prev_k + '0'; buf[1] = 0;
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
