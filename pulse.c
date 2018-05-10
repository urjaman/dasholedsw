#include "main.h"
#include "timer.h"
#include "pulse.h"

/* Capture channels:
 * PC7 = RPM
 * PC6 = ROADSPD
 * PC5 = ECULAMP (mhh...)
 * PC4 = DAUX (hmm...)
 */

#define PULSE_CHANNELS 4

struct p_int {
	uint16_t edge_counter[PULSE_CHANNELS];
	uint16_t base[PULSE_CHANNELS];
	uint8_t  sample_cnt[PULSE_CHANNELS];
	uint16_t samp[PULSE_CHANNELS];
};
static volatile struct p_int pulse;

#define HZ_SHIFT PULSE_HZ_SHIFT
#define TIMER_HZ PULSE_TIMER_HZ

/* 1 second. */
#define ZERO_TIMEOUT TIMER_HZ

/* 5 Hz max output */
#define SAMPLETIME (TIMER_HZ/5)

static uint24_t pulse_hz[PULSE_CHANNELS];

static uint16_t last_out[PULSE_CHANNELS];

#define nop() asm volatile("nop")

static uint16_t read_cli_16b(volatile uint16_t * v) {
	volatile uint16_t r;
	asm volatile (
		"cli"		"\n\t"
		"ld %A0, %a1"	"\n\t"
		"sei"		"\n\t"
		"ldd %B0, %a1+1" "\n\t"
	: "=&r" (r)
	: "b" (v)
	);
	return r;
}

static uint16_t read_tcc(void) {
	return read_cli_16b(&TCC4_CNT);
}

static void process_hz(uint8_t ch) {
	cli();
	uint8_t c = pulse.sample_cnt[ch];
	uint16_t s,b;
	s = pulse.samp[ch];
	b = pulse.base[ch];
	pulse.base[ch] = s;
	pulse.sample_cnt[ch] = 1;
	sei();
	uint16_t td = (s - b);
	if (!td) { // what?
		return;
	}
	pulse_hz[ch] = ( ( (((uint32_t)TIMER_HZ) << HZ_SHIFT) * (c-1) ) + (td/2) ) / td;
	last_out[ch] = read_tcc(); // used to remember to zero idle channels
}

static void tcc4_cc(uint8_t ch, uint16_t s) {
	pulse.samp[ch] = s;
	uint8_t sc = pulse.sample_cnt[ch]++;
	if (!sc) {
		pulse.base[ch] = pulse.samp[ch];
	}
	if (sc >= 128) timer_set_waiting(); /* nice bit on which to trip early processing... */
	pulse.edge_counter[ch]++;
}

#define FLATTEN __attribute__((flatten))

ISR(TCC4_CCA_vect, FLATTEN ) {
	tcc4_cc(0, TCC4_CCA);
}

ISR(TCC4_CCB_vect, FLATTEN ) {
	tcc4_cc(1, TCC4_CCB);
}

ISR(TCC4_CCC_vect, FLATTEN ) {
	tcc4_cc(2, TCC4_CCC);
}

ISR(TCC4_CCD_vect, FLATTEN ) {
	tcc4_cc(3, TCC4_CCD);
}


//#define F_DEBUG

#ifdef F_DEBUG
ISR(TCD5_OVF_vect) {
	VPORT1_OUT ^= _BV(6);
	TCD5_INTFLAGS = 0x01;
}

static void debug_setup(void) {
	TCD5_PER = 50000 - 1;
	TCD5_CTRLA = TC45_CLKSEL_DIV2_gc;
	TCD5_INTCTRLA = 3;
	VPORT1_DIR |= _BV(6);
}
#endif

void pulse_init(void) {
	TCC4_PER = 0xFFFF;

	TCC4_CTRLD = TC45_EVSEL_CH4_gc; // Events 4-7
	TCC4_CTRLE = 0b10101010;

	/* Pull 8192Hz from the RTC compare updates. */
	EVSYS_CH3MUX = EVSYS_CHMUX_RTC_CMP_gc;

	EVSYS_CH4MUX = EVSYS_CHMUX_PORTC_PIN4_gc; /* Port C pin 4 and so on. */
	EVSYS_CH5MUX = EVSYS_CHMUX_PORTC_PIN5_gc;
	EVSYS_CH6MUX = EVSYS_CHMUX_PORTC_PIN6_gc;
	EVSYS_CH7MUX = EVSYS_CHMUX_PORTC_PIN7_gc;

	TCC4_CTRLA = TC45_CLKSEL_EVCH3_gc;
	TCC4_INTCTRLB = 0xFF;

	/* Default RPM to falling trigger. */
	pulse_set_ch_mode(PCH_RPM, PM_FALLING);

#ifdef F_DEBUG
	debug_setup();
#endif

}

void pulse_run(void) {
	for (uint8_t i=0;i<PULSE_CHANNELS;i++) {
		uint8_t sc = pulse.sample_cnt[i];
		if (sc > 64) { // fastpath
			process_hz(i);
			continue;
		}
		if (sc < 2) { // checking for zero...
			uint16_t now = read_tcc();
			if  ((now - last_out[i]) >= ZERO_TIMEOUT) {
				cli();
				pulse.sample_cnt[i] = 0;
				pulse_hz[i] = 0;
				last_out[i] = now;
				sei();
			}
			continue;
		}
		// checking for nice sample rate...
		cli();
		uint16_t td = (TCC4_CNT - pulse.base[i]);
		if (td >= SAMPLETIME) {
			// ok, eat it
			process_hz(i);
		}
		sei(); // anyways...
	}
}


uint8_t pulse_state(const enum pulse_ch ch, uint16_t *edges) {
	const uint8_t ich = ch-4;
	uint8_t r;
	if (edges) {
		uint16_t e1, e2;
		do {
			e1 = read_cli_16b(&pulse.edge_counter[ich]);
			r = VPORT1_IN;
			e2 = read_cli_16b(&pulse.edge_counter[ich]);
		} while (e1 != e2);
		*edges = e1;
	} else {
		/* Since they're not interested in the edgectr position relative to state... */
		r = VPORT1_IN;
	}
	return !! (r & _BV(ch));
}


uint16_t pulse_edge_ctr(const enum pulse_ch ch, uint16_t *time) {
	const uint8_t ich = ch-4;
	if (time) {
		uint16_t e1, e2, t;
		do {
			e1 = read_cli_16b(&pulse.edge_counter[ich]);
			t = read_cli_16b(&pulse.samp[ich]);
			e2 = read_cli_16b(&pulse.edge_counter[ich]);
		} while (e1 != e2);
		*time = t;
		return e1;
	} else {
		return read_cli_16b(&pulse.edge_counter[ich]);
	}
}

uint24_t pulse_get_hz(const enum pulse_ch ch, uint16_t *age) {
	const uint8_t ich = ch-4;
	if (age) {
		*age = read_tcc() - last_out[ich];
	}
	return pulse_hz[ich];
}

void pulse_set_ch_mode(const enum pulse_ch ch, enum pulse_mode pm) {
	switch (ch) {
		default: break;
		case PCH_DAUX:    PORTC_PIN4CTRL = pm; break;
		case PCH_ECULAMP: PORTC_PIN5CTRL = pm; break;
		case PCH_ROADSPD: PORTC_PIN6CTRL = pm; break;
		case PCH_RPM:     PORTC_PIN7CTRL = pm; break;
	}
}
