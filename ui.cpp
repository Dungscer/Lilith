#include "ui.h"
#include "config.h"
#include "lgfx_config.h"
#include "lilith_demon.h"
#include "lilith_logo.h"
#include <Wire.h>

static LGFX tft;

// -- layout ----------------------------------------------------------------
// Menu screen (split)
#define PX      141   // demon girl panel width
#define RW      179   // right panel width  (320-141)
#define LOGO_H   59   // logo box height (y 0-58)
#define RP_Y     59   // right panel top — starts immediately under logo box
#define RP_H    161   // right panel height (59-219)
#define ITEM_H   40   // menu item height (161/4)

// Active mode screens (full screen)
#define ACT_TH   16   // title bar height
#define ACT_CY   16   // content area top
#define ACT_CW  320   // content area width
#define ACT_CH  204   // content area height (220-16)

// Status bar
#define STAT_Y  220
#define STAT_H   20

// -- LoRa state ------------------------------------------------------------
static float   s_freq    = 868.1f;
static uint8_t s_sf      = 7;
static float   s_snr     = 0.0f;
static int16_t s_rssi    = 0;
static bool    s_got_pkt = false;

// -- menu cursor -----------------------------------------------------------
static uint8_t s_menu_sel = 0;

// forward
static void _stat_draw();

// -- battery ---------------------------------------------------------------

static uint8_t _batt_pct() {
    int mv   = analogReadMilliVolts(VBAT_ADC_PIN);
    int vbat = mv * 2;
    if (vbat >= 4200) return 100;
    if (vbat <= 3300) return 0;
    return (uint8_t)((vbat - 3300) * 100 / 900);
}

// -- status bar ------------------------------------------------------------

static void _stat_draw() {
    tft.fillRect(0, STAT_Y + 1, 320, STAT_H - 1, C_FOOT);
    tft.setFont(&lgfx::fonts::Font2);

    char buf[40];

    // Left: freq + SF
    snprintf(buf, sizeof(buf), "((o)) %.1fMHz SF%d", s_freq, s_sf);
    tft.setTextColor(C_DIM, C_FOOT);
    tft.setCursor(4, STAT_Y + 6);
    tft.print(buf);

    // Center: SNR + RSSI (only if packet seen)
    if (s_got_pkt) {
        snprintf(buf, sizeof(buf), "SNR%.1f  RSSI%d", s_snr, (int)s_rssi);
        tft.setTextColor(C_FOUND, C_FOOT);
        tft.setCursor(138, STAT_Y + 6);
        tft.print(buf);
    }

    // Right: battery %
    uint8_t pct = _batt_pct();
    snprintf(buf, sizeof(buf), "%d%%", (int)pct);
    // right-align: 4 chars max = 24px; start at 320-4-24=292
    tft.setTextColor(pct <= 20 ? C_WARN : C_DIM, C_FOOT);
    tft.setCursor(292, STAT_Y + 6);
    tft.print(buf);
}

void ui_status_tick() { _stat_draw(); }

// -- init ------------------------------------------------------------------

void ui_init() {
    pinMode(10, OUTPUT); digitalWrite(10, HIGH);
    pinMode(42, OUTPUT); digitalWrite(42, HIGH);
    analogSetAttenuation(ADC_11db);
    delay(200);   // keyboard controller (ESP32-C3) boot time

    Wire.begin(KB_SDA, KB_SCL);
    Wire.setTimeout(10);  // 10ms I2C timeout
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(C_BG);

    // Static left panel — drawn once here before radio init
    tft.drawBitmap(0, 0, LILITH_DEMON, LILITH_DEMON_W, LILITH_DEMON_H,
                   C_TITLE, C_BG);

    // Logo box
    tft.drawRect(PX,   0,   RW,    LOGO_H,   C_TITLE);
    tft.drawRect(PX+2, 2,   RW-4,  LOGO_H-4, C_HDBG);
    int lx = PX + (RW - LILITH_LOGO_W) / 2;
    int ly = (LOGO_H - LILITH_LOGO_H) / 2;
    tft.drawBitmap(lx, ly, LILITH_LOGO, LILITH_LOGO_W, LILITH_LOGO_H,
                   C_TEXT, C_BG);
    tft.drawFastHLine(PX, RP_Y, RW, C_TITLE);

    // Status bar
    tft.fillRect(0, STAT_Y, 320, STAT_H, C_FOOT);
    tft.drawFastHLine(0, STAT_Y, 320, C_TITLE);
    _stat_draw();
    // Right panel menu area filled by ui_show_menu() in setup()
}

