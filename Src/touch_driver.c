#include "touch_driver.h"
#include "disp_driver.h"
#include "spi_manager.h"
#define TOUCH_SAMPLES 15

TouchState_t TS_State = {0, 0, 0,0,0};
extern SPI_HandleTypeDef hspi1;

#define CMD_RDX 0xD0
#define CMD_RDY 0x90

uint8_t was_pressed = 0;
static uint16_t TOUCH_ReadADC_Raw(uint8_t cmd) {
    uint8_t tx[3] = {cmd, 0x00, 0x00};
    uint8_t rx[3] = {0};

    // Usar TransmitReceive para no tener interrupciones en la señal de reloj
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 3, 10);

    // rx[0] es basura (mientras se envía el comando), la data real de 12 bits está en rx[1] y rx[2]
    return ((rx[1] << 8) | rx[2]) >> 4;
}

static uint16_t TOUCH_ReadADC_Average(uint8_t command) {
    uint16_t samples[TOUCH_SAMPLES];
    uint32_t sum = 0;

    SPI_Bus_Select(SPI_DEV_TOUCH);

    // 1. Dummy read
    TOUCH_ReadADC_Raw(command);

    // 2. Tomar muestras
    for (uint8_t i = 0; i < TOUCH_SAMPLES; i++) {
        samples[i] = TOUCH_ReadADC_Raw(command);
    }
    SPI_Bus_Deselect();

    // 3. Ordenar arreglo (Bubble Sort)
    for (uint8_t i = 0; i < TOUCH_SAMPLES - 1; i++) {
        for (uint8_t j = i + 1; j < TOUCH_SAMPLES; j++) {
            if (samples[i] > samples[j]) {
                uint16_t temp = samples[i];
                samples[i] = samples[j];
                samples[j] = temp;
            }
        }
    }

    // 4. Chequeo de varianza en el bloque central (índices 4 a 10)
    // Si la diferencia entre el valor central más alto y más bajo es muy grande,
    // significa que el dedo rebotó o la lectura es basura.
    if ((samples[10] - samples[4]) > 50) {
        return 0; // Retornamos 0 indicando error/ruido
    }

    // 5. Promediar estrictamente el núcleo central (7 muestras)
    for (uint8_t i = 4; i < 11; i++) {
        sum += samples[i];
    }

    return (uint16_t)(sum / 7);
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

    // Landscape mapping (Rotado 90 grados a la derecha):
    // El táctil también rota 90°.
    // Asignamos el ADC Y (raw_y) al eje X lógico de la pantalla e invertimos (espejo)
    *cal_x = (uint16_t)(((TOUCH_Y_MAX - y) * DISP_WIDTH) / (TOUCH_Y_MAX - TOUCH_Y_MIN));

    // Asignamos el ADC X (raw_x) al eje Y lógico de la pantalla e invertimos (espejo)
    *cal_y = (uint16_t)(((TOUCH_X_MAX - x) * DISP_HEIGHT) / (TOUCH_X_MAX - TOUCH_X_MIN));
}


void TOUCH_Update(void) {
    uint8_t currently_pressed = (HAL_GPIO_ReadPin(TOUCH_INT_GPIO_Port, TOUCH_INT_Pin) == GPIO_PIN_RESET);

    TS_State.just_pressed  = (currently_pressed && !was_pressed);
    TS_State.just_released = (!currently_pressed && was_pressed);

    if (currently_pressed) {
        uint16_t raw_x = TOUCH_ReadADC_Average(CMD_RDX);
        uint16_t raw_y = TOUCH_ReadADC_Average(CMD_RDY);

        if (raw_x != 0 && raw_y != 0) {
            TOUCH_GetCalibratedPoint(raw_x, raw_y, &TS_State.x, &TS_State.y);
            TS_State.is_pressed = 1;

            if (TS_State.just_pressed) {
                TS_State.pressed_x = TS_State.x;
                TS_State.pressed_y = TS_State.y;
            }
        } else {
            // Lectura rechazada por varianza alta (ruido). 
            // Seguimos considerando que está pulsado para no cortar el arrastre.
            TS_State.is_pressed = 1;
        }
    } else {
        TS_State.is_pressed = 0;
    }

    was_pressed = currently_pressed;
}
