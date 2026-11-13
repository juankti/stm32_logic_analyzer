#include "i2c.h"
#include <stdbool.h>

extern volatile uint8_t buffer_decodificado[];
extern volatile uint32_t indice_decodificado;
extern volatile uint32_t tiempos_buffer[];
extern void Guardar_Caracter_Decodificado(uint8_t c, uint32_t indice_inicio, uint32_t indice_fin);

void I2C_FSM(uint16_t *estados_buffer, uint32_t longitud_buffer, i2c_config_t *config) {
    uint8_t byte_actual = 0;
    uint8_t contador_bits = 0;
    uint16_t estado_previo = 0xFFFF; // asumir scl y sda altos inicialmente
    uint32_t tiempo_previo = 0;
    uint32_t indice_inicio_paquete = 0;
    
    indice_decodificado = 0; // limpiar buffer salida

    for (uint32_t i = 0; i < longitud_buffer; i++) {
        uint16_t estado_actual = estados_buffer[i];
        uint32_t tiempo_actual = tiempos_buffer[i];
        
        bool estado_scl = (estado_actual & (1 << config->pin_scl)) ? 1 : 0;
        bool estado_scl_previo = (estado_previo & (1 << config->pin_scl)) ? 1 : 0;
        bool estado_sda = (estado_actual & (1 << config->pin_sda)) ? 1 : 0;
        
        // pausa mayor a 1 ms es nuevo mensaje
        if ((tiempo_actual - tiempo_previo) > 1000) {
            contador_bits = 0;
            byte_actual = 0;
        }

        // en i2c los datos se leen en el flanco de subida de scl
        if (!estado_scl_previo && estado_scl) {
            
            if (contador_bits < 8) {
                // leer 8 bits de datos (msb primero)
                byte_actual = (byte_actual << 1) | estado_sda;
                contador_bits++;
            } else {
                // hemos llegado al bit 9 (ack/nack)
                // ignoramos valor del ack y guardamos el byte
                Guardar_Caracter_Decodificado(byte_actual, indice_inicio_paquete, i);
                contador_bits = 0;
                byte_actual = 0;
            }
        }
        
        estado_previo = estado_actual;
        tiempo_previo = tiempo_actual;
    }
}
