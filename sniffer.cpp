#include "sniffer.h"
#include "config.h"
#include <RadioLib.h>
#include <driver/spi_master.h>

// -- Custom RadioLib HAL ---------------------------------------------------
// Shares SPI2 with LovyanGFX via IDF spi_master device handles.
// Arduino's global SPI is NEVER initialized — avoids GPIO matrix corruption.

static spi_device_handle_t g_lora_dev = nullptr;

class LoRaIdfHal : public ArduinoHal {
public:
    LoRaIdfHal() : ArduinoHal(SPI, SPISettings(2000000, MSBFIRST, SPI_MODE0)) {}

    void spiBegin() override {}   // bus owned by LovyanGFX
    void spiEnd()   override {}
    void spiBeginTransaction() override {}
    void spiEndTransaction()   override {}

    void spiTransfer(uint8_t* out, size_t len, uint8_t* in) override {
        if (!g_lora_dev || !len) return;
        spi_transaction_t t = {};
        t.length    = len * 8;
        t.tx_buffer = out;
        t.rx_buffer = in;
        spi_device_polling_transmit(g_lora_dev, &t);
    }
};

static LoRaIdfHal g_lora_hal;
static SX1262 radio = new Module(&g_lora_hal, LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY);

// -- state -----------------------------------------------------------------

uint8_t g_sniff_ch = 0;
uint8_t g_sniff_sf = 7;

static float   s_last_snr  = 0.0f;
static int16_t s_last_rssi = 0;

static uint32_t s_dwell_start = 0;
static bool     s_rx_started  = false;

static const uint8_t SF_CYCLE[] = {7, 8, 9, 10, 11, 12};
static const uint8_t SF_COUNT   = sizeof(SF_CYCLE);
static uint8_t s_sf_idx = 0;

// -- helpers ---------------------------------------------------------------

static bool radio_start_rx() {
    return radio.startReceive() == RADIOLIB_ERR_NONE;
}

static void next_channel() {
    g_sniff_ch = (g_sniff_ch + 1) % EU868_NUM_CH;
    if (g_sniff_ch == 0) {
        s_sf_idx   = (s_sf_idx + 1) % SF_COUNT;
        g_sniff_sf = SF_CYCLE[s_sf_idx];
    }
    radio.setFrequency(EU868_FREQS[g_sniff_ch]);
    radio.setSpreadingFactor(g_sniff_sf);
    s_rx_started  = false;
    s_dwell_start = millis();
}

// -- public ----------------------------------------------------------------

bool sniffer_init() {
    // Register SX1262 as IDF SPI device on SPI2 (already initialized by LovyanGFX)
    spi_device_interface_config_t devcfg = {};
    devcfg.clock_speed_hz = 2000000;
    devcfg.mode           = 0;
    devcfg.spics_io_num   = -1;   // CS managed manually by RadioLib via GPIO
    devcfg.queue_size     = 1;

    esp_err_t err = spi_bus_add_device(SPI2_HOST, &devcfg, &g_lora_dev);
    if (err != ESP_OK) {
        Serial.printf("[sniffer] spi_bus_add_device failed: %d\n", err);
        return false;
    }

    int state = radio.begin(EU868_FREQS[0], EU868_BW, g_sniff_sf,
                            EU868_CR, LORAWAN_SYNC, 17, 8);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[sniffer] radio.begin() failed: %d\n", state);
        spi_bus_remove_device(g_lora_dev);
        g_lora_dev = nullptr;
        return false;
    }
    radio.setCRC(2);
    s_dwell_start = millis();
    return true;
}

bool sniffer_tick(JoinRequest* out) {
    if (!s_rx_started) {
        radio_start_rx();
        s_rx_started = true;
    }
    if (radio.available()) {
        size_t  pkt_len = radio.getPacketLength();
        uint8_t buf[256];
        int state = radio.readData(buf, sizeof(buf));
        if (state == RADIOLIB_ERR_NONE && pkt_len > 0) {
            s_last_snr  = radio.getSNR();
            s_last_rssi = radio.getRSSI();
            if (lorawan_parse_join(buf, pkt_len, out)) {
                s_rx_started = false;
                return true;
            }
        }
        s_rx_started = false;
    }
    if (millis() - s_dwell_start >= SNIFF_DWELL_MS)
        next_channel();
    return false;
}

void sniffer_sleep() {
    radio.sleep();
    s_rx_started = false;
}

float   sniffer_snr()  { return s_last_snr;  }
int16_t sniffer_rssi() { return s_last_rssi; }
