#ifndef INTERFACE_H
#define INTERFACE_H

#include "main.h"
#include "disp_ui.h"
#include "touch_driver.h"
#include "interface_conf.h"
/* Standard RGB565 Colors */
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF

/* Signals theme palette */
#define COLOR_BG_DARK     0x10A2  /* Very dark charcoal background */
#define COLOR_BG_DARKER   0x0842  /* Darker panel background */
#define COLOR_SIG_HIGH    0x07E0  /* Bright green - logic HIGH */
#define COLOR_SIG_LOW     0x02B2  /* Dim blue - logic LOW */
#define COLOR_SIG_LABEL   0xFFFF  /* White - channel labels */
#define COLOR_SIG_SEP     0x2104  /* Subtle dark gray separator */
#define COLOR_UI_BTN      0x2945  /* Dark blue-gray button */
#define COLOR_UI_BTN_ACT  0x07FF  /* Cyan button active/pressed */
#define COLOR_UI_ACCENT   0x07FF  /* Cyan accent */
#define COLOR_UI_STATUS   0x0124  /* Very dark status bar */
#define COLOR_UI_STATUS_T 0xFFFF  /* Status bar text */
#define COLOR_UI_REC      0xF800  /* Red for REC button */




//Interface State
typedef enum {
	STATE_MAIN_MENU,
	STATE_VIEWER,
	STATE_SETTINGS,
} AppState_t;

// Views
typedef struct {
    Button_t* buttons;     // Pointer to array of buttons
    uint8_t buttonCount;   // Button count
    void (*onDraw)(void);  // Screen drawing callback
} View_t;

typedef struct {
    uint8_t samples[LOGIC_NUM_CHANNELS][LOGIC_BUFFER_SIZE];
    uint16_t total_samples;
    uint8_t active_channels;
    const char *channel_labels[LOGIC_NUM_CHANNELS];
} CaptureBuffer_t;

extern CaptureBuffer_t capture;
extern int16_t viewer_scroll;
extern uint8_t viewer_zoom;
extern uint8_t viewer_needs_full_redraw;

extern AppState_t current_state;
extern uint8_t needs_refresh;

extern Button_t main_menu_buttons[];
extern Button_t settings_buttons[];
extern Button_t viewer_buttons[];
extern View_t views[];

// Buttons callbacks
void OnOptionsClick(void);
void OnRedClick(void);
void OnCaptureClick(void);
void OnBackClick(void);

void OnScrollLeftClick(void);
void OnScrollRightClick(void);
void OnZoomInClick(void);
void OnZoomOutClick(void);

void OnProtocolClick(void);
void OnFreqClick(void);

// Views callbacks
void Draw_MainMenu();
void Draw_Viewer();
void Draw_Settings();

#endif
