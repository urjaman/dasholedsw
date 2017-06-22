#pragma once

struct tui_mod {
	PGM_P name;
	void(*function)(uint8_t,uint8_t,uint8_t); // x, y, par
	int(*parselect)(uint8_t); // prev
	uint8_t width;
	uint8_t lines;
};

/* This does more than is apparent; used by make_tuidb.sh to generate the list. */
#define TUI_MOD(fn,ps,mn,wh,lc) void fn(uint8_t x, uint8_t y, uint8_t par); void fn(uint8_t x UNUSED, uint8_t y UNUSED, uint8_t par UNUSED)

void tui_draw_mods(void);
void tui_modules_editor(void);
