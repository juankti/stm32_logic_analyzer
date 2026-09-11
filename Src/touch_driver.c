#include "touch_driver.h"
#include "disp_driver.h"
#include "spi_manager.h"
#define TOUCH_SAMPLES 8

TouchState_t TS_State = {0, 0, 0,0,0};
extern SPI_HandleTypeDef hspi1;

#define CMD_RDX 0xD0
#define CMD_RDY 0x90

uint8_t was_pressed = 0;
static uint16_t TOUCH_ReadADC_Raw(uint8_t cmd) {
    uint8_t rx[2] = {0};

    HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    HAL_SPI_Receive(&hspi1, rx, 2, 10);

    return ((rx[0] << 8) | rx[1]) >> 4;
}


static uint16_t TOUCH_ReadADC_Average(uint8_t command) {
    uint32_t sum = 0;

    SPI_Bus_Select(SPI_DEV_TOUCH);

    for (uint8_t i = 0; i < TOUCH_SAMPLES; i++) {
        sum += TOUCH_ReadADC_Raw(command);
    }

    SPI_Bus_Deselect();

    return (uint16_t)(sum / TOUCH_SAMPLES);
}


#define TOUCH_X_MIN   220
#define TOUCH_X_MAX  1750

#define TOUCH_Y_MIN   250
#define TOUCH_Y_MAX  1950

void TOUCH_GetCalibratedPoint(uint16_t raw_x, uint16_t raw_y, uint16_t *cal_x, uint16_t *cal_y) {
    int32_t x = raw_x;
    int32_t y = raw_y;

    // 1. Limitar (Clamp) los valores para evitar salirse de pantalla si hay ruido
    if (x < TOUCH_X_MIN) x = TOUCH_X_MIN;
    if (x > TOUCH_X_MAX) x = TOUCH_X_MAX;
    if (y < TOUCH_Y_MIN) y = TOUCH_Y_MIN;
    if (y > TOUCH_Y_MAX) y = TOUCH_Y_MAX;

    // 2. Mapear X (Escalar de TOUCH_X_MIN..TOUCH_X_MAX a 0..DISP_WIDTH)
    *cal_x = (uint16_t)(((x - TOUCH_X_MIN) * DISP_WIDTH) / (TOUCH_X_MAX - TOUCH_X_MIN));

    // 3. Mapear Y (Como el eje Y está invertido en el sensor, mapeamos invertido: Y_MAX es 0, Y_MIN es DISP_HEIGHT)
    *cal_y = (uint16_t)(((TOUCH_Y_MAX - y) * DISP_HEIGHT) / (TOUCH_Y_MAX - TOUCH_Y_MIN));
}


void TOUCH_Update(void) {
    uint8_t currently_pressed = (HAL_GPIO_ReadPin(TOUCH_INT_GPIO_Port, TOUCH_INT_Pin) == GPIO_PIN_RESET);

    TS_State.just_pressed  = (currently_pressed && !was_pressed);
    TS_State.just_released = (!currently_pressed && was_pressed);

    if (currently_pressed) {
        uint16_t raw_x = TOUCH_ReadADC_Average(CMD_RDX);
        uint16_t raw_y = TOUCH_ReadADC_Average(CMD_RDY);

        TOUCH_GetCalibratedPoint(raw_x, raw_y, &TS_State.x, &TS_State.y);
        TS_State.is_pressed = 1;

        if (TS_State.just_pressed) {
            TS_State.pressed_x = TS_State.x;
            TS_State.pressed_y = TS_State.y;
        }
    } else {
        TS_State.is_pressed = 0;
    }

    was_pressed = currently_pressed;
}
