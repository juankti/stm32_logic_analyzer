#include "touch_driver.h"
#include "disp_driver.h"
#include "spi_manager.h"
#define TOUCH_SAMPLES 7

TouchState_t TS_State = {0, 0, 0,0,0};
extern SPI_HandleTypeDef hspi1;

#define CMD_RDX 0xD0
#define CMD_RDY 0x90
#define CMD_RDZ1 0xB0
#define CMD_RDZ2 0xC0
#define TOUCH_Z_THRESHOLD 200
uint8_t was_pressed = 0;

static uint16_t TOUCH_ReadADC_Raw(uint8_t cmd);

void TOUCH_Init(void) {
    SPI_Bus_Select(SPI_DEV_TOUCH);
    TOUCH_ReadADC_Raw(CMD_RDX); // Send command with PD0=0, PD1=0 to enable PENIRQ
    SPI_Bus_Deselect();
}

static uint16_t TOUCH_ReadADC_Raw(uint8_t cmd) {
    uint8_t tx[3] = {cmd, 0x00, 0x00};
    uint8_t rx[3] = {0};

    // Usar TransmitReceive para no tener interrupciones en la señal de reloj
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 3, 10);

    // rx[0] es basura (mientras se envía el comando), la data real de 12 bits está en rx[1] y rx[2]
    return ((rx[1] << 8) | rx[2]) >> 4;
}


static uint16_t TOUCH_ReadPressure(void) {
    SPI_Bus_Select(SPI_DEV_TOUCH);
    
    // Mantenemos encendido el ADC durante la lectura de Z
    uint8_t cmd_z1 = CMD_RDZ1 | 0x03;
    uint8_t cmd_z2 = CMD_RDZ2 | 0x03;

    // 1 dummy read de z1 para estabilizar
    TOUCH_ReadADC_Raw(cmd_z1);
    
    uint16_t z1 = TOUCH_ReadADC_Raw(cmd_z1);
    uint16_t z2 = TOUCH_ReadADC_Raw(cmd_z2);
    
    // Restauramos el modo Power-Down
    TOUCH_ReadADC_Raw(CMD_RDX); 
    
    SPI_Bus_Deselect();
    
    if (z1 == 0 || z2 == 0) return 0;
    
    int16_t z = z1 + 4095 - z2;
    if (z < 0) return 0;
    return (uint16_t)z;
}

static uint16_t TOUCH_ReadADC_Average(uint8_t command) {
    uint16_t samples[TOUCH_SAMPLES];
    uint32_t sum = 0;

    // Modificamos el comando para mantener el ADC encendido durante la ráfaga (PD1=1, PD0=1)
    // Esto evita el ruido extremo por estabilización en cada muestra.
    uint8_t cmd_keep_on = command | 0x03; 

    SPI_Bus_Select(SPI_DEV_TOUCH);

    // 1. Dummy reads para estabilizar la tensión
    TOUCH_ReadADC_Raw(cmd_keep_on);
    TOUCH_ReadADC_Raw(cmd_keep_on);

    // 2. Tomar ráfaga masiva de muestras
    for (uint8_t i = 0; i < TOUCH_SAMPLES; i++) {
        samples[i] = TOUCH_ReadADC_Raw(cmd_keep_on);
    }
    
    // 3. Restaurar el modo Power-Down (PD1=0, PD0=0) enviando el comando original
    // Es vital para que el pin de interrupción (PENIRQ) vuelva a funcionar.
    TOUCH_ReadADC_Raw(command);

    SPI_Bus_Deselect();

    // 4. Ordenar arreglo (Bubble Sort)
    for (uint8_t i = 0; i < TOUCH_SAMPLES - 1; i++) {
        for (uint8_t j = i + 1; j < TOUCH_SAMPLES; j++) {
            if (samples[i] > samples[j]) {
                uint16_t temp = samples[i];
                samples[i] = samples[j];
                samples[j] = temp;
            }
        }
    }

    // 5. Chequeo de varianza (indices 2, 3, 4 -> 3 muestras centrales)
    if ((samples[4] - samples[2]) > 150) {
        return 0; // Rechazamos si hay ruido
    }

    // 6. Promediar estrictamente el núcleo central
    sum = samples[2] + samples[3] + samples[4];

    return (uint16_t)(sum / 3);
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
        uint16_t pressure = TOUCH_ReadPressure();

        if (pressure >= TOUCH_Z_THRESHOLD) {
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
                // Lectura rechazada por varianza alta. Mantenemos el estado de toque.
                TS_State.is_pressed = 1;
            }
        } else {
            // Falso toque (rebote, ruido o presión muy leve)
            currently_pressed = 0;
            TS_State.is_pressed = 0;
            
            // Si antes estaba presionado, corregir los flags
            if (was_pressed) {
                TS_State.just_released = 1;
            }
            TS_State.just_pressed = 0;
        }
    } else {
        TS_State.is_pressed = 0;
    }

    was_pressed = currently_pressed;
}
