#pragma once
#include <Arduino.h>
#include "lorawan.h"

// Call once in setup()
bool sniffer_init();

// Call in loop() during SNIFF / DETECT states.
// Returns true and fills *out if a Join Request was received.
// Also updates g_sniff_ch / g_sniff_sf displayed on screen.
bool sniffer_tick(JoinRequest* out);

// Current channel index and SF (for UI display)
extern uint8_t g_sniff_ch;
extern uint8_t g_sniff_sf;

// Put radio to sleep (call when leaving sniff mode)
void sniffer_sleep();

// Last received packet signal quality (valid after sniffer_tick() returns true)
float   sniffer_snr();
int16_t sniffer_rssi();
