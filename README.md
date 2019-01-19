# esp32_arduino_spi_slave

ESP32 の SPI スレーブ通信を Arduino で動かすサンプルです．

## 概要

SPI マスターは VSPI で動かし，SPI スレーブは HSPI で動かしています．

ESP32 が 1 つしかない場合でも，VSPI の端子と HSPI の端子を直結してやれば，
SPI マスターとスレーブの双方で SPI 双方向(Duplx) 通信が行えます．

## 詳細

SPI マスター側はなるべく Arduino のライブラリを使う形で記載しています．

SPI スレーブは driver/spi_slave.h の関数を使って実装しています．
流れとしては下記のようになります．

1. バスを spi_slave_initialize で初期化

2. spi_slave_queue_trans で送信データをセット

3. マスターが通信を行うと，spi_slave_interface_config_t の post_trans_cb に指定したコールバック関数が呼ばれる

4. 必要に応じて，受信値の読出しや次回の通信用の送信データのセットを行う．

5. 3.に戻る．
