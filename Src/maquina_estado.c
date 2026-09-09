#include "maquina_estado.h"

static Estado_t deviceState = ESTADO_READY;

void Estado_FSM() {
	switch(deviceState){
        case ESTADO_READY:
            if (User_Pressed_Start()) {
                deviceState = ESTADO_CONFIG;
            }
            break;

        case ESTADO_CONFIG:
            Conf_Captura();
            Init_Captura();
        	deviceState = ESTADO_CAPTURA;
            break;

        case ESTADO_CAPTURA:
            if (Capture_IsFinished()) {
                Stop_Captura();
                deviceState = ESTADO_DECOD;
            }
            break;

        case ESTADO_DECOD:
            switch (config_global.protocolo.protocolo_activo) {
            // acá para escalar, conviene usar punteros a funciones
                case UART:
                    UART_FSM((uint16_t*)buffer_states, NUM_MUESTRAS, &config_global.protocolo.settings.uart);
                    break;
                case SPI:
                    SPI_FSM((uint16_t*)buffer_states, NUM_MUESTRAS, &config_global.protocolo.settings.spi);
                    break;
                case I2C:
                    I2C_FSM((uint16_t*)buffer_states, NUM_MUESTRAS, &config_global.protocolo.settings.i2c);
                    break;
                case NONE:
                default:
                    break;
            }
            Update_Display_And_Log();
            deviceState = ESTADO_READY;
            break;

        case ESTADO_IDLE:
        case ESTADO_EXPORT:
        default:
            break;
    }
}

