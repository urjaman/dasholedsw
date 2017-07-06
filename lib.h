// Binary values to number strings, return string length.
uint8_t uint2str(unsigned char *buf, uint16_t val);
uint8_t uint2xstr(unsigned char *buf, uint16_t val);
uint8_t uchar2str(unsigned char *buf, uint8_t val);
uint8_t uchar2xstr(unsigned char *buf, uint8_t val);
uint8_t luint2str(unsigned char *buf, uint32_t val);
uint8_t luint2xstr(unsigned char*buf, uint32_t val);
/* With zero padding to width-wide field. Can return longer than width.. */
uint8_t luint2str_zp(uint8_t *buf, uint32_t val, uint8_t width);

// Number string to binary value.
uint8_t str2uchar(unsigned char *buf);
uint8_t xstr2uchar(unsigned char *buf);
uint32_t astr2luint(unsigned char *buf);

