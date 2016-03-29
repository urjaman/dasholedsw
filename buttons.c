#include "main.h"
#include "backlight.h"
#include "buttons.h"
#include "timer.h"

#define BTNS (_BV(4)|_BV(5)|_BV(6)|_BV(7))

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
		if (sv == v){
			return v;
		}
		v = sv;
	}
}

