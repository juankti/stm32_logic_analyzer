#include "config_analizador.h"
#include "interface_conf.h"

Config_Analizador_t config_global;

void Configurar_Disparo_Protocolo(void) {
    const ProtocolPinConfig_t* cfg = Config_ObtenerConfigPin(config_global.protocolo.protocolo_activo);
    
    // mapeo dinamico del pin de interrupcion
    // arreglo critico: dado que el dma tim1_ch1 esta cableado a pa8 (ch1),
    // la señal que dispara la captura debe estar en ch1
    switch(config_global.protocolo.protocolo_activo) {
        case UART:
            config_global.protocolo.ajustes.uart.pin_rx = __builtin_ctz(cfg->canales[0].pin); // rx debe ser ch1
            config_global.pin_disparo = config_global.protocolo.ajustes.uart.pin_rx;
            break;
            
        case SPI:
            config_global.protocolo.ajustes.spi.pin_sck = __builtin_ctz(cfg->canales[0].pin); // sck debe ser ch1
            config_global.protocolo.ajustes.spi.pin_cs = __builtin_ctz(cfg->canales[1].pin); // cs en ch2
            config_global.protocolo.ajustes.spi.pin_mosi = __builtin_ctz(cfg->canales[2].pin); // mosi en ch3
            config_global.protocolo.ajustes.spi.pin_miso = __builtin_ctz(cfg->canales[3].pin); // miso en ch4
            config_global.pin_disparo = config_global.protocolo.ajustes.spi.pin_cs; // disparo en cs
            break;
            
        case I2C:
            config_global.protocolo.ajustes.i2c.pin_sda = __builtin_ctz(cfg->canales[0].pin); // sda debe ser ch1
            config_global.protocolo.ajustes.i2c.pin_scl = __builtin_ctz(cfg->canales[1].pin); // scl en ch2
            config_global.pin_disparo = config_global.protocolo.ajustes.i2c.pin_sda;
            break;
            
        case NONE:
            config_global.pin_disparo = __builtin_ctz(cfg->canales[0].pin); // ch1
            break;
        default:
            break;
    }
}
