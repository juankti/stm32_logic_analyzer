#include "uart.h"

static UART_t uartState= UART_SEARCH_START;


void UART_FSM(uint16_t* buffer, uint32_t buffer_length, UART_Config_t* config) {
	uartState = UART_SEARCH_START;
	uint32_t sample_idx = 0;

    uint32_t baudrate = config->baudrate;
    uint8_t current_byte = 0;
    uint8_t bit_count = 0;
    uint32_t rx_pin_mask = (1 << config->pin_rx);
    uint32_t t_target = 0;
    uint32_t packet_start_idx = 0;

    uint8_t idle_state = config->inverted ? 0 : 1;
    uint8_t start_bit_state = config->inverted ? 1 : 0;
    uint8_t parity_bits = (config->paridad == UART_PARITY_SIN) ? 0 : 1;
    uint8_t stop_bits = (config->stop_bits == UART_STOPBITS_2) ? 2 : 1;
    uint32_t total_bits = 1 + config->data_bits + parity_bits + stop_bits;

    while (sample_idx < buffer_length) {
        bool rx_pin_state = (buffer[sample_idx] & rx_pin_mask) ? 1 : 0;

        switch (uartState) {
            case UART_SEARCH_START:
            	if (rx_pin_state == start_bit_state) {
            	    t_target = buffer_timestamps[sample_idx] + (uint32_t)(((uint64_t)TIMER_FREQ_HZ * 3) / (baudrate * 2));
                    current_byte = 0;
                    bit_count = 0;
                    uartState = UART_SAMPLE_DATA;
                    packet_start_idx = sample_idx;
                }
                sample_idx++; 
                break;

            case UART_SAMPLE_DATA:
            case UART_VERIFY_STOP:
                if (buffer_timestamps[sample_idx] > t_target) {
                    
                    uint8_t state_at_target = (buffer[sample_idx > 0 ? sample_idx - 1 : 0] & rx_pin_mask) ? 1 : 0;

                    if (uartState == UART_SAMPLE_DATA) {
                        uint8_t bit_val = config->inverted ? !state_at_target : state_at_target;
                        if (bit_count < config->data_bits) {
                            if (bit_val) {
                                if (config->lsb_first) {
                                    current_byte |= (1 << bit_count);
                                } else {
                                    current_byte |= (1 << (config->data_bits - 1 - bit_count));
                                }
                            }
                        }
                        bit_count++;

                        if (bit_count == config->data_bits + parity_bits) {
                            uartState = UART_VERIFY_STOP;
                        }
                        t_target = buffer_timestamps[packet_start_idx] + (uint32_t)(((uint64_t)TIMER_FREQ_HZ * (2 * bit_count + 3)) / (baudrate * 2));
                    } 
                    else if (uartState == UART_VERIFY_STOP) {
                        // For stop bit, state should be idle_state
                        if (state_at_target == idle_state) {
                            extern void Save_Decoded_Char_With_Times(uint8_t c, uint32_t start_idx, uint32_t end_idx, uint32_t t_start, uint32_t t_end);
                            uint32_t t_end = buffer_timestamps[packet_start_idx] + (uint32_t)(((uint64_t)TIMER_FREQ_HZ * total_bits) / baudrate);
                            Save_Decoded_Char_With_Times(current_byte, packet_start_idx, sample_idx, buffer_timestamps[packet_start_idx], t_end);
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
