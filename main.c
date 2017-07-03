#include "main.h"
#include <avr/power.h>
#include "uart.h"
#include "console.h"
#include "appdb.h"
#include "lib.h"
#include "timer.h"
#include "backlight.h"
#include "lcd.h"
#include "buttons.h"
#include "adc.h"
#include "relay.h"
#include "tui.h"
#include "pulse.h"
#include "saver.h"
#include "eecounter.h"

#ifdef ENABLE_UARTIF
#define RECVBUFLEN 64
const unsigned char prompt[] PROGMEM = "\x0D\x0AX32E5>";
unsigned char recvbuf[RECVBUFLEN];
unsigned char token_count;
unsigned char* tokenptrs[MAXTOKENS];

static void uartif_run(void)
{
	void(*func)(void);
	if (getline_mc(recvbuf,RECVBUFLEN)) {
		tokenize(recvbuf,tokenptrs, &token_count);
		if (token_count) {
			func = find_appdb(tokenptrs[0]);
			func();
		}
		sendstr_P((PGM_P)prompt);
	}
}
#else
static void uartif_run(void) { }
#endif


void mini_mainloop(void)
{
	timer_run();
	adc_run();
	backlight_run();
	relay_run();
	pulse_run();
	eec_run();
	uartif_run();
}

void main (void) __attribute__ ((noreturn));

static void xmega_clocks(void)
{
	OSC.CTRL = OSC_RC32MEN_bm; //enable 32MHz oscillator
	while(!(OSC.STATUS & OSC_RC32MRDY_bm));	//wait for stability
	CCP = CCP_IOREG_gc; //secured access
	CLK.CTRL = 0x01; //choose this osc source as clk
}

/* VPORT0 = A, VPORT1 = C, VPORT2 = D, VPORT3 = R */
static void xmega_pins(void)
{
#if 0
	PORTCFG_MPCMASK = 0x03;
	PORTR_PIN0CTRL = PORT_OPC_PULLUP_gc;



	VPORT1_OUT = 0x10;
	VPORT1_DIR = 0xFF;

#endif

	PORTCFG_MPCMASK = 0x97;
	PORTA_PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;

	PORTCFG_MPCMASK = 0xFC;
	PORTD_PIN0CTRL = PORT_OPC_PULLUP_gc;
	VPORT2_OUT = 0x08;
	VPORT2_DIR = 0x0B;

}

void main(void)
{
	cli();
	xmega_clocks();
	xmega_pins();
	timer_init();
	uart_init();
	lcd_init();
	backlight_init();
	buttons_init();
	adc_init();
	relay_init();
	pulse_init();
	eec_init();
	tui_init();
	saver_load_settings(); // we load, but dont report errors
	PMIC_CTRL = 0x87;
	sei();
#ifdef ENABLE_UARTIF
	sendstr_P((PGM_P)prompt); // initial prompt
#endif
	for(;;) {
		mini_mainloop();
		tui_run();
	}
}


