#include "uart.h"

static UART_t uartState= UART_SEARCH_START;


void UART_FSM(uint16_t* buffer, uint32_t longitud_buffer, UART_Config_t* config) {
	uartState = UART_SEARCH_START;
	uint32_t indice_muestra = 0;

    uint32_t tasa_baudios = config->tasa_baudios;
    uint8_t byte_actual = 0;
    uint8_t conteo_bits = 0;
    uint32_t mascara_pin_rx = (1 << config->pin_rx);
    uint32_t objetivo_t = 0;
    uint32_t indice_inicio_paquete = 0;

    uint8_t estado_reposo = config->invertido ? 0 : 1;
    uint8_t estado_bit_inicio = config->invertido ? 1 : 0;
    uint8_t bits_paridad = (config->paridad == UART_PARITY_SIN) ? 0 : 1;
    uint8_t bits_parada = (config->bits_parada == UART_STOPBITS_2) ? 2 : 1;
    uint32_t bits_totales = 1 + config->bits_datos + bits_paridad + bits_parada;

    while (indice_muestra < longitud_buffer) {
        bool estado_pin_rx = (buffer[indice_muestra] & mascara_pin_rx) ? 1 : 0;

        switch (uartState) {
            case UART_SEARCH_START:
            	if (estado_pin_rx == estado_bit_inicio) {
            	    objetivo_t = tiempos_buffer[indice_muestra] + (uint32_t)(((uint64_t)TIMER_FREQ_HZ * 3) / (tasa_baudios * 2));
                    byte_actual = 0;
                    conteo_bits = 0;
                    uartState = UART_SAMPLE_DATA;
                    indice_inicio_paquete = indice_muestra;
                }
                indice_muestra++; 
                break;

            case UART_SAMPLE_DATA:
            case UART_VERIFY_STOP:
                if (tiempos_buffer[indice_muestra] > objetivo_t) {
                    
                    uint8_t estado_en_objetivo = (buffer[indice_muestra > 0 ? indice_muestra - 1 : 0] & mascara_pin_rx) ? 1 : 0;

                    if (uartState == UART_SAMPLE_DATA) {
                        uint8_t valor_bit = config->invertido ? !estado_en_objetivo : estado_en_objetivo;
                        if (conteo_bits < config->bits_datos) {
                            if (valor_bit) {
                                if (config->lsb_primero) {
                                    byte_actual |= (1 << conteo_bits);
                                } else {
                                    byte_actual |= (1 << (config->bits_datos - 1 - conteo_bits));
                                }
                            }
                        }
                        conteo_bits++;

                        if (conteo_bits == config->bits_datos + bits_paridad) {
                            uartState = UART_VERIFY_STOP;
                        }
                        objetivo_t = tiempos_buffer[indice_inicio_paquete] + (uint32_t)(((uint64_t)TIMER_FREQ_HZ * (2 * conteo_bits + 3)) / (tasa_baudios * 2));
                    } 
                    else if (uartState == UART_VERIFY_STOP) {
                        // para bit parada, el estado debe ser idle
                        if (estado_en_objetivo == estado_reposo) {
                            extern void Guardar_Caracter_Decodificado_Con_Tiempos(uint8_t c, uint32_t indice_inicio, uint32_t indice_fin, uint32_t inicio_t, uint32_t fin_t);
                            uint32_t fin_t = tiempos_buffer[indice_inicio_paquete] + (uint32_t)(((uint64_t)TIMER_FREQ_HZ * bits_totales) / tasa_baudios);
                            Guardar_Caracter_Decodificado_Con_Tiempos(byte_actual, indice_inicio_paquete, indice_muestra, tiempos_buffer[indice_inicio_paquete], fin_t);
                        }
                        uartState = UART_SEARCH_START;
                    }
                } else {
                    if (indice_muestra == longitud_buffer - 1) {
                        // llegamos al final del buffer y objetivo_t esta mas alla
                        // forzamos salida para evitar bucle infinito
                        indice_muestra++; 
                    } else {
                        // aun no hemos llegado a objetivo_t, vemos la siguiente transicion
                        indice_muestra++;
                    }
                }
                break;
                
            case UART_SAVE_BYTE:
                // ya no pasamos por aqui, guardamos directo
                uartState = UART_SEARCH_START;
                break;
        }
    }
}
