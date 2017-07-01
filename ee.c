#include "main.h"
#include "ee.h"

static void nvm_execute(void) {
	CCP = 0xD8;
	NVM_CTRLA = 0x01;
	while (NVM_STATUS & NVM_NVMBUSY_bm);
}

void ee_init(void) {
	/* Check that the NVM is not busy. */
	while (NVM_STATUS & NVM_NVMBUSY_bm);
	/* Flush EEPROM load buffer */
	if (NVM_STATUS & NVM_EELOAD_bm) {
		NVM_CMD = NVM_CMD_ERASE_EEPROM_BUFFER_gc;
		nvm_execute();
	}
}

void ee_read_block(void*to, const void* from, size_t len) {
	const void *eemap = (void*)(0x1000 + (uint16_t)from);
	memcpy(to, eemap, len);
}

uint8_t ee_read_byte(const void* from) {
	uint8_t r;
	ee_read_block(&r, from, 1);
	return r;
}

static void ee_update_page(const uint8_t*from, volatile uint8_t* to, size_t pl) {
	uint16_t pa = (uint16_t)to & (EE_SZ-1);
	NVM_ADDR0 = pa&0xFF;
	NVM_ADDR1 = pa>>8;
	NVM_ADDR2 = 0;

	/* Load page buffer for erase */
	uint8_t erase = 0;
	for (uint8_t i=0;i<pl;i++) {
		if ((to[i] & from[i]) != from[i]) {
			to[i] = 0xFF;
			erase = 1;
		}
	}
	if (erase) {
		NVM_CMD = NVM_CMD_ERASE_EEPROM_PAGE_gc;
		nvm_execute();
	}
	/* Load for write */
	uint8_t write = 0;
	for (uint8_t i=0;i<pl;i++) {
		if (to[i] != from[i]) {
			to[i] = from[i];
			write = 1;
		}
	}
	if (write) {
		NVM_CMD = NVM_CMD_WRITE_EEPROM_PAGE_gc;
		nvm_execute();
	} else if (erase) {
		/* Flush EEPROM load buffer */
		if (NVM_STATUS & NVM_EELOAD_bm) {
			NVM_CMD = NVM_CMD_ERASE_EEPROM_BUFFER_gc;
			nvm_execute();
		}
	}

}

void ee_update_block(const void*fromv, void* tov, size_t len) {
	uint16_t psv = (uint16_t)tov;
	volatile uint8_t *to = (uint8_t*)(0x1000 + psv);
	const uint8_t *from = (const uint8_t*)fromv;
	for (uint16_t p=psv/EE_PAGESZ;p <= (psv+len-1)/EE_PAGESZ;p++) {
		uint8_t po = psv & (EE_PAGESZ-1);
		uint8_t mlh = EE_PAGESZ - po;
		uint8_t lh = len > mlh ? mlh : len;
		ee_update_page(from, to, lh);
		psv += lh;
		to += lh;
		from += lh;
		len -= lh;
	}
}
