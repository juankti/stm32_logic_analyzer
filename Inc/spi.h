#ifndef SPI_H
#define SPI_H

#include <stdint.h>

// modos spi (cpol y cpha)
typedef enum {
    SPI_MODE_0 = 0, // cpol=0, cpha=0 (1er flanco subida)
    SPI_MODE_1 = 1, // cpol=0, cpha=1 (2do flanco bajada)
    SPI_MODE_2 = 2, // cpol=1, cpha=0 (1er flanco bajada)
    SPI_MODE_3 = 3  // cpol=1, cpha=1 (2do flanco subida)
} spi_mode_t;

// orden de bits
typedef enum {
    SPI_MSB_FIRST = 0,
    SPI_LSB_FIRST = 1
} spi_bit_order_t;

// estados fsm spi
typedef enum {
    SPI_IDLE,         // esperar cs bajo
    SPI_WAIT_CS,      // cs activo
    SPI_WAIT_EDGE,    // esperar flanco sck
    SPI_SAMPLE_BIT,   // muestrear bit
    SPI_SAVE_BYTE,    // guardar byte
    SPI_ERROR         // error
} spi_decoder_state_t;

// config spi
typedef struct {
    uint32_t pin_mosi; // pin mosi
    uint32_t pin_miso; // pin miso
    uint32_t pin_sck;  // pin sck
    uint32_t pin_cs;   // pin cs
    
    spi_mode_t mode;             // modo spi
    spi_bit_order_t bit_order;   // msb o lsb
    uint8_t data_size;           // tamaño de palabra
} spi_config_t;

// contexto fsm
typedef struct {
    spi_config_t config;
    spi_decoder_state_t state;
    
    uint32_t sample_idx;         // indice del buffer
    uint32_t prev_pins_state;    // estado anterior
    
    uint16_t current_mosi_data;  // dato mosi
    uint16_t current_miso_data;  // dato miso
    uint8_t bit_counter;         // contador bits
    
    uint32_t bytes_decoded;      // total bytes
} spi_decoder_ctx_t;

void SPI_FSM(uint16_t *buffer_states, uint32_t buffer_length, spi_config_t *config);

#endif
