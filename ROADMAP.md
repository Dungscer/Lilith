# ROADMAP — LilyGo T-Deck Plus / Attack Station

Portable, standalone LoRaWAN attacker device.
Menu-driven on the display, controlled from the keyboard, no laptop needed for a demo.

!!Back up the base firmware before starting work!!

---

## Hardware

| Component | Value |
|-----------|-------|
| MCU | ESP32-S3 |
| Radio | SX1262 |
| Display | TFT (ST7789, 320×240) |
| Input | QWERTY keyboard (I2C) |
| Power | battery |

**Pin mapping (verify against the schematic!):**

| Signal | GPIO |
|--------|------|
| LORA_NSS | 9 |
| LORA_SCK | 40 |
| LORA_MOSI | 41 |
| LORA_MISO | 38 |
| LORA_RST | 17 |
| LORA_DIO1 | 45 |
| LORA_BUSY | 13 |

---

## Features (order needed for the demo)

| # | Feature             | Description                                                                                                                       |
|---|---------------------|-------------------------------------------------------------------------------------------------------------------------------------|
| 1 | **Sniff mode**      | SX1262 passively listens on EU868, catches a Join Request, shows fields on the display                                             |
| 2 | **On-device crack** | ESP32-S3 AES-128-CMAC wordlist brute-force against the captured MIC → key shown on the display                                     |
| 3 | **Rogue Join**      | sends a Join Request with a known DevEUI + AppKey → receives Join Accept → derives and displays session keys                        |
| 4 | **Menu UI**         | main menu: `[S] Sniff  [C] Crack  [J] Rogue Join` — navigable with the keyboard                                                     |
| 5 | **Detect mode**      | watches for LoRaWAN traffic in the area and, if an SD card is present, logs what it finds there                                    |

---

## Tasks

| # | Task | Notes |
|---|------|-------|
| 1 | Verify pin mapping | confirm GPIO numbers against the T-Deck Plus schematic/datasheet |
| 2 | RadioLib SX1262 init | `SX1262 radio = new Module(NSS, DIO1, RST, BUSY)` |
| 3 | LoRa sniff mode | scan EU868 channels (868.1 / 868.3 / 868.5 MHz), SF7–SF12, detect Join Request (23B, MType 0x00) |
| 4 | Port LoRaWAN parser | rewrite `lorawan_parser.py` logic in C++ (extract AppEUI/DevEUI/MIC) |
| 5 | On-device AES-128-CMAC | `mbedTLS` (built into ESP-IDF) or the Arduino `Crypto` lib |
| 6 | Wordlist in flash | known weak keys loaded into `PROGMEM` |
| 7 | TFT UI | `TFT_eSPI` library, fields shown in a table, `[KEY FOUND]` screen |
| 8 | Keyboard handling | I2C keyboard polling, menu navigation |
| 9 | Rogue Join | LMIC or RadioLib LoRaWAN stack with the known AppKey |

---

## Libraries

```
RadioLib          → SX1262 sniff + raw TX/RX
TFT_eSPI          → display
LMIC / arduino-lorawan → LoRaWAN stack (for Rogue Join)
mbedTLS           → AES-128-CMAC (ESP-IDF built-in)
```

---

## Demo flow

```
[S] → T-Deck starts listening
      Heltec V2 node powers on → broadcasts a Join Request
      T-Deck display: AppEUI / DevEUI / MIC

[C] → on-device wordlist crack starts
      "Trying 1024 keys..." → "KEY FOUND: 2B7E..."

      → AppKey handed off to the M5 Cardputer (next step)
```
