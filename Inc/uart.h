#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"
extern volatile uint32_t tiempos_buffer[MAX_NUM_MUESTRAS];
extern volatile uint16_t  estados_buffer[MAX_NUM_MUESTRAS];
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
    uint32_t        tasa_baudios;
    uint8_t         bits_datos;  // ej: 8
    UART_Paridad_t   paridad;
    UART_StopBits_t bits_parada;
    uint32_t        pin_rx;     // pin rx
    uint8_t         lsb_primero;  // 1: lsb primero, 0: msb primero
    uint8_t         invertido;   // 1: logica invertida, 0: normal
} UART_Config_t;

typedef enum{
	UART_SEARCH_START,
	UART_SAMPLE_DATA,
	UART_VERIFY_STOP,
	UART_SAVE_BYTE,
} UART_t;

void UART_FSM(uint16_t* buffer, uint32_t longitud_buffer, UART_Config_t* config);

#endif
