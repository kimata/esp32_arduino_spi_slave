#include "driver/spi_master.h"
#include "driver/spi_slave.h"
#include <Arduino.h>
#include <SPI.h>

// SPI 通信周波数
static const uint32_t SPI_CLK_HZ = 4500000;

// 通信サイズ
static const uint32_t TRANS_SIZE = 8192;

// 通信バッファ
uint8_t* spi_master_tx_buf;
uint8_t* spi_master_rx_buf;
uint8_t* spi_slave_tx_buf;
uint8_t* spi_slave_rx_buf;

// SPI マスタの設定
spi_transaction_t spi_master_trans;
spi_device_interface_config_t spi_master_cfg;
spi_device_handle_t spi_master_handle;
spi_bus_config_t spi_master_bus;

// SPI スレーブの設定
spi_slave_transaction_t spi_slave_trans;
spi_slave_interface_config_t spi_slave_cfg;
spi_bus_config_t spi_slave_bus;

// VSPI の端子設定
static const uint8_t SPI_MASTER_CS = 5;
static const uint8_t SPI_MASTER_CLK = 18;
static const uint8_t SPI_MASTER_MOSI = 23;
static const uint8_t SPI_MASTER_MISO = 19;

// HSPI の端子設定
static const uint8_t SPI_SLAVE_CS = 15;
static const uint8_t SPI_SLAVE_CLK = 14;
static const uint8_t SPI_SLAVE_MOSI = 13;
static const uint8_t SPI_SLAVE_MISO = 12;

// デバッグ用のダンプ関数
void dump_buf(const char* title, uint8_t* buf, uint32_t start, uint32_t len)
{
    if (len == 1) {
        Serial.printf("%s [%d]: ", title, start);
    } else {
        Serial.printf("%s [%d-%d]: ", title, start, start + len - 1);
    }
    for (uint32_t i = 0; i < len; i++) {
        Serial.printf("%02X ", buf[start + i]);
    }
    Serial.println();
}

// デバッグ用の比較関数
void cmp_bug(const char* a_title, uint8_t* a_buf,
    const char* b_title, uint8_t* b_buf, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        uint32_t j = 1;
        if (a_buf[i] == b_buf[i]) {
            continue;
        }
        while (a_buf[i + j] != b_buf[i + j]) {
            j++;
        }
        dump_buf(a_title, a_buf, i, j);
        dump_buf(b_title, b_buf, i, j);
        i += j - 1;
    }
}

// SPI 通信に使用するバッファの初期化
void spi_buf_init()
{
    spi_master_tx_buf = (uint8_t*)heap_caps_malloc(TRANS_SIZE, MALLOC_CAP_DMA);
    spi_master_rx_buf = (uint8_t*)heap_caps_malloc(TRANS_SIZE, MALLOC_CAP_DMA);
    spi_slave_tx_buf = (uint8_t*)heap_caps_malloc(TRANS_SIZE, MALLOC_CAP_DMA);
    spi_slave_rx_buf = (uint8_t*)heap_caps_malloc(TRANS_SIZE, MALLOC_CAP_DMA);

    for (uint32_t i = 0; i < TRANS_SIZE; i++) {
        spi_master_tx_buf[i] = i & 0xFF;
        spi_slave_tx_buf[i] = (0xFF - i) & 0xFF;
    }
    memset(spi_master_rx_buf, 0, TRANS_SIZE);
    memset(spi_slave_rx_buf, 0, TRANS_SIZE);
}

// マスタとして動作させる VSPI の初期化
void spi_master_init()
{
    spi_master_trans.flags = 0;
    spi_master_trans.length = 8 * TRANS_SIZE;
    spi_master_trans.rx_buffer = spi_master_rx_buf;
    spi_master_trans.tx_buffer = spi_master_tx_buf;

    spi_master_cfg.mode = SPI_MODE3;
    spi_master_cfg.clock_speed_hz = SPI_CLK_HZ;
    spi_master_cfg.spics_io_num = SPI_MASTER_CS;
    spi_master_cfg.queue_size = 1; // キューサイズ
    spi_master_cfg.flags = SPI_DEVICE_NO_DUMMY;
    spi_master_cfg.queue_size = 1;
    spi_master_cfg.pre_cb = NULL;
    spi_master_cfg.post_cb = NULL;

    spi_master_bus.sclk_io_num = SPI_MASTER_CLK;
    spi_master_bus.mosi_io_num = SPI_MASTER_MOSI;
    spi_master_bus.miso_io_num = SPI_MASTER_MISO;
    spi_master_bus.max_transfer_sz = 8192;

    ESP_ERROR_CHECK(spi_bus_initialize(VSPI_HOST, &spi_master_bus, 1)); // DMA 1ch
    ESP_ERROR_CHECK(
        spi_bus_add_device(VSPI_HOST, &spi_master_cfg, &spi_master_handle));
}

// スレーブの通信完了後に呼ばれるコールバック
void spi_slave_tans_done(spi_slave_transaction_t* trans)
{
    Serial.println("SPI Slave 通信完了");
}

// スレーブとして動作させる HSPI の初期化
void spi_slave_init()
{
    spi_slave_trans.length = 8 * TRANS_SIZE;
    spi_slave_trans.rx_buffer = spi_slave_rx_buf;
    spi_slave_trans.tx_buffer = spi_slave_tx_buf;

    spi_slave_cfg.spics_io_num = SPI_SLAVE_CS;
    spi_slave_cfg.flags = 0;
    spi_slave_cfg.queue_size = 1;
    spi_slave_cfg.mode = SPI_MODE3;
    spi_slave_cfg.post_setup_cb = NULL;
    spi_slave_cfg.post_trans_cb = spi_slave_tans_done;

    spi_slave_bus.sclk_io_num = SPI_SLAVE_CLK;
    spi_slave_bus.mosi_io_num = SPI_SLAVE_MOSI;
    spi_slave_bus.miso_io_num = SPI_SLAVE_MISO;
    spi_slave_bus.max_transfer_sz = 8192;

    ESP_ERROR_CHECK(
        spi_slave_initialize(HSPI_HOST, &spi_slave_bus, &spi_slave_cfg, 2)); // DMA 2ch
}

void spi_init()
{
    spi_buf_init();
    spi_master_init();
    spi_slave_init();
}

void setup()
{
    Serial.begin(9600);
    spi_init();
}

void loop()
{
    spi_transaction_t* spi_trans;

    Serial.println("[LOOP]");

    // スレーブの送信準備
    ESP_ERROR_CHECK(
        spi_slave_queue_trans(HSPI_HOST, &spi_slave_trans, portMAX_DELAY));
    // マスターの送信開始
    ESP_ERROR_CHECK(spi_device_queue_trans(spi_master_handle, &spi_master_trans,
        portMAX_DELAY));

    // 通信完了待ち
    spi_device_get_trans_result(spi_master_handle, &spi_trans, portMAX_DELAY);

    Serial.println("SPI Master 通信完了");

    // チェック
    if (memcmp(spi_master_rx_buf, spi_slave_tx_buf, TRANS_SIZE)) {
        Serial.println("マスターの受信値が期待値と一致しません．");
        cmp_bug("受信値", spi_master_rx_buf, "期待値", spi_slave_tx_buf,
            TRANS_SIZE);
    }
    if (memcmp(spi_slave_rx_buf, spi_master_tx_buf, TRANS_SIZE)) {
        Serial.println("スレーブの受信値が期待値と一致しません．");
        cmp_bug("受信値", spi_slave_rx_buf, "期待値", spi_master_tx_buf,
            TRANS_SIZE);
    }

    delay(1000);
}