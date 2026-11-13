#include "spi.h"
#include <stdbool.h>

extern volatile uint8_t buffer_decodificado[];
extern volatile uint32_t indice_decodificado;
extern void Guardar_Caracter_Decodificado(uint8_t c, uint32_t indice_inicio, uint32_t indice_fin);

void SPI_FSM(uint16_t *estados_buffer, uint32_t longitud_buffer, spi_config_t *config) {
    uint8_t byte_actual = 0;
    uint8_t contador_bits = 0;
    uint16_t estado_previo = longitud_buffer > 0 ? estados_buffer[0] : 0xFFFF;
    
    // el dma dispara exactamente en el primer flanco de sck
    // el estado de sck justo antes de primera muestra era el opuesto
    // al invertirlo en estado_previo, la primera iteracion detecta transicion
    if (longitud_buffer > 0) {
        estado_previo ^= (1 << config->pin_sck);
    }
    
    uint32_t indice_inicio_paquete = 0; // asumir alto inicialmente
    
    indice_decodificado = 0; // limpiar buffer salida

    for (uint32_t i = 0; i < longitud_buffer; i++) {
        uint16_t estado_actual = estados_buffer[i];
        
        bool estado_sck = (estado_actual & (1 << config->pin_sck)) ? 1 : 0;
        bool estado_cs = (estado_actual & (1 << config->pin_cs)) ? 1 : 0;
        bool estado_sck_previo = (estado_previo & (1 << config->pin_sck)) ? 1 : 0;
        bool estado_mosi = (estado_actual & (1 << config->pin_mosi)) ? 1 : 0;
        
        if (contador_bits == 0) indice_inicio_paquete = i;
        
        bool estado_cs_prev = (estado_previo & (1 << config->pin_cs)) ? 1 : 0;
        if (estado_cs == 1 && estado_cs_prev == 0) {
            // flanco subida en cs (inactivo), reiniciar estado
            contador_bits = 0;
            byte_actual = 0;
        } 
        
        // procesar flancos siempre
        bool es_subida = !estado_sck_previo && estado_sck;
        bool es_bajada = estado_sck_previo && !estado_sck;
            bool muestra_ahora = false;
            
            if (config->modo == SPI_MODE_0 || config->modo == SPI_MODE_3) {
                muestra_ahora = es_subida;
            } else {
                muestra_ahora = es_bajada;
            }
            
            if (muestra_ahora) {
                // extraer bit mosi
                if (config->orden_bits == SPI_MSB_FIRST) {
                    byte_actual = (byte_actual << 1) | estado_mosi;
                } else {
                    byte_actual = byte_actual | (estado_mosi << contador_bits);
                }
                
                contador_bits++;
                
                if (contador_bits >= config->tamaño_datos && config->tamaño_datos > 0) {
                    // guardar byte ensamblado
                    Guardar_Caracter_Decodificado(byte_actual, indice_inicio_paquete, i);
                    contador_bits = 0;
                    byte_actual = 0;
                }
            }
        estado_previo = estado_actual;
    }
}
