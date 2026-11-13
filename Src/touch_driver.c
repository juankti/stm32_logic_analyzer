#include "touch_driver.h"
#include "disp_driver.h"
#include "spi_manager.h"
#define TOUCH_SAMPLES 7

TouchState_t TS_State = {0, 0, 0, 0};
extern SPI_HandleTypeDef hspi1;

#define CMD_RDX 0xD0
#define CMD_RDY 0x90
#define CMD_RDZ1 0xB0
#define CMD_RDZ2 0xC0
#define TOUCH_Z_THRESHOLD 200
uint8_t fue_presionado = 0;

static uint16_t TOUCH_LeerADC_Crudo(uint8_t cmd);

void TOUCH_Inicializar(void) {
    SPI_Bus_Select(SPI_DEV_TOUCH);
    TOUCH_LeerADC_Crudo(CMD_RDX); // enviar comando con pd0=0, pd1=0 para habilitar penirq
    SPI_Bus_Deselect();
}

static uint16_t TOUCH_LeerADC_Crudo(uint8_t cmd) {
    uint8_t tx[3] = {cmd, 0x00, 0x00};
    uint8_t rx[3] = {0};

    // usar transmitreceive para no interrumpir señal reloj
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 3, 10);

    // rx[0] es basura, la datos real esta en rx[1] y rx[2]
    return ((rx[1] << 8) | rx[2]) >> 4;
}


static uint16_t TOUCH_LeerPresion(void) {
    SPI_Bus_Select(SPI_DEV_TOUCH);
    
    // mantenemos encendido el adc durante la lectura de z
    uint8_t cmd_z1 = CMD_RDZ1 | 0x03;
    uint8_t cmd_z2 = CMD_RDZ2 | 0x03;

    // 1 lectura tonta de z1 para estabilizar
    TOUCH_LeerADC_Crudo(cmd_z1);
    
    uint16_t z1 = TOUCH_LeerADC_Crudo(cmd_z1);
    uint16_t z2 = TOUCH_LeerADC_Crudo(cmd_z2);
    
    // restauramos modo power-down
    TOUCH_LeerADC_Crudo(CMD_RDX); 
    
    SPI_Bus_Deselect();
    
    if (z1 == 0 || z2 == 0) return 0;
    
    int16_t z = z1 + 4095 - z2;
    if (z < 0) return 0;
    return (uint16_t)z;
}

static uint16_t TOUCH_LeerADC_Promedio(uint8_t comando) {
    uint16_t muestras[TOUCH_SAMPLES];
    uint32_t suma = 0;

    // modificamos comando para mantener adc encendido
    // evita ruido extremo por estabilizacion en cada muestra
    uint8_t cmd_mantener_encendido = comando | 0x03; 

    SPI_Bus_Select(SPI_DEV_TOUCH);

    // 1. lecturas falsas para estabilizar
    TOUCH_LeerADC_Crudo(cmd_mantener_encendido);
    TOUCH_LeerADC_Crudo(cmd_mantener_encendido);

    // 2. tomar rafaga de muestras
    for (uint8_t i = 0; i < TOUCH_SAMPLES; i++) {
        muestras[i] = TOUCH_LeerADC_Crudo(cmd_mantener_encendido);
    }
    
    // 3. restaurar modo power-down
    // vital para que el pin de interrupcion vuelva a funcionar
    TOUCH_LeerADC_Crudo(comando);

    SPI_Bus_Deselect();

    // 4. ordenar arreglo (bubble sort)
    for (uint8_t i = 0; i < TOUCH_SAMPLES - 1; i++) {
        for (uint8_t j = i + 1; j < TOUCH_SAMPLES; j++) {
            if (muestras[i] > muestras[j]) {
                uint16_t temp = muestras[i];
                muestras[i] = muestras[j];
                muestras[j] = temp;
            }
        }
    }

    // 5. chequeo varianza
    if ((muestras[4] - muestras[2]) > 150) {
        return 0; // rechazamos si hay ruido
    }

    // 6. promediar estrictamente el nucleo central
    suma = muestras[2] + muestras[3] + muestras[4];

    return (uint16_t)(suma / 3);
}


#define TOUCH_X_MIN   220
#define TOUCH_X_MAX  1750

#define TOUCH_Y_MIN   250
#define TOUCH_Y_MAX  1950

void TOUCH_ObtenerPuntoCalibrado(uint16_t x_crudo, uint16_t y_crudo, uint16_t *cal_x, uint16_t *cal_y) {
    int32_t x = x_crudo;
    int32_t y = y_crudo;

    // 1. limitar valores para evitar salir de pantalla
    if (x < TOUCH_X_MIN) x = TOUCH_X_MIN;
    if (x > TOUCH_X_MAX) x = TOUCH_X_MAX;
    if (y < TOUCH_Y_MIN) y = TOUCH_Y_MIN;
    if (y > TOUCH_Y_MAX) y = TOUCH_Y_MAX;

    // mapeo apaisado (rotado 90 grados)
    // el tactil tambien rota 90°
    // asignamos el adc y al eje x logico e invertimos
    *cal_x = (uint16_t)(((TOUCH_Y_MAX - y) * DISP_WIDTH) / (TOUCH_Y_MAX - TOUCH_Y_MIN));

    // asignamos adc x al eje y logico de pantalla e invertimos
    *cal_y = (uint16_t)(((TOUCH_X_MAX - x) * DISP_HEIGHT) / (TOUCH_X_MAX - TOUCH_X_MIN));
}


void TOUCH_Actualizar(void) {
    uint8_t actualmente_presionado = (HAL_GPIO_ReadPin(TOUCH_INT_GPIO_Port, TOUCH_INT_Pin) == GPIO_PIN_RESET);

    // Ya no guardamos recien_presionado en TS_State, podemos usar una variable local si se necesita.
    // En este caso ni siquiera se requiere para la lógica actual.
    TS_State.recien_soltado = (!actualmente_presionado && fue_presionado);

    if (actualmente_presionado) {
        uint16_t presion = TOUCH_LeerPresion();

        if (presion >= TOUCH_Z_THRESHOLD) {
            uint16_t x_crudo = TOUCH_LeerADC_Promedio(CMD_RDX);
            uint16_t y_crudo = TOUCH_LeerADC_Promedio(CMD_RDY);

            if (x_crudo != 0 && y_crudo != 0) {
                TOUCH_ObtenerPuntoCalibrado(x_crudo, y_crudo, &TS_State.x, &TS_State.y);
                TS_State.esta_presionado = 1;
            } else {
                // lectura rechazada por varianza alta. mantenemos estado
                TS_State.esta_presionado = 1;
            }
        } else {
            // falso toque (rebote, ruido o presion leve)
            actualmente_presionado = 0;
            TS_State.esta_presionado = 0;
            
            // si antes estaba presionado, corregir flags
            if (fue_presionado) {
                TS_State.recien_soltado = 1;
            }
        }
    } else {
        TS_State.esta_presionado = 0;
    }

    fue_presionado = actualmente_presionado;
}
