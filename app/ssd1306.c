/*
 * ssd1306.c — SSD1306 OLED 128x64 I2C driver
 *
 * 実機では /dev/i2c-1 経由で本物の OLED を制御。
 * EC2 では cuse_i2c が 0x3C アドレスをハンドルしてシミュレートする。
 */

#include "ssd1306.h"
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* 6x8 font (ASCII printable subset) */
extern const uint8_t ssd1306_font6x8[][6];

static uint8_t framebuf[SSD1306_WIDTH * SSD1306_PAGES];

static int i2c_write(int fd, const uint8_t *buf, int len) {
    return (write(fd, buf, len) == len) ? 0 : -1;
}

int ssd1306_open(const char *i2c_dev, uint8_t addr) {
    int fd = open(i2c_dev, O_RDWR);
    if (fd < 0) { perror(i2c_dev); return -1; }
    if (ioctl(fd, I2C_SLAVE, addr) < 0) {
        perror("I2C_SLAVE");
        close(fd);
        return -1;
    }
    return fd;
}

void ssd1306_close(int fd) {
    if (fd >= 0) close(fd);
}

static void cmd(int fd, uint8_t c) {
    uint8_t buf[2] = { 0x00, c };
    i2c_write(fd, buf, 2);
}

void ssd1306_init(int fd) {
    cmd(fd, 0xAE);              /* display off */
    cmd(fd, 0xD5); cmd(fd, 0x80);
    cmd(fd, 0xA8); cmd(fd, 0x3F);
    cmd(fd, 0xD3); cmd(fd, 0x00);
    cmd(fd, 0x40);
    cmd(fd, 0x8D); cmd(fd, 0x14); /* charge pump on */
    cmd(fd, 0x20); cmd(fd, 0x00); /* horizontal addressing */
    cmd(fd, 0xA1);                /* segment remap */
    cmd(fd, 0xC8);                /* COM scan dec */
    cmd(fd, 0xDA); cmd(fd, 0x12);
    cmd(fd, 0x81); cmd(fd, 0xCF);
    cmd(fd, 0xD9); cmd(fd, 0xF1);
    cmd(fd, 0xDB); cmd(fd, 0x40);
    cmd(fd, 0xA4);                /* normal display */
    cmd(fd, 0xA6);
    cmd(fd, 0xAF);                /* display on */
    ssd1306_clear(fd);
    ssd1306_flush(fd);
}

void ssd1306_clear(int fd) {
    (void)fd;
    memset(framebuf, 0, sizeof(framebuf));
}

void ssd1306_flush(int fd) {
    cmd(fd, 0x21); cmd(fd, 0); cmd(fd, SSD1306_WIDTH - 1);   /* col 0..127 */
    cmd(fd, 0x22); cmd(fd, 0); cmd(fd, SSD1306_PAGES - 1);   /* page 0..7 */

    /* Send framebuffer in chunks of 16 bytes prefixed with 0x40 */
    uint8_t chunk[17];
    chunk[0] = 0x40;
    for (int i = 0; i < (int)sizeof(framebuf); i += 16) {
        int n = (sizeof(framebuf) - i) >= 16 ? 16 : (sizeof(framebuf) - i);
        memcpy(chunk + 1, framebuf + i, n);
        i2c_write(fd, chunk, n + 1);
    }
}

void ssd1306_put_text(int col, int page, const char *text) {
    if (page < 0 || page >= SSD1306_PAGES) return;
    while (*text && col < SSD1306_WIDTH - 5) {
        unsigned char ch = (unsigned char)*text++;
        if (ch < 0x20 || ch > 0x7E) ch = '?';
        const uint8_t *glyph = ssd1306_font6x8[ch - 0x20];
        for (int i = 0; i < 6 && col + i < SSD1306_WIDTH; i++) {
            framebuf[page * SSD1306_WIDTH + col + i] = glyph[i];
        }
        col += 6;
    }
}

void ssd1306_put_centered(int page, const char *text) {
    int len = (int)strlen(text);
    int width_px = len * 6;
    int col = (SSD1306_WIDTH - width_px) / 2;
    if (col < 0) col = 0;
    ssd1306_put_text(col, page, text);
}