// -- keyboard --------------------------------------------------------------

uint8_t ui_read_key() {
    // Poll at most every 20ms to avoid hammering the I2C bus
    static uint32_t last_ms  = 0;
    static uint8_t  prev_key = 0;
    uint32_t now = millis();
    if (now - last_ms < 20) return 0;
    last_ms = now;

    Wire.requestFrom((uint8_t)KB_ADDR, (uint8_t)1);
    uint8_t k = Wire.available() ? Wire.read() : 0;
    if (k) Serial.printf("[key] 0x%02X '%c'\n", k, k >= 32 && k < 127 ? k : '?');

    // Edge detection: only return key on press (0 → k), not while held
    uint8_t result = (k && !prev_key) ? k : 0;
    prev_key = k;
    return result;
}

// -- menu screen -----------------------------------------------------------

struct MenuItem {
    const char* name;
    const char* key;
};

static const MenuItem MENU_ITEMS[4] = {
    { "SNIFF",      "S" },
    { "CRACK",      "C" },
    { "ROGUE JOIN", "J" },
    { "DETECT",     "D" },
};

static void _menu_rp_draw() {
    // Clear right panel in strips — safe for post-radio-init LGFX calls
    for (int sy = 0; sy < STAT_Y; sy += 40)
        tft.fillRect(PX, sy, RW, (sy + 40 > STAT_Y ? STAT_Y - sy : 40), C_BG);

    // Logo box
    tft.drawRect(PX,   0,   RW,    LOGO_H,   C_TITLE);
    tft.drawRect(PX+2, 2,   RW-4,  LOGO_H-4, C_HDBG);
    int lx = PX + (RW - LILITH_LOGO_W) / 2;
    int ly = (LOGO_H - LILITH_LOGO_H) / 2;
    tft.drawBitmap(lx, ly, LILITH_LOGO, LILITH_LOGO_W, LILITH_LOGO_H,
                   C_TEXT, C_BG);

    // Separator
    tft.drawFastHLine(PX, RP_Y, RW, C_TITLE);

    // 4 menu items — name only, no descriptions
    for (int i = 0; i < 4; i++) {
        int y   = RP_Y + i * ITEM_H;
        bool sel = (i == s_menu_sel);

        if (i > 0) tft.drawFastHLine(PX, y, RW, C_HDBG);
        if (sel) tft.fillRect(PX+1, y+1, RW-2, ITEM_H-2, C_HDBG);

        tft.setFont(&lgfx::fonts::Font2);

        // Cursor
        tft.setTextColor(sel ? C_TITLE : C_BG, sel ? C_HDBG : C_BG);
        tft.setCursor(PX + 4, y + 16);
        tft.print(">");

        // Mode name
        tft.setTextColor(sel ? C_KEY : C_TEXT, sel ? C_HDBG : C_BG);
        tft.setCursor(PX + 16, y + 16);
        tft.print(MENU_ITEMS[i].name);

        // Key shortcut — right aligned
        tft.setTextColor(sel ? C_TEXT : C_DIM, sel ? C_HDBG : C_BG);
        tft.setCursor(PX + RW - 16, y + 16);
        tft.print(MENU_ITEMS[i].key);
    }

    tft.fillRect(0, STAT_Y, 320, STAT_H, C_FOOT);
    tft.drawFastHLine(0, STAT_Y, 320, C_TITLE);
    _stat_draw();
}

void ui_show_menu() {
    s_menu_sel = 0;
    _menu_rp_draw();
}

void ui_menu_set_sel(uint8_t idx) {
    s_menu_sel = idx;
    _menu_rp_draw();
}

// -- active mode helpers ---------------------------------------------------

// Draws title bar + black content area; returns y for first content line
static void _act_begin(const char* title, uint16_t bar_col) {
    tft.fillRect(0, 0, 320, 220, C_BG);

    // Title bar
    tft.fillRect(0, 0, 320, ACT_TH, bar_col);
    tft.setFont(&lgfx::fonts::Font2);
    tft.setTextColor(C_BG, bar_col);
    tft.setCursor(6, 4);
    tft.print(title);

    // ESC hint on the right of title bar
    tft.setTextColor(C_BG, bar_col);
    tft.setCursor(248, 4);
    tft.print("[ESC=back]");

    // Status bar
    tft.fillRect(0, STAT_Y, 320, STAT_H, C_FOOT);
    tft.drawFastHLine(0, STAT_Y, 320, C_TITLE);
    _stat_draw();
}

