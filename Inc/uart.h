#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

extern volatile uint32_t buffer_timestamps[];
extern volatile uint8_t  buffer_states[NUM_MUESTRAS];

typedef enum{
	UART_PARITY_SIN,
	UART_PARITY_PAR,
	UART_PARITY_IMPAR
}UART_Paridad_t;


typedef enum{
	UART_STOPBITS_1,
	UART_STOPBITS_2
} UART_StopBits_t;

typedef struct {
    uint32_t        baudrate;
    uint8_t         data_bits;  // ej: 8
    UART_Paridad_t   paridad;
    UART_StopBits_t stop_bits;
    uint32_t        pin_rx;     // pin rx
} UART_Config_t;

typedef enum{
	UART_SEARCH_START,
	UART_SAMPLE_DATA,
	UART_VERIFY_STOP,
	UART_SAVE_BYTE,
} UART_t;

void UART_FSM(uint8_t* buffer, uint32_t buffer_length, UART_Config_t* config);

#endif
