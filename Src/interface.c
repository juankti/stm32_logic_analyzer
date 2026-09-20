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
#define GET_LANE_HEIGHT(n)       (WAVE_AREA_HEIGHT / ((n) ? (n) : 1))
#define LABEL_WIDTH       36
#define WAVE_HIGH_Y(base) ((base) + 8)
#define WAVE_LOW_Y(base, lh)  ((base) + (lh) - 10)

#define WAVE_COLOR_HIGH   COLOR_SIG_HIGH
#define WAVE_COLOR_LOW    COLOR_SIG_LOW
#define WAVE_COLOR_LABEL  COLOR_SIG_LABEL
#define WAVE_COLOR_SEP    COLOR_SIG_SEP

// Lookup tables

const char* trigger_strings[]  = {"RISING", "FALLING", "NONE"};

// Capture buffer and viewer state
CaptureBuffer_t capture;
uint32_t viewer_t_scroll = 0;
uint8_t viewer_zoom = 6;   // index into zoom_divisors[]; 6 -> divisor=10.0f (~100px per 9600-baud frame)
uint8_t viewer_needs_full_redraw = 1;
uint8_t encoder_zoom_mode = 0;
int8_t selected_button_idx = -1;
bool has_captured = false;

extern void OnBackToViewerClick(void);
extern void OnSizeClick(void);

// Main Menu Buttons
Button_t main_menu_buttons[5] = {
    {150, 40,  120, 35, COLOR_UI_BTN, "Freq",  OnFreqClick, 0},
    {150, 85,  120, 35, COLOR_UI_BTN, "Size",  OnSizeClick, 0},
    {150, 130, 120, 35, COLOR_UI_BTN, "Config", OnConfigClick, 0},
    {150, 175, 120, 35, COLOR_UI_REC, "REC",   OnCaptureClick, 0},
    {0,   210, 80,  30, COLOR_UI_BTN, "Back", OnBackToViewerClick, 0},
};

