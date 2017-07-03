#include "main.h"
#include "timer.h"
#include "ee.h"
#include "saver.h"
#include "eecounter.h"

/* We implement the AVR appnote suggested circular buffer idea, but instead with
 * bigger blocks that have magic, crc, and a never-overflowing (well, 4 billion cycles
 * would be cool, but very not expected) writecounter that indicates both used EEPROM life,
 * and the most recent block. Writing happens in a circular fashion, but bad EEPROM sectors
 * can be detected and tolerated by skipping.
 * Since the data-structure is a bunch of 32-bit counters (plus magic and crc) where one
 * half is much more likely to change than the other (magic doesnt, crc does, low bytes
 * of counters do change much more often than high bytes), we occasionally switch the
 * 16-bit halves around to wear-level the EEPROM sectors. Doing this constantly would
 * just burn EEPROM life unnecessarily, but switching around sometimes makes use of
 * both sides of bytes. */

#define EECOUNTER_SECTORS 8

/* Logical values are 2 for 16byte sectors or 6 for 32byte sectors. */
#define EEC_COUNTERS 2

/* We encode the number of counters we implement in the magic, in case
 * we want to be able to convert between sector sizes, one day. */

#define EEC_MAGIC (0xEEC | (EEC_COUNTERS << 12))

struct ee_counters {
	uint16_t crc;
	uint16_t magic;
	uint32_t writecounter;
	uint32_t counters[EEC_COUNTERS];
};

/* At max dirty time, we will write without being triggered,
 * between min and max we will write if triggered, and
 * we never write below min dirty time. */

#define MAX_DIRTY_TIME (5*60)
#define MIN_DIRTY_TIME 5

static struct ee_counters cur = { 0 };
static uint32_t last_write = 0;
static uint8_t cur_dirty;

#define CONSISTENCY_POLL_TIME 60
#define CONSISTENCY_INITIAL_POLL 2

static uint32_t consistency_check = CONSISTENCY_POLL_TIME;
static uint16_t consistency_errors;
static uint8_t wr_offset;

#define EE_BASE (EE_SZ - (EECOUNTER_SECTORS * sizeof(struct ee_counters)))

static void flip_ctrs(void* d) {
	uint16_t *wp = d;
	for (uint8_t n=0;n<sizeof(struct ee_counters)/4;n++) {
		uint16_t t = wp[n*2];
		wp[n*2] = wp[n*2+1];
		wp[n*2+1] = t;
	}
}

static void read_state(void) {
	struct ee_counters *ee_ctrs = (struct ee_counters*)EE_BASE;
	struct ee_counters cn;
	ee_init();
	for (uint8_t n=0;n<EECOUNTER_SECTORS;n++) {
		ee_read_block(&cn, &ee_ctrs[n], sizeof(struct ee_counters));
		if (cn.magic == EEC_MAGIC) {
			// fine (avoid looking at CRC for flipped magic)
		} else if (cn.crc == EEC_MAGIC) { // flipped one
			flip_ctrs(&cn);
		} else {
			continue;
		}
		// check crc
		uint16_t crc = 0xFFFF;
		crc = crc16(crc, &cn.magic, sizeof(struct ee_counters)-2);
		if (crc != cn.crc) continue;
		if (cn.writecounter > cur.writecounter) {
			cur = cn;
			wr_offset = n;
		}
	}
	if (cur.writecounter) srandom(cur.writecounter);
}

static void do_consistency_check(void) {
	if (!cur.writecounter) return;
	uint16_t crc = 0xFFFF;
	struct ee_counters *ee_ctrs = (struct ee_counters*)EE_BASE;
	struct ee_counters chk;
	/* Read previous block to determine flip state. */
	ee_read_block(&chk, &ee_ctrs[wr_offset], sizeof(struct ee_counters));
	if (chk.magic == EEC_MAGIC) {
		// not flipped (avoid looking at CRC for flipped magic)
	} else if (chk.crc == EEC_MAGIC) { // flipped one
		flip_ctrs(&chk);
	} else {
		goto err;
	}
	// check crc
	crc = crc16(crc, &chk.magic, sizeof(struct ee_counters)-2);
	if (crc != chk.crc) {
		err:
		cur_dirty = 3; /* We just set the "write soon" flag... */
		consistency_errors++;
	}
	consistency_check = timer_get() + CONSISTENCY_POLL_TIME;
}

void check_write_state(void) {
	if (!cur_dirty) return;
	uint32_t tdiff = timer_get() - last_write;
	uint32_t tlimit;
	switch (cur_dirty) {
		default:
		case 1: tlimit = MAX_DIRTY_TIME; break;
		case 2: tlimit = MIN_DIRTY_TIME; break;
		case 3: tlimit = 1; /* special error->re-write */ break;
	}
	if (tdiff < tlimit) return;
	cur.magic = EEC_MAGIC;
	cur.writecounter++;
	cur.crc = crc16(0xFFFF, &cur.magic, sizeof(struct ee_counters)-2);
	uint8_t flipped = 0;
	wr_offset = (wr_offset+1) % EECOUNTER_SECTORS;
	struct ee_counters *ee_ctrs = (struct ee_counters*)EE_BASE;
	/* Read previous block to determine flip state. */
	struct ee_counters mem;
	ee_read_block(&mem, &ee_ctrs[wr_offset], sizeof(struct ee_counters));
	if (mem.magic == EEC_MAGIC) {
		// not flipped (avoid looking at CRC for flipped magic)
	} else if (mem.crc == EEC_MAGIC) { // flipped one
		flipped = 1;
	}
	/* Once in a blue moon, flip it ... */
	uint32_t r = random();
	r = (r ^ (r >> 12)) & 0xFFF; // 0-4095 whilst using most of the provided random bits...
	if (r == 1234) flipped ^= 1;
	/* Never flip if it would look non-flipped due to crc==magic. */
	if (cur.crc == EEC_MAGIC) flipped = 0;
	mem = cur; /* Dont flip the canonical one we read :P, so use the stack mem. */
	if (flipped) {
		flip_ctrs(&mem);
	}
	ee_update_block(&mem, &ee_ctrs[wr_offset], sizeof(struct ee_counters));
	cur_dirty = 0;
	/* Do one check now, then another check 2s from now, and then every minute. */
	do_consistency_check();
	consistency_check = timer_get() + CONSISTENCY_INITIAL_POLL;
	last_write = timer_get();
}


void eec_set_counter(uint8_t ctr, uint32_t val) {
	if (ctr>=EEC_COUNTERS) return;
	if (val != cur.counters[ctr]) {
		if (!cur_dirty) cur_dirty = 1;
	}
	cur.counters[ctr] = val;
}

void eec_trigger_write(void) {
	if (cur_dirty) {
		if (cur_dirty<2) cur_dirty = 2;
		check_write_state();
	}
}

uint32_t eec_get_counter(uint8_t ctr) {
	if (ctr>=EEC_COUNTERS) return 0;
	return cur.counters[ctr];
}

void eec_init(void) {
	read_state();
}

void eec_run(void) {
	if (timer_get_1hzp()) {
		check_write_state();
		if (timer_get() >= consistency_check) {
			do_consistency_check();
		}
	}
}
