#include "main.h"
#include "console.h"
#include "appdb.h"

#ifdef ENABLE_UARTIF

const unsigned char echostr[] PROGMEM = "ECHO";
const unsigned char calcstr[] PROGMEM = "CALC";
const unsigned char lcdinitstr[] PROGMEM = "LCDINIT";
const unsigned char lcdsaystr[] PROGMEM = "LCDSAY";
const unsigned char blsetstr[] PROGMEM = "BLSET";
const unsigned char timerstr[] PROGMEM = "TIMER";
const unsigned char btnsstr[] PROGMEM = "BTNS";
const unsigned char adcstr[] PROGMEM = "ADC";
const unsigned char relaystr[] PROGMEM = "RELAY";
const unsigned char helpstr[] PROGMEM = "?";

const struct command_t appdb[] PROGMEM = {
	{(PGM_P)echostr, &(echo_cmd)},
	{(PGM_P)calcstr, &(calc_cmd)},
	{(PGM_P)lcdinitstr, &(lcdinit_cmd)},
	{(PGM_P)lcdsaystr, &(lcdsay_cmd)},
	{(PGM_P)blsetstr, &(blset_cmd)},
	{(PGM_P)timerstr, &(timer_cmd)},
	{(PGM_P)btnsstr, &(btns_cmd)},
	{(PGM_P)adcstr, &(adc_cmd)},
	{(PGM_P)relaystr, &(relay_cmd)},
	{(PGM_P)helpstr, &(help_cmd)},
	{NULL,NULL}
};

void invalid_command(void)
{
	sendstr(tokenptrs[0]);
	sendstr_P(PSTR(": not found"));
}

void *find_appdb(unsigned char* cmd)
{
	uint8_t i;
	const struct command_t * ctptr;
	PGM_P name;
	void* fp;
	for(i=0;; i++) {
		ctptr = &(appdb[i]);
		name = (PGM_P)pgm_read_word(&(ctptr->name));
		fp = (void*)pgm_read_word(&(ctptr->function));
		if (!name) break;
		if (strcmp_P((char*)cmd,name) == 0) {
			return fp;
		}
	}
	return &(invalid_command);
}

#endif
