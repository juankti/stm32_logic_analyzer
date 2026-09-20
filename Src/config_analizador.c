#include "config_analizador.h"
#include "interface_conf.h"

Config_Analizador_t config_global;

void Configurar_Disparo_Protocolo(void) {
    const ProtocolPinConfig_t* cfg = Config_GetPinConfig(config_global.protocolo.protocolo_activo);
    
    // mapeo dinamico del pin de interrupcion
    // CRITICAL FIX: Since TIM1_CH1 DMA is physically hardwired to PA8 (CH1),
    // the signal that triggers the capture MUST be on CH1.
    switch(config_global.protocolo.protocolo_activo) {
        case UART:
            config_global.protocolo.settings.uart.pin_rx = __builtin_ctz(cfg->channels[0].pin); // RX MUST be CH1
            config_global.trigger_pin = config_global.protocolo.settings.uart.pin_rx;
            break;
            
        case SPI:
            config_global.protocolo.settings.spi.pin_sck = __builtin_ctz(cfg->channels[0].pin); // SCK MUST be CH1
            config_global.protocolo.settings.spi.pin_cs = __builtin_ctz(cfg->channels[1].pin); // CS on CH2
            config_global.protocolo.settings.spi.pin_mosi = __builtin_ctz(cfg->channels[2].pin); // MOSI on CH3
            config_global.protocolo.settings.spi.pin_miso = __builtin_ctz(cfg->channels[3].pin); // MISO on CH4
            config_global.trigger_pin = config_global.protocolo.settings.spi.pin_cs; // Trigger on CS
            break;
            
        case I2C:
            config_global.protocolo.settings.i2c.pin_sda = __builtin_ctz(cfg->channels[0].pin); // SDA MUST be CH1
            config_global.protocolo.settings.i2c.pin_scl = __builtin_ctz(cfg->channels[1].pin); // SCL on CH2
            config_global.trigger_pin = config_global.protocolo.settings.i2c.pin_sda;
            break;
            
        case NONE:
            config_global.trigger_pin = __builtin_ctz(cfg->channels[0].pin); // CH1
            break;
        default:
            break;
    }
}
