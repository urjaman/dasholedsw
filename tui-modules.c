#include "main.h"
#include "tui.h"
#include "tui-lib.h"
#include "tui-mod.h"
#include "adc.h"
#include "relay.h"
#include "timer.h"
#include "pulse.h"
#include "lib.h"
#include "lcd.h"
#include "speedo.h"
#include "odo.h"

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

TUI_MOD(tui_bv_mod,tui_adcsel,"Volts (AVG)",8*LCD_CHARW,1)
{
	tui_gbv_mod(x, tui_ch_char(par), ':', adc_read_ch(par));
}

TUI_MOD(tui_maxbv_mod,tui_adcsel,"Volts (MAX)",8*LCD_CHARW,1)
{
	tui_gbv_mod(x, tui_ch_char(par), '+', adc_read_maxv(par));
}

TUI_MOD(tui_minbv_mod,tui_adcsel,"Volts (MIN)",8*LCD_CHARW,1)
{
	tui_gbv_mod(x, tui_ch_char(par), '-', adc_read_minv(par));
}

TUI_MOD(tui_ripple_mod,tui_adcsel,"Ripple",8*LCD_CHARW,1)
{
	tui_gbv_mod(x, tui_ch_char(par), 'R', adc_read_maxv(par)-adc_read_minv(par));
}


TUI_MOD(tui_rlst_mod,null_ps,"Relay Info",5*LCD_CHARW,1)
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


static const unsigned char tui_pch_s0[] PROGMEM = "DAUX";
static const unsigned char tui_pch_s1[] PROGMEM = "ECULAMP";
static const unsigned char tui_pch_s2[] PROGMEM = "ROADSPD";
static const unsigned char tui_pch_s3[] PROGMEM = "RPM";
static PGM_P const tui_pch_table[] PROGMEM = {
	(PGM_P)tui_pch_s0,
	(PGM_P)tui_pch_s1,
	(PGM_P)tui_pch_s2,
	(PGM_P)tui_pch_s3,
	(PGM_P)tui_exit_menu,
};

int tui_pulsesel(uint8_t par);
int tui_pulsesel(uint8_t par)
{
	par = tui_gen_listmenu(PSTR("Channel"), tui_pch_table, 5, par);
	if (par >= 4) return -1;
	return par;
}


TUI_MOD(tui_pstate_mod,tui_pulsesel,"Edge Ctr",8*LCD_CHARW,1)
{
	uint8_t buf[9];
	uint16_t e;
	uint8_t s = pulse_state(par+4, &e);
	buf[0] = '4'+par;
	buf[1] = ':';
	buf[2] = '0'+s;
	buf[3] = '|';
	uchar2xstr(buf+4, e>>8);
	uchar2xstr(buf+6, e&0xFF);
	tui_modfinish(buf,8,x,8*LCD_CHARW);
}

TUI_MOD(tui_pulsehz_mod,tui_pulsesel,"Freq",9*LCD_CHARW,1)
{
	uint8_t buf[10];
	uint24_t hz = pulse_get_hz(par+4, NULL);
	buf[0] = '4'+par;
	buf[1] = ':';
	uint24_t hzi = hz >> PULSE_HZ_SHIFT;
	if (hzi>99999) hzi = 99999;
	uint8_t n = luint2str(buf+2, hzi) + 2;
	buf[n] = '.';
	buf[n+1] = '0' + ( (hz & (_BV(PULSE_HZ_SHIFT)-1) * 10) >> PULSE_HZ_SHIFT );
	buf[n+2] = 0;
 	tui_modfinish(buf,n+2,x,9*LCD_CHARW);
}


TUI_MOD(tui_speedo_mod,null_ps,"Speedo", 6*LCD_CHARW,2)
{
	const uint8_t lcdw = 6*LCD_CHARW;
	uint8_t buf[8];
	uint16_t kmh = (speedo_get_kmh10()+5)/10;
	if (kmh>999) kmh=999;
	uint2str(buf,kmh);
	uint8_t off = lcdw - lcd_strwidth_big(buf);
	lcd_clear_big_dw(off);
	lcd_puts_big(buf);
}

TUI_MOD(tui_big_speedo_mod,null_ps,"Big Speedo", 12*LCD_CHARW, 4)
{
	const uint8_t lcdw = 12*LCD_CHARW;
	uint8_t buf[8];
	uint16_t kmh = (speedo_get_kmh10()+5)/10;
	if (kmh>999) kmh=999;
	uint2str(buf,kmh);
	uint8_t off = lcdw - lcd_strwidth_dbig(buf);
	lcd_clear_dbig_dw(off);
	lcd_puts_dbig(buf);
}

TUI_MOD(tui_odometer_mod,null_ps,"Odometer", 12*LCD_CHARW, 4)
{
	uint8_t buf[8];
	uint32_t m = odo_get(NULL);
	uint24_t km = m/1000;
	m = m%1000;
	if (km>999999) km %= 1000000;
	luint2str_zp(buf, km, 6);
	tui_set_color(TTC_HL1);
	lcd_puts_big(buf);
	tui_unset_color();

	lcd_gotoxy_dw(x,y+2);
	luint2str_zp(buf, m, 3);
	lcd_puts(buf);

	PGM_P kms = PSTR("KM");
	uint8_t skip = 1*LCD_CHARW;
	lcd_clear_big_dw(skip);
	tui_set_color(TTC_HEAD1);
	lcd_puts_big_P(kms);
	tui_unset_color();
	lcd_clear_big_eol();

	lcd_gotoxy_dw(x,y+3);
	lcd_clear_dw(4*LCD_CHARW);
}
