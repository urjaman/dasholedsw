#pragma once

#define SPEEDO_M_SHIFT 16

uint16_t speedo_get_mpp(void);
uint16_t speedo_get_kmh10(void);

/* TUI */
void speedo_calibrate(void);

/* the saver API */
uint8_t speedo_save(void**ptr);
void speedo_load(void *b, uint8_t sz);
