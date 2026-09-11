#include "interface.h"
#include <stdio.h>

AppState_t current_state = STATE_MAIN_MENU;
uint8_t needs_refresh = 1;

// Layout constants
#define ROW_H 30
#define ROW_SPACE 5
#define ROW_0_Y 40
#define ROW_1_Y (ROW_0_Y + ROW_H + ROW_SPACE)
#define ROW_2_Y (ROW_1_Y + ROW_H + ROW_SPACE)

// Waveform viewer layout
#define WAVE_Y_START      18
#define WAVE_Y_END        (DISP_HEIGHT - 30)
#define WAVE_AREA_HEIGHT  (WAVE_Y_END - WAVE_Y_START)
#define LANE_HEIGHT       (WAVE_AREA_HEIGHT / LOGIC_NUM_CHANNELS)
#define LABEL_WIDTH       32
#define WAVE_HIGH_Y(base) ((base) + 8)
#define WAVE_LOW_Y(base)  ((base) + LANE_HEIGHT - 10)

#define WAVE_COLOR_HIGH   COLOR_SIG_HIGH
#define WAVE_COLOR_LOW    COLOR_SIG_LOW
#define WAVE_COLOR_LABEL  COLOR_SIG_LABEL
#define WAVE_COLOR_SEP    COLOR_SIG_SEP

// Lookup tables
const char* protocol_strings[] = {"I2C", "SPI", "UART"};
const char* trigger_strings[]  = {"RISING", "FALLING", "NONE"};

// Capture buffer and viewer state
CaptureBuffer_t capture;
int16_t viewer_scroll = 0;
uint8_t viewer_zoom = 1;
uint8_t viewer_needs_full_redraw = 1;

// Main Menu Buttons
Button_t main_menu_buttons[] = {
    {DISP_WIDTH/3 * 2, DISP_HEIGHT-30, DISP_WIDTH/3, ROW_H, COLOR_UI_BTN, "Options",  OnOptionsClick, 0},
    {DISP_WIDTH/3 * 1, DISP_HEIGHT-30, DISP_WIDTH/3, ROW_H, COLOR_UI_REC, "REC",      OnRedClick,     0},
    {DISP_WIDTH/3 * 0, DISP_HEIGHT-30, DISP_WIDTH/3, ROW_H, COLOR_UI_BTN, "Capture",  OnCaptureClick, 0},
};

