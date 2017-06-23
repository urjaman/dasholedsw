#include "main.h"
#include "tui.h"
#include "tui-lib.h"
#include "tui-mod.h"
#include "adc.h"
#include "relay.h"
#include "timer.h"
#include "lib.h"
#include "lcd.h"

int null_ps(uint8_t prv);
int null_ps(uint8_t prv)
{
	(void)prv;
	return 0;
}

static void tui_modfinish(uint8_t*t, uint8_t ml, uint8_t x, uint8_t w)
{
	t[ml] = 0;
	uint8_t mw = lcd_strwidth(t);
	if (mw > w) mw = w; // this is a problem, but i'll leave that for the mods to avoid,
	//  this just fixes math locally here :P
	uint8_t cl = w - mw;
	if ((x+w) >= LCDWIDTH) { // right align
		lcd_clear_dw(cl);
		lcd_puts_dw(t);
	} else {
		lcd_puts_dw(t);
		lcd_clear_dw(cl);
	}
}

static uint8_t tui_ch_char(uint8_t ch)
{
	if (ch==ADC_CH_BACKLIGHT) return 'L';
	if (ch==ADC_CH_TEMP) return 'T';
	if (ch==ADC_CH_FUEL) return 'F';
	if (ch==ADC_CH_R10V) return 'R';
	if (ch==ADC_CH_MB) return 'M';
	return '?';
}

static const unsigned char tui_adcch_s0[] PROGMEM = "BACKLIGHT";
static const unsigned char tui_adcch_s1[] PROGMEM = "TEMP";
static const unsigned char tui_adcch_s2[] PROGMEM = "FUEL";
static const unsigned char tui_adcch_s3[] PROGMEM = "10V REF";
static const unsigned char tui_adcch_s4[] PROGMEM = "INPUT VOLTS";
static PGM_P const tui_adcch_table[] PROGMEM = {
	(PGM_P)tui_adcch_s0,
	(PGM_P)tui_adcch_s1,
	(PGM_P)tui_adcch_s2,
	(PGM_P)tui_adcch_s3,
	(PGM_P)tui_adcch_s4,
	(PGM_P)tui_exit_menu,
};

int tui_adcsel(uint8_t par);
int tui_adcsel(uint8_t par)
{
	par = tui_gen_listmenu(PSTR("ADC CHANNEL"), tui_adcch_table, 6, par);
	if (par >= 5) return -1;
	return par;
}

static void tui_gbv_mod(uint8_t x, uint8_t c1, uint8_t c2, uint16_t v)
{
	uint8_t mb[9];
	mb[0] = c1;
	mb[1] = c2;
	adc_print_v(&(mb[2]),v);
	tui_modfinish(mb,8,x,8*LCD_CHARW);
}

TUI_MOD(tui_bv_mod,tui_adcsel,"VOLTS (AVG)",8*LCD_CHARW,1)
{
	tui_gbv_mod(x, tui_ch_char(par), ':', adc_read_ch(par));
}

TUI_MOD(tui_maxbv_mod,tui_adcsel,"VOLTS (MAX)",8*LCD_CHARW,1)
{
	tui_gbv_mod(x, tui_ch_char(par), '+', adc_read_maxv(par));
}

TUI_MOD(tui_minbv_mod,tui_adcsel,"VOLTS (MIN)",8*LCD_CHARW,1)
{
	tui_gbv_mod(x, tui_ch_char(par), '-', adc_read_minv(par));
}

TUI_MOD(tui_ripple_mod,tui_adcsel,"RIPPLE",8*LCD_CHARW,1)
{
	tui_gbv_mod(x, tui_ch_char(par), 'R', adc_read_maxv(par)-adc_read_minv(par));
}

TUI_MOD(tui_rlst_mod,null_ps,"RELAY STATE",5*LCD_CHARW,1)
{
	uint8_t mb[6];
	mb[0] = 'R';
	mb[1] = ':';
	switch (relay_get_mode()) {
		default:
		case RLY_MODE_OFF:
			mb[2] = '0';
			break;
		case RLY_MODE_ON:
			mb[2] = '1';
			break;
		case RLY_MODE_AUTO:
			mb[2] = 'A';
			break;
	}
	mb[3] = '>'; // "->"-like char
	switch (relay_get()) {
		default:
		case RLY_MODE_OFF:
			mb[4] = '0';
			break;
		case RLY_MODE_ON:
			mb[4] = '1';
			break;
	}
	tui_modfinish(mb,5,x,5*LCD_CHARW);
}

