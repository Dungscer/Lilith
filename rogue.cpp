#include "rogue.h"

void rogue_run(const uint8_t* app_eui_8, const uint8_t* dev_eui_8,
               const uint8_t* app_key_16) {
    // TODO: RadioLib LoRaWANNode OTAA join with known credentials
    // LoRaWANNode node(&radio, &EU868);
    // node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
    // int16_t state = node.activateOTAA();
    (void)app_eui_8; (void)dev_eui_8; (void)app_key_16;
}
