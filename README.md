# embedded-poc-app

Gapless Agent Runtime の組み込み PoC 用サンプルアプリです。

主な内容:

- `app/`: GPIO + I2C OLED + SPI RFID を使うサンプルアプリ
- `scenarios/`: シミュレーション実行用のシナリオ定義
- `docs/`: 実機配線と PoC 結果メモ

## Build

```bash
make
make clean
```

Codespace build VM では ARM64 向けにビルドします。EC2 への転送と simulation runtime 操作は WSL hub 側の Gapless Agent Runtime から行います。
