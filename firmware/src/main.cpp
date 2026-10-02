// OpenSigns v0.2 -- Pitch calling with real audio clips
// M5StickC Plus2 -> A2DP -> Earbuds
// BtnA (front): cycle pitch type
// BtnB (side):  send selected pitch audio to earbuds
#include <Arduino.h>
#include <M5Unified.h>
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include <string.h>

// Audio clips (44100Hz 16-bit stereo PCM in PROGMEM)
#include "audio/fb_wav.h"
#include "audio/cb_wav.h"
#include "audio/sl_wav.h"
#include "audio/ch_wav.h"

// --- Config ---
static const char* TARGET_NAME = "V80013";

// --- Pitch definitions ---
struct PitchDef {
    const char* name;
    const char* abbrev;
    uint32_t color;
    const int16_t* pcm_data;
    int pcm_len; // total int16_t samples (stereo pairs * 2)
};

static const PitchDef PITCHES[] = {
    {"Fastball",  "FB",  0xFF2222, fb_pcm, fb_pcm_len},
    {"Curveball", "CB",  0x22CC22, cb_pcm, cb_pcm_len},
    {"Slider",    "SL",  0x2288FF, sl_pcm, sl_pcm_len},
    {"Changeup",  "CH",  0xFFAA00, ch_pcm, ch_pcm_len},
};
static const int NUM_PITCHES = sizeof(PITCHES) / sizeof(PITCHES[0]);

// --- State ---
static bool s_connected = false;
static bool s_audio_started = false;
static esp_bd_addr_t s_peer_addr;
static bool init_ok = false;

static int selected_pitch = 0;
static bool needs_redraw = true;

// UI state machine: SELECTING shows pitch options, SENT shows confirmation only
enum UiState { UI_SELECTING, UI_SENT };
static UiState ui_state = UI_SELECTING;

// Audio playback state
static volatile const int16_t* play_buf = nullptr;
static volatile int play_len = 0;
static volatile int play_pos = 0;

// Button debounce
#define BTN_A 37
#define BTN_B 39
static const unsigned long DEBOUNCE_MS = 200;

