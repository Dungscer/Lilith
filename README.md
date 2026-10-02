# Lilith — LilyGo T-Deck Plus LoRaWAN Attack Station

Portable, standalone LoRaWAN testing/attack device built for thesis
research (LoRaWAN security testing, on a private EU868 lab setup, against
my own nodes). Menu-driven on the built-in display, controlled from the
I2C keyboard — no laptop needed for a demo.

Previously lived as part of a larger research repo
(`LoRaWAN_Testing/Firmwares/LilyGo_T_Deck_Plus/Lilith/`), split out into
its own repo on 2026-10-02.

## Hardware

| Component | Value |
|-----------|-------|
| MCU | ESP32-S3 |
| Radio | SX1262 |
| Display | TFT ST7789 320×240 |
| Input | QWERTY keyboard (I2C, BB Q20, 0x55) |
| Power | battery |

Pin mapping lives in `config.h` (taken from the T-Deck Plus schematic);
design notes and details are in [`ROADMAP.md`](ROADMAP.md).

## Modes

- `[S] Sniff` — passive EU868 listener, catches Join Requests
- `[C] Crack` — on-device AES-128-CMAC wordlist brute-force against the captured MIC
- `[J] Rogue` — Rogue Join with a known/cracked AppKey (stub)
- `[D] Detect` — logs all LoRaWAN traffic

## Build & flash

Arduino IDE / arduino-cli, ESP32-S3 target. After building:

```bash
./flash_lilith.sh [port]   # default port: /dev/ttyUSB0
```

## Status

**Written, not yet verified on real hardware.** The `config.h` pin
mapping and the sniff/crack/rogue flow still need to be checked on an
actual T-Deck Plus; Rogue mode is currently a stub.
