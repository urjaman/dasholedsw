void backlight_init(void);
void backlight_set(uint8_t v);
void backlight_set_dv(uint8_t v);
void backlight_set_to(uint8_t to);
void backlight_activate(void);
void backlight_run(void);
uint8_t backlight_get(void);
uint8_t backlight_get_dv(void);
uint8_t backlight_get_to(void);

/* saver API */
uint8_t backlight_save(void**ptr);
void backlight_load(void *b, uint8_t sz);

void backlight_set_dv_always(uint8_t v);
uint8_t backlight_get_dv_always(void);
