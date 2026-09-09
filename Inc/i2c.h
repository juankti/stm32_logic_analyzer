#ifndef I2C_H
#define I2C_H

#include <stdint.h>

// estados fsm i2c
typedef enum {
    I2C_IDLE,             // esperar start
    I2C_READ_ADDRESS,     // leer direccion
    I2C_ACK_ADDRESS,      // leer ack dir
    I2C_READ_DATA,        // leer datos
    I2C_ACK_DATA,         // leer ack datos
    I2C_WAIT_STOP,        // esperar stop
    I2C_ERROR             // error
} i2c_decoder_state_t;

// config i2c
typedef struct {
    uint32_t pin_sda; // pin sda
    uint32_t pin_scl; // pin scl
} i2c_config_t;

// contexto fsm
typedef struct {
    i2c_config_t config;
    i2c_decoder_state_t state;
    
    uint32_t sample_idx;         // indice actual
    uint32_t prev_pins_state;    // estado anterior
    
    uint8_t current_data;        // dato temporal
    uint8_t bit_counter;         // contador bits
    uint8_t is_read_op;          // 1: leer, 0: escribir
    uint8_t last_ack;            // 0: ack, 1: nack
    
    uint32_t bytes_decoded;      // total bytes
} i2c_decoder_ctx_t;

void I2C_FSM(uint16_t *buffer_states, uint32_t buffer_length, i2c_config_t *config);

#endif
