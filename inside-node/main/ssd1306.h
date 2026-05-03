#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

#define OLED_ADDR      0x3D
#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT 64
#define SSD1306_PAGES  8

void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_draw_text(uint8_t row, uint8_t col, const char *text);

#endif
