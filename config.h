#pragma once
#include <Arduino.h>

// -- LoRa SX1262 (verify against T-Deck Plus schematic) --------------------
#define LORA_NSS    9
#define LORA_SCK   40
#define LORA_MOSI  41
#define LORA_MISO  38
#define LORA_RST   17
#define LORA_DIO1  45
#define LORA_BUSY  13

// -- TFT ST7789 320×240 ----------------------------------------------------
#define TFT_CS_PIN   12
#define TFT_DC_PIN   11
#define TFT_RST_PIN  -1
#define TFT_BL_PIN   42

// -- I2C keyboard (BB Q20) -------------------------------------------------
#define KB_SDA   18
#define KB_SCL    8
#define KB_ADDR  0x55

// -- Battery ADC (VBAT via 1:2 voltage divider) ---------------------------
#define VBAT_ADC_PIN  4

// -- Display colors — red hacker theme (RGB565) ----------------------------
#define C_BG     0x0000u  // black background
#define C_HDBG   0x8000u  // dark red  (header / key-box bg)
#define C_TITLE  0xF800u  // bright red (accents, separators)
#define C_TEXT   0xFFFFu  // white
#define C_DIM    0x8410u  // medium gray (descriptions, dim text)
#define C_FOOT   0x1800u  // very dark red (footer bar)
#define C_WARN   0xFD20u  // orange (warnings)
#define C_KEY    0xFFE0u  // yellow (key labels)
#define C_FOUND  0x07E0u  // bright green (success)
#define C_WHITE  0xFFFFu

// -- EU868 channels --------------------------------------------------------
#define EU868_NUM_CH  8
static const float EU868_FREQS[EU868_NUM_CH] = {
    868.1f, 868.3f, 868.5f,
    867.1f, 867.3f, 867.5f, 867.7f, 867.9f
};
#define EU868_BW      125.0f
#define EU868_CR      5
#define LORAWAN_SYNC  0x34

// Channel dwell time in sniff mode (ms per channel)
#define SNIFF_DWELL_MS  600
