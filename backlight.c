#include "main.h"
#include "timer.h"
#include "adc.h"
#include "relay.h"
#include "backlight.h"
#include "lcd.h"

#define DRV_ALWAYS 0x80
#define DRV_VAL 0x7F

static struct bl_save {
	uint8_t drv_value;
	uint8_t value;
	uint8_t to;
} bl;

uint8_t backlight_save(void**ptr) {
	*ptr = &bl;
	return sizeof(struct bl_save);
}

void backlight_load(void *b, uint8_t sz) {
	if (sz != sizeof(struct bl_save)) return; // sorry, no.
	memcpy(&bl, b, sz);
}


static uint32_t bl_last_sec=0;
static int8_t bl_v_now;
static int8_t bl_v_fadeto;

static void backlight_simple_set(int8_t v)
{
#ifdef DP_HAS_BL
	dp_set_bl(v+1); // 0=off
#endif
	bl_v_now = v;
}

static void backlight_fader(void)
{
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

void backlight_init(void)
{
	const uint8_t backlight_default = 14;
	bl.to = 10;
	backlight_set(backlight_default);
	backlight_simple_set(backlight_default);
	bl_v_fadeto = -1;
	bl.drv_value = 7;
}

void backlight_set(uint8_t v)
{
	if (v>16) v=16;
	if ((bl_v_now == bl.value)&&(bl_v_fadeto == bl.value)) {
		backlight_simple_set(v);
		bl_v_fadeto = v;
	}
	bl.value = v;
	if ((bl.drv_value&DRV_VAL)>v) bl.drv_value = (bl.drv_value & DRV_ALWAYS) + v;
}


void backlight_set_dv_always(uint8_t v)
{
	if (v) {
		bl.drv_value |= DRV_ALWAYS;
	} else {
		bl.drv_value &= ~DRV_ALWAYS;
	}
}

uint8_t backlight_get_dv_always(void)
{
	return !!(bl.drv_value & DRV_ALWAYS);
}

void backlight_set_dv(uint8_t v)
{
	if (v>16) v=16;
	if (v>bl.value) v=bl.value;
	bl.drv_value = (bl.drv_value & DRV_ALWAYS) + v;
}

uint8_t backlight_get(void)
{
	return bl.value;
}

uint8_t backlight_get_dv(void)
{
	return bl.drv_value & DRV_VAL;
}

uint8_t backlight_get_to(void)
{
	return bl.to;
}

void backlight_set_to(uint8_t to)
{
	bl.to = to;
}

void backlight_activate(void)
{
	bl_last_sec = timer_get();
	bl_v_fadeto = bl.value;
	backlight_fader();
}

void backlight_run(void)
{
	uint32_t diff = timer_get() - bl_last_sec;
	if (diff >= bl.to) {
		if ((bl.drv_value&DRV_ALWAYS)||(relay_get_autodecision() == RLY_MODE_ON)) {
			bl_v_fadeto = bl.drv_value & DRV_VAL;
		} else {
			bl_v_fadeto = -1;
		}
	} else {
		bl_v_fadeto = bl.value;
	}
	backlight_fader();
}