static void _act_label(int x, int y, const char* lbl, const char* val,
                        uint16_t vcol = C_TEXT) {
    tft.setFont(&lgfx::fonts::Font2);
    tft.setTextColor(C_DIM, C_BG);
    tft.setCursor(x, y);
    tft.print(lbl);
    tft.setTextColor(vcol, C_BG);
    tft.setCursor(x + (int)(strlen(lbl) * 6), y);
    tft.print(val);
}

// -- sniff mode ------------------------------------------------------------

void ui_show_sniff(uint8_t ch, uint8_t sf) {
    s_freq = EU868_FREQS[ch];
    s_sf   = sf;

    _act_begin("SNIFF  —  EU868 passive listen", C_HDBG);

    int y = ACT_CY + 8;
    char buf[32];

    tft.drawFastHLine(8, y, 304, C_HDBG);
    y += 4;

    snprintf(buf, sizeof(buf), "%.1f MHz  SF%d  BW125  CR4/5", EU868_FREQS[ch], sf);
    _act_label(8, y, "Channel: ", buf, C_TEXT);
    y += 14;

    tft.setFont(&lgfx::fonts::Font2);
    tft.setTextColor(C_WARN, C_BG);
    tft.setCursor(8, y);
    tft.print("Listening...");
    y += 16;

    tft.drawFastHLine(8, y, 304, C_HDBG);
    y += 6;

    tft.setFont(&lgfx::fonts::Font2);
    tft.setTextColor(C_DIM, C_BG);
    tft.setCursor(8, y); tft.print("Last packet:");
    y += 12;
    tft.setCursor(8, y); tft.print("AppEUI: --              ");
    y += 12;
    tft.setCursor(8, y); tft.print("DevEUI: --              ");
    y += 12;
    tft.setCursor(8, y); tft.print("MIC:    --              ");
    y += 16;

    tft.drawFastHLine(8, y, 304, C_HDBG);
    y += 6;
    tft.setTextColor(C_DIM, C_BG);
    tft.setCursor(8, y); tft.print("SNR: --     RSSI: --");

    _stat_draw();
}

void ui_update_sniff_packet(const JoinRequest* jr) {
    // Packet section starts after the fixed header rows
    // y = ACT_CY+8+4+14+14+16+6 = 78
    int y = ACT_CY + 62;
    char hex[33];

    tft.fillRect(0, y, 320, 60, C_BG);
    tft.setFont(&lgfx::fonts::Font2);

    tft.setTextColor(C_FOUND, C_BG);
    tft.setCursor(8, y); tft.print("JOIN REQUEST:");
    y += 12;

    bytes_to_hex(jr->app_eui, 8, hex);
    _act_label(8, y, "AppEUI: ", hex);  y += 12;

    bytes_to_hex(jr->dev_eui, 8, hex);
    _act_label(8, y, "DevEUI: ", hex);  y += 12;

    bytes_to_hex(jr->mic, 4, hex);
    _act_label(8, y, "MIC:    ", hex, C_WARN);
}

void ui_refresh_channel(const char* buf) {
    // Overwrite the channel line: y = ACT_CY+8+4 = 28
    int y = ACT_CY + 18;
    tft.fillRect(0, y, 320, 12, C_BG);
    _act_label(8, y, "Channel: ", buf, C_TEXT);

    float freq = s_freq; uint8_t sf = s_sf;
    sscanf(buf, "%f MHz SF%hhu", &freq, &sf);
    s_freq = freq; s_sf = sf;
    _stat_draw();
}

// -- crack mode ------------------------------------------------------------

void ui_show_crack(uint32_t tried) {
    _act_begin("CRACK  —  AES-128-CMAC wordlist", C_HDBG);

    int y = ACT_CY + 12;
    tft.setFont(&lgfx::fonts::Font2);
    tft.setTextColor(C_WARN, C_BG);
    tft.setCursor(8, y); tft.print("Cracking AppKey...");
    y += 14;

    char buf[32];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)tried);
    _act_label(8, y, "Tried: ", buf, C_TEXT);
    y += 14;

    tft.setTextColor(C_DIM, C_BG);
    tft.setCursor(8, y); tft.print("MIC: CMAC-AES128(AppKey, data)");
}

