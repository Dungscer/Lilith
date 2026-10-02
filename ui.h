#pragma once
#include <Arduino.h>
#include "lorawan.h"
#include "cracker.h"

// Call once in setup()
void ui_init();

// Screens
void ui_show_menu();
void ui_show_sniff(uint8_t ch, uint8_t sf);
void ui_update_sniff_packet(const JoinRequest* jr);
void ui_show_crack(uint32_t tried);
void ui_show_crack_result(const CrackResult* res);
void ui_show_rogue();
void ui_show_detect(uint32_t count);
void ui_show_error(const char* msg);

// Update channel/SF status line while in sniff/detect mode
void ui_refresh_channel(const char* buf);

// Move menu cursor (0-3)
void ui_menu_set_sel(uint8_t idx);

// Update LoRa info panel (right lower) — call when channel changes
void ui_lora_info_update(float freq, uint8_t sf);

// Call when a packet is received to update SNR/RSSI readout
void ui_lora_packet_seen(float snr, int16_t rssi);

// Refresh status bar (battery %, freq, SNR) — call periodically from loop()
void ui_status_tick();

// Show "Radio init..." in status bar during lazy radio initialization
void ui_radio_init_start();

// Update detect mode packet counter without full screen redraw
void ui_detect_update_count(uint32_t count);

// Read one key from I2C keyboard (0 = no key pressed)
uint8_t ui_read_key();
