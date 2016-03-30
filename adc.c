#include "main.h"
#include "adc.h"
#include "timer.h"

// Thus MB_SCALE = 3489,28 / 3515 * 65536 => 65056,45919
#define ADC_MB_SCALE 65536
// SB_SCALE = 3489,28 / 3507 * 65536 => 65204,86284
#define ADC_SB_SCALE 65536

// Calib is 65536+diff, thus saved is diff = calib - 65536.
int16_t adc_calibration_diff[ADC_MUX_CNT] = { ADC_MB_SCALE-65536, ADC_SB_SCALE-65536 };

uint16_t adc_raw_values[ADC_MUX_CNT];
uint16_t adc_raw_minv[ADC_MUX_CNT];
uint16_t adc_raw_maxv[ADC_MUX_CNT];
static uint16_t adc_supersample;
static uint8_t adc_ss_cnt;

static uint16_t adc_values[ADC_MUX_CNT];
uint16_t adc_minv[ADC_MUX_CNT];
uint16_t adc_maxv[ADC_MUX_CNT];
static int16_t adc_bat_diff;
uint16_t adc_avg_cnt=0;

static void adc_set_mux(uint8_t c) {
	ADCA_CH0_MUXCTRL = (4+c) << 3;
}

static uint16_t adc_single_read(uint8_t c) {
	uint16_t rv = 0;
	adc_set_mux(c);
	ADCA_CTRLA = 0x05;
	while (!(ADCA_CH0_INTFLAGS & 1));
	ADCA_CH0_INTFLAGS = 1;
	rv = ADCA_CH0_RES;
	if (rv >= 205) rv -= 205;
	else rv = 0;
	return rv;
}

uint16_t adc_read_mb(void) {
	return adc_values[0];
}

uint16_t adc_read_sb(void) {
	return adc_values[1];
}

int16_t adc_read_diff(void) {
	return adc_bat_diff;
}

uint16_t adc_read_minv(uint8_t ch) {
	return adc_minv[ch];
}

uint16_t adc_read_maxv(uint8_t ch) {
	return adc_maxv[ch];
}

uint16_t adc_to_dV(uint16_t v) {
	v = ((((uint32_t)v)*390625UL)+500000UL) / 1000000UL;
	return v;
}

uint16_t adc_from_dV(uint16_t v) {
	v = ((((uint32_t)v)*256UL)+50UL) / 100UL;
	return v;
}

void adc_print_v(unsigned char* buf, uint16_t v) {
	v = adc_to_dV(v);
	adc_print_dV(buf,v);
}

void adc_print_dV(unsigned char* buf, uint16_t v) {
	buf[0] = (v/1000)|0x30; v = v%1000;
	buf[1] = (v/100 )|0x30; v = v%100;
	buf[2] = '.';
	buf[3] = (v/10  )|0x30; v = v%10;
	buf[4] =  v      |0x30;
	buf[5] = 'V';
	buf[6] = 0;
	if (buf[0] == '0') buf[0] = ' ';
}

static void adc_values_scale(void) {
	uint32_t adc_scale_mb = ( ((int32_t)65536L) + adc_calibration_diff[0] );
	uint32_t adc_scale_sb = ( ((int32_t)65536L) + adc_calibration_diff[1] );
	adc_values[0] = (((uint32_t)adc_raw_values[0])*adc_scale_mb)/65536;
	adc_values[1] = (((uint32_t)adc_raw_values[1])*adc_scale_sb)/65536;
	adc_minv[0] = (((uint32_t)adc_raw_minv[0])*adc_scale_mb)/65536;
	adc_minv[1] = (((uint32_t)adc_raw_minv[1])*adc_scale_sb)/65536;
	adc_maxv[0] = (((uint32_t)adc_raw_maxv[0])*adc_scale_mb)/65536;
	adc_maxv[1] = (((uint32_t)adc_raw_maxv[1])*adc_scale_sb)/65536;
	adc_bat_diff = adc_values[0] - adc_values[1];
}

void adc_ll_init(void) {
	for(uint8_t i=0;i<ADC_MUX_CNT;i++) {
		adc_raw_values[i] = 0;
		adc_raw_maxv[i] = 0;
		adc_raw_minv[i] = 0xFFFF;
		adc_supersample = 0;
		adc_ss_cnt = 0;
	}
}

void adc_init(void) {
	uint8_t i;
	ADCA_REFCTRL = ADC_BANDGAP_bm;
	ADCA_PRESCALER = ADC_PRESCALER_DIV256_gc;
	ADCA_SAMPCTRL = 2;
	for (i=0;i<ADC_MUX_CNT;i++) {
		adc_raw_values[i] = adc_single_read(i);
		adc_raw_minv[i] = adc_raw_values[i];
		adc_raw_maxv[i] = adc_raw_values[i];
	}
	adc_values_scale();
	adc_ll_init();
	adc_set_mux(0);
#if 0
	ADCSRA |= _BV(ADSC) | _BV(ADIE);
#endif
	ADCA_CTRLB = 0x08;
	ADCA_CTRLA = 0x01;

}

void adc_run(void) {
#if 0
	if (timer_get_1hzp()) {
		adc_avg_cnt = adc_run_ll(adc_raw_values,adc_raw_minv,adc_raw_maxv);
		if (adc_avg_cnt) adc_values_scale();
	}
#endif
}
