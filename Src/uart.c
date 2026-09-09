#include "uart.h"

static UART_t uartState= UART_SEARCH_START;


void UART_FSM(uint16_t* buffer, uint32_t buffer_length, UART_Config_t* config) {
	uartState = UART_SEARCH_START;
	uint32_t sample_idx = 0;

    uint32_t ticks_per_bit = TIMER_FREQ_HZ / config->baudrate;
    uint8_t current_byte = 0;
    uint8_t bit_count = 0;
    uint32_t rx_pin_mask = (1 << config->pin_rx);
    uint32_t t_target = 0;

    while (sample_idx < buffer_length) {
        bool rx_pin_state = (buffer[sample_idx] & rx_pin_mask) ? 1 : 0;

        switch (uartState) {
            case UART_SEARCH_START:
            	if (rx_pin_state == 0) {
            	    t_target = buffer_timestamps[sample_idx] + (ticks_per_bit * 3 / 2);
                    current_byte = 0;
                    bit_count = 0;
                    uartState = UART_SAMPLE_DATA;
                }
                sample_idx++; // Siempre avanzamos en START hasta encontrar un flanco de bajada
                break;

            case UART_SAMPLE_DATA:
            case UART_VERIFY_STOP:
                if (buffer_timestamps[sample_idx] > t_target) {
                    
                    uint8_t state_at_target = (buffer[sample_idx > 0 ? sample_idx - 1 : 0] & rx_pin_mask) ? 1 : 0;

                    if (uartState == UART_SAMPLE_DATA) {
                        if (state_at_target) {
                            current_byte |= (1 << bit_count);
                        }
                        bit_count++;

                        if (bit_count == config->data_bits) {
                            uartState = UART_VERIFY_STOP;
                        }
                        t_target += ticks_per_bit;
                    } 
                    else if (uartState == UART_VERIFY_STOP) {
                        if (state_at_target == 1) {
                            Save_Decoded_Char(current_byte);
                        }
                        uartState = UART_SEARCH_START;
                    }
                } else {
                    if (sample_idx == buffer_length - 1) {
                        // Llegamos al final del buffer y t_target está más allá del último flanco.
                        // Forzamos la salida para no entrar en bucle infinito.
                        sample_idx++; 
                    } else {
                        // Aún no hemos llegado en el tiempo a t_target, vemos la siguiente transición
                        sample_idx++;
                    }
                }
                break;
                
            case UART_SAVE_BYTE:
                // Ya no pasamos por aquí, guardamos directo en VERIFY_STOP
                uartState = UART_SEARCH_START;
                break;
        }
    }
}
