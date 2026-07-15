#include "i2c.h"
#include <stdbool.h>

void i2c_decoder_init(i2c_decoder_ctx_t *ctx, const i2c_config_t *config) {
    ctx->config = *config;
    ctx->state = I2C_IDLE;
    ctx->sample_idx = 0;
    // idle en alto por los pull-ups
    ctx->prev_pins_state = 0xFFFFFFFF; 
    ctx->current_data = 0;
    ctx->bit_counter = 0;
    ctx->is_read_op = 0;
    ctx->last_ack = 0;
    ctx->bytes_decoded = 0;
}

int32_t i2c_decoder_process(i2c_decoder_ctx_t *ctx, uint32_t *buffer_states, uint32_t *buffer_timestamps, uint32_t buffer_length) {
    // iterar muestra a muestra
    while (ctx->sample_idx < buffer_length) {
        uint32_t current_sample = buffer_states[ctx->sample_idx];
        
        // estado logico actual
        bool sda_state = (current_sample & (1 << ctx->config.pin_sda)) ? 1 : 0;
        bool scl_state = (current_sample & (1 << ctx->config.pin_scl)) ? 1 : 0;
        
        // estado logico anterior
        bool prev_sda_state = (ctx->prev_pins_state & (1 << ctx->config.pin_sda)) ? 1 : 0;
        bool prev_scl_state = (ctx->prev_pins_state & (1 << ctx->config.pin_scl)) ? 1 : 0;
        
        // detectar flancos
        bool sda_falling_edge = prev_sda_state && !sda_state;
        bool sda_rising_edge  = !prev_sda_state && sda_state;
        bool scl_rising_edge  = !prev_scl_state && scl_state;
        
        // prioridad asincrona: start y stop
        if (scl_state == 1) { 
            if (sda_falling_edge) {
                // start o repeated start
                ctx->state = I2C_READ_ADDRESS;
                ctx->bit_counter = 0;
                ctx->current_data = 0;
            } else if (sda_rising_edge) {
                // stop
                ctx->state = I2C_IDLE;
            }
        }
        
        // maquina de estados i2c
        switch (ctx->state) {
            case I2C_IDLE:
                // esperar asincronamente el start
                break;
                
            case I2C_READ_ADDRESS:
                // muestrear solo con scl en alto
                if (scl_rising_edge) {
                    ctx->current_data = (ctx->current_data << 1) | sda_state;
                    ctx->bit_counter++;
                    
                    if (ctx->bit_counter == 8) {
                        ctx->is_read_op = sda_state; // bit r/w
                        ctx->state = I2C_ACK_ADDRESS;
                    }
                }
                break;
                
            case I2C_ACK_ADDRESS:
                if (scl_rising_edge) {
                    // leer ack/nack
                    ctx->last_ack = sda_state; 
                    
                    // avanzar a datos (si hay nack viene un stop luego)
                    ctx->state = I2C_READ_DATA;
                    ctx->bit_counter = 0;
                    ctx->current_data = 0;
                }
                break;
                
            case I2C_READ_DATA:
                if (scl_rising_edge) {
                    ctx->current_data = (ctx->current_data << 1) | sda_state;
                    ctx->bit_counter++;
                    
                    if (ctx->bit_counter == 8) {
                        ctx->state = I2C_ACK_DATA;
                    }
                }
                break;
                
            case I2C_ACK_DATA:
                if (scl_rising_edge) {
                    ctx->last_ack = sda_state;
                    ctx->bytes_decoded++;
                    
                    // preparar proximo byte
                    ctx->state = I2C_READ_DATA;
                    ctx->bit_counter = 0;
                    ctx->current_data = 0;
                }
                break;
                
            case I2C_WAIT_STOP:
                // esperar stop o repeated start asincrono
                break;
                
            case I2C_ERROR:
                // recuperar en proximo start
                break;
        }
        
        // guardar estado previo
        ctx->prev_pins_state = current_sample;
        ctx->sample_idx++;
    }
    
    return (int32_t)ctx->bytes_decoded;
}