// Settings Buttons
#define SETTINGS_CONF_SIZE 5
Button_t settings_buttons[SETTINGS_CONF_SIZE] = {
    {DISP_WIDTH/3, ROW_0_Y, (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "I2C",       OnProtocolClick, 0},
    {DISP_WIDTH/3, ROW_1_Y, (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "frequency", OnFreqClick,     0},
    {DISP_WIDTH/3, ROW_2_Y, (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "RISING",    OnTriggerClick, 0},
    {DISP_WIDTH/3, ROW_2_Y + ROW_H + 5, (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "HEX", OnRadixClick, 0},
    {0, DISP_HEIGHT-30,     DISP_WIDTH/3,       ROW_H, COLOR_UI_BTN, "Back",      OnBackClick,     0},
};

// Viewer Buttons
#define VIEWER_BTN_W (DISP_WIDTH / 4)
Button_t viewer_buttons[5] = {
    {VIEWER_BTN_W * 0, DISP_HEIGHT - 30, VIEWER_BTN_W, 30, COLOR_UI_BTN, "Config",  OnConfigClick,      0},
    {VIEWER_BTN_W * 1, DISP_HEIGHT - 30, VIEWER_BTN_W, 30, COLOR_UI_BTN, "<",    OnScrollLeftClick,  0},
    {VIEWER_BTN_W * 2, DISP_HEIGHT - 30, VIEWER_BTN_W, 30, COLOR_UI_BTN, ">",    OnScrollRightClick, 0},
    {VIEWER_BTN_W * 3, DISP_HEIGHT - 30, VIEWER_BTN_W, 30, COLOR_UI_BTN, "Back", OnBackClick,        0},
    {0,                0,                30,           18, COLOR_UI_BTN, "S",    OnModeToggleClick,  0},
};

// Protocol Config Buttons
extern void OnUARTLsbInvClick(void);
Button_t protocol_config_buttons[7] = {
    {DISP_WIDTH/3, ROW_0_Y, (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "I2C",       OnProtocolClick, 0},
    {DISP_WIDTH/3, ROW_1_Y, (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "Setting",   OnSettingClick,  0},
    {DISP_WIDTH/3, ROW_2_Y, (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "HEX",       OnRadixClick, 0},
    {DISP_WIDTH/3, ROW_2_Y + ROW_H + 5, (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "8N1",       OnUARTParamsClick, 0},
    {DISP_WIDTH/3, ROW_2_Y + 2*(ROW_H + 5), (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "LSB/Norm",  OnUARTLsbInvClick, 0},
    {0, DISP_HEIGHT-30,     DISP_WIDTH/3,       ROW_H, COLOR_UI_BTN, "Back",      OnConfigBackClick, 0},
    {DISP_WIDTH/3*2, DISP_HEIGHT-30, DISP_WIDTH/3, ROW_H, COLOR_UI_BTN, "Decode", OnDecodeClick,     0},
};

// Table View Buttons
Button_t table_view_buttons[1] = {
    {0, DISP_HEIGHT-30, DISP_WIDTH/3, ROW_H, COLOR_UI_BTN, "Back", OnConfigBackClick, 0},
};

View_t views[] = {
    [STATE_MAIN_MENU]       = {main_menu_buttons, 5, Draw_MainMenu},
    [STATE_VIEWER]          = {viewer_buttons, 5, Draw_Viewer},
    [STATE_SETTINGS]        = {settings_buttons, SETTINGS_CONF_SIZE, Draw_Settings},
    [STATE_PROTOCOL_CONFIG] = {protocol_config_buttons, 7, Draw_ProtocolConfig},
    [STATE_TABLE_VIEW]      = {table_view_buttons, 1, Draw_TableView},
};

// Settings helpers
void UpdateSettingsMenu(void) {
    settings_buttons[0].text = (char*)Config_GetProtocolText(conf.protocol);
    settings_buttons[1].text = (char*)Config_GetFreqText(conf.frequency);
    settings_buttons[2].text = (char*)trigger_strings[config_global.flanco];
    settings_buttons[3].text = (char*)Config_GetRadixText(conf.radix);
}

void WipeButtons(){
    DISP_FillRect(0, DISP_HEIGHT-30, DISP_WIDTH, 30, COLOR_BG_DARKER);
}

void WipeMain(){
    DISP_FillRect(0, 18, DISP_WIDTH, DISP_HEIGHT - 30, COLOR_BG_DARK);
}

void DrawStatusBar(){
    DISP_FillRect(0, 0, DISP_WIDTH, 18, COLOR_UI_STATUS);
    
    const char* title = "SETTINGS";
    switch(current_state) {
        case STATE_MAIN_MENU: title = "MAIN MENU"; break;
        case STATE_VIEWER: title = "SIGNAL"; break;
        case STATE_SETTINGS: title = "SETTINGS"; break;
        case STATE_PROTOCOL_CONFIG: title = "ANALYZER CONFIGURATION"; break;
        case STATE_TABLE_VIEW: title = "DATA TABLE"; break;
    }
    DISP_DrawCenteredText(DISP_WIDTH/3, 0, DISP_WIDTH/3, 18, title, COLOR_UI_STATUS_T, COLOR_UI_STATUS);
}

// Draw callbacks
void Draw_MainMenu(){
    DISP_FillRect(0, 0, DISP_WIDTH, DISP_HEIGHT, COLOR_BG_DARK);
    
    // Labels on the left, buttons on the right
    DISP_DrawCenteredText(0, 47, 140, 20, "Sampling Freq", COLOR_SIG_LABEL, COLOR_BG_DARK);
    DISP_DrawCenteredText(0, 92, 140, 20, "Buffer Size (B)", COLOR_SIG_LABEL, COLOR_BG_DARK);
    
    main_menu_buttons[0].text = (char*)Config_GetFreqText(conf.frequency);
    main_menu_buttons[1].text = (char*)Config_GetSizeText(conf.buffer_size);
    
    if (has_captured) {
        views[STATE_MAIN_MENU].buttonCount = 5;
    } else {
        views[STATE_MAIN_MENU].buttonCount = 4;
    }
    
    View_t view = views[current_state];
    for (int i = 0; i < view.buttonCount; i++) {
        view.buttons[i].isActive = (i == selected_button_idx);
        UI_DrawButton(&view.buttons[i]);
    }
}

static void Draw_ViewerStatic(void) {
    DISP_FillRect(0, WAVE_Y_START, DISP_WIDTH, WAVE_AREA_HEIGHT, COLOR_BG_DARK);
    DrawStatusBar();

    const ProtocolPinConfig_t *cfg = Config_GetPinConfig(conf.protocol);
    uint16_t lane_height = GET_LANE_HEIGHT(cfg->num_active_channels);

    for (uint8_t ch = 0; ch < cfg->num_active_channels; ch++) {
        uint16_t lane_y = WAVE_Y_START + ch * lane_height;

        if (ch > 0) {
            DISP_DrawHLine(0, lane_y, DISP_WIDTH, WAVE_COLOR_SEP);
        }

        uint16_t label_y = lane_y + (lane_height / 2) - 6;
        for (uint8_t i = 0; i < 4 && cfg->channels[ch].label[i] != '\0'; i++) {
            DISP_DrawChar(2 + i * 8, label_y, cfg->channels[ch].label[i],
                          WAVE_COLOR_LABEL, COLOR_BG_DARK);
        }
    }
}


// Array of zoom divisors. Index 0 is 1/1 (no zoom out), higher indices zoom out more.
const float zoom_divisors[] = {
    0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f, 50000.0f, 100000.0f
};
#define MAX_ZOOM_IDX (sizeof(zoom_divisors)/sizeof(zoom_divisors[0]) - 1)

static void Draw_ViewerWaveforms(void) {
    extern volatile uint32_t buffer_timestamps[];
    uint32_t t_scroll = viewer_t_scroll;
    
    uint16_t first_visible = 0;
    for (uint16_t i = 0; i < capture.total_samples; i++) {
        if (buffer_timestamps[i] <= t_scroll) {
            first_visible = i;
        } else {
            break;
        }
    }

    if (viewer_zoom > MAX_ZOOM_IDX) viewer_zoom = MAX_ZOOM_IDX;
    float time_divisor = zoom_divisors[viewer_zoom];

    const ProtocolPinConfig_t *cfg = Config_GetPinConfig(conf.protocol);
    uint16_t lane_height = GET_LANE_HEIGHT(cfg->num_active_channels);

    // --- Draw waveform signals ---
    for (uint8_t ch = 0; ch < cfg->num_active_channels; ch++) {
        uint16_t lane_y = WAVE_Y_START + ch * lane_height;
        uint16_t high_y = WAVE_HIGH_Y(lane_y);
        uint16_t low_y  = WAVE_LOW_Y(lane_y, lane_height);

        if (cfg->num_active_channels == 1) {
            uint16_t orig_amp = low_y - high_y;
            uint16_t mid_y = high_y + orig_amp / 2;
            high_y = mid_y - orig_amp / 4;
            low_y  = mid_y + orig_amp / 4;
        }

        uint8_t  run_state = capture.samples[ch][first_visible];
        uint16_t run_x0    = LABEL_WIDTH;
        uint16_t run_y     = run_state ? high_y : low_y;

        for (uint16_t s = first_visible + 1; s < capture.total_samples; s++) {
            // Using int32_t for diff to handle wrap-around gracefully
            int32_t t_diff = (int32_t)(buffer_timestamps[s] - t_scroll);
            if (t_diff < 0) t_diff = 0; // fallback if somehow negative
            
            uint32_t px32 = LABEL_WIDTH + (t_diff) / time_divisor;
            uint16_t pixel_x = (px32 > DISP_WIDTH) ? DISP_WIDTH : (uint16_t)px32;

            uint8_t cur_state = capture.samples[ch][s];

            if (cur_state != run_state || pixel_x >= DISP_WIDTH) {
                // Draw horizontal run
                uint16_t run_len = pixel_x - run_x0;
                if (run_len > 0) {
                    DISP_DrawHLine(run_x0, run_y, run_len, run_state ? WAVE_COLOR_HIGH : WAVE_COLOR_LOW);
                }

                // Draw vertical transition edge
                if (pixel_x < DISP_WIDTH && cur_state != run_state) {
                    uint16_t new_y = cur_state ? high_y : low_y;
                    uint16_t top   = (new_y < run_y) ? new_y : run_y;
                    uint16_t h     = (new_y > run_y) ? (new_y - run_y + 1) : (run_y - new_y + 1);
                    DISP_DrawVLine(pixel_x, top, h, cur_state ? WAVE_COLOR_HIGH : WAVE_COLOR_LOW);
                }

                run_state = cur_state;
                run_x0    = pixel_x;
                run_y     = cur_state ? high_y : low_y;
            }

            if (pixel_x >= DISP_WIDTH) break;
        }

        // Flush remaining if we didn't reach edge of screen
        if (run_x0 < DISP_WIDTH) {
            DISP_DrawHLine(run_x0, run_y, DISP_WIDTH - run_x0, run_state ? WAVE_COLOR_HIGH : WAVE_COLOR_LOW);
        }
    }

    // --- Draw Decoder: bit-grid + packet delimiters + HEX label ---
    extern volatile uint32_t decoded_idx;
    extern volatile uint8_t  decoded_buffer[];
    extern volatile uint32_t decoded_start_timestamps[];
    extern volatile uint32_t decoded_end_timestamps[];

    // Compute bit width in ticks.
    // TIM2 PSC=83 → TIMER_FREQ_HZ = 84MHz/84 = 1 MHz (1 tick = 1 µs)
    // This MUST match the same formula used in uart.c: TIMER_FREQ_HZ / baudrate
    uint32_t baud = 0;
    if (conf.protocol == UART) {
        baud = config_global.protocolo.settings.uart.baudrate;
    }

    for (uint32_t i = 0; i < decoded_idx; i++) {
        uint32_t t_start = decoded_start_timestamps[i];
        uint32_t t_end   = decoded_end_timestamps[i];

        // Compute pixel positions
        int32_t ds = (int32_t)(t_start - t_scroll);
        int32_t de = (int32_t)(t_end   - t_scroll);

        if (conf.protocol == SPI) {
            uint8_t data_size = config_global.protocolo.settings.spi.data_size;
            if (data_size > 1 && t_end > t_start) {
                uint32_t dt_bit = (t_end - t_start) / (data_size - 1);
                ds -= (int32_t)(dt_bit / 2);
                de += (int32_t)(dt_bit / 2);
            }
        }

        // Skip if entirely before viewport
        if (de < 0) continue;

        uint16_t px_start = (ds < 0) ? LABEL_WIDTH
                          : (uint16_t)((uint32_t)ds / time_divisor + LABEL_WIDTH);
        uint16_t px_end   = (uint16_t)((uint32_t)(de > 0 ? de : 0) / time_divisor + LABEL_WIDTH);

        if (px_start >= DISP_WIDTH) continue;   // entirely beyond viewport
        if (px_end   >  DISP_WIDTH) px_end = DISP_WIDTH;

        // Determine active lane for gridlines
        uint16_t lane_top = WAVE_Y_START;
        uint16_t lane_bottom;
        
        if (conf.protocol == SPI) {
            lane_bottom = WAVE_Y_START + cfg->num_active_channels * lane_height;
        } else {
            lane_bottom = lane_top + lane_height;
        }

        // 1. Bit-width grid (gray dotted vertical lines every ticks_per_bit)
        if (conf.protocol == UART && baud > 0) {
            uint8_t parity_bits = (config_global.protocolo.settings.uart.paridad == UART_PARITY_SIN) ? 0 : 1;
            uint8_t stop_bits = (config_global.protocolo.settings.uart.stop_bits == UART_STOPBITS_2) ? 2 : 1;
            uint8_t total_bits = 1 + config_global.protocolo.settings.uart.data_bits + parity_bits + stop_bits;
            
            for (uint8_t bit = 1; bit < total_bits; bit++) { // start at 1, don't draw over start/end red lines
                int32_t dt_bit = (int32_t)((t_start + (uint32_t)(((uint64_t)bit * TIMER_FREQ_HZ) / baud)) - t_scroll);
                if (dt_bit < 0) continue;
                uint16_t px_bit = (uint16_t)((uint32_t)dt_bit / time_divisor + LABEL_WIDTH);
                if (px_bit >= DISP_WIDTH) break;
                // Draw every other pixel for dashed effect
                for (uint16_t yy = lane_top; yy < lane_bottom; yy += 2) {
                    DISP_DrawVLine(px_bit, yy, 1, 0x632C);
                }
            }
        } else if (conf.protocol == SPI) {
            uint8_t data_size = config_global.protocolo.settings.spi.data_size;
            if (data_size > 1 && t_end > t_start) {
                uint32_t dt_bit = (t_end - t_start) / (data_size - 1);
                for (uint8_t bit = 1; bit < data_size; bit++) {
                    int32_t dt_grid = (int32_t)((t_start + bit * dt_bit - dt_bit / 2) - t_scroll);
                    if (dt_grid < 0) continue;
                    uint16_t px_bit = (uint16_t)((uint32_t)dt_grid / time_divisor + LABEL_WIDTH);
                    if (px_bit >= DISP_WIDTH) break;
                    // Draw every other pixel for dashed effect
                    for (uint16_t yy = lane_top; yy < lane_bottom; yy += 2) {
                        DISP_DrawVLine(px_bit, yy, 1, 0x632C);
                    }
                }
            }
        }

        // 2. Red dashed vertical line at packet start and end
        for (uint16_t yy = lane_top; yy < lane_bottom; yy += 4) {
            if (px_start < DISP_WIDTH) {
                DISP_DrawVLine(px_start, yy, 2, COLOR_UI_REC);
            }
            if (px_end < DISP_WIDTH) {
                // Don't draw the end line if there's another packet immediately following it
                uint8_t draw_end = 1;
                if (i + 1 < decoded_idx && decoded_start_timestamps[i+1] == t_end) {
                    draw_end = 0;
                }
                if (draw_end) DISP_DrawVLine(px_end, yy, 2, COLOR_UI_REC);
            }
        }

        // 3. HEX label drawn on top of signal in first lane — no fill, text only
        uint16_t label_width = px_end - px_start;
        if (label_width > 10) {
            char valStr[5];
            uint8_t radix_mode = conf.radix;
            if (radix_mode == RADIX_HEX)
                snprintf(valStr, sizeof(valStr), "%02X", decoded_buffer[i]);
            else if (radix_mode == RADIX_DEC)
                snprintf(valStr, sizeof(valStr), "%u",  decoded_buffer[i]);
            else {
                uint8_t c = decoded_buffer[i];
                if (c >= 32 && c <= 126)
                    snprintf(valStr, sizeof(valStr), "'%c'", c);
                else
                    snprintf(valStr, sizeof(valStr), "%02X", c);
            }
            // Draw centered in the packet span on the top of wave area (just below status bar)
            uint16_t label_y = WAVE_Y_START + 2;
            DISP_DrawString(px_start + 2, label_y, valStr, COLOR_UI_ACCENT, COLOR_BG_DARK);
        }
    }
}



void Draw_Viewer(){
    uint8_t full_redraw_copy = viewer_needs_full_redraw;
    if (viewer_needs_full_redraw) {
        DISP_FillRect(0, 0, DISP_WIDTH, DISP_HEIGHT, COLOR_BG_DARK);
        Draw_ViewerStatic();
        viewer_needs_full_redraw = 0;
    } else {
        // Clear only the waveform area (skip labels/separators)
        DISP_FillRect(LABEL_WIDTH, WAVE_Y_START, DISP_WIDTH - LABEL_WIDTH, WAVE_AREA_HEIGHT, COLOR_BG_DARK);
    }

    Draw_ViewerWaveforms();

    // Update zoom/scroll mode button
    viewer_buttons[4].text = encoder_zoom_mode ? "Z" : "S";

    // Only draw buttons again if we did a full redraw, otherwise they might flicker
    // except for the mode toggle button which might have changed text
    static uint8_t last_encoder_zoom_mode = 0xFF;
    if (full_redraw_copy) {
        View_t view = views[current_state];
        for (int i = 0; i < view.buttonCount; i++) {
            view.buttons[i].isActive = (i == selected_button_idx);
        UI_DrawButton(&view.buttons[i]);
        }
    } else if (last_encoder_zoom_mode != encoder_zoom_mode) {
        UI_DrawButton(&viewer_buttons[4]);
    }
    last_encoder_zoom_mode = encoder_zoom_mode;
}

void Draw_Settings(){
    WipeMain();
    DISP_FillRect(0, 40, DISP_WIDTH, 30, COLOR_UI_ACCENT);

    DISP_DrawCenteredText(0, ROW_0_Y, DISP_WIDTH/3, ROW_H, "Protocol",  COLOR_BG_DARK, COLOR_UI_ACCENT);
    DISP_DrawCenteredText(0, ROW_1_Y, DISP_WIDTH/3, ROW_H, "Frequency", COLOR_BG_DARK, COLOR_UI_ACCENT);
    DISP_DrawCenteredText(0, ROW_2_Y, DISP_WIDTH/3, ROW_H, "Trigger",   COLOR_BG_DARK, COLOR_UI_ACCENT);
    DISP_DrawCenteredText(0, ROW_2_Y + ROW_H + 5, DISP_WIDTH/3, ROW_H, "Radix",     COLOR_BG_DARK, COLOR_UI_ACCENT);

    DrawStatusBar();
    WipeButtons();

    UpdateSettingsMenu();

    View_t view = views[current_state];
    for (int i = 0; i < view.buttonCount; i++) {
        view.buttons[i].isActive = (i == selected_button_idx);
        UI_DrawButton(&view.buttons[i]);
    }
}

void Draw_ProtocolConfig(){
    DISP_FillRect(0, 0, DISP_WIDTH, DISP_HEIGHT, COLOR_BG_DARK);
    DISP_FillRect(0, 40, DISP_WIDTH, 30, COLOR_UI_ACCENT);

    DISP_DrawCenteredText(0, ROW_0_Y, DISP_WIDTH/3, ROW_H, "Protocol",  COLOR_BG_DARK, COLOR_UI_ACCENT);
    
    // Update buttons texts based on configuration
    protocol_config_buttons[0].text = (char*)Config_GetProtocolText(conf.protocol);
    
    if (conf.protocol == UART) {
        DISP_DrawCenteredText(0, ROW_1_Y, DISP_WIDTH/3, ROW_H, "Baudrate", COLOR_SIG_LABEL, COLOR_BG_DARK);
        protocol_config_buttons[1].text = config_global.protocolo.settings.uart.baudrate == 9600 ? "9600" : "115200";
    } else if (conf.protocol == SPI) {
        DISP_DrawCenteredText(0, ROW_1_Y, DISP_WIDTH/3, ROW_H, "Mode", COLOR_SIG_LABEL, COLOR_BG_DARK);
        static char spi_mode_str[16];
        snprintf(spi_mode_str, sizeof(spi_mode_str), "Mode %d", config_global.protocolo.settings.spi.mode);
        protocol_config_buttons[1].text = spi_mode_str;
    } else {
        DISP_DrawCenteredText(0, ROW_1_Y, DISP_WIDTH/3, ROW_H, "Setting", COLOR_SIG_LABEL, COLOR_BG_DARK);
        protocol_config_buttons[1].text = "N/A";
    }

    DISP_DrawCenteredText(0, ROW_2_Y, DISP_WIDTH/3, ROW_H, "Radix", COLOR_SIG_LABEL, COLOR_BG_DARK);
    protocol_config_buttons[2].text = (char*)Config_GetRadixText(conf.radix);
    
    if (conf.protocol == UART) {
        views[STATE_PROTOCOL_CONFIG].buttonCount = 7;
        DISP_DrawCenteredText(0, ROW_2_Y + ROW_H + 5, DISP_WIDTH/3, ROW_H, "Format", COLOR_SIG_LABEL, COLOR_BG_DARK);
        
        static char uart_fmt_str[8];
        char p = config_global.protocolo.settings.uart.paridad == UART_PARITY_SIN ? 'N' : (config_global.protocolo.settings.uart.paridad == UART_PARITY_PAR ? 'E' : 'O');
        char s = config_global.protocolo.settings.uart.stop_bits == UART_STOPBITS_2 ? '2' : '1';
        snprintf(uart_fmt_str, sizeof(uart_fmt_str), "%d%c%c", config_global.protocolo.settings.uart.data_bits, p, s);
        protocol_config_buttons[3].text = uart_fmt_str;
        
        DISP_DrawCenteredText(0, ROW_2_Y + 2*(ROW_H + 5), DISP_WIDTH/3, ROW_H, "LSB/Inv", COLOR_SIG_LABEL, COLOR_BG_DARK);
        
        static char uart_lsb_str[16];
        snprintf(uart_lsb_str, sizeof(uart_lsb_str), "%s %s", config_global.protocolo.settings.uart.lsb_first ? "LSB" : "MSB", config_global.protocolo.settings.uart.inverted ? "INV" : "NRM");
        protocol_config_buttons[4].text = uart_lsb_str;
    } else {
        views[STATE_PROTOCOL_CONFIG].buttonCount = 7;
        protocol_config_buttons[3].text = "";
        protocol_config_buttons[4].text = "";
    }

    DrawStatusBar();

    View_t view = views[current_state];
    for (int i = 0; i < view.buttonCount; i++) {
        if (view.buttons[i].text[0] != '\0') {
            view.buttons[i].isActive = (i == selected_button_idx);
        UI_DrawButton(&view.buttons[i]);
        }
    }
}

extern volatile uint8_t decoded_buffer[];
extern volatile uint32_t decoded_idx;

int16_t table_scroll = 0;   // row index of first visible decoded entry

void Draw_TableView(){
    extern volatile uint32_t decoded_start_timestamps[];
    WipeMain();
    DrawStatusBar();
    WipeButtons();

    #define TABLE_ROW_H   14
    #define TABLE_FIRST_Y 38
    #define TABLE_ROWS_VISIBLE  ((DISP_HEIGHT - 40 - TABLE_FIRST_Y) / TABLE_ROW_H)

    if (decoded_idx == 0) {
        DISP_DrawCenteredText(0, 50, DISP_WIDTH, 20, "No data decoded yet.", COLOR_SIG_LABEL, COLOR_BG_DARK);
    } else {
        // Clamp scroll
        int16_t max_scroll = (int16_t)decoded_idx - TABLE_ROWS_VISIBLE;
        if (max_scroll < 0) max_scroll = 0;
        if (table_scroll > max_scroll) table_scroll = max_scroll;
        if (table_scroll < 0) table_scroll = 0;

        char buf[64];
        uint16_t row_y = TABLE_FIRST_Y;
        
        // Draw Header
        DISP_DrawString(5, row_y, "#    Time(t)      HEX  ASCII", COLOR_UI_ACCENT, COLOR_BG_DARK);
        row_y += TABLE_ROW_H;

        for (uint32_t i = (uint32_t)table_scroll; i < decoded_idx; i++) {
            if (row_y + TABLE_ROW_H > DISP_HEIGHT - 30) break;

            char ascii_c = (decoded_buffer[i] >= 32 && decoded_buffer[i] <= 126) ? decoded_buffer[i] : '.';
            snprintf(buf, sizeof(buf), "%-4lu %-12lu 0x%02X  %c", (unsigned long)i,
                     decoded_start_timestamps[i], decoded_buffer[i], ascii_c);
            
            DISP_DrawString(5, row_y, buf, COLOR_SIG_LABEL, COLOR_BG_DARK);
            row_y += TABLE_ROW_H;
        }

        // Scrollbar
        if (decoded_idx > (uint32_t)TABLE_ROWS_VISIBLE) {
            uint16_t bar_h = (DISP_HEIGHT - 40 - TABLE_FIRST_Y) * TABLE_ROWS_VISIBLE / decoded_idx;
            if (bar_h < 5) bar_h = 5;
            uint16_t bar_y = TABLE_FIRST_Y + (DISP_HEIGHT - 40 - TABLE_FIRST_Y - bar_h) * table_scroll / max_scroll;
            DISP_DrawVLine(DISP_WIDTH - 2, bar_y, bar_h, COLOR_UI_ACCENT);
        }
    }
    
    View_t view = views[current_state];
    for (int i = 0; i < view.buttonCount; i++) {
        view.buttons[i].isActive = (i == selected_button_idx);
        UI_DrawButton(&view.buttons[i]);
    }
}

// Capture logic
void LogicAnalyzer_Capture(void) {
    const ProtocolPinConfig_t *cfg = Config_GetPinConfig(conf.protocol);
    capture.active_channels = cfg->num_active_channels;
    capture.total_samples = Config_GetSizeValue(conf.buffer_size);

    for (uint8_t ch = 0; ch < LOGIC_NUM_CHANNELS; ch++) {
        capture.channel_labels[ch] = cfg->channels[ch].label;
    }

    for (uint16_t s = 0; s < capture.total_samples; s++) {
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

extern volatile bool start_capture_flag;

void OnCaptureClick(void) {
    start_capture_flag = true;
    DISP_FillRect(0, 0, DISP_WIDTH, DISP_HEIGHT, COLOR_BG_DARK);
    DISP_DrawCenteredText(0, DISP_HEIGHT/2 - 10, DISP_WIDTH, 20, "Capturing...", COLOR_UI_ACCENT, COLOR_BG_DARK);
}

void OnBackClick(void) {
    current_state = STATE_MAIN_MENU;
    needs_refresh = 1;
}

void OnBackToViewerClick(void) {
    current_state = STATE_VIEWER;
    viewer_needs_full_redraw = 1;
    needs_refresh = 1;
}


AppState_t prev_config_state = STATE_MAIN_MENU;

void OnConfigClick(void) {
    prev_config_state = current_state;
    current_state = STATE_PROTOCOL_CONFIG;
    needs_refresh = 1;
}


void OnConfigBackClick(void) {
    current_state = prev_config_state;
    if (current_state == STATE_VIEWER) {
        viewer_needs_full_redraw = 1;
    }
    needs_refresh = 1;
}

extern volatile uint32_t decoded_idx;
extern volatile uint16_t buffer_states[];
extern volatile uint16_t decoded_start_indices[];

void FocusOnPacket(int32_t packet_idx);

void OnDecodeClick(void) {
    decoded_idx = 0; // Reset decodings
    
    config_global.protocolo.protocolo_activo = conf.protocol; // Sync the protocol
    
    extern void Configurar_Disparo_Protocolo(void);
    Configurar_Disparo_Protocolo(); // Update pin assignments for the new protocol
    
    switch (conf.protocol) {
        case UART:
            UART_FSM((uint16_t*)buffer_states, capture.total_samples, &config_global.protocolo.settings.uart);
            break;
        case SPI:
            SPI_FSM((uint16_t*)buffer_states, capture.total_samples, &config_global.protocolo.settings.spi);
            break;
        case I2C:
            I2C_FSM((uint16_t*)buffer_states, capture.total_samples, &config_global.protocolo.settings.i2c);
            break;
        default: break;
    }
    
    if (decoded_idx > 0) {
        FocusOnPacket(0);
    }
    
    current_state = STATE_VIEWER;
    viewer_needs_full_redraw = 1;
    needs_refresh = 1;
}

void OnSettingClick(void) {
    if (conf.protocol == UART) {
        if (config_global.protocolo.settings.uart.baudrate == 9600) {
            config_global.protocolo.settings.uart.baudrate = 115200;
        } else {
            config_global.protocolo.settings.uart.baudrate = 9600;
        }
    } else if (conf.protocol == SPI) {
        config_global.protocolo.settings.spi.mode = (config_global.protocolo.settings.spi.mode + 1) % 4;
    }
    needs_refresh = 1;
}

void OnTableClick(void) {
    if(current_state == STATE_TABLE_VIEW) {
        current_state = STATE_VIEWER;
        viewer_needs_full_redraw = 1;
    } else {
        current_state = STATE_TABLE_VIEW;
        table_scroll = 0;  // start at first entry
    }
    needs_refresh = 1;
}

volatile uint8_t refresh_zoom_btn = 0;

void OnModeToggleClick(void) {
    encoder_zoom_mode = !encoder_zoom_mode;
    needs_refresh = 1;
}

void FocusOnPacket(int32_t packet_idx) {
    extern volatile uint32_t decoded_start_timestamps[];
    extern volatile uint32_t decoded_end_timestamps[];

    if (packet_idx < 0 || packet_idx >= decoded_idx) return;

    uint32_t t_start = decoded_start_timestamps[packet_idx];
    uint32_t t_end = decoded_end_timestamps[packet_idx];
    uint32_t dt = t_end - t_start;

    // We want the packet to take up about 250 pixels.
    float target_divisor = (float)dt / 250.0f;
    if (target_divisor <= 0.0f) target_divisor = 0.1f;

    uint8_t best_zoom = MAX_ZOOM_IDX;
    for (uint8_t i = 0; i <= MAX_ZOOM_IDX; i++) {
        if (zoom_divisors[i] >= target_divisor) {
            best_zoom = i;
            break;
        }
    }
    
    viewer_zoom = best_zoom;

    // Leave a small margin on the left (e.g. 20 pixels)
    uint32_t margin_time = (uint32_t)(20.0f * zoom_divisors[best_zoom]);
    uint32_t target_t_scroll = (t_start > margin_time) ? (t_start - margin_time) : 0;
    
    viewer_t_scroll = target_t_scroll;
    viewer_needs_full_redraw = 1;
    needs_refresh = 1;
}

int32_t GetCurrentPacketIndex(void) {
    extern volatile uint32_t buffer_timestamps[];
    extern volatile uint32_t decoded_idx;
    extern volatile uint32_t decoded_start_timestamps[];
    extern volatile uint32_t decoded_end_timestamps[];

    if (decoded_idx == 0) return -1;
    
    uint32_t t_center = viewer_t_scroll + (uint32_t)(((DISP_WIDTH - LABEL_WIDTH) / 2.0f) * zoom_divisors[viewer_zoom]);
    
    int32_t best_idx = 0;
    uint32_t min_dist = 0xFFFFFFFF;
    
    for (uint32_t i = 0; i < decoded_idx; i++) {
        uint32_t mid = decoded_start_timestamps[i] + (decoded_end_timestamps[i] - decoded_start_timestamps[i])/2;
        uint32_t dist = (t_center > mid) ? (t_center - mid) : (mid - t_center);
        if (dist < min_dist) {
            min_dist = dist;
            best_idx = i;
        }
    }
    return best_idx;
}

void OnScrollLeftClick(void) {
    int32_t cur = GetCurrentPacketIndex();
    if (cur > 0) {
        FocusOnPacket(cur - 1);
    } else if (cur == 0) {
        FocusOnPacket(0);
    }
}

void OnScrollRightClick(void) {
    int32_t cur = GetCurrentPacketIndex();
    extern volatile uint32_t decoded_idx;
    if (cur >= 0 && cur < decoded_idx - 1) {
        FocusOnPacket(cur + 1);
    } else if (cur == decoded_idx - 1) {
        FocusOnPacket(cur);
    }
}

void OnEncoderScrollLeft(void) {
    if (current_state == STATE_MAIN_MENU || current_state == STATE_PROTOCOL_CONFIG || current_state == STATE_SETTINGS) {
        View_t view = views[current_state];
        do {
            if (selected_button_idx <= 0) {
                selected_button_idx = view.buttonCount - 1;
            } else {
                selected_button_idx--;
            }
        } while (view.buttons[selected_button_idx].text[0] == '\0');
        needs_refresh = 1;
        return;
    }
    if (current_state == STATE_TABLE_VIEW) {
        if (table_scroll > 0) {
            table_scroll--;
            needs_refresh = 1;
        }
        return;
    }
    if (current_state == STATE_VIEWER) {
        if (encoder_zoom_mode) {
            if (viewer_zoom > 1) {
                viewer_zoom--;
                viewer_needs_full_redraw = 1;
                needs_refresh = 1;
            }
        } else {
            OnScrollLeftClick();
        }
    }
}

void OnEncoderScrollRight(void) {
    if (current_state == STATE_MAIN_MENU || current_state == STATE_PROTOCOL_CONFIG || current_state == STATE_SETTINGS) {
        View_t view = views[current_state];
        do {
            if (selected_button_idx < 0) {
                selected_button_idx = 0;
            } else {
                selected_button_idx = (selected_button_idx + 1) % view.buttonCount;
            }
        } while (view.buttons[selected_button_idx].text[0] == '\0');
        needs_refresh = 1;
        return;
    }
    if (current_state == STATE_TABLE_VIEW) {
        extern volatile uint32_t decoded_idx;
        if (table_scroll < (int16_t)decoded_idx - 1) {
            table_scroll++;
            needs_refresh = 1;
        }
        return;
    }
    if (current_state == STATE_VIEWER) {
        if (encoder_zoom_mode) {
            if (viewer_zoom < MAX_ZOOM_IDX) {
                viewer_zoom++;
                viewer_needs_full_redraw = 1;
                needs_refresh = 1;
            }
        } else {
            OnScrollRightClick();
        }
    }
}

void OnEncoderButtonLongPress(void) {
    if (current_state == STATE_VIEWER || current_state == STATE_TABLE_VIEW) {
        OnTableClick();
    }
}

void OnEncoderButtonPress(void) {
    if (current_state == STATE_VIEWER) {
        OnModeToggleClick();
    } else if (current_state == STATE_MAIN_MENU || current_state == STATE_PROTOCOL_CONFIG || current_state == STATE_SETTINGS) {
        View_t view = views[current_state];
        if (selected_button_idx >= 0 && selected_button_idx < view.buttonCount) {
            if (view.buttons[selected_button_idx].onClick) {
                view.buttons[selected_button_idx].onClick();
            }
        }
    }
}


void OnZoomInClick(void) {
    if (viewer_zoom > 0) {
        viewer_zoom--;
        viewer_needs_full_redraw = 1;
        needs_refresh = 1;
    }
}

void OnZoomOutClick(void) {
    if (viewer_zoom < MAX_ZOOM_IDX) {
        viewer_zoom++;
        viewer_needs_full_redraw = 1;
        needs_refresh = 1;
    }
}

// Settings callbacks
void OnProtocolClick(void) {
    conf.protocol = (conf.protocol + 1) % PROTOCOL_COUNT;
    needs_refresh = 1;
}

void OnFreqClick(void){
    conf.frequency = (conf.frequency + 1) % FREQUENCY_COUNT;
    needs_refresh = 1;
}

void OnSizeClick(void){
    conf.buffer_size = (conf.buffer_size + 1) % 4;
    needs_refresh = 1;
}

void OnTriggerClick(void){
    config_global.flanco = (config_global.flanco + 1) % 3;
    needs_refresh = 1;
}

void OnRadixClick(void){
    conf.radix = (conf.radix + 1) % 3;
    needs_refresh = 1;
}


void OnUARTParamsClick(void){
    if (config_global.protocolo.settings.uart.data_bits == 8 && config_global.protocolo.settings.uart.paridad == UART_PARITY_SIN && config_global.protocolo.settings.uart.stop_bits == UART_STOPBITS_1) {
        config_global.protocolo.settings.uart.paridad = UART_PARITY_PAR; // 8E1
    } else if (config_global.protocolo.settings.uart.data_bits == 8 && config_global.protocolo.settings.uart.paridad == UART_PARITY_PAR) {
        config_global.protocolo.settings.uart.paridad = UART_PARITY_IMPAR; // 8O1
    } else if (config_global.protocolo.settings.uart.data_bits == 8 && config_global.protocolo.settings.uart.paridad == UART_PARITY_IMPAR) {
        config_global.protocolo.settings.uart.paridad = UART_PARITY_SIN;
        config_global.protocolo.settings.uart.stop_bits = UART_STOPBITS_2; // 8N2
    } else if (config_global.protocolo.settings.uart.data_bits == 8 && config_global.protocolo.settings.uart.stop_bits == UART_STOPBITS_2) {
        config_global.protocolo.settings.uart.stop_bits = UART_STOPBITS_1;
        config_global.protocolo.settings.uart.data_bits = 7; // 7N1
    } else if (config_global.protocolo.settings.uart.data_bits == 7) {
        config_global.protocolo.settings.uart.data_bits = 9; // 9N1
    } else {
        config_global.protocolo.settings.uart.data_bits = 8; // 8N1
        config_global.protocolo.settings.uart.paridad = UART_PARITY_SIN;
        config_global.protocolo.settings.uart.stop_bits = UART_STOPBITS_1;
    }
    viewer_needs_full_redraw = 1;
    needs_refresh = 1;
}

void OnUARTLsbInvClick(void){
    if (config_global.protocolo.settings.uart.lsb_first == 1 && config_global.protocolo.settings.uart.inverted == 0) {
        config_global.protocolo.settings.uart.lsb_first = 0; // MSB, NRM
    } else if (config_global.protocolo.settings.uart.lsb_first == 0 && config_global.protocolo.settings.uart.inverted == 0) {
        config_global.protocolo.settings.uart.lsb_first = 1;
        config_global.protocolo.settings.uart.inverted = 1; // LSB, INV
    } else if (config_global.protocolo.settings.uart.lsb_first == 1 && config_global.protocolo.settings.uart.inverted == 1) {
        config_global.protocolo.settings.uart.lsb_first = 0; // MSB, INV
    } else {
        config_global.protocolo.settings.uart.lsb_first = 1;
        config_global.protocolo.settings.uart.inverted = 0; // LSB, NRM
    }
    viewer_needs_full_redraw = 1;
    needs_refresh = 1;
}
