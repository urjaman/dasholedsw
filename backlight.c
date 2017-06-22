#include "main.h"
#include "timer.h"
#include "adc.h"
#include "relay.h"
#include "backlight.h"
#include "lcd.h"

static uint8_t bl_drv_value;
static uint8_t bl_value;
static uint8_t bl_to;
static uint32_t bl_last_sec=0;

static int8_t bl_v_now;
static int8_t bl_v_fadeto;

void backlight_simple_set(int8_t v) {
#ifdef DP_HAS_BL
	dp_set_bl(v+1); // 0=off
#endif
	bl_v_now = v;
}

static void backlight_fader(void) {
	if (bl_v_fadeto < bl_v_now) {
		backlight_simple_set(bl_v_now-1);
		timer_delay_ms(50);
		goto ret;
	}
	if (bl_v_fadeto > bl_v_now) {
		backlight_simple_set(bl_v_fadeto);
		goto ret;
	}
ret:
	if (bl_v_fadeto != bl_v_now) timer_set_waiting();
	return;
}

void backlight_init(void) {
	const uint8_t backlight_default = 14;
	/* These are just obsolete stuff for timer to work...*/
	TCC4_PER = 1023;
	TCC4_CTRLA = TC45_CLKSEL_DIV1_gc;
	bl_to = 10;
	backlight_set(backlight_default);
	backlight_simple_set(backlight_default);
	bl_v_fadeto = -1;
	bl_drv_value = 7;
}

void backlight_set(uint8_t v) {
	if (v>16) v=16;
	if ((bl_v_now == bl_value)&&(bl_v_fadeto == bl_value)) {
		backlight_simple_set(v);
		bl_v_fadeto = v;
	}
	bl_value = v;
	if (bl_drv_value>v) bl_drv_value = v;
}


void backlight_set_dv(uint8_t v) {
	if (v>16) v=16;
	if (v>bl_value) v=bl_value;
	bl_drv_value = v;
}

uint8_t backlight_get(void) {
	return bl_value;
}

uint8_t backlight_get_dv(void) {
	return bl_drv_value;
}

uint8_t backlight_get_to(void) {
	return bl_to;
}

void backlight_set_to(uint8_t to) {
	bl_to = to;
}

void backlight_activate(void) {
	bl_last_sec = timer_get();
	bl_v_fadeto = bl_value;
	backlight_fader();
}

void backlight_run(void) {
	uint32_t diff = timer_get() - bl_last_sec;
	if (diff >= bl_to) {
		if (relay_get_autodecision() == RLY_MODE_ON) {
			bl_v_fadeto = bl_drv_value;
		} else {
			bl_v_fadeto = -1;
		}
	} else {
		bl_v_fadeto = bl_value;
	}
	backlight_fader();
}



