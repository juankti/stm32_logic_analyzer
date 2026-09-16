#include "maquina_estado.h"
#include "interface_conf.h"
#include "interface.h"

static Estado_t deviceState = ESTADO_READY;
volatile bool start_capture_flag = false;

void Estado_FSM() {
	switch(deviceState){
        case ESTADO_READY:
            if (User_Pressed_Start()) {
                deviceState = ESTADO_CONFIG;
            }
            break;

        case ESTADO_CONFIG: {
            extern volatile uint32_t decoded_idx;
            decoded_idx = 0;
            config_global.protocolo.protocolo_activo = conf.protocol;
            Configurar_Disparo_Protocolo();
            Conf_Captura();
            Hardware_Config_Trigger();
            Init_Captura();
        	deviceState = ESTADO_CAPTURA;
            break;
        }

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
            extern volatile uint32_t decoded_idx;
            extern volatile uint16_t decoded_start_indices[];
            extern int16_t viewer_scroll;
            if (decoded_idx > 0) {
                viewer_scroll = decoded_start_indices[0];
            }
            Update_Display_And_Log();
            
            extern AppState_t current_state;
            extern uint8_t needs_refresh;
            current_state = STATE_VIEWER;
            needs_refresh = 1;

            deviceState = ESTADO_READY;
            break;

        case ESTADO_IDLE:
        case ESTADO_EXPORT:
        default:
            break;
    }
}

