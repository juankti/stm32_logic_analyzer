#include "maquina_estado.h"
#include "interface_conf.h"
#include "interface.h"

static Estado_t estadoDispositivo = ESTADO_READY;
volatile bool bandera_inicio_captura = false;

void Estado_FSM() {
	switch(estadoDispositivo){
        case ESTADO_READY:
            if (Usuario_Presiono_Inicio()) {
                estadoDispositivo = ESTADO_CONFIG;
            }
            break;

        case ESTADO_CONFIG: {
            extern volatile uint32_t indice_decodificado;
            indice_decodificado = 0;
            config_global.protocolo.protocolo_activo = conf.protocolo;
            Configurar_Disparo_Protocolo();
            Conf_Captura();
            Hardware_Configurar_Disparo();
            Inicializar_Captura();
        	estadoDispositivo = ESTADO_CAPTURA;
            break;
        }

        case ESTADO_CAPTURA:
            if (Captura_Terminada()) {
                Detener_Captura();
                estadoDispositivo = ESTADO_DECOD;
            }
            break;

        case ESTADO_DECOD:
            switch (config_global.protocolo.protocolo_activo) {
            // aca para escalar, conviene usar punteros a funciones
                case UART:
                    UART_FSM((uint16_t*)estados_buffer, captura.muestras_totales, &config_global.protocolo.ajustes.uart);
                    break;
                case SPI:
                    SPI_FSM((uint16_t*)estados_buffer, captura.muestras_totales, &config_global.protocolo.ajustes.spi);
                    break;
                case I2C:
                    I2C_FSM((uint16_t*)estados_buffer, captura.muestras_totales, &config_global.protocolo.ajustes.i2c);
                    break;
                case NONE:
                default:
                    break;
            }
            extern volatile uint32_t indice_decodificado;
            extern void EnfocarEnPaquete(int32_t);
            if (indice_decodificado > 0) {
                EnfocarEnPaquete(0);
            }
            Actualizar_Pantalla_Y_Registro();
            
            extern AppState_t estado_actual;
            extern uint8_t necesita_refresco;
            estado_actual = STATE_VIEWER;
            necesita_refresco = 1;

            estadoDispositivo = ESTADO_READY;
            break;

        default:
            break;
    }
}

