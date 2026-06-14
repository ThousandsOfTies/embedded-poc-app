#include "pico/stdio.h"
#include "hardware/gpio.h"

// 最小実験: ボタン -> LED。Renode(sim) と実機で同一 .elf を動かし、
// 「バイナリ透過（同じバイナリが sim と実機で動く）」を確認するためのファーム。
// まずは I2C/SPI センサを足さず GPIO だけに絞る。

#define LED_PIN    25   // オンボLED想定。USB-C互換ボード(YD-RP2040等)は要確認
#define BUTTON_PIN 14   // 任意のGPIO。プルアップ + GND ボタン

int main(void) {
    stdio_init_all();               // UART ログ（Renode の UART で観測可）
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);       // 押すと L

    printf("blink_button: start\n");
    bool prev_pressed = false;
    while (true) {
        bool pressed = !gpio_get(BUTTON_PIN);
        gpio_put(LED_PIN, pressed);
        if (pressed != prev_pressed) {   // エッジでログ
            printf("[btn] %s -> LED %s\n",
                   pressed ? "DOWN" : "UP",
                   pressed ? "ON" : "OFF");
            prev_pressed = pressed;
        }
    }
}
