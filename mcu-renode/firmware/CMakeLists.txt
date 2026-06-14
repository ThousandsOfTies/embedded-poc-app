cmake_minimum_required(VERSION 3.13)

# pico-sdk を取り込む（PICO_SDK_PATH 環境変数が必要）
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(blink_button C CXX ASM)

# まずは公式 pico(RP2040) 定義でビルド。
# 実機が USB-C 互換ボード(YD-RP2040 等)でも、同じ .elf がそのまま動く。
set(PICO_BOARD pico)

pico_sdk_init()

add_executable(blink_button blink_button.c)

# printf を UART へ（Renode の UART アナライザで観測する）
pico_enable_stdio_usb(blink_button 0)
pico_enable_stdio_uart(blink_button 1)

target_link_libraries(blink_button pico_stdlib hardware_gpio)

# .elf / .uf2 / .bin を生成（.elf=Renode用, .uf2=実機書き込み用）
pico_add_extra_outputs(blink_button)
