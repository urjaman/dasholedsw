void relay_init(void);
void relay_set(uint8_t mode);
void relay_set_autovoltage(uint16_t v);
void relay_set_keepon(uint8_t s);
uint8_t relay_get(void);
uint8_t relay_get_mode(void);
uint16_t relay_get_autovoltage(void);
uint8_t relay_get_autodecision(void);
uint8_t relay_get_keepon(void);

/* saver API */
uint8_t relay_save(void**ptr);
void relay_load(void *b, uint8_t sz);

void relay_run(void);
#define RLY_MODE_OFF 0
#define RLY_MODE_ON 1
#define RLY_MODE_AUTO 2
