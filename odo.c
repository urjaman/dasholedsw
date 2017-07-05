#include "main.h"
#include "timer.h"
#include "pulse.h"
#include "eecounter.h"
#include "speedo.h"
#include "odo.h"

static uint32_t odo_metres;
static uint16_t odo_subm;
static uint16_t odo_last_ctr;
static uint8_t driving;

static uint32_t round_metres(void) {
	uint32_t r = odo_metres;
	if (odo_subm & _BV(SPEEDO_M_SHIFT-1)) r++;
	return r;
}

void odo_init(void) {
	odo_metres = eec_get_counter(EEC_CTR_ODO);
	odo_last_ctr = pulse_edge_ctr(PCH_ROADSPD, NULL);
}

void odo_run(void) {
	if (timer_get_5hzp()) {
		/* Full accuracy value is stored and kept in SRAM, a rounded to metres version
		 * is stored in the EEPROM counter. */
		uint16_t edgec = pulse_edge_ctr(PCH_ROADSPD, NULL);
		uint16_t e = edgec - odo_last_ctr;
		uint16_t mpp = speedo_get_mpp();
		uint32_t addm = ((uint32_t)mpp*e) + odo_subm;
		odo_last_ctr = edgec;
		uint16_t metres = addm >> SPEEDO_M_SHIFT;
		odo_subm = addm & (_BV(SPEEDO_M_SHIFT)-1);
		odo_metres += metres;

		/* Compute the rounded value and poke that to the EEPROM counter. */
		eec_set_counter(EEC_CTR_ODO, round_metres());

		uint8_t new_drv = pulse_get_hz(PCH_ROADSPD, NULL) ? 1 : 0;
		if ((driving)&&(!new_drv)) { // trigger ODO save on stopping
			eec_trigger_write();
		}
		driving = new_drv;
	}
}

/* Get either the full resolution odometer or the rounded to metres one.. */
uint32_t odo_get(uint16_t* subm) {
	if (subm) {
		*subm = odo_subm;
		return odo_metres;
	}
	return round_metres();
}

/* Rarely you need to be able to set the odometer. */
void odo_set(uint32_t metres) {
	odo_metres = metres;
	odo_subm = 0;
	eec_set_counter(EEC_CTR_ODO, metres);
	eec_trigger_write();
}

