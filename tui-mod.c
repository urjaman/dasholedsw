/*
 * Copyright (C) 2017 Urja Rannikko <urjaman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "main.h"
#include "tui-lib.h"
#include "tui-mod.h"
#include "lcd.h"
#include "timer.h"
#include "buttons.h"
#include "lib.h"

/* Include the generated database. */
#include "tuidb_db.c"

#define TUI_MODS_MAX 16

#define TUI_MOD_PAGE_SHIFT 5

#define TUI_MOD_Y_MASK (_BV(TUI_MOD_PAGE_SHIFT)-1)
struct mod_info {
	uint8_t dbid;
	uint8_t par;
	uint8_t x;
	uint8_t y;
};

static uint8_t tui_mod_cnt;

struct tui_modsave {
	struct mod_info mods[TUI_MODS_MAX];
} tm;

uint8_t tui_mods_save(void**ptr) {
	/* We save EEPROM by encoding the mod cnt into the size :) */
	/* This also makes it easy to change TUI_MODS_MAX. */
	*ptr = &tm;
	return sizeof(struct mod_info) * tui_mod_cnt;
}

void tui_mods_load(void *b, uint8_t sz) {
	uint8_t *ub = b;
	uint8_t cnt = sz / sizeof(struct mod_info);

	uint8_t rm;
	/* This detects the old format with the appended mod cnt */
	if ((rm = (sz % sizeof(struct mod_info)) )) {
		if (rm != 1) return; /* Nope. */
		cnt = ub[sz-1];
		if ((cnt * sizeof(struct mod_info)) > sz) return; /* Nope 2. */
		if (cnt > TUI_MODS_MAX) cnt = TUI_MODS_MAX;
		sz = cnt * sizeof(struct mod_info); /* Only take so much. */
	}

	if (cnt > TUI_MODS_MAX) cnt = TUI_MODS_MAX;
	tui_mod_cnt = cnt;
	memcpy(&tm, b, sz);
}

void tui_draw_mods(uint8_t page)
{
	/* Dont clear because that might flicker the lcd unnecessarily as we re-draw. */
	/* Modules are required to draw their entire area. */
	for (uint8_t i=0; i<tui_mod_cnt; i++) {
		uint8_t p = tm.mods[i].y >> TUI_MOD_PAGE_SHIFT;
		if (p != page) continue;
		uint8_t y = tm.mods[i].y & TUI_MOD_Y_MASK;
		void(*fp)(uint8_t,uint8_t,uint8_t);
		const struct tui_mod * dbptr = &(tuidb[tm.mods[i].dbid]);
		lcd_gotoxy_dw(tm.mods[i].x,y); // we help out, so the mods dont absolutely have to
		fp = (void*)pgm_read_ptr(&(dbptr->function));
		fp(tm.mods[i].x,y ,tm.mods[i].par);
	}
}

uint8_t tui_mods_pages(void)
{
	uint8_t mp = 0;
	for (uint8_t i=0; i<tui_mod_cnt; i++) {
		uint8_t p = tm.mods[i].y >> TUI_MOD_PAGE_SHIFT;
		if (p > mp) mp = p;
	}
	return mp + 1;
}

static void tui_dbid_printer(uint8_t id)
{
	const struct tui_mod * dbptr = &(tuidb[id]);
	PGM_P strp = (PGM_P)pgm_read_ptr(&(dbptr->name));
	lcd_puts_dw_P(strp);
	lcd_clear_eol();
}

static int tui_select_mod_id(uint8_t prv)
{
	int cnt = sizeof(tuidb)/sizeof(tuidb[0]);
	return tui_enh_listmenu(PSTR("Pick Module"), tui_dbid_printer, cnt, prv);
}

