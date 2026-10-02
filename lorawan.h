#pragma once
#include <Arduino.h>

#define JR_LEN  23

struct JoinRequest {
    uint8_t raw[JR_LEN];
    uint8_t app_eui[8];   // little-endian as transmitted
    uint8_t dev_eui[8];   // little-endian as transmitted
    uint8_t dev_nonce[2];
    uint8_t mic[4];
};

// Returns true if buf is a valid 23-byte Join Request (MType 0x00)
bool lorawan_parse_join(const uint8_t* buf, size_t len, JoinRequest* out);

// AES-128-CMAC MIC check per LoRaWAN spec
bool lorawan_check_mic(const JoinRequest* jr, const uint8_t* app_key_16);

// Hex string helpers
void bytes_to_hex(const uint8_t* b, size_t len, char* out);  // out must be 2*len+1
