#pragma once

/* NULL on success, a PSTR() on failure. */
const PGM_P saver_load_settings(void);

/* returns size of saved settings (informational) */
uint16_t saver_save_settings(void);
