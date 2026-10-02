#pragma once
#include <LovyanGFX.hpp>

// T-Deck Plus display config — matches Meshtastic original firmware (LovyanGFX)
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 _panel;
    lgfx::Bus_SPI      _bus;
    lgfx::Light_PWM    _light;

public:
    LGFX() {
        {
            auto cfg = _bus.config();
            cfg.spi_host   = SPI2_HOST;  // FSPI on ESP32-S3
            cfg.freq_write = 40000000;
            cfg.freq_read  =  6000000;
            cfg.pin_sclk   = 40;
            cfg.pin_mosi   = 41;
            cfg.pin_miso   = 38;   // needed: IDF bus must have MISO for LoRa read ops
            cfg.pin_dc     = 11;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg = _panel.config();
            cfg.pin_cs      = 12;
            cfg.pin_rst     = -1;
            cfg.panel_width = 240;
            cfg.panel_height= 320;
            cfg.invert      = true;   // ST7789 on T-Deck needs INVON
            cfg.rgb_order   = false;
            _panel.config(cfg);
        }
        {
            auto cfg = _light.config();
            cfg.pin_bl      = -1;   // manual control in setup() — GPIO 42 shared with KB
            cfg.invert      = false;
            cfg.freq        = 44100;
            cfg.pwm_channel = 0;
            _light.config(cfg);
            _panel.setLight(&_light);
        }
        setPanel(&_panel);
    }
};