// Settings Buttons
#define SETTINGS_CONF_SIZE 4
Button_t settings_buttons[SETTINGS_CONF_SIZE] = {
    {DISP_WIDTH/3, ROW_0_Y, (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "I2C",       OnProtocolClick, 0},
    {DISP_WIDTH/3, ROW_1_Y, (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "frequency", OnFreqClick,     0},
    {DISP_WIDTH/3, ROW_2_Y, (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "RISING",    OnProtocolClick, 0},
    {0, DISP_HEIGHT-30,     DISP_WIDTH/3,       ROW_H, COLOR_UI_BTN, "Back",      OnBackClick,     0},
};

// Viewer Buttons
#define VIEWER_BTN_W (DISP_WIDTH / 5)
Button_t viewer_buttons[5] = {
    {VIEWER_BTN_W * 0, DISP_HEIGHT - 30, VIEWER_BTN_W, 30, COLOR_UI_BTN, "<",    OnScrollLeftClick,  0},
    {VIEWER_BTN_W * 1, DISP_HEIGHT - 30, VIEWER_BTN_W, 30, COLOR_UI_BTN, ">",    OnScrollRightClick, 0},
    {VIEWER_BTN_W * 2, DISP_HEIGHT - 30, VIEWER_BTN_W, 30, COLOR_UI_BTN, "Z-",   OnZoomInClick,      0},
    {VIEWER_BTN_W * 3, DISP_HEIGHT - 30, VIEWER_BTN_W, 30, COLOR_UI_BTN, "Z+",   OnZoomOutClick,     0},
    {VIEWER_BTN_W * 4, DISP_HEIGHT - 30, VIEWER_BTN_W, 30, COLOR_UI_BTN, "Back", OnBackClick,        0},
};

View_t views[] = {
    [STATE_MAIN_MENU] = {main_menu_buttons, 3, Draw_MainMenu},
    [STATE_VIEWER]    = {viewer_buttons, 5, Draw_Viewer},
    [STATE_SETTINGS]  = {settings_buttons, SETTINGS_CONF_SIZE, Draw_Settings},
};

// Settings helpers
void UpdateSettingsMenu(void) {
    settings_buttons[0].text = (char*)protocol_strings[conf.protocol];
    settings_buttons[1].text = Config_GetFreqText(conf.frequency);

    if (conf.protocol == I2C) {
        settings_buttons[2].text = (char*)trigger_strings[0];
    } else if (conf.protocol == SPI) {
        settings_buttons[2].text = (char*)trigger_strings[1];
    } else {
        settings_buttons[2].text = (char*)trigger_strings[2];
    }
}

void WipeButtons(){
    DISP_FillRect(0, DISP_HEIGHT-30, DISP_WIDTH, 30, COLOR_BG_DARKER);
}

void WipeMain(){
    DISP_FillRect(0, 18, DISP_WIDTH, DISP_HEIGHT - 30, COLOR_BG_DARK);
}

void DrawStatusBar(){
    DISP_FillRect(0, 0, DISP_WIDTH, 18, COLOR_UI_STATUS);
    DISP_DrawCenteredText(DISP_WIDTH - 38, 0, 38, 18, "100%", COLOR_UI_STATUS_T, COLOR_UI_STATUS);
    DISP_DrawCenteredText(DISP_WIDTH/3, 0, DISP_WIDTH/3, 18, "SETTINGS", COLOR_UI_STATUS_T, COLOR_UI_STATUS);
}

// Draw callbacks
void Draw_MainMenu(){
    WipeMain();
    DrawStatusBar();
    View_t view = views[current_state];
    for (int i = 0; i < view.buttonCount; i++) {
        UI_DrawButton(&view.buttons[i]);
    }
}

static void Draw_ViewerStatic(void) {
    DISP_FillRect(0, WAVE_Y_START, DISP_WIDTH, WAVE_AREA_HEIGHT, COLOR_BG_DARK);
    DrawStatusBar();

    const ProtocolPinConfig_t *cfg = Config_GetPinConfig(conf.protocol);

    for (uint8_t ch = 0; ch < LOGIC_NUM_CHANNELS; ch++) {
        uint16_t lane_y = WAVE_Y_START + ch * LANE_HEIGHT;

        if (ch > 0) {
            DISP_DrawHLine(0, lane_y, DISP_WIDTH, WAVE_COLOR_SEP);
        }

        uint16_t label_y = lane_y + (LANE_HEIGHT / 2) - 6;
        for (uint8_t i = 0; i < 3 && cfg->channels[ch].label[i] != '\0'; i++) {
            DISP_DrawChar(2 + i * 8, label_y, cfg->channels[ch].label[i],
                          WAVE_COLOR_LABEL, COLOR_BG_DARK);
        }
    }
}

static void Draw_ViewerWaveforms(void) {
    for (uint8_t ch = 0; ch < LOGIC_NUM_CHANNELS; ch++) {
        uint16_t lane_y = WAVE_Y_START + ch * LANE_HEIGHT;
        uint16_t high_y = WAVE_HIGH_Y(lane_y);
        uint16_t low_y = WAVE_LOW_Y(lane_y);
        uint16_t wave_x_start = LABEL_WIDTH;

        uint16_t first_visible = viewer_scroll;
        uint16_t visible_samples = (DISP_WIDTH - LABEL_WIDTH) / viewer_zoom;
        if (visible_samples > capture.total_samples - first_visible) {
            visible_samples = capture.total_samples - first_visible;
        }
        if (visible_samples == 0) continue;

        uint16_t pixel_x = wave_x_start;
        uint8_t run_state = capture.samples[ch][first_visible];
        uint16_t run_len = viewer_zoom;

        for (uint16_t s = 1; s < visible_samples; s++) {
            uint16_t sample_idx = first_visible + s;
            if (sample_idx >= capture.total_samples) break;

            uint8_t cur_state = capture.samples[ch][sample_idx];

            if (cur_state == run_state) {
                run_len += viewer_zoom;
            } else {
                uint16_t run_y = run_state ? high_y : low_y;
                uint16_t run_start = pixel_x - run_len;
                uint16_t draw_len = run_len;
                if (run_start + draw_len > DISP_WIDTH) {
                    draw_len = DISP_WIDTH - run_start;
                }
                DISP_DrawHLine(run_start, run_y, draw_len,
                               run_state ? WAVE_COLOR_HIGH : WAVE_COLOR_LOW);

                uint16_t new_y = cur_state ? high_y : low_y;
                if (pixel_x < DISP_WIDTH) {
                    uint16_t top = (new_y < run_y) ? new_y : run_y;
                    uint16_t h = (new_y > run_y) ? (new_y - run_y + 1) : (run_y - new_y + 1);
                    DISP_DrawVLine(pixel_x, top, h, cur_state ? WAVE_COLOR_HIGH : WAVE_COLOR_LOW);
                }

                run_state = cur_state;
                run_len = viewer_zoom;
            }

            pixel_x += viewer_zoom;
        }

        uint16_t run_y = run_state ? high_y : low_y;
        uint16_t run_start = pixel_x - run_len;
        if (run_start < DISP_WIDTH) {
            uint16_t draw_len = run_len;
            if (run_start + draw_len > DISP_WIDTH) {
                draw_len = DISP_WIDTH - run_start;
            }
            DISP_DrawHLine(run_start, run_y, draw_len,
                           run_state ? WAVE_COLOR_HIGH : WAVE_COLOR_LOW);
        }
    }
}

void Draw_Viewer(){
    if (viewer_needs_full_redraw) {
        DISP_FillRect(0, 0, DISP_WIDTH, DISP_HEIGHT, COLOR_BG_DARK);
        Draw_ViewerStatic();
        viewer_needs_full_redraw = 0;
    } else {
        // Clear only the waveform area (skip labels/separators)
        DISP_FillRect(LABEL_WIDTH, WAVE_Y_START, DISP_WIDTH - LABEL_WIDTH, WAVE_AREA_HEIGHT, COLOR_BG_DARK);
    }

    Draw_ViewerWaveforms();

    View_t view = views[current_state];
    for (int i = 0; i < view.buttonCount; i++) {
        UI_DrawButton(&view.buttons[i]);
    }
}

void Draw_Settings(){
    WipeMain();
    DISP_FillRect(0, 40, DISP_WIDTH, 30, COLOR_UI_ACCENT);

    DISP_DrawCenteredText(0, ROW_0_Y, DISP_WIDTH/3, ROW_H, "Protocol",  COLOR_BG_DARK, COLOR_UI_ACCENT);
    DISP_DrawCenteredText(0, ROW_1_Y, DISP_WIDTH/3, ROW_H, "Frequency", COLOR_BG_DARK, COLOR_UI_ACCENT);
    DISP_DrawCenteredText(0, ROW_2_Y, DISP_WIDTH/3, ROW_H, "Trigger",   COLOR_BG_DARK, COLOR_UI_ACCENT);

    DrawStatusBar();
    WipeButtons();

    UpdateSettingsMenu();

    View_t view = views[current_state];
    for (int i = 0; i < view.buttonCount; i++) {
        UI_DrawButton(&view.buttons[i]);
    }
}

// Capture logic
void LogicAnalyzer_Capture(void) {
    const ProtocolPinConfig_t *cfg = Config_GetPinConfig(conf.protocol);
    capture.active_channels = cfg->num_active_channels;
    capture.total_samples = LOGIC_BUFFER_SIZE;

    for (uint8_t ch = 0; ch < LOGIC_NUM_CHANNELS; ch++) {
        capture.channel_labels[ch] = cfg->channels[ch].label;
    }

    for (uint16_t s = 0; s < LOGIC_BUFFER_SIZE; s++) {
        for (uint8_t ch = 0; ch < LOGIC_NUM_CHANNELS; ch++) {
            GPIO_TypeDef *port = cfg->channels[ch].port;
            uint16_t pin = cfg->channels[ch].pin;
            capture.samples[ch][s] = (port->IDR & pin) ? 1 : 0;
        }
    }
}

// Button callbacks
void OnOptionsClick(void) {
    current_state = STATE_SETTINGS;
    needs_refresh = 1;
}

void OnRedClick(void) {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
}

void OnCaptureClick(void) {
    LogicAnalyzer_Capture();
    viewer_scroll = 0;
    viewer_zoom = 1;
    viewer_needs_full_redraw = 1;
    current_state = STATE_VIEWER;
    needs_refresh = 1;
}

void OnBackClick(void) {
    current_state = STATE_MAIN_MENU;
    needs_refresh = 1;
}

void OnScrollLeftClick(void) {
    if (viewer_scroll > 0) {
        viewer_scroll -= 10;
        if (viewer_scroll < 0) viewer_scroll = 0;
        needs_refresh = 1;
    }
}

void OnScrollRightClick(void) {
    int16_t max_scroll = capture.total_samples - (DISP_WIDTH - LABEL_WIDTH) / viewer_zoom;
    if (max_scroll < 0) max_scroll = 0;
    if (viewer_scroll < max_scroll) {
        viewer_scroll += 10;
        if (viewer_scroll > max_scroll) viewer_scroll = max_scroll;
        needs_refresh = 1;
    }
}

void OnZoomInClick(void) {
    if (viewer_zoom > 1) {
        viewer_zoom--;
        needs_refresh = 1;
    }
}

void OnZoomOutClick(void) {
    if (viewer_zoom < 40) {
        viewer_zoom++;
        needs_refresh = 1;
    }
}

// Settings callbacks
void OnProtocolClick(void) {
    conf.protocol = (conf.protocol + 1) % PROTOCOL_COUNT;
    UpdateSettingsMenu();
    needs_refresh = 1;
}

void OnFreqClick(void){
    conf.frequency = (conf.frequency + 1) % FREQUENCY_COUNT;
    UpdateSettingsMenu();
    needs_refresh = 1;
}
