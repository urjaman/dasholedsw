#include "main.h"
#include "timer.h"
#include "lcd.h"
#include "tui-lib.h"
#include "pulse.h"
#include "saver.h"
#include "backlight.h"
#include "lib.h"
#include "buttons.h"
#include "speedo.h"

struct spd_save {
	uint16_t mpp; // 0.16 fixed point meters per edge of the roadspd sensor
};

static struct spd_save ss;


uint8_t speedo_save(void**ptr) {
	*ptr = &ss;
	return sizeof(struct spd_save);
}

void speedo_load(void *b, uint8_t sz) {
	if (sz != sizeof(struct spd_save)) return; // sorry, no.
	memcpy(&ss, b, sz);
}

static uint16_t speedo_kmh_math(uint16_t mpp) {
	uint32_t spd = (pulse_get_hz(PCH_ROADSPD, NULL) * (uint32_t)mpp) >> PULSE_HZ_SHIFT;
	// Now it is in 16.16 m/s, lets take it down to 0.1 km/h resolution
	spd = ((spd * 36)+32768) >> SPEEDO_M_SHIFT;
	return spd;
}

void speedo_calibrate(void) {
	const uint8_t cal_kmh = tui_gen_nummenu(PSTR("Speed (km/h)"), 10, 200, 80);
	if (!tui_yes_no(PSTR("Calibrate Now?"),0)) return;
	uint8_t secs = 3;
	do {
		mini_mainloop();
	} while (!timer_get_1hzp());
	uint16_t t1, t2;
	uint16_t base = pulse_edge_ctr(PCH_ROADSPD, &t1);
	do {
		lcd_huge_char('0'+secs);
		do {
			mini_mainloop();
		} while (!timer_get_1hzp());
	} while (--secs);
	uint16_t count = pulse_edge_ctr(PCH_ROADSPD, &t2) - base;
	uint16_t time_delta = (t2 - t1); // In units of 8192/sec
	uint32_t time_secs = time_delta * 8; // 8192 => 65536 (N.B. PULSE_TIMER_HZ AND SPEEDO_M_SHIFT)
	uint32_t meters = ((cal_kmh * time_secs * 10)+18) / 36;
	if (!count) {
		tui_gen_message(PSTR("Count was"), PSTR("zero :("));
		return;
	}
	uint32_t nmpp = (meters+(count/2)) / count;
	if (nmpp > 65535) {
		tui_gen_message(PSTR("Signal is"), PSTR("too slow"));
		return;
	}
	uint8_t k = 0;
	do {
		PGM_P msg = PSTR("Speed:");
		uint8_t off1 = (LCDWIDTH - lcd_strwidth_P(msg))/2;
		lcd_gotoxy_dw(0,0);
		lcd_clear_dw(off1);
		lcd_puts_dw_P(msg);
		lcd_clear_eol();
		uint16_t kmh10 = speedo_kmh_math(nmpp);
		uint8_t buf[16];
		uint8_t n = uint2str(buf, kmh10/10);
		buf[n] = '.';
		buf[n+1] = '0' + kmh10%10;
		buf[n+2] = 0;
		uint8_t off2 = (LCDWIDTH - lcd_strwidth_big(buf))/2;
		lcd_clear_big_dw(off2);
		lcd_puts_big(buf);
		lcd_clear_big_eol();
		lcd_clear_eos();
		backlight_activate(); // No dozing off in this screen :P
		do {
			mini_mainloop();
			k = buttons_get();
			if (timer_get_5hzp()) break;
		} while (!k);
	} while (!k);
	if (!tui_yes_no(PSTR("Calibration OK?"),0)) return;
	ss.mpp = nmpp;
	tui_gen_message(PSTR("CALIBRATION"), PSTR("SAVED"));
}

uint16_t speedo_get_mpp(void) {
	return ss.mpp;
}

uint16_t speedo_get_kmh10(void) {
	return speedo_kmh_math(ss.mpp);
}
