#ifndef MFRC522_H
#define MFRC522_H

#include <stdint.h>

#define MFRC522_UID_MAX 10

int  mfrc522_open(const char *spi_dev);
void mfrc522_close(int fd);
void mfrc522_init(int fd);

/* カードを検出して UID を読む。
 *   戻り値: UID 長 (4/7/10) を返す。なし or エラーは 0。
 *   uid_out は MFRC522_UID_MAX バイトのバッファ。
 */
int  mfrc522_read_uid(int fd, uint8_t *uid_out);

#endif
