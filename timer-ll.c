#include "main.h"
#include "timer.h"

#define HAS_EX32K


extern uint8_t timer_waiting;
volatile uint8_t timer_run_todo=0;
#define S(s) str(s)
#define str(s) #s
#define VL (((F_CPU+512)/1024)&0xFF)
#define VH (((F_CPU+512)/1024)/256)


ISR(RTC_OVF_vect, ISR_NAKED) {
	asm volatile (
	"push r1\n\t"
	"in r1, __SREG__\n\t"
	"push r24\n\t"
	"lds r24, timer_run_todo\n\t"
	"inc r24\n\t"
	"sts timer_run_todo, r24\n\t"
	"pop r24\n\t"
	"out __SREG__, r1\n\t"
	"pop r1\n\t"
	"reti\n\t"
	::);
}

ISR(RTC_COMP_vect, ISR_NAKED) {
	asm volatile (
	"push r1\n\t"
	"in r1, __SREG__\n\t"
	"push r24\n\t"
	"push r25\n\t"
	"lds r24, %0\n\t" // COMPL
	"lds r25, %0+1\n\t" // COMPH
	"adiw r24, 4\n\t"
	"andi r25, 0x7F\n\t"
	"sts %0, r24\n\t" // COMPL
	"sts %0+1, r25\n\t" // COMPH
	"pop r25\n\t"
	"pop r24\n\t"
	"out __SREG__, r1\n\t"
	"pop r1\n\t"
	"reti\n\t"
	:: "m" (RTC_COMP)
	);
}


uint16_t timer_get_subsectimer(void) {
	uint16_t rv;
	asm (
	"cli\n\t"
	"lds %A0, %1\n\t" // CNTL
	"sei\n\t"
	"lds %B0, %1+1\n\t" // CNTH
	: "=r" (rv)
	: "m" (RTC_CNT) );
	return rv;
}

static void run_dfll(uint8_t src)
{
	const uint16_t comp = 31250;
	OSC_DFLLCTRL = src;
	DFLLRC32M_COMP2 = comp >> 8;
	DFLLRC32M_COMP1 = comp & 0xFF;
	DFLLRC32M_CTRL = 1;
}

void timer_init(void) {
	timer_run_todo=0;
	timer_waiting=1;

	OSC_CTRL |= OSC_RC32KEN_bm; //enable internal 32kHz oscillator
#ifdef HAS_EX32K
	OSC_XOSCCTRL = OSC_XOSCSEL_32KHz_gc;
	OSC_CTRL |= OSC_XOSCEN_bm; //enable external 32kHz oscillator
#endif
	/* Pick the internal RC first, since i hear the external 32kHz has huge startup time */
	uint8_t ready = 0;
	do {
		uint8_t s = OSC_STATUS;
		if (s & OSC_RC32KRDY_bm) ready = CLK_RTCSRC_RCOSC32_gc;
	} while (!ready);
	CLK_RTCCTRL = ready | CLK_RTCEN_bm;

	RTC_INTFLAGS = 3;
	RTC_INTCTRL = 0xF;
	RTC_PER = 32768 - 1;
	RTC_CNT = 0;
	RTC_COMP = 4;
	RTC_CTRL = 1; // DIV1

#ifndef HAS_EX32K
	run_dfll(OSC_RC32MCREF_RC32K_gc);
#endif

}

uint8_t timer_getdec_todo(void) {
	uint8_t rv;
	cli();
	rv = timer_run_todo;
	if (rv) timer_run_todo = rv-1;
	sei();
#ifdef HAS_EX32K
	if (rv) {
		/* If we're not already running on the TOSC, check if we can ... */
		if (CLK_RTCCTRL != (CLK_RTCSRC_TOSC32_gc | CLK_RTCEN_bm)) {
			if (OSC_STATUS & OSC_XOSCRDY_bm) { // and the 32kHz ext crystal is ready
				CLK_RTCCTRL = CLK_RTCSRC_TOSC32_gc | CLK_RTCEN_bm;
				/* OK, switch off the internal 32kHz RC, thanks for your services :) */
				OSC_CTRL &= ~OSC_RC32KEN_bm;
				/* Switch on the DFLL to calibrate the 32Mhz RC. */
				run_dfll(OSC_RC32MCREF_XOSC32K_gc);
			}
		}
	}
#endif
	return rv;
}

uint24_t timer_get_linear_ss_time(void) {
	uint8_t todo;
	uint16_t sstimer;
	cli();
	todo = timer_run_todo;
	sstimer = timer_get_subsectimer(); // sei() happens here
	return (uint24_t)todo*SSTC + sstimer;
}

uint16_t timer_get_lin_ss_u16(void) {
	/* This is the above, but inline optimized and for u16 return. */
	uint8_t todo;
	uint16_t sstimer;
	cli();
	todo = timer_run_todo;
	asm (
	"lds %A0, %1\n\t" // CNTL
	"sei\n\t"
	"lds %B0, %1+1\n\t" // CNTH
	: "=r" (sstimer)
	: "m" (RTC_CNT) );
	if (likely(!todo)) return sstimer;
	if (likely(todo==1)) {
		return SSTC+sstimer;
	} else {
		return 0xFFFF;
	}
}

uint8_t timer_get_todo(void) {
	uint8_t rv;
	// cli is not needed here since timer_run_todo is uint8_t
	// and it is not modified here
	rv = timer_run_todo;
	return rv;
}
