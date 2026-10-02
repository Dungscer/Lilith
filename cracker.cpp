#include "cracker.h"
#include "wordlist.h"

CrackResult cracker_run(const JoinRequest* jr) {
    CrackResult res = {false, {0}, 0};
    uint8_t key[16];

    for (size_t i = 0; i < WORDLIST_LEN; i++) {
        memcpy_P(key, WORDLIST[i], 16);
        res.tried++;
        if (lorawan_check_mic(jr, key)) {
            res.found = true;
            memcpy(res.app_key, key, 16);
            return res;
        }
    }
    return res;
}
