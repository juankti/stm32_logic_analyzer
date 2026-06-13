#include "maquina_estado.h"

static Estado_t deviceState = ESTADO_IDLE;

void Estado_FSM() {
	switch(deviceState){
        case ESTADO_READY:
            if (User_Pressed_Start()) {
                deviceState = ESTADO_CONFIG;
            }
            break;

        case ESTADO_CONFIG:
        	Conf_Captura(&config_global.captura);
        	deviceState = ESTADO_CAPTURA;
            break;

        case ESTADO_CAPTURA:
        	// nada, todo por hardware
            break;

        case ESTADO_DECOD:
            {
                FSM((uint8_t*)buffer_states, NUM_MUESTRAS, &menu_config);
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

