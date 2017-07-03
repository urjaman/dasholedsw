#pragma once

/* NULL on success, a PSTR() on failure. */
const PGM_P saver_load_settings(void);

/* returns size of saved settings (informational) */
uint16_t saver_save_settings(void);

/* just for convenience for others, the CRC16 used by saver ;) */
uint16_t crc16(uint16_t crc_in, const void *bufv, uint16_t cnt);

