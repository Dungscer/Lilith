#include "lorawan.h"
#include "mbedtls/aes.h"

// -- AES-128-CMAC (RFC 4493) -----------------------------------------------

static const uint8_t CMAC_RB[16] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x87
};

static void xor16(uint8_t* dst, const uint8_t* a, const uint8_t* b) {
    for (int i = 0; i < 16; i++) dst[i] = a[i] ^ b[i];
}

static void shl1(uint8_t* out, const uint8_t* in) {
    uint8_t carry = 0;
    for (int i = 15; i >= 0; i--) {
        out[i] = (uint8_t)((in[i] << 1) | carry);
        carry  = (in[i] >> 7) & 1;
    }
}

static bool aes128_cmac(const uint8_t* key, const uint8_t* msg, size_t len,
                         uint8_t* out16) {
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    if (mbedtls_aes_setkey_enc(&ctx, key, 128) != 0) {
        mbedtls_aes_free(&ctx);
        return false;
    }

    // Subkey generation
    uint8_t L[16] = {0};
    mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, L, L);

    uint8_t K1[16], K2[16];
    shl1(K1, L);
    if (L[0] & 0x80) xor16(K1, K1, CMAC_RB);
    shl1(K2, K1);
    if (K1[0] & 0x80) xor16(K2, K2, CMAC_RB);

    // Block count
    int  n    = (int)((len + 15) / 16);
    bool full = (n > 0) && (len % 16 == 0);
    if (n == 0) { n = 1; full = false; }

    // Last block (padded if needed)
    uint8_t last[16] = {0};
    size_t  rem = (full ? 16 : len % 16);
    memcpy(last, msg + (n - 1) * 16, (rem == 0 ? 16 : rem));
    if (!full && rem < 16) last[rem] = 0x80;
    xor16(last, last, full ? K1 : K2);

    // CBC-MAC
    uint8_t X[16] = {0}, Y[16];
    for (int i = 0; i < n - 1; i++) {
        xor16(Y, X, msg + i * 16);
        mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, Y, X);
    }
    xor16(Y, X, last);
    mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, Y, out16);

    mbedtls_aes_free(&ctx);
    return true;
}

// -- public API ------------------------------------------------------------

bool lorawan_parse_join(const uint8_t* buf, size_t len, JoinRequest* out) {
    if (len < JR_LEN) return false;
    if ((buf[0] & 0xE0) != 0x00) return false;

    memcpy(out->raw,       buf,      JR_LEN);
    memcpy(out->app_eui,   buf + 1,  8);
    memcpy(out->dev_eui,   buf + 9,  8);
    memcpy(out->dev_nonce, buf + 17, 2);
    memcpy(out->mic,       buf + 19, 4);
    return true;
}

bool lorawan_check_mic(const JoinRequest* jr, const uint8_t* app_key_16) {
    uint8_t msg[19];
    msg[0] = jr->raw[0];
    memcpy(msg + 1,  jr->app_eui,   8);
    memcpy(msg + 9,  jr->dev_eui,   8);
    memcpy(msg + 17, jr->dev_nonce, 2);

    uint8_t cmac[16];
    if (!aes128_cmac(app_key_16, msg, sizeof(msg), cmac)) return false;
    return memcmp(cmac, jr->mic, 4) == 0;
}

void bytes_to_hex(const uint8_t* b, size_t len, char* out) {
    static const char HCHARS[] = "0123456789ABCDEF";
    for (size_t i = 0; i < len; i++) {
        out[2*i]   = HCHARS[b[i] >> 4];
        out[2*i+1] = HCHARS[b[i] & 0x0F];
    }
    out[2*len] = '\0';
}
