#include "main.h"
#include "lib.h"

static unsigned char hextab_func(unsigned char offset)
{
	offset |= 0x30;
	if (offset > 0x39) offset += 7;
	return offset;
}

static unsigned char reverse_hextab(unsigned char hexchar)
{
	if (hexchar > 0x39) hexchar = hexchar - 7;
	hexchar &= 0x0F;
	return hexchar;
}

static unsigned char isvalid(unsigned char c, unsigned char base)
{
	if (base == 16) {
		if ((c > 'F') || (c < '0')) return 0;
		if ((c > '9') && (c < 'A')) return 0;
	} else {
		if ((c > '9') || (c < '0')) return 0;
	}
	return 1;
}

uint8_t str2uchar(unsigned char *buf)
{
	uint8_t rv;
	for (rv=0; *buf; buf++) {
		if ((*buf >= '0')||(*buf <= '9')) {
			rv *= 10;
			rv = rv + (*buf &0x0F);
		}
	}
	return rv;
}

uint8_t xstr2uchar(unsigned char *buf)
{
	uint8_t rv;
	rv = (reverse_hextab(*buf)<<4);
	buf++;
	rv |= reverse_hextab(*buf);
	return rv;
}

uint32_t astr2luint(unsigned char *buf)
{
	uint8_t i, len, base=10;
	uint32_t rv;
	strupr((char*)buf);
	len = strlen((char*)buf);
	if (buf[len-1] == 'H') base=16;
	if ((buf[0] == '0')&&(buf[1] == 'X')) base=16;
	if ((buf[0] == '~')&&(buf[1] == '0')&&(buf[2] == 'X')) base=16;
	rv = 0;
	for(i=0; i<len; i++) {
		if (!(isvalid(buf[i],base))) continue;
		rv = rv * base;
		rv += reverse_hextab(buf[i]); // RVHEXTAB works also for base 10
	}
	if (buf[0] == '~') rv = ~rv;
	return rv;
}

uint8_t uint2str(unsigned char *buf, uint16_t val)
{
	return luint2str(buf,(uint32_t)val);
}

uint8_t uint2xstr(unsigned char *buf,uint16_t val)
{
	return luint2xstr(buf,(uint32_t)val);
}


uint8_t uchar2str(unsigned char *buf, uint8_t val)
{
	return uint2str(buf,(uint16_t)val);
}

uint8_t uchar2xstr(unsigned char *buf,uint8_t val)
{
	unsigned char offset;
	offset = ((val>>4)&0x0F);
	buf[0] = hextab_func(offset);
	offset = (val&0x0F);
	buf[1] = hextab_func(offset);
	buf[2] = 0;
	return 2;
}

uint8_t luint2str(unsigned char *buf, uint32_t val)
{
	ultoa(val,(char*)buf,10);
	return strlen((char*)buf);
}

uint8_t luint2str_zp(uint8_t *buf, uint32_t val, uint8_t width)
{
	uint8_t vw = 1;
	uint32_t vc = 10;
	while (vc < val) {
		vc *= 10;
		vw++;
	}
	uint8_t xw = width - vw;
	if (vw > width) xw = 0;
	if (xw) {
		memset(buf, '0', xw);
	}
	return xw+luint2str(buf+xw,val);
}

uint8_t luint2xstr(unsigned char *buf, uint32_t val)
{
	ultoa(val,(char*)buf,16);
	strupr((char*)buf); // i dont like "aaaah"...
	return strlen((char*)buf);
}
