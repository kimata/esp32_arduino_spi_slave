#include <Arduino.h>
#include <SPI.h>
#include "driver/spi_slave.h"

static const uint32_t TRANS_SIZE = 64;

uint8_t *spi_master_tx_buf;
uint8_t *spi_master_rx_buf;
uint8_t *spi_slave_tx_buf;
uint8_t *spi_slave_rx_buf;

spi_slave_transaction_t spi_slave_trans;
spi_slave_interface_config_t spi_slave_cfg;
spi_bus_config_t spi_slave_bus;

// VSPI
static const uint8_t SPI_MASTER_CS = 5;
static const uint8_t SPI_MASTER_CLK = 18;
static const uint8_t SPI_MASTER_MOSI = 23;
static const uint8_t SPI_MASTER_MISO = 19;

// HSPI
static const uint8_t SPI_SLAVE_CS = 15;
static const uint8_t SPI_SLAVE_CLK = 14;
static const uint8_t SPI_SLAVE_MOSI = 13;
static const uint8_t SPI_SLAVE_MISO = 12;

// デバッグ用のダンプ関数
void dump_buf(const String &title, uint8_t *buf, uint32_t size)
{
    Serial.print(title + "(" + String(size) + "): ");

    for (uint32_t i = 0; i < size; i++)
    {
        Serial.printf("%02X ", buf[i]);
    }
    Serial.println();
}

// SPI 通信に使用するバッファの初期化
void spi_buf_init()
{
    spi_master_tx_buf = (uint8_t *)heap_caps_malloc(TRANS_SIZE, MALLOC_CAP_DMA);
    spi_master_rx_buf = (uint8_t *)heap_caps_malloc(TRANS_SIZE, MALLOC_CAP_DMA);
    spi_slave_tx_buf = (uint8_t *)heap_caps_malloc(TRANS_SIZE, MALLOC_CAP_DMA);
    spi_slave_rx_buf = (uint8_t *)heap_caps_malloc(TRANS_SIZE, MALLOC_CAP_DMA);

    for (uint32_t i = 0; i < TRANS_SIZE; i++)
    {
        spi_master_tx_buf[i] = i & 0xFF;
        spi_slave_tx_buf[i] = (0xFF - i) & 0xFF;
    }
}

// マスタとして動作させる VSPI の初期化
void spi_master_init()
{
    SPI.begin(SPI_MASTER_CLK, SPI_MASTER_MISO, SPI_MASTER_MOSI, SPI_MASTER_CS);
    SPI.setFrequency(200000);
    SPI.setDataMode(SPI_MODE3);
    SPI.setHwCs(true);
}

// スレーブの通信完了後に呼ばれるコールバック
void spi_slave_tans_done(spi_slave_transaction_t *trans)
{
    dump_buf("SLAVE", spi_slave_rx_buf, trans->trans_len / 8);

    spi_slave_queue_trans(HSPI_HOST, &spi_slave_trans, portMAX_DELAY);
}

// スレーブとして動作させる HSPI の初期化
void spi_slave_init()
{
    spi_slave_trans.length = 8 * TRANS_SIZE;
    spi_slave_trans.trans_len = 0;
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

    spi_slave_initialize(HSPI_HOST, &spi_slave_bus, &spi_slave_cfg, 1);
    spi_slave_queue_trans(HSPI_HOST, &spi_slave_trans, portMAX_DELAY);
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
    uint32_t trans_size = 16;

    Serial.println("[LOOP]");

    // SPI マスターによる通信開始 (Full Duplex)
    SPI.transferBytes(spi_master_tx_buf, spi_master_rx_buf, trans_size);

    dump_buf("MASTER", spi_master_rx_buf, trans_size);

    delay(1000);
}