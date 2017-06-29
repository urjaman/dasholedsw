#ifndef _TIMER_H_
#define _TIMER_H_
#include "time.h"

void timer_run(void);
uint32_t timer_get(void);
uint8_t timer_get_1hzp(void);
uint8_t timer_get_todo(void);
uint8_t timer_get_5hzp(void);
uint8_t timer_get_5hz_cnt(void);
void timer_set_waiting(void);


// Delay us and delay ms, both have a limit of 200ms (for 5hzp)
// and a granularity of US_PER_SSUNIT (32us)
void timer_delay_us(uint24_t us);
void timer_delay_ms(uint8_t ms);


/* timer-ll.c */
void timer_init(void);
uint16_t timer_get_subsectimer(void);
uint8_t timer_getdec_todo(void);
uint8_t timer_get_todo(void);
uint24_t timer_get_linear_ss_time(void);
uint16_t timer_get_lin_ss_u16(void);

/* timer-ll.c is used to consolidate access to todo and subsectimer 
   to a single object */

#define SSTC (32768)
#define US_PER_SSUNIT 31
#endif
