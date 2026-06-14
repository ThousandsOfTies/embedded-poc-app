# mcu-renode — Renode(sim) ↔ 実機 バイナリ透過の最小実験

Raspberry Pi Pico 系 MCU（RP2040 / RP2350）で、**同一の `.elf`/`.uf2` を Renode(sim) と
実機の両方で動かす**ことを確認する最小実験。狙いは「開発・検証・実機」を貫く
**バイナリ透過性**を、Linux SBC だけでなく MCU/ベアメタル領域でも示すこと。

最初は I2C/SPI センサを足さず、**GPIO の「ボタン → LED」だけ**に絞る。

## 構成

- `firmware/blink_button.c` — ボタン押下で LED 点灯、UART にログ出力
- `firmware/CMakeLists.txt` — pico-sdk ビルド定義（`.elf`/`.uf2` 生成）
- `sim/pico_blink.resc` — Renode 起動スクリプト（ELF ロード + 仮想ボタン操作）

## 1. Renode を導入

```bash
# 正本リポの gar setup から導入できる（シミュレート環境 → Renode を選択）
gar setup
```

## 2. ツールチェーンと pico-sdk を用意

```bash
sudo apt-get install -y cmake gcc-arm-none-eabi build-essential
git clone --depth 1 https://github.com/raspberrypi/pico-sdk
cd pico-sdk && git submodule update --init && cd ..
export PICO_SDK_PATH="$PWD/pico-sdk"
```

## 3. ファームをビルド（`.elf` 生成）

```bash
cd firmware && mkdir -p build && cd build
cmake .. && make -j
# → build/blink_button.elf（Renode 用）/ build/blink_button.uf2（実機用）
```

## 4. Renode(sim) で起動（実機不要）

```bash
cd ../../   # mcu-renode/ 直下に戻る
renode sim/pico_blink.resc
```

Renode モニタ上で:

```
(pico) runMacro $press     # ボタン押下をエミュレート
(pico) runMacro $release   # 離す
```

UART アナライザに `[btn] DOWN -> LED ON` が出れば成功。

## 5. 実機が届いたら（同一バイナリ確認）

```bash
# BOOTSEL を押しながら USB 接続 → マスストレージとしてマウントされる
cp firmware/build/blink_button.uf2 /media/$USER/RPI-RP2/
# 物理ボタンを押すと LED 点灯。Renode で見た挙動と一致 = バイナリ透過の実証
```

## 注意 / 既知の未確定点

- **`.repl` のパス/名前** は Renode の版で異なりうる。`sim/pico_blink.resc` の
  `platforms/cpus/rp2040.repl` や GPIO ペリフェラル登録名（`gpio`）/`WritePin` API は、
  動かない場合に Renode の `peripherals` / 同梱 `platforms/` で確認して合わせる。
- **オンボLEDのピン番号** はボード差あり。USB-C 互換ボード（YD-RP2040 等）は公式 Pico と
  異なる場合があるので実機側で要確認（sim と `.elf` は共通で良い）。
- これは **`gar sim env` への本配線前の単体実験**。Renode を `gar` から起動・操作する統合は、
  この手動実験で感触を得てから着手する（正本リポ TODO の「ターゲット抽象の引き直し」を参照）。
