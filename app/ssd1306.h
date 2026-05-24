#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  64
#define SSD1306_PAGES   (SSD1306_HEIGHT / 8)

int  ssd1306_open(const char *i2c_dev, uint8_t addr);
void ssd1306_close(int fd);

void ssd1306_init(int fd);
void ssd1306_clear(int fd);
void ssd1306_flush(int fd);

void ssd1306_put_text(int col, int page, const char *text);
void ssd1306_put_centered(int page, const char *text);

#endif
