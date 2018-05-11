#include "main.h"
#include "tui-lib.h"
#include "ssd1331.h"


struct tui_colors {
	color_t fg;
	color_t bg;
};

struct tui_theme {
	struct tui_colors c[TTC_COLOR_COUNT];
};


static struct tui_color_save {
	uint8_t theme_id; /* If it's a fixed theme, only this byte is saved. */
	struct tui_theme t;
} s;

static uint8_t current_color;
static uint8_t prev_color;

void tui_set_color(enum tui_theme_col n) {
	prev_color = current_color;
	current_color = n;
	dp_set_fg_bg(s.t.c[n].fg, s.t.c[n].bg);
}

void tui_unset_color(void) {
	current_color = prev_color;
	dp_set_fg_bg(s.t.c[current_color].fg, s.t.c[current_color].bg);
}

void tui_default_color(void) {
	tui_set_color(TTC_LN1);
	tui_set_color(TTC_LN1);
}

static const struct tui_theme tui_themes[2] PROGMEM = {
	{ {	/* Default Theme */
		{ rgb(  0,   0, 255), rgb(  0,   0,   0) }, // LN1
		{ rgb( 33,  33, 255), rgb(  0,   0,  33) }, // LN2
		{ rgb(128,   0, 255), rgb(  0,   0,   0) }, // HL1
		{ rgb(144,  33, 255), rgb( 16,   0,  33) }, // HL2
		{ rgb(  0, 226, 128), rgb(  0,   0,   0) }, // HEAD1
		{ rgb(226,  79,   0), rgb(  0,   0,   0) }  // HEAD2
	} },
	{ {	/* Balls (to the wall) Theme */
		{ rgb(226, 219,   0), rgb(  0,   0,   0) }, // LN1
		{ rgb(102, 226,   0), rgb( 30,  61,   6) }, // LN2
		{ rgb(255,  99,   2), rgb(  0,   0,   0) }, // HL1
		{ rgb(195, 226,   0), rgb(  6,  51,  61) }, // HL2
		{ rgb(255,   0, 255), rgb(  0,  96,   0) }, // HEAD1
		{ rgb(255,   0,   0), rgb(  0,  96,   0) }  // HEAD2
	} }
};


const unsigned char tui_themes_s1[] PROGMEM = "Default";
const unsigned char tui_themes_s2[] PROGMEM = "Balls";

PGM_P const tui_theme_table[] PROGMEM = {
	(PGM_P)tui_themes_s1,
	(PGM_P)tui_themes_s2,
	(PGM_P)tui_exit_menu
};

static void tui_load_theme(void) {
	memcpy_P(&s.t, &(tui_themes[s.theme_id]), sizeof(struct tui_theme));
	dp_set_fg_bg(s.t.c[current_color].fg, s.t.c[current_color].bg);
}

void tui_pick_theme(void) {
	uint8_t sel = tui_gen_listmenu(PSTR("Theme"), tui_theme_table, 3, s.theme_id);
	if (sel>=2) return;
	s.theme_id = sel;
	tui_load_theme();
}

void tui_init_themes(void) {
	tui_load_theme();
}
