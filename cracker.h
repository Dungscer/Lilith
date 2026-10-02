#pragma once
#include <Arduino.h>
#include "lorawan.h"

struct CrackResult {
    bool     found;
    uint8_t  app_key[16];
    uint32_t tried;
};

// Wordlist attack — runs to completion, returns result
CrackResult cracker_run(const JoinRequest* jr);
