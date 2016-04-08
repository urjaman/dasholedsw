#include "main.h"
#include "backlight.h"
#include "buttons.h"
#include "timer.h"

#define BTNS (_BV(4)|_BV(5)|_BV(6)|_BV(7))

/* Actually installed buttons: */
#define BUTMODE 1

/* Modes: */
/* 1 button: short press: next, long press: ok */
/* 2 buttons: buttons are next and prev, both are ok */
/* 3+ buttons: as-is. */

void buttons_init(void) {
}

uint8_t buttons_get_v(void) {
	uint8_t rv = (uint8_t)~VPORT2_IN;
	rv = (uint8_t)rv & (uint8_t)BTNS;
	rv = (uint8_t)rv >> (uint8_t)4;
	return rv;
}

uint8_t buttons_get(void) {
	uint8_t v = buttons_get_v();
	if (!v) return 0;
	backlight_activate();
	timer_delay_ms(180);
	for(;;) {
		uint8_t sv;
		timer_delay_ms(32);
		sv = buttons_get_v();
		if (sv == v) {
			break;
		}
		v = sv;
	}
#if (BUTMODE == 1)
	/* Button debounced, now determine long or short. */
	uint8_t lngcnt=0;
	if (v==BUTTON_NEXT) while (buttons_get_v() == v) {
		timer_delay_ms(10);
		lngcnt++;
		if (lngcnt >= 50) {
			v = BUTTON_OK;
			break;
		}
	}
	if (v==BUTTON_NEXT) timer_delay_ms(25); /* Debounce */
#elif (BUTMODE == 2)
	if (v==(BUTTON_NEXT|BUTTON_PREV)) v = BUTTON_OK;
#endif
	return v;
}

/* Returns how many actual buttons installed, as a hint for better UI. */
uint8_t buttons_hw_count(void) {
	return BUTMODE;
}
