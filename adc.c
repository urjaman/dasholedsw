#include "main.h"
#include "adc.h"
#include "timer.h"



// Thus MB_SCALE = 3489,28 / 3515 * 65536 => 65056,45919
#define ADC_MB_SCALE 65536
// SB_SCALE = 3489,28 / 3507 * 65536 => 65204,86284
#define ADC_SB_SCALE 65536

// Calib is 65536+diff, thus saved is diff = calib - 65536.
int16_t adc_calibration_diff[ADC_MUX_CNT] = { ADC_MB_SCALE-65536, ADC_SB_SCALE-65536 };

static uint32_t adc_raw_values[ADC_MUX_CNT];
static uint16_t adc_raw_minv[ADC_MUX_CNT];
static uint16_t adc_raw_maxv[ADC_MUX_CNT];
static uint16_t adc_raw_v_cnt;

static uint16_t adc_supersample[ADC_MUX_CNT];
static uint8_t adc_ss_cnt;
static uint8_t adc_ss_ch;

static uint16_t adc_values[ADC_MUX_CNT];
uint16_t adc_minv[ADC_MUX_CNT];
uint16_t adc_maxv[ADC_MUX_CNT];
static int16_t adc_bat_diff;
uint16_t adc_avg_cnt=0;

#define ADC_ZERO_LEVEL 205

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
	if (rv >= ADC_ZERO_LEVEL) rv -= ADC_ZERO_LEVEL;
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

static void adc_values_scale(uint32_t *raw, uint16_t *min, uint16_t *max) {
	uint32_t adc_scale_mb = ( ((int32_t)65536L) + adc_calibration_diff[0] );
	uint32_t adc_scale_sb = ( ((int32_t)65536L) + adc_calibration_diff[1] );
	adc_values[0] = (((uint32_t)raw[0])*adc_scale_mb)/65536;
	adc_values[1] = (((uint32_t)raw[1])*adc_scale_sb)/65536;
	adc_minv[0] = (((uint32_t)min[0])*adc_scale_mb)/65536;
	adc_minv[1] = (((uint32_t)min[1])*adc_scale_sb)/65536;
	adc_maxv[0] = (((uint32_t)max[0])*adc_scale_mb)/65536;
	adc_maxv[1] = (((uint32_t)max[1])*adc_scale_sb)/65536;
	adc_bat_diff = adc_values[0] - adc_values[1];
}

static void adc_ll_init(void) {
	for(uint8_t i=0;i<ADC_MUX_CNT;i++) {
		adc_raw_values[i] = 0;
		adc_raw_maxv[i] = 0;
		adc_raw_minv[i] = 0xFFFF;
		adc_supersample[i] = 0;
	}
	adc_raw_v_cnt = 0;
	adc_ss_cnt = 0;
	adc_ss_ch = 0;
}

static void adc_ll_run(void) {
	uint32_t raw[ADC_MUX_CNT];
	uint16_t min[ADC_MUX_CNT];
	uint16_t max[ADC_MUX_CNT];
	uint16_t cnt;
	cli();
	if ((cnt=adc_raw_v_cnt)) {
		for (uint8_t i=0;i<ADC_MUX_CNT;i++) {
			raw[i] = adc_raw_values[i];
			adc_raw_values[i] = 0;
			min[i] = adc_raw_minv[i];
			adc_raw_minv[i] = 0xFFFF;
			max[i] = adc_raw_maxv[i];
			adc_raw_maxv[i] = 0;
		}
		adc_raw_v_cnt = 0;
	}
	sei();
	if (cnt) {
		for (uint8_t i=0;i<ADC_MUX_CNT;i++) {
			raw[i] = (raw[i] + (cnt/2)) / cnt;
		}
		adc_values_scale(raw, min, max);
		adc_avg_cnt = cnt;
	}
}


ISR(ADCA_CH0_vect) {
	uint8_t this_ch = adc_ss_ch;
	if (++adc_ss_cnt >= 16) {
		adc_set_mux(adc_ss_ch ^= 1);
		adc_ss_cnt = 0;
	}
	ADCA_CTRLA = 0x05;
	uint16_t v = ADCA_CH0_RES;
	if (v >= ADC_ZERO_LEVEL) v -= ADC_ZERO_LEVEL;
	else v = 0;
	adc_supersample[this_ch] += v;
	if (adc_raw_minv[this_ch] > v) adc_raw_minv[this_ch] = v;
	if (adc_raw_maxv[this_ch] < v) adc_raw_maxv[this_ch] = v;
	if ((this_ch != adc_ss_ch)&&(adc_ss_ch==0)) {
		for (uint8_t i=0;i<ADC_MUX_CNT;i++) {
			adc_raw_values[i] += (adc_supersample[i]+8) >> 4;
			adc_supersample[i] = 0;
		}
		adc_raw_v_cnt++;
	}
}



static uint8_t ReadSignatureByte(volatile void* Address) {
	NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;
	uint8_t Result;
	__asm__ ("lpm %0, Z\n" : "=r" (Result) : "z" (Address));
	NVM_CMD = NVM_CMD_NO_OPERATION_gc;
	return Result;
}

void adc_init(void) {
	uint8_t i;
	uint16_t calv = ReadSignatureByte(&PRODSIGNATURES_ADCACAL0);
	calv |= ReadSignatureByte(&PRODSIGNATURES_ADCACAL1) << 8;
	ADCA_CAL = calv;
	ADCA_REFCTRL = ADC_BANDGAP_bm;
	ADCA_PRESCALER = ADC_PRESCALER_DIV256_gc;
	ADCA_SAMPCTRL = 2;
	ADCA_CH0_CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;
	for (i=0;i<ADC_MUX_CNT;i++) {
		adc_raw_values[i] = adc_single_read(i);
		adc_raw_minv[i] = adc_raw_values[i];
		adc_raw_maxv[i] = adc_raw_values[i];
	}
	adc_values_scale(adc_raw_values, adc_raw_minv, adc_raw_maxv);
	adc_avg_cnt = 1;
	adc_ll_init();
	adc_set_mux(0);
	ADCA_CH0_INTFLAGS = 1;
	ADCA_CH0_INTCTRL = 1;
	ADCA_CTRLB = 0x00;
	ADCA_CTRLA = 0x05;
}

void adc_run(void) {
	if (timer_get_1hzp()) {
		adc_ll_run();
	}
}
