#include "spi.h"
#include <stdbool.h>

extern volatile uint8_t decoded_buffer[];
extern volatile uint32_t decoded_idx;
extern void Save_Decoded_Char(uint8_t c, uint32_t start_idx, uint32_t end_idx);

void SPI_FSM(uint16_t *buffer_states, uint32_t buffer_length, spi_config_t *config) {
    uint8_t current_byte = 0;
    uint8_t bit_counter = 0;
    uint16_t prev_state = 0xFFFF;
    uint32_t packet_start_idx = 0; // Asumir alto inicialmente
    
    decoded_idx = 0; // Limpiar buffer de salida

    for (uint32_t i = 0; i < buffer_length; i++) {
        uint16_t current_state = buffer_states[i];
        
        bool sck_state = (current_state & (1 << config->pin_sck)) ? 1 : 0;
        bool cs_state = (current_state & (1 << config->pin_cs)) ? 1 : 0;
        bool prev_sck_state = (prev_state & (1 << config->pin_sck)) ? 1 : 0;
        bool mosi_state = (current_state & (1 << config->pin_mosi)) ? 1 : 0;
        
        if (bit_counter == 0) packet_start_idx = i;
            if (cs_state == 1) {
            // CS está inactivo, reiniciar estado
            bit_counter = 0;
            current_byte = 0;
        } else {
            // CS está activo (LOW), procesar flancos
            // Buscar flanco de subida en SCK (para CPHA=0)
            if (!prev_sck_state && sck_state) {
                // Extraer bit de MOSI
                if (config->bit_order == SPI_MSB_FIRST) {
                    current_byte = (current_byte << 1) | mosi_state;
                } else {
                    current_byte = current_byte | (mosi_state << bit_counter);
                }
                
                bit_counter++;
                
                if (bit_counter >= config->data_size) {
                    // Guardar byte ensamblado
                    Save_Decoded_Char(current_byte, packet_start_idx, i);
                    bit_counter = 0;
                    current_byte = 0;
                }
            }
        }
        
        prev_state = current_state;
    }
}
