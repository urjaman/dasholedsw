#pragma once

#define EE_PAGESZ 32
#define EE_SZ 1024

void ee_init(void);
void ee_read_block(void*to, const void* from, size_t len);
uint8_t ee_read_byte(const void* from);
void ee_update_block(const void* fromv, void* tov, size_t len);
