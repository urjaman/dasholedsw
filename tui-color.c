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
		{ rgb( 33,  33, 255), rgb(  0,   0,  15) }, // LN2
		{ rgb(128,   0, 255), rgb(  0,   0,   0) }, // HL1
		{ rgb(144,  33, 255), rgb(  8,   0,  15) }, // HL2
		{ rgb(  4, 255,  24), rgb(  0,   0,   0) }, // HEAD1
		{ rgb(255,  32,  32), rgb(  0,   0,   0) }  // HEAD2
	} },
	{ {	/* Balls (to the wall) Theme */
		{ rgb(226, 219,   0), rgb(  0,   0,   0) }, // LN1
		{ rgb(226, 180,   0), rgb( 10,   5,   0) }, // LN2
		{ rgb(255,  99,   2), rgb(  0,   0,   0) }, // HL1
		{ rgb(226, 100,   0), rgb( 10,   5,   0) }, // HL2
		{ rgb(255,  15,   0), rgb(  0,   0,   0) }, // HEAD1
		{ rgb(  0,  50,   0), rgb(  0,   0,   0) }  // HEAD2
	} }
};


const unsigned char tui_themes_s1[] PROGMEM = "Blueish";
const unsigned char tui_themes_s2[] PROGMEM = "Reddish";

PGM_P const tui_theme_table[] PROGMEM = {
	(PGM_P)tui_themes_s1,
	(PGM_P)tui_themes_s2,
	(PGM_P)tui_exit_menu
};

uint8_t theme_save(void**ptr) {
	*ptr = &s.theme_id;
	return 1;
}



static void tui_load_theme(void) {
	memcpy_P(&s.t, &(tui_themes[s.theme_id]), sizeof(struct tui_theme));
	dp_set_fg_bg(s.t.c[current_color].fg, s.t.c[current_color].bg);
}

void theme_load(void *b, uint8_t sz) {
	if (sz != 1) return; // sorry, no.
	memcpy(&s, b, sz);
	if (s.theme_id > 1) s.theme_id = 0;
	tui_load_theme();
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
