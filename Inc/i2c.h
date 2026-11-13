#ifndef I2C_H
#define I2C_H

#include <stdint.h>

// estados fsm i2c
typedef enum {
    I2C_IDLE,             // esperar inicio
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
    i2c_decoder_state_t estado;
    
    uint32_t indice_muestra;         // indice actual
    uint32_t estado_pines_previo;    // estado anterior
    
    uint8_t datos_actuales;        // dato temporal
    uint8_t contador_bits;         // contador bits
    uint8_t es_op_lectura;          // 1: leer, 0: escribir
    uint8_t ultimo_ack;            // 0: ack, 1: nack
    
    uint32_t bytes_decodificados;      // bytes totales
} i2c_decoder_ctx_t;

void I2C_FSM(uint16_t *estados_buffer, uint32_t longitud_buffer, i2c_config_t *config);

#endif
