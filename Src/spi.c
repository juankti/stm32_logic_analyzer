#include "spi.h"
#include <stdbool.h>

extern volatile uint8_t decoded_buffer[];
extern volatile uint32_t decoded_idx;
extern void Save_Decoded_Char(uint8_t c, uint32_t start_idx, uint32_t end_idx);

void SPI_FSM(uint16_t *buffer_states, uint32_t buffer_length, spi_config_t *config) {
    uint8_t current_byte = 0;
    uint8_t bit_counter = 0;
    uint16_t prev_state = buffer_length > 0 ? buffer_states[0] : 0xFFFF;
    
    // The DMA triggers exactly on the first edge of SCK.
    // Therefore, the state of SCK right before this first sample was the opposite of what is recorded.
    // By inverting it in prev_state, the first iteration of the loop will correctly detect the transition.
    if (buffer_length > 0) {
        prev_state ^= (1 << config->pin_sck);
    }
    
    uint32_t packet_start_idx = 0; // Asumir alto inicialmente
    
    decoded_idx = 0; // Limpiar buffer de salida

    for (uint32_t i = 0; i < buffer_length; i++) {
        uint16_t current_state = buffer_states[i];
        
        bool sck_state = (current_state & (1 << config->pin_sck)) ? 1 : 0;
        bool cs_state = (current_state & (1 << config->pin_cs)) ? 1 : 0;
        bool prev_sck_state = (prev_state & (1 << config->pin_sck)) ? 1 : 0;
        bool mosi_state = (current_state & (1 << config->pin_mosi)) ? 1 : 0;
        
        if (bit_counter == 0) packet_start_idx = i;
        
        bool prev_cs_state = (prev_state & (1 << config->pin_cs)) ? 1 : 0;
        if (cs_state == 1 && prev_cs_state == 0) {
            // Flanco de subida en CS (pasa a inactivo), reiniciar estado
            bit_counter = 0;
            current_byte = 0;
        } 
        
        // Procesar flancos siempre (si CS está high fijo por no estar conectado, igual decodifica)
        bool is_rising = !prev_sck_state && sck_state;
        bool is_falling = prev_sck_state && !sck_state;
            bool sample_now = false;
            
            if (config->mode == SPI_MODE_0 || config->mode == SPI_MODE_3) {
                sample_now = is_rising;
            } else {
                sample_now = is_falling;
            }
            
            if (sample_now) {
                // Extraer bit de MOSI
                if (config->bit_order == SPI_MSB_FIRST) {
                    current_byte = (current_byte << 1) | mosi_state;
                } else {
                    current_byte = current_byte | (mosi_state << bit_counter);
                }
                
                bit_counter++;
                
                if (bit_counter >= config->data_size && config->data_size > 0) {
                    // Guardar byte ensamblado
                    Save_Decoded_Char(current_byte, packet_start_idx, i);
                    bit_counter = 0;
                    current_byte = 0;
                }
            }
        prev_state = current_state;
    }
}
