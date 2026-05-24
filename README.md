# embedded-poc-app

AgentCockpit の組み込み PoC 用サンプルアプリです。

主な内容:

- `app/`: GPIO + I2C OLED + SPI RFID を使うサンプルアプリ
- `scenarios/`: シミュレーション実行用のシナリオ定義
- `docs/`: 実機配線と PoC 結果メモ

## Build

```bash
make cross
make native
```

`agp-tools` と並べて clone している場合、EC2 へのツール・アプリ転送とシミュレーション起動は `agp-tools` 側から実行します。

```bash
cd ../agp-tools
make deploy-ec2 EC2=vibecode-graviton APP_BINARY=../embedded-poc-app/app/sensor_demo
make sim-start EC2=vibecode-graviton
```
