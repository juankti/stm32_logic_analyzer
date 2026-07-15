#include "config_analizador.h"

Config_Analizador_t config_global;

void Configurar_Disparo_Protocolo(void) {
    // usar flanco de bajada para el inicio de trama
    config_global.flanco = 1; 
    
    // mapeo dinamico del pin de interrupcion
    switch(config_global.protocolo.protocolo_activo) {
        case UART:
            config_global.trigger_pin = config_global.protocolo.settings.uart.pin_rx;
            break;
            
        case SPI:
            // ráfaga spi empieza con cs a gnd
            config_global.trigger_pin = config_global.protocolo.settings.spi.pin_cs;
            break;
            
        case I2C:
            // condicion start (sda bajando)
            config_global.trigger_pin = config_global.protocolo.settings.i2c.pin_sda;
            break;
            
        case NONE:
        default:
            break;
    }
}
