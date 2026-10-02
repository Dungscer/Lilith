#define USER_SETUP_LOADED 1

// ESP32-S3: FSPI=0 → REG_SPI_BASE(0)=0 (invalid), USE_FSPI_PORT forces SPI_PORT=2
#define USE_FSPI_PORT

#define ST7789_DRIVER

// Physical panel is portrait-native 240×320; setRotation(1) gives landscape 320×240
#define TFT_WIDTH   240
#define TFT_HEIGHT  320

// Display requires INVON command (invert=true in LovyanGFX Meshtastic config)
#define TFT_INVERSION_ON

#define TFT_MOSI   41
#define TFT_SCLK   40
#define TFT_CS     12
#define TFT_DC     11
#define TFT_RST    -1
#define TFT_MISO   38

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY        40000000
#define SPI_READ_FREQUENCY    6000000
