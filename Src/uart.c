#include "uart.h"

static UART_t uartState= UART_SEARCH_START;


void UART_FSM(uint8_t* buffer, uint32_t buffer_length, UART_Config_t* config) {
	uartState = UART_SEARCH_START;
	uint32_t sample_idx = 0;

    uint32_t samples_per_bit = TIMER_FREQ_HZ / config->baudrate;

    uint8_t current_byte = 0;
    uint8_t bit_count = 0;
    uint8_t rx_pin_mask = 0x01; // Assuming UART RX is on GPIOA_PIN_0 (Bit 0)

    while (sample_idx < buffer_length) {
        bool rx_pin_state = (buffer[sample_idx] & rx_pin_mask) ? 1 : 0;

        switch (uartState) {
            case UART_SEARCH_START:
            	if ((buffer_states[sample_idx] & 0x01) == 0) { 
            	    uint32_t t_start = buffer_timestamps[sample_idx];
            	    uint32_t t_target = t_start + (samples_per_bit * 3 / 2);
                    uartState = UART_SAMPLE_DATA;
                } else {
                    sample_idx++;
                }
                break;

            case UART_SAMPLE_DATA:
                if (rx_pin_state) {
                    current_byte |= (1 << bit_count);
                }
                bit_count++;

                if (bit_count == config->data_bits) {
                    // Todos los bits de datos leidos.
                    // TODO: Aqui agregarias la logica si config->paridad != UART_PARITY_SIN
                    
                    // Saltamos para chequear el bit de Stop
                    sample_idx += samples_per_bit;
                    uartState = UART_VERIFY_STOP;
                } else {
                    // Jump 1 full bit time to the middle of the next bit
                    sample_idx += samples_per_bit;
                }
                break;

            case UART_VERIFY_STOP:
                // The Stop bit MUST be HIGH.
                if (rx_pin_state == 1) {
                    uartState = UART_SAVE_BYTE;
                } else {
                    // Framing error detected
                    Log_Error("Framing Error");
                    uartState = UART_SEARCH_START;
                }
                break;

            case UART_SAVE_BYTE:
                // Add the decoded character to your final payload array
                Save_Decoded_Char(current_byte);
                uartState = UART_SEARCH_START;
                break;
        }
    }
}
