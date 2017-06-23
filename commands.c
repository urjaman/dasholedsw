#include "main.h"
#include "uart.h"
#include "console.h"
#include "lib.h"
#include "appdb.h"
#include "lcd.h"
#include "backlight.h"
#include "timer.h"
#include "buttons.h"
#include "adc.h"
#include "relay.h"

#ifdef ENABLE_UARTIF

static void sendcrlf(void)
{
	sendstr_P(PSTR("\r\n"));
}

void echo_cmd(void)
{
	uint8_t i;
	for (i=1; i<token_count; i++) {
		sendstr(tokenptrs[i]);
		SEND(' ');
	}
}

void lcdinit_cmd(void)
{
	lcd_init();
}

void lcdsay_cmd(void)
{
	uint8_t i,n=0;
	lcd_clear();
	for (i=1; i<token_count; i++) {
		n += strlen((char*)tokenptrs[i])+1;
		if (n>32) break;
		if ((n > 16)&&(n<=32)) lcd_gotoxy(n&0xF,1);
		lcd_puts(tokenptrs[i]);
		lcd_putchar(' ');
	}
}

void blset_cmd(void)
{
	if (token_count >= 2) {
		uint8_t brightness= astr2luint(tokenptrs[1]);
		backlight_activate();
		backlight_set(brightness);
		if (token_count >= 3) {
			uint8_t to = astr2luint(tokenptrs[2]);
			backlight_set_to(to);
		}
	}
}

void btns_cmd(void)
{
	uint8_t v = buttons_get();
	sendstr_P(PSTR("BUTTON_"));
	switch (v) {
		default:
			sendstr_P(PSTR("UNKNOWN"));
			break;
		case BUTTON_S1:
			sendstr_P(PSTR("S1"));
			break;
		case BUTTON_S2:
			sendstr_P(PSTR("S2"));
			break;
		case BUTTON_NONE:
			sendstr_P(PSTR("NONE"));
			break;
		case BUTTON_BOTH:
			sendstr_P(PSTR("BOTH"));
			break;
	}
}

void adc_cmd(void)
{
	unsigned char buf[7];
	adc_print_v(buf,adc_read_mb());
	sendstr(buf);
	sendcrlf();
	adc_print_v(buf,adc_read_sb());
	sendstr(buf);
}

void relay_cmd(void)
{
	if (token_count >= 2) {
		uint8_t mode= astr2luint(tokenptrs[1]);
		relay_set(mode);
		if (token_count >= 3) {
			uint16_t v = astr2luint(tokenptrs[2]);
			relay_set_autovoltage(v);
		}
	}
}

unsigned long int calc_opdo(uint32_t val1, uint32_t val2, unsigned char *op)
{
	switch (*op) {
		case '+':
			val1 += val2;
			break;
		case '-':
			val1 -= val2;
			break;
		case '*':
			val1 *= val2;
			break;
		case '/':
			val1 /= val2;
			break;
		case '%':
			val1 %= val2;
			break;
		case '&':
			val1 &= val2;
			break;
		case '|':
			val1 |= val2;
			break;
	}
	return val1;
}

void luint2outdual(uint32_t val)
{
	unsigned char buf[11];
	luint2str(buf,val);
	sendstr(buf);
	sendstr_P(PSTR(" ("));
	luint2xstr(buf,val);
	sendstr(buf);
	sendstr_P(PSTR("h) "));
}

unsigned long int closureparser(unsigned char firsttok, unsigned char*ptr)
{
	unsigned char *op=NULL;
	unsigned char i,n;
	unsigned long int val1, val2;
	if (token_count <= firsttok) return 0;
	val1 = astr2luint(tokenptrs[firsttok]);
	sendstr_P(PSTR("{ "));
	luint2outdual(val1);
	n=0;
	for(i=firsttok+1; i<token_count; i++) {
		if (n&1) {
			sendstr(op);
			SEND(' ');
			if (*(tokenptrs[i]) == '(') {
				val2 = closureparser((i+1),&i);
			} else {
				val2 = astr2luint(tokenptrs[i]);
				luint2outdual(val2);
			}
			val1 = calc_opdo(val1,val2,op);
		} else {
			if (*(tokenptrs[i]) == ')') {
				sendstr_P(PSTR("} "));
				*ptr = i+1;
				return val1;
			}
			op = tokenptrs[i];
		}
		n++;
	}
	return val1;
}

void calc_cmd(void)
{
	unsigned char *op=NULL;
	unsigned char i,n;
	unsigned long int val1;
	unsigned long int val2;
	if (token_count < 2) return;

	if (*(tokenptrs[1]) == '(') {
		val1 = closureparser(2,&i);
	} else {
		val1 = astr2luint(tokenptrs[1]);
		luint2outdual(val1);
		i=2;
	}
	n=0;
	for (; i<token_count; i++) {
		if (n&1) {
			sendstr(op);
			SEND(' ');
			if (*(tokenptrs[i]) == '(') {
				val2 = closureparser((i+1),&i);
			} else {
				val2 = astr2luint(tokenptrs[i]);
				luint2outdual(val2);
			}
			val1 = calc_opdo(val1,val2,op);
		} else {
			op = tokenptrs[i];
		}
		n++;
	}
	sendstr_P(PSTR("= "));
	luint2outdual(val1);
}


void timer_cmd(void)
{
	luint2outdual(timer_get());
}

void help_cmd(void)
{
	uint8_t i;
	const struct command_t * ctptr;
	PGM_P name;
	for(i=0;; i++) {
		ctptr = &(appdb[i]);
		name = (PGM_P)pgm_read_word(&(ctptr->name));
		if (!name) break;
		sendstr_P(name);
		SEND(' ');
	}
}

#endif
