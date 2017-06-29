#pragma once

/* Frequency is represented as 20.4 fixed point Hz. */
#define PULSE_HZ_SHIFT 4

/* The age output is specified in units of this. */
#define PULSE_TIMER_HZ 8192

enum pulse_ch {
	PCH_DAUX = 4,
	PCH_ECULAMP,
	PCH_ROADSPD,
	PCH_RPM
};

void pulse_init(void);

void pulse_run(void);

/* Return the state (0/1) and optionally edge counter of a pulse channel. */
uint8_t pulse_state(const enum pulse_ch ch, uint16_t *edges);

/* Just the edge counter... */
uint16_t pulse_edge_ctr(const enum pulse_ch ch);

/* Return the computed frequency of a pulse channel :) (and optionally how old is this info) */
uint24_t pulse_get_hz(const enum pulse_ch ch, uint16_t *age);

/* Default is both edges, but you can also pick rising or falling. */
enum pulse_mode {
	PM_BOTH_EDGES = 0,
	PM_RISING,
	PM_FALLING
};

void pulse_set_ch_mode(const enum pulse_ch ch, enum pulse_mode pm);
