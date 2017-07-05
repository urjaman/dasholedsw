#pragma once

void eec_init(void);
void eec_run(void);

uint32_t eec_get_counter(uint8_t ctr);
void eec_set_counter(uint8_t ctr, uint32_t val);
void eec_trigger_write(void);

#define EEC_CTR_ODO 0
