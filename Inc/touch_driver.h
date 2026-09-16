#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H
#include "main.h"

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t pressed_x;
    uint16_t pressed_y;
    uint8_t is_pressed;
    uint8_t just_pressed;
    uint8_t just_released;
} TouchState_t;

extern TouchState_t TS_State;
extern volatile uint8_t touch_event_occurred;

void TOUCH_Init(void);
void TOUCH_Update(void);
#endif