// Init log
static char init_log[512] = {0};
static int init_log_len = 0;
#define LOG(fmt, ...) do { \
    int n = snprintf(init_log + init_log_len, sizeof(init_log) - init_log_len, fmt "\n", ##__VA_ARGS__); \
    if (n > 0) init_log_len += n; \
    Serial.printf(fmt "\n", ##__VA_ARGS__); \
} while(0)

// --- Audio callback ---
// Called by A2DP stack to fill the output buffer.
// Streams from play_buf when active, silence otherwise.
static int32_t audio_data_cb(uint8_t* data, int32_t len) {
    if (len <= 0 || data == nullptr) return 0;
    int16_t* out = (int16_t*)data;
    int frames_requested = len / 4; // stereo 16-bit = 4 bytes per frame

    if (play_buf != nullptr && play_pos < play_len) {
        int frames_available = (play_len - play_pos) / 2; // 2 int16 per stereo frame
        int frames_to_copy = min(frames_requested, frames_available);

        // Copy from PROGMEM
        for (int i = 0; i < frames_to_copy * 2; i++) {
            out[i] = (int16_t)pgm_read_word(&play_buf[play_pos + i]);
        }
        play_pos += frames_to_copy * 2;

        // Zero-fill remainder if clip ended mid-buffer
        if (frames_to_copy < frames_requested) {
            memset(out + frames_to_copy * 2, 0, (frames_requested - frames_to_copy) * 4);
            play_buf = nullptr;
        }
    } else {
        memset(data, 0, len);
        if (play_buf != nullptr) {
            play_buf = nullptr; // clip finished
        }
    }
    return len;
}

// --- Display ---
static void draw_ui() {
    auto& lcd = M5.Lcd;
    lcd.fillScreen(TFT_BLACK);

    // Connection status bar
    lcd.setTextSize(1);
    lcd.setTextColor(s_connected ? TFT_GREEN : TFT_RED);
    lcd.setCursor(4, 4);
    lcd.printf("BT: %s", s_connected ? "CONNECTED" : "SCANNING...");

    if (s_audio_started) {
        lcd.setTextColor(TFT_GREEN);
        lcd.setCursor(lcd.width() - 60, 4);
        lcd.print("AUDIO OK");
    }

    if (ui_state == UI_SENT) {
        // Confirmation screen -- no pitch info visible to opposing team
        lcd.setTextSize(3);
        lcd.setTextColor(TFT_GREEN);
        int16_t tw = lcd.textWidth("PITCH");
        lcd.setCursor((lcd.width() - tw) / 2, 40);
        lcd.print("PITCH");
        tw = lcd.textWidth("SENT");
        lcd.setCursor((lcd.width() - tw) / 2, 75);
        lcd.print("SENT");

        // Button label: only A to return to selection
        lcd.setTextSize(1);
        lcd.setTextColor(TFT_YELLOW);
        const char* label = "[A] New Pitch";
        int16_t lw = lcd.textWidth(label);
        lcd.setCursor((lcd.width() - lw) / 2, lcd.height() - 14);
        lcd.print(label);
    } else {
        // SELECTING: show pitch choice
        const PitchDef& p = PITCHES[selected_pitch];
        lcd.setTextSize(3);
        lcd.setTextColor(p.color);
        int16_t tw = lcd.textWidth(p.name);
        lcd.setCursor((lcd.width() - tw) / 2, 40);
        lcd.print(p.name);

        lcd.setTextSize(2);
        lcd.setTextColor(TFT_WHITE);
        tw = lcd.textWidth(p.abbrev);
        lcd.setCursor((lcd.width() - tw) / 2, 80);
        lcd.print(p.abbrev);

        lcd.setTextSize(1);
        lcd.setTextColor(TFT_DARKGREY);
        char buf[16];
        snprintf(buf, sizeof(buf), "%d / %d", selected_pitch + 1, NUM_PITCHES);
        tw = lcd.textWidth(buf);
        lcd.setCursor((lcd.width() - tw) / 2, 105);
        lcd.print(buf);

        lcd.setTextSize(1);
        lcd.setTextColor(TFT_YELLOW);
        lcd.setCursor(4, lcd.height() - 14);
        lcd.print("[A] Next");
        lcd.setCursor(lcd.width() - 52, lcd.height() - 14);
        lcd.print("[B] Send");
    }

    needs_redraw = false;
}

// --- GAP callback ---
static void gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t* param) {
    switch (event) {
    case ESP_BT_GAP_DISC_RES_EVT: {
        char name[64] = {0};
        bool has_name = false;
        for (int i = 0; i < param->disc_res.num_prop; i++) {
            if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_BDNAME &&
                param->disc_res.prop[i].len > 0) {
                strncpy(name, (char*)param->disc_res.prop[i].val, sizeof(name) - 1);
                has_name = true;
            }
        }
        if (!has_name) {
            esp_bt_gap_read_remote_name(param->disc_res.bda);
            break;
        }
        Serial.printf("[SCAN] \"%s\"\n", name);
        if (strstr(name, TARGET_NAME) != nullptr) {
            Serial.println("[SCAN] MATCH! Connecting...");
            memcpy(s_peer_addr, param->disc_res.bda, ESP_BD_ADDR_LEN);
            esp_bt_gap_cancel_discovery();
            esp_a2d_source_connect(s_peer_addr);
        }
        break;
    }
    case ESP_BT_GAP_READ_REMOTE_NAME_EVT: {
        if (param->read_rmt_name.stat == ESP_BT_STATUS_SUCCESS) {
            char rname[64] = {0};
            strncpy(rname, (char*)param->read_rmt_name.rmt_name, sizeof(rname) - 1);
            Serial.printf("[SCAN] Remote name: \"%s\"\n", rname);
            if (strstr(rname, TARGET_NAME) != nullptr) {
                Serial.println("[SCAN] MATCH! Connecting...");
                memcpy(s_peer_addr, param->read_rmt_name.bda, ESP_BD_ADDR_LEN);
                esp_bt_gap_cancel_discovery();
                esp_a2d_source_connect(s_peer_addr);
            }
        } else {
            Serial.println("[SCAN] Name request failed");
        }
        break;
    }
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
            Serial.println("[SCAN] Stopped");
            if (!s_connected) {
                delay(2000);
                Serial.println("[SCAN] Restarting...");
                esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
            }
        } else {
            Serial.println("[SCAN] Started");
        }
        break;
    default:
        Serial.printf("[GAP] evt=%d\n", event);
        break;
    }
}

// --- A2DP callback ---
static void a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t* param) {
    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT:
        Serial.printf("[A2DP] conn state=%d\n", param->conn_stat.state);
        s_connected = (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED);
        needs_redraw = true;
        if (s_connected) {
            Serial.println("[A2DP] Connected! Checking source ready...");
            esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_CHECK_SRC_RDY);
        } else if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
            s_audio_started = false;
            esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
        }
        break;
    case ESP_A2D_AUDIO_STATE_EVT:
        Serial.printf("[A2DP] audio state=%d\n", param->audio_stat.state);
        if (param->audio_stat.state == ESP_A2D_AUDIO_STATE_STARTED) {
            s_audio_started = true;
            Serial.println("[A2DP] STREAMING READY");
        } else {
            s_audio_started = false;
        }
        needs_redraw = true;
        break;
    case ESP_A2D_MEDIA_CTRL_ACK_EVT:
        Serial.printf("[A2DP] media ctrl ack: cmd=%d status=%d\n",
            param->media_ctrl_stat.cmd, param->media_ctrl_stat.status);
        if (param->media_ctrl_stat.cmd == ESP_A2D_MEDIA_CTRL_CHECK_SRC_RDY &&
            param->media_ctrl_stat.status == ESP_A2D_MEDIA_CTRL_ACK_SUCCESS) {
            Serial.println("[A2DP] Source ready, starting stream...");
            esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_START);
        }
        break;
    default:
        Serial.printf("[A2DP] evt=%d\n", event);
        break;
    }
}

