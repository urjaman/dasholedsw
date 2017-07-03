#include "main.h"
#include "backlight.h"
#include "relay.h"
#include "saver.h"
#include "tui-mod.h"
#include "adc.h"
#include "speedo.h"
#include "ee.h"
#include <util/crc16.h>

struct saver_userdb_entry {
	uint8_t (*save_cb)(void**);
	void (*load_cb)(void *, uint8_t);
	char id;
};

static const struct saver_userdb_entry userdb[] PROGMEM = {
	{ tui_mods_save, tui_mods_load, 'T' },
	{ relay_save, relay_load, 'R' },
	{ backlight_save, backlight_load, 'B' },
	{ speedo_save, speedo_load, 'S' },
};

struct saver_mdat_header {
	uint8_t sz;
	char id;
//	uint8_t dat[];
};

#define SAVER_MAGIC 0xE5

struct saver_settings {
	uint8_t magic;
	uint8_t subcnt; // count of sub-items in the data region
	uint16_t subsize; // size of the entire data region
	uint16_t subcrc; // CRC of the entire data region
	uint16_t hdrcrc; // CRC of this header
	//struct saver_mdat_entry mdat[];
};

#define SETTINGS_OFFSET (32)
#define DATA_OFFSET (SETTINGS_OFFSET+sizeof(struct saver_settings))

uint16_t crc16(uint16_t crc_in, const void *bufv, uint16_t cnt) {
	const uint8_t *b = bufv;
	for(uint16_t i=0;i < cnt; i++) {
		crc_in = _crc16_update(crc_in, b[i]);
	}
	return crc_in;
}

const PGM_P saver_load_settings(void) {
	uint16_t crc=0xFFFF;
	struct saver_settings st;
	ee_init();
	ee_read_block(&st,(void*)SETTINGS_OFFSET, sizeof(struct saver_settings));
	if (st.magic != SAVER_MAGIC) return PSTR("magic");
	// Sanity check the header, just in case
	if (st.subsize > 1024) return PSTR("subsize > 1024"); // We only have 1K of EEPROM
	if (st.subcnt > 32) return PSTR("subcnt > 32"); // I dont expect THAT many users
	crc = crc16(crc, &st, sizeof(struct saver_settings)-2);

	if (crc != st.hdrcrc) return PSTR("header crc");

	uint16_t subcrc=0xFFFF;
	const uint8_t* ee_data = (uint8_t*)DATA_OFFSET;
	for (uint16_t i=0;i<st.subsize;i++) {
		subcrc = _crc16_update(subcrc, ee_read_byte(ee_data+i));
	}
	if (subcrc != st.subcrc) return PSTR("data crc");
	for (uint8_t i=0;i<st.subcnt;i++) {
		struct saver_mdat_header hd;
		uint8_t load_buffer[256]; // big temporary buffer
		ee_read_block(&hd, ee_data, sizeof(struct saver_mdat_header));
		if (hd.id&0x80) return PSTR("id not ASCII"); // They're ASCII characters, not binary data
		ee_data += sizeof(struct saver_mdat_header);
		ee_read_block(load_buffer, ee_data, hd.sz);
		ee_data += hd.sz;
		// Then we go around the user database to see who wants this id :)
		const uint8_t cnt = sizeof(userdb)/sizeof(userdb[0]);
		for (uint8_t n=0;n<cnt;n++) {
			if (pgm_read_byte(&(userdb[n].id)) == hd.id) {
				void (*load_cb)(void*, uint8_t);
				load_cb = pgm_read_ptr(&(userdb[n].load_cb));
				load_cb(load_buffer, hd.sz);
				break;
			}
		}
	}
	return NULL;
}

uint16_t saver_save_settings(void) {
	struct saver_settings st = { SAVER_MAGIC, 0, 0, 0xFFFF, 0xFFFF };
	uint8_t* ee_data = (uint8_t*)DATA_OFFSET;
	const uint8_t cnt = sizeof(userdb)/sizeof(userdb[0]);
	for (uint8_t n=0;n<cnt;n++) {
		uint8_t *p = NULL;
		struct saver_mdat_header hd;
		hd.id = pgm_read_byte(&(userdb[n].id));
		uint8_t (*save_cb)(void**);
		save_cb = pgm_read_ptr(&(userdb[n].save_cb));
		hd.sz = save_cb((void**)&p);
		if (!p) continue;
		if (!hd.sz) continue;
		ee_update_block(&hd, ee_data, sizeof(struct saver_mdat_header));
		ee_data += sizeof(struct saver_mdat_header);
		ee_update_block(p, ee_data, hd.sz);
		ee_data += hd.sz;
		st.subcnt++;
		st.subsize += sizeof(struct saver_mdat_header) + hd.sz;
		st.subcrc = crc16(st.subcrc, &hd, sizeof(struct saver_mdat_header));
		st.subcrc = crc16(st.subcrc, p, hd.sz);
	}
	st.hdrcrc = crc16(st.hdrcrc, &st, sizeof(struct saver_settings)-2);
	ee_update_block(&st, (void*)SETTINGS_OFFSET, sizeof(struct saver_settings));
	return sizeof(struct saver_settings) + st.subsize;
}
