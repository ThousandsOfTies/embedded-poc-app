/*
 * mfrc522.c — MFRC-522 RFID リーダー SPI ドライバ
 *
 * 実機: /dev/spidev0.0 経由で本物の MFRC-522 を制御
 * EC2:  spi_shim が /dev/spidev0.0 をハンドルしてシミュレート
 *
 * 参考: NXP MFRC522 datasheet, ST AN10833
 */

#include "mfrc522.h"
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* ---- レジスタ定義 ---- */
#define COMMAND_REG     0x01
#define COM_IRQ_REG     0x04
#define DIV_IRQ_REG     0x05
#define ERROR_REG       0x06
#define FIFO_DATA_REG   0x09
#define FIFO_LEVEL_REG  0x0A
#define CONTROL_REG     0x0C
#define BIT_FRAMING_REG 0x0D
#define MODE_REG        0x11
#define TX_CONTROL_REG  0x14
#define TX_AUTO_REG     0x15
#define VERSION_REG     0x37
#define T_MODE_REG      0x2A
#define T_PRESCALER_REG 0x2B
#define T_RELOAD_H_REG  0x2C
#define T_RELOAD_L_REG  0x2D

/* コマンド */
#define CMD_IDLE        0x00
#define CMD_TRANSCEIVE  0x0C
#define CMD_SOFT_RESET  0x0F

/* PICC コマンド */
#define PICC_REQA       0x26
#define PICC_ANTICOLL   0x93

/* ---- 低レベル SPI 読み書き ---- */
static int spi_xfer(int fd, const uint8_t *tx, uint8_t *rx, int len) {
    struct spi_ioc_transfer tr = {
        .tx_buf = (uintptr_t)tx,
        .rx_buf = (uintptr_t)rx,
        .len    = len,
        .speed_hz = 1000000,
        .bits_per_word = 8,
    };
    return ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1 ? -1 : 0;
}

static void write_reg(int fd, uint8_t reg, uint8_t val) {
    uint8_t tx[2] = { (uint8_t)((reg << 1) & 0x7E), val };
    uint8_t rx[2];
    spi_xfer(fd, tx, rx, 2);
}

static uint8_t read_reg(int fd, uint8_t reg) {
    uint8_t tx[2] = { (uint8_t)(((reg << 1) & 0x7E) | 0x80), 0x00 };
    uint8_t rx[2];
    spi_xfer(fd, tx, rx, 2);
    return rx[1];
}

static void set_bits(int fd, uint8_t reg, uint8_t mask) {
    write_reg(fd, reg, read_reg(fd, reg) | mask);
}

static void clr_bits(int fd, uint8_t reg, uint8_t mask) {
    write_reg(fd, reg, read_reg(fd, reg) & ~mask);
}

/* ---- 初期化 ---- */
int mfrc522_open(const char *spi_dev) {
    int fd = open(spi_dev, O_RDWR);
    if (fd < 0) { perror(spi_dev); return -1; }
    uint8_t mode = SPI_MODE_0;
    uint32_t speed = 1000000;
    ioctl(fd, SPI_IOC_WR_MODE, &mode);
    ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    return fd;
}

void mfrc522_close(int fd) {
    if (fd >= 0) close(fd);
}

void mfrc522_init(int fd) {
    write_reg(fd, COMMAND_REG, CMD_SOFT_RESET);
    usleep(50000);
    while (read_reg(fd, COMMAND_REG) & 0x10) usleep(1000);

    write_reg(fd, T_MODE_REG, 0x8D);
    write_reg(fd, T_PRESCALER_REG, 0x3E);
    write_reg(fd, T_RELOAD_H_REG, 0x00);
    write_reg(fd, T_RELOAD_L_REG, 0x1E);
    write_reg(fd, TX_AUTO_REG, 0x40);
    write_reg(fd, MODE_REG, 0x3D);

    /* アンテナ ON */
    if (!(read_reg(fd, TX_CONTROL_REG) & 0x03))
        set_bits(fd, TX_CONTROL_REG, 0x03);
}

/* ---- カードと通信して応答を取得 ---- */
static int transceive(int fd, const uint8_t *tx, int tx_len,
                      uint8_t *rx, int *rx_len_bits) {
    write_reg(fd, COMMAND_REG, CMD_IDLE);
    write_reg(fd, COM_IRQ_REG, 0x7F);          /* clear all IRQ */
    set_bits(fd, FIFO_LEVEL_REG, 0x80);        /* flush FIFO */

    for (int i = 0; i < tx_len; i++)
        write_reg(fd, FIFO_DATA_REG, tx[i]);

    write_reg(fd, COMMAND_REG, CMD_TRANSCEIVE);
    set_bits(fd, BIT_FRAMING_REG, 0x80);       /* StartSend */

    /* IRQ 待ち（タイムアウト約 25ms） */
    int i;
    for (i = 0; i < 2000; i++) {
        uint8_t irq = read_reg(fd, COM_IRQ_REG);
        if (irq & 0x30) break;                 /* RxIRq or IdleIRq */
        if (irq & 0x01) return -1;             /* TimerIRq → no card */
        usleep(10);
    }
    if (i >= 2000) return -1;

    clr_bits(fd, BIT_FRAMING_REG, 0x80);

    if (read_reg(fd, ERROR_REG) & 0x13) return -1;

    int n = read_reg(fd, FIFO_LEVEL_REG);
    int last_bits = read_reg(fd, CONTROL_REG) & 0x07;
    *rx_len_bits = (last_bits != 0) ? ((n - 1) * 8 + last_bits) : (n * 8);

    if (n > 16) n = 16;
    for (int j = 0; j < n; j++) rx[j] = read_reg(fd, FIFO_DATA_REG);
    return 0;
}

/* ---- カード検出 + UID 取得 ---- */
int mfrc522_read_uid(int fd, uint8_t *uid_out) {
    uint8_t buf[16];
    int rx_bits;

    /* REQA: 7-bit short frame */
    write_reg(fd, BIT_FRAMING_REG, 0x07);
    uint8_t reqa = PICC_REQA;
    if (transceive(fd, &reqa, 1, buf, &rx_bits) < 0 || rx_bits != 16)
        return 0;

    /* Anticollision: 標準 4byte UID 取得 */
    write_reg(fd, BIT_FRAMING_REG, 0x00);
    uint8_t anti[2] = { PICC_ANTICOLL, 0x20 };
    if (transceive(fd, anti, 2, buf, &rx_bits) < 0 || rx_bits != 40)
        return 0;

    /* buf[0..3] が UID, buf[4] が BCC */
    memcpy(uid_out, buf, 4);
    return 4;
}