// --- Setup ---
void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);
    delay(3000);

    M5.Lcd.setRotation(1);
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 40);
    M5.Lcd.print("OpenSigns v0.2");
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(10, 70);
    M5.Lcd.print("Initializing BT...");

    pinMode(BTN_A, INPUT_PULLUP);
    pinMode(BTN_B, INPUT_PULLUP);

    LOG("=== OPENSIGNS v0.2 ===");
    LOG("Heap: %d", ESP.getFreeHeap());
    LOG("Audio clips: FB=%d CB=%d SL=%d CH=%d samples",
        fb_pcm_len, cb_pcm_len, sl_pcm_len, ch_pcm_len);

    if (!btStart()) {
        LOG("ERROR: btStart() failed!");
        return;
    }
    LOG("btStart OK");

    esp_err_t ret;
    ret = esp_bluedroid_init();
    LOG("bluedroid init: %s", esp_err_to_name(ret));
    if (ret != ESP_OK) return;

    ret = esp_bluedroid_enable();
    LOG("bluedroid enable: %s", esp_err_to_name(ret));
    if (ret != ESP_OK) return;

    esp_bt_dev_set_device_name("OpenSigns");

    esp_bt_gap_register_callback(gap_cb);
    esp_a2d_register_callback(a2d_cb);
    esp_a2d_source_register_data_callback(audio_data_cb);

    ret = esp_a2d_source_init();
    LOG("a2d source init: %s", esp_err_to_name(ret));
    if (ret != ESP_OK) return;

    ret = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    LOG("scan mode: %s", esp_err_to_name(ret));

    LOG("Starting discovery...");
    ret = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
    LOG("discovery start: %s", esp_err_to_name(ret));

    init_ok = (ret == ESP_OK);
    LOG("Heap after init: %d", ESP.getFreeHeap());

    needs_redraw = true;
}

// --- Main loop ---
void loop() {
    M5.update();

    // Init log replay for late-connecting monitors
    static bool log_printed = false;
    if (!log_printed && millis() > 8000) {
        Serial.println("--- INIT LOG REPLAY ---");
        Serial.print(init_log);
        Serial.println("--- END REPLAY ---");
        log_printed = true;
    }

    // Button A behavior depends on state:
    //   SELECTING -> cycle to next pitch
    //   SENT      -> return to selection (reset to first pitch for consistency)
    static unsigned long btnA_last = 0;
    if (digitalRead(BTN_A) == LOW && millis() - btnA_last > DEBOUNCE_MS) {
        btnA_last = millis();
        if (ui_state == UI_SENT) {
            ui_state = UI_SELECTING;
            selected_pitch = 0;
            Serial.println("[UI] New pitch cycle -- back to selection");
        } else {
            selected_pitch = (selected_pitch + 1) % NUM_PITCHES;
            Serial.printf("[UI] Selected: %s\n", PITCHES[selected_pitch].name);
        }
        needs_redraw = true;
    }

    // Button B: send pitch call audio (only in SELECTING state)
    static unsigned long btnB_last = 0;
    if (digitalRead(BTN_B) == LOW && millis() - btnB_last > DEBOUNCE_MS) {
        btnB_last = millis();
        if (ui_state != UI_SELECTING) {
            // Ignore B in confirmation state
        } else if (s_connected && s_audio_started) {
            const PitchDef& p = PITCHES[selected_pitch];
            play_buf = p.pcm_data;
            play_len = p.pcm_len;
            play_pos = 0;
            ui_state = UI_SENT;
            needs_redraw = true;
            Serial.printf("[SEND] %s (%d samples) -> SENT screen\n", p.name, p.pcm_len);
        } else {
            Serial.println("[SEND] Not ready (no BT audio)");
        }
    }

    // Redraw when state changes
    if (needs_redraw) {
        draw_ui();
    }

    // Periodic status
    static unsigned long last = 0;
    if (millis() - last > 5000) {
        Serial.printf("[%lus] heap=%d conn=%d audio=%d pitch=%s playing=%d\n",
            millis()/1000, ESP.getFreeHeap(), s_connected, s_audio_started,
            PITCHES[selected_pitch].abbrev, play_buf != nullptr);
        last = millis();
    }

    delay(10);
}
