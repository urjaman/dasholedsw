
#define LCDWIDTH 84
#define LCDHEIGHT 48

void pcd8544_drawpixel(uint16_t x, uint16_t y, uint8_t c);
void pcd8544_init(void);
void pcd8544_display(void);
void pcd8544_clear(void);

void pcd8544_write_block_P(const PGM_P buffer, uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void pcd8544_write_block(const uint8_t *buffer, uint8_t x, uint8_t y, uint8_t w, uint8_t h);