// 1= cancel'd
static uint8_t tui_edit_mod(uint8_t idx)
{
	int r;
	if ((r=tui_select_mod_id(tm.mods[idx].dbid))<0) return 1;
	uint8_t dbid = r;
	const struct tui_mod * dbptr = &(tuidb[dbid]);
	int (*ps)(uint8_t);
	ps = (void*)pgm_read_ptr(&(dbptr->parselect));
	uint8_t par = tm.mods[idx].par;
	if ((r=ps(tm.mods[idx].par))<0) return 1;
	uint8_t np = tui_gen_nummenu(PSTR("PAGE"), 1, 8, 1 + (tm.mods[idx].y >> TUI_MOD_PAGE_SHIFT)) - 1;
	uint8_t lbm = buttons_hw_count()==1?1:0;
	par = r;
	uint8_t wh = pgm_read_byte(&(dbptr->width));
	uint8_t lc = pgm_read_byte(&(dbptr->lines));
	uint8_t nx = tm.mods[idx].x;
	uint8_t ny = tm.mods[idx].y & TUI_MOD_Y_MASK;

	uint8_t dir = 0;
	uint8_t pk = 0;
	uint8_t rdl = 0;
	// positioning ... this is the wild stuff we need to actually do :P
	while (1) {
		uint8_t inhabited[LCDWIDTH * ((LCD_MAXY+7) / 8)] = {};
		uint8_t lcdb[LCDWIDTH];
		// we draw all other mods as boxes and record the areas they inhabit.
		for (uint8_t i=0; i<tui_mod_cnt; i++) {
			if (i==idx) continue; // all other...
			if ((tm.mods[i].y >> TUI_MOD_PAGE_SHIFT) != np) continue; /* only this page */
			uint8_t w = pgm_read_byte(&(tuidb[tm.mods[i].dbid].width));
			uint8_t l = pgm_read_byte(&(tuidb[tm.mods[i].dbid].lines));
			for (uint8_t n=0; n<l; n++) {
				const uint8_t iy = tm.mods[i].y & TUI_MOD_Y_MASK;
				const uint8_t ihmark = _BV((iy+n)&7);
				uint8_t * ihb = inhabited + tm.mods[i].x + ((iy+n)/8)*LCDWIDTH;
				uint8_t idbase = 0;
				if (n==0) idbase |= 0x01;
				if (n==l-1) idbase |= 0x80;
				for (uint8_t z=0; z<w; z++) {
					uint8_t indic = idbase;
					ihb[z] |= ihmark;
					if ((z==0)||(z==w-1)) indic = ~idbase;
					lcdb[z] = indic;
				}
				lcd_gotoxy_dw(tm.mods[i].x, iy+n);
				lcd_write_dwb(lcdb, w);
			}
		}
		// then we draw this mod either as filled or crosshatch (if conflicting pos)
		for (uint8_t n=0; n<lc; n++) {
			const uint8_t ihtest = _BV((ny+n)&7);
			uint8_t * ihb = inhabited + nx + ((ny+n)/8)*LCDWIDTH;
			for (uint8_t z=0; z<wh; z++) {
				uint8_t indic = 0xFF;
				if (ihb[z] & ihtest) indic = z&1?0xAA:0x55;
				ihb[z] |= ihtest; /* mark this mod too for clearing unused space */
				lcdb[z] = indic;
			}
			lcd_gotoxy_dw(nx, ny+n);
			lcd_write_dwb(lcdb, wh);
		}
		/* clear the area of screen not touched. */
		for (uint8_t n=0; n<LCD_MAXY; n++) {
			const uint8_t ihtest = _BV((n)&7);
			uint8_t * ihb = inhabited + ((n)/8)*LCDWIDTH;
			for (uint8_t z=0; z<LCDWIDTH; z++) {
				if (ihb[z] & ihtest) continue; /* touched, skip. */
				/* oh, lets seek to end... */
				uint8_t w;
				for (w=z+1; w<LCDWIDTH; w++) {
					if (ihb[w] & ihtest) break;
				}
				uint8_t l = w - z;
				lcd_gotoxy_dw(z, n);
				lcd_clear_dw(l);
				z += (l-1);
			}
		}
		uint8_t k=0;
		timer_delay_ms(25);
		if ((pk == BUTTON_NEXT)&&(buttons_get_v() == BUTTON_NEXT)) {
			if (!rdl) timer_delay_ms(50);
			if (buttons_get_v() == BUTTON_NEXT) {
				k = pk;
				rdl = 1;
				mini_mainloop(); // we're taking over the helpers here so remember this
			}
		}
		if (k==0) {
			rdl = 0;
			k = tui_waitforkey();
		}
		if ((lbm)&&(dir!=3)&&(k==BUTTON_OK)) k = BUTTON_PREV;
		pk = k;
		switch (k) {
			case BUTTON_OK:
				tm.mods[idx].dbid = dbid;
				tm.mods[idx].par = par;
				tm.mods[idx].x = nx;
				tm.mods[idx].y = ny | (np << TUI_MOD_PAGE_SHIFT);
				return 0;
			case BUTTON_CANCEL:
				return 1;
			case BUTTON_NEXT:
				switch (dir) {
					case 0: if ((nx+wh) < LCDWIDTH) nx++; break;
					case 1: if ((ny+lc) < LCD_MAXY) ny++; break;
					case 2: if (nx) nx--; break;
					case 3: if (ny) ny--; break;
				}
				break;
			case BUTTON_PREV: {
				PGM_P dn;
				dir = (dir+1) & 3;
				switch (dir) {
					case 0: dn = PSTR(" DIR: RIGHT "); break;
					case 1: dn = PSTR(" DIR: DOWN "); break;
					case 2: dn = PSTR(" DIR: LEFT "); break;
					case 3: dn = PSTR(" DIR: UP "); break;
				}
				lcd_gotoxy_dw(((LCDWIDTH - lcd_strwidth_P(dn))/2),0);
				lcd_puts_dw_P(dn);
				timer_delay_ms(150);
				mini_mainloop();
				timer_delay_ms(150);
			}
			break;
		}
	}
}




