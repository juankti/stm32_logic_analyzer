#include "spi.h"
#include <stdbool.h>

void spi_decoder_init(spi_decoder_ctx_t *ctx, const spi_config_t *config) {
    ctx->config = *config;
    ctx->state = SPI_IDLE;
    ctx->sample_idx = 0;
    // se inicializa en alto para evitar falsos flancos
    ctx->prev_pins_state = 0xFFFFFFFF; 
    ctx->current_mosi_data = 0;
    ctx->current_miso_data = 0;
    ctx->bit_counter = 0;
    ctx->bytes_decoded = 0;
}

int32_t spi_decoder_process(spi_decoder_ctx_t *ctx, uint32_t *buffer_states, uint32_t *buffer_timestamps, uint32_t buffer_length) {
    // iterar muestra a muestra
    while (ctx->sample_idx < buffer_length) {
        uint32_t current_sample = buffer_states[ctx->sample_idx];
        
        // estado logico actual
        bool cs_state   = (current_sample & (1 << ctx->config.pin_cs)) ? 1 : 0;
        bool sck_state  = (current_sample & (1 << ctx->config.pin_sck)) ? 1 : 0;
        bool mosi_state = (current_sample & (1 << ctx->config.pin_mosi)) ? 1 : 0;
        bool miso_state = (current_sample & (1 << ctx->config.pin_miso)) ? 1 : 0;
        
        // estado logico anterior
        bool prev_cs_state  = (ctx->prev_pins_state & (1 << ctx->config.pin_cs)) ? 1 : 0;
        bool prev_sck_state = (ctx->prev_pins_state & (1 << ctx->config.pin_sck)) ? 1 : 0;
        
        // detectar flancos
        bool cs_falling_edge  = prev_cs_state && !cs_state;
        bool sck_rising_edge  = !prev_sck_state && sck_state;
        bool sck_falling_edge = prev_sck_state && !sck_state;
        
        // forzar a idle si cs se inactiva
        if (cs_state == 1 && ctx->state != SPI_IDLE) {
            ctx->state = SPI_IDLE;
            ctx->bit_counter = 0;
        }
        
        // maquina de estados spi
        switch (ctx->state) {
            case SPI_IDLE:
                // buscar inicio de trama
                if (cs_falling_edge || cs_state == 0) {
                    ctx->state = SPI_WAIT_EDGE;
                    ctx->bit_counter = 0;
                    ctx->current_mosi_data = 0;
                    ctx->current_miso_data = 0;
                }
                break;
                
            case SPI_WAIT_CS:
                break;
                
            case SPI_WAIT_EDGE:
            case SPI_SAMPLE_BIT:
                if (cs_state == 0) {
                    bool sample_now = false;
                    
                    // sincronismo por cpol y cpha
                    if (ctx->config.mode == SPI_MODE_0 || ctx->config.mode == SPI_MODE_3) {
                        if (sck_rising_edge) sample_now = true;
                    } else if (ctx->config.mode == SPI_MODE_1 || ctx->config.mode == SPI_MODE_2) {
                        if (sck_falling_edge) sample_now = true;
                    }
                    
                    if (sample_now) {
                        // orden de bits
                        if (ctx->config.bit_order == SPI_MSB_FIRST) {
                            ctx->current_mosi_data = (ctx->current_mosi_data << 1) | mosi_state;
                            ctx->current_miso_data = (ctx->current_miso_data << 1) | miso_state;
                        } else {
                            ctx->current_mosi_data |= (mosi_state << ctx->bit_counter);
                            ctx->current_miso_data |= (miso_state << ctx->bit_counter);
                        }
                        
                        ctx->bit_counter++;
                        
                        if (ctx->bit_counter >= ctx->config.data_size) {
                            ctx->state = SPI_SAVE_BYTE;
                        }
                    }
                }
                break;
                
            case SPI_SAVE_BYTE:
                // palabra lista
                ctx->bytes_decoded += (ctx->config.data_size > 8) ? 2 : 1;
                
                // reiniciar para proxima palabra
                ctx->bit_counter = 0;
                ctx->current_mosi_data = 0;
                ctx->current_miso_data = 0;
                
                if (cs_state == 0) {
                    ctx->state = SPI_WAIT_EDGE; // seguir leyendo
                } else {
                    ctx->state = SPI_IDLE; // trama terminada
                }
                break;
                
            case SPI_ERROR:
                if (cs_state == 1) {
                    ctx->state = SPI_IDLE; // recuperar
                }
                break;
        }
        
        // guardar estado previo
        ctx->prev_pins_state = current_sample;
        ctx->sample_idx++;
    }
    
    return (int32_t)ctx->bytes_decoded;
}