void ui_show_crack_result(const CrackResult* res) {
    if (!res->found) {
        _act_begin("CRACK  —  Result", C_HDBG);
        int y = ACT_CY + 12;
        tft.setFont(&lgfx::fonts::Font2);
        tft.setTextColor(C_WARN, C_BG);
        tft.setCursor(8, y); tft.print("No key found.");
        y += 14;
        char buf[32];
        snprintf(buf, sizeof(buf), "%lu keys tried", (unsigned long)res->tried);
        tft.setTextColor(C_DIM, C_BG);
        tft.setCursor(8, y); tft.print(buf);
        return;
    }

    _act_begin("CRACK  —  KEY FOUND!", C_FOUND);
    int y = ACT_CY + 12;
    tft.setFont(&lgfx::fonts::Font2);

    tft.setTextColor(C_FOUND, C_BG);
    tft.setCursor(8, y); tft.print("AppKey cracked!");
    y += 16;

    char hex[33];
    bytes_to_hex(res->app_key,   8, hex);
    tft.setTextColor(C_KEY, C_BG);
    tft.setCursor(8, y); tft.print(hex);
    y += 12;
    bytes_to_hex(res->app_key+8, 8, hex);
    tft.setCursor(8, y); tft.print(hex);
    y += 18;

    tft.setTextColor(C_DIM, C_BG);
    char buf[40];
    snprintf(buf, sizeof(buf), "%lu keys tried", (unsigned long)res->tried);
    tft.setCursor(8, y); tft.print(buf);
}

// -- rogue join mode -------------------------------------------------------

void ui_show_rogue() {
    _act_begin("ROGUE JOIN  —  Fake gateway", C_HDBG);

    int y = ACT_CY + 12;
    tft.setFont(&lgfx::fonts::Font2);
    tft.setTextColor(C_WARN, C_BG);
    tft.setCursor(8, y); tft.print("Prerequisite:");
    y += 12;
    tft.setTextColor(C_TEXT, C_BG);
    tft.setCursor(8, y); tft.print("Cracked AppKey required.");
    y += 14;
    tft.setTextColor(C_DIM, C_BG);
    tft.setCursor(8, y); tft.print("1. [S] Sniff  —  capture Join Request");
    y += 12;
    tft.setCursor(8, y); tft.print("2. [C] Crack  —  recover AppKey");
    y += 12;
    tft.setCursor(8, y); tft.print("3. [J] Rogue  —  restart to retry");
}

// -- detect mode -----------------------------------------------------------

void ui_show_detect(uint32_t count) {
    _act_begin("DETECT  —  Traffic monitor", C_HDBG);

    int y = ACT_CY + 12;
    tft.setFont(&lgfx::fonts::Font2);
    tft.setTextColor(C_DIM, C_BG);
    tft.setCursor(8, y); tft.print("Packets captured:");
    y += 14;

    tft.setFont(&lgfx::fonts::Font4);
    char buf[16];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)count);
    tft.setTextColor(count ? C_FOUND : C_DIM, C_BG);
    tft.setCursor(8, y); tft.print(buf);
    y += 22;

    tft.setFont(&lgfx::fonts::Font2);
    tft.setTextColor(C_DIM, C_BG);
    tft.setCursor(8, y); tft.print("See serial output for details.");
}

// -- error screen ----------------------------------------------------------

void ui_show_error(const char* msg) {
    _act_begin("ERROR", C_WARN);
    tft.setFont(&lgfx::fonts::Font2);
    tft.setTextColor(C_TEXT, C_BG);
    tft.setCursor(8, ACT_CY + 12);
    tft.print(msg);
}

// -- LoRa state updates ----------------------------------------------------

void ui_radio_init_start() {
    tft.fillRect(0, STAT_Y + 1, 180, STAT_H - 1, C_FOOT);
    tft.setFont(&lgfx::fonts::Font2);
    tft.setTextColor(C_WARN, C_FOOT);
    tft.setCursor(4, STAT_Y + 6);
    tft.print("Radio init...");
}

void ui_detect_update_count(uint32_t count) {
    int y = ACT_CY + 26;
    tft.fillRect(0, y, 120, 22, C_BG);
    tft.setFont(&lgfx::fonts::Font4);
    char buf[16];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)count);
    tft.setTextColor(count ? C_FOUND : C_DIM, C_BG);
    tft.setCursor(8, y);
    tft.print(buf);
}

void ui_lora_info_update(float freq, uint8_t sf) {
    s_freq = freq;
    s_sf   = sf;
    _stat_draw();
}

void ui_lora_packet_seen(float snr, int16_t rssi) {
    s_snr     = snr;
    s_rssi    = rssi;
    s_got_pkt = true;
    _stat_draw();
}