static const unsigned char tui_tmm_s1[] PROGMEM = "Edit Module";
static const unsigned char tui_tmm_s2[] PROGMEM = "Delete Module";
static PGM_P const tui_tmm_table[] PROGMEM = { // Tui mod menu
	(PGM_P)tui_tmm_s1,
	(PGM_P)tui_tmm_s2,
	(PGM_P)tui_exit_menu
};

static void tui_mod_menu(uint8_t idx)
{
	uint8_t sel = tui_gen_listmenu(PSTR("Module Menu"), tui_tmm_table, 3, 0);
	if (sel == 1) { // delete
		for (uint8_t n=idx; n<(tui_mod_cnt-1); n++) tm.mods[n] = tm.mods[n+1];
		tui_mod_cnt--;
		return;
	}
	if (sel == 0) {
		tui_edit_mod(idx);
	}
	// exit is default
}

static void tui_mlist_printer(uint8_t id)
{
	if (id==tui_mod_cnt) {
		lcd_puts_dw_P(PSTR("Add new.."));
	} else if (id==tui_mod_cnt+1) {
		lcd_puts_dw_P((PGM_P)tui_exit_menu);
	} else {
		uint8_t buf[32];
		const struct tui_mod * dbptr = &(tuidb[tm.mods[id].dbid]);
		PGM_P strp = (PGM_P)pgm_read_ptr(&(dbptr->name));
		uint8_t l = uchar2str(buf, id);
		buf[l++] = ':';
		buf[l++] = ' ';
		strncpy_P((char*)buf+l, strp, 26-l);
		l = strlen((char*)buf);
		buf[l++] = '(';
		l += uchar2str(buf+l,tm.mods[id].par);
		buf[l++] = ')';
		buf[l] = 0;
		lcd_puts_dw(buf);
	}
	lcd_clear_eol();
}

void tui_modules_editor(void)
{
	uint8_t mi = 0;
	while (1) {
		uint8_t cnt = tui_mod_cnt + 2; // all the mods, then add new, then exit menu
		int r = tui_enh_listmenu(PSTR("Module List"), tui_mlist_printer, cnt, mi);
		if (r<0) return; // exit
		mi = r;
		if (mi>=cnt-1) { // exit
			return;
		} else if (mi==cnt-2) { // add new
			if (tui_mod_cnt >= TUI_MODS_MAX) {
				tui_gen_message(PSTR("Too Many"), PSTR("Modules :("));
			} else {
				struct mod_info n = { 0, 0, 0, 0 };
				tm.mods[tui_mod_cnt++] = n;
				if (tui_edit_mod(tui_mod_cnt-1)) {
					// canceled
					tui_mod_cnt--;
				}
			}
		} else { // picked a module
			tui_mod_menu(mi);
		}
	}
}

