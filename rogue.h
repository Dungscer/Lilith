#pragma once
#include <Arduino.h>

// Rogue Join: send a Join Request with known DevEUI + AppKey,
// receive Join Accept, derive and display session keys.
// Requires cracked AppKey from cracker_run().
//
// TODO: implement with RadioLib LoRaWANNode (ROADMAP task #9)

void rogue_run(const uint8_t* app_eui_8, const uint8_t* dev_eui_8,
               const uint8_t* app_key_16);
