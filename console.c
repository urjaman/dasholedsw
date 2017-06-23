// CONSOLE SUBSYSTEM
#include "main.h"
#include "uart.h"
#include "console.h"

#define CR 0x0D
#define LF 0x0A
#define BS 0x08
#define DEL 0x7F
#define SPACE 0x20

#ifdef ENABLE_UARTIF
void sendstr_P(PGM_P str)
{
	unsigned char val;
	for(;;) {
		val = pgm_read_byte(str);
		if (val) SEND(val);
		else break;
		str++;
	}
}

// rv=1 == got a line, 0= call me again
unsigned char getline_mc(unsigned char* buf, uint8_t len)
{
	static uint8_t i=0;
	unsigned char v;
	if (i==0) memset(buf,0,len);
	for(; i<len; i++) {
		if (uart_isdata()) {
			v = RECEIVE();
			if (((v == BS)||(v == DEL))&&(i)) { SEND(BS); SEND(SPACE); SEND(BS); i = i-2; continue; }; // Understand BS or DEL
			if (v == CR) { SEND(CR); SEND(LF); buf[i] = 0; i=0; return 1; }; // Understand CR
			if ((v < 32)||(v == DEL)) { i--; continue; }; // Filter ASCII control codes
			buf[i] = v;
			SEND(v);
		} else {
			return 0;
		}
	}
	buf[len-1] = 0;
	i=0;
	return 1;
}


void sendstr(const unsigned char * str)
{
	unsigned char val;
	for(;;) {
		val = *str;
		if (val) SEND(val);
		else break;
		str++;
	}
}

unsigned char* scanfor_notspace(unsigned char *buf)
{
	for (;;) {
		if (!(*buf)) return buf;
		if (!isspace(*buf)) return buf;
		buf++;
	}
}

unsigned char* scanfor_space(unsigned char *buf)
{
	for (;;) {
		if (!(*buf)) return buf;
		if (isspace(*buf)) return buf;
		buf++;
	}
}

static unsigned char count_tokens(unsigned char *rcvbuf)
{
	unsigned char tokens=0;
	for (;;) {
		rcvbuf = scanfor_notspace(rcvbuf);
		if(!(*rcvbuf)) break;
		tokens++;
		rcvbuf = scanfor_space(rcvbuf);
		if (!(*rcvbuf)) break;
	}
	return tokens;
}

void tokenize(unsigned char *rcvbuf,unsigned char** ptrs, uint8_t* tkcntptr)
{
	uint8_t i;
	uint8_t tokens;

	tokens = count_tokens(rcvbuf);
	if (tokens > MAXTOKENS) tokens = MAXTOKENS;
	if (tkcntptr) *tkcntptr = tokens;

	for (i=0; i<tokens; i++) {
		rcvbuf = scanfor_notspace(rcvbuf);
		if (!(*rcvbuf)) break;
		ptrs[i] = rcvbuf;
		rcvbuf = scanfor_space(rcvbuf);
		if (*rcvbuf) { *rcvbuf = 0; rcvbuf++; };
	}
	if (ptrs[0]) strupr((char*)ptrs[0]);
	return;
}

#endif
