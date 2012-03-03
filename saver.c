#include "main.h"
#include "backlight.h"
#include "relay.h"
#include "saver.h"
#include "tui.h"
#include "batlvl.h"
#include <util/crc16.h>

extern int16_t tui_temprefresh_lp_temp;
extern int16_t tui_temprefresh_hp_temp;
extern uint8_t tui_temprefresh_lp_interval;
extern uint8_t tui_temprefresh_hp_interval;

#ifdef ALARMCLOCK
#define SAVER_MAGIC1 0xA5C5
#else
#define SAVER_MAGIC1 0xA551
#endif

#define SAVER_MAGIC3 0xE5
struct __attribute__ ((__packed__)) sys_settings {
	uint16_t magic1;
	uint16_t relay_autovoltage;
	uint8_t relay_mode;
	uint8_t relay_keepon;
	uint8_t backlight_brightness;
	uint8_t backlight_timeout;
	uint8_t backlight_dv;
	uint8_t lcd_contrast;
	uint8_t tui_mp_mods[4][TUI_MODS_MAXDEPTH];
	batlvl_setting_t batlvl_settings;
	int16_t tui_temprefresh_lp_temp;
	int16_t tui_temprefresh_hp_temp;
	uint8_t tui_temprefresh_lp_interval;
	uint8_t tui_temprefresh_hp_interval;
	uint8_t magic3;
	uint16_t crc16;
};

void saver_load_settings(void) {
	uint16_t crc=0xFFFF;
	uint8_t i;
	struct sys_settings st;
	eeprom_read_block(&st,(void*)0, sizeof(struct sys_settings));
	if (st.magic1 != SAVER_MAGIC1) return;
	if (st.magic3 != SAVER_MAGIC3) return;
	for(i=0;i<(sizeof(struct sys_settings)-2);i++) {
		crc = _crc16_update(crc, (((uint8_t*)&st)[i]) );
	}
	if (crc != st.crc16) return;
#ifdef ALARMCLOCK
	// Load alarm stuff from relay variables
#else
	relay_set_autovoltage(st.relay_autovoltage);
	relay_set_keepon(st.relay_keepon);
	relay_set(st.relay_mode);
#endif
	backlight_set(st.backlight_brightness);
	backlight_set_to(st.backlight_timeout);
	backlight_set_dv(st.backlight_dv);
	backlight_set_contrast(st.lcd_contrast);
	memcpy(tui_mp_mods,st.tui_mp_mods,TUI_MODS_MAXDEPTH*4);
	batlvl_settings = st.batlvl_settings;
	tui_temprefresh_lp_temp = st.tui_temprefresh_lp_temp;
	tui_temprefresh_hp_temp = st.tui_temprefresh_hp_temp;
	tui_temprefresh_lp_interval = st.tui_temprefresh_lp_interval;
	tui_temprefresh_hp_interval = st.tui_temprefresh_hp_interval;
}

void saver_save_settings(void) {
	uint16_t crc=0xFFFF;
	uint8_t i;
	struct sys_settings st;
	st.magic1 = SAVER_MAGIC1;
#ifdef ALARMCLOCK
	//Save alarm stuff in relay variables
#else
	st.relay_autovoltage = relay_get_autovoltage();
	st.relay_keepon = relay_get_keepon();
	st.relay_mode = relay_get_mode();
#endif
	st.backlight_brightness = backlight_get();
	st.backlight_timeout = backlight_get_to();
	st.backlight_dv = backlight_get_dv();
	st.lcd_contrast = backlight_get_contrast();
	memcpy(st.tui_mp_mods,tui_mp_mods,TUI_MODS_MAXDEPTH*4);
	st.batlvl_settings = batlvl_settings;
	st.tui_temprefresh_lp_temp = tui_temprefresh_lp_temp;
	st.tui_temprefresh_hp_temp = tui_temprefresh_hp_temp;
	st.tui_temprefresh_lp_interval = tui_temprefresh_lp_interval;
	st.tui_temprefresh_hp_interval = tui_temprefresh_hp_interval;
	st.magic3 = SAVER_MAGIC3;
	for(i=0;i<(sizeof(struct sys_settings)-2);i++) {
		crc = _crc16_update(crc, (((uint8_t*)&st)[i]) );
	}
	st.crc16 = crc;
	eeprom_update_block(&st,(void*)0, sizeof(struct sys_settings));
}
