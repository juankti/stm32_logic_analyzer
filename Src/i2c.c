#include "i2c.h"
#include <stdbool.h>

extern volatile uint8_t decoded_buffer[];
extern volatile uint32_t decoded_idx;
extern volatile uint32_t buffer_timestamps[];
extern void Save_Decoded_Char(uint8_t c);

void I2C_FSM(uint16_t *buffer_states, uint32_t buffer_length, i2c_config_t *config) {
    uint8_t current_byte = 0;
    uint8_t bit_counter = 0;
    uint16_t prev_state = 0xFFFF; // Asumir SCL y SDA en alto inicialmente (idle)
    uint32_t prev_time = 0;
    
    decoded_idx = 0; // Limpiar buffer de salida

    for (uint32_t i = 0; i < buffer_length; i++) {
        uint16_t current_state = buffer_states[i];
        uint32_t current_time = buffer_timestamps[i];
        
        bool scl_state = (current_state & (1 << config->pin_scl)) ? 1 : 0;
        bool prev_scl_state = (prev_state & (1 << config->pin_scl)) ? 1 : 0;
        bool sda_state = (current_state & (1 << config->pin_sda)) ? 1 : 0;
        
        // Si hay una pausa mayor a 1 milisegundo (1000 microsegundos), es un nuevo mensaje
        if ((current_time - prev_time) > 1000) {
            bit_counter = 0;
            current_byte = 0;
        }

        // En I2C los datos se leen en el flanco de subida de SCL
        if (!prev_scl_state && scl_state) {
            
            if (bit_counter < 8) {
                // Leer 8 bits de datos (MSB first)
                current_byte = (current_byte << 1) | sda_state;
                bit_counter++;
            } else {
                // Hemos llegado al bit 9 (ACK/NACK)
                // Ignoramos el valor del ACK y guardamos el byte
                Save_Decoded_Char(current_byte);
                bit_counter = 0;
                current_byte = 0;
            }
        }
        
        prev_state = current_state;
        prev_time = current_time;
    }
}
