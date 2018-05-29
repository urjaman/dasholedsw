#pragma once

enum tui_theme_col {
	TTC_LN1 = 0,
	TTC_LN2,
	TTC_HL1,
	TTC_HL2,
	TTC_HEAD1,
	TTC_HEAD2,
	TTC_COLOR_COUNT
};

void tui_set_color(enum tui_theme_col n);
void tui_unset_color(void);
void tui_default_color(void);


void tui_pick_theme(void);
void tui_init_themes(void);

uint8_t theme_save(void**ptr);
void theme_load(void *b, uint8_t sz);
