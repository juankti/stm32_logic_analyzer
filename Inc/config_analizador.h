#ifndef CONFIG_ANALIZADOR_H
#define CONFIG_ANALIZADOR_H

#include <stdint.h>
#include <stdbool.h>

#include "uart.h"
#include "spi.h"
#include "i2c.h"

typedef enum{
	NONE,
	UART,
	SPI,
	I2C
}Protocolo_t;

typedef struct{
	Protocolo_t protocolo_activo;
	union{
		UART_Config_t uart;
		spi_config_t spi;
		i2c_config_t i2c;
	}ajustes;
}config_protocolo;

typedef struct{
	// config captura

	uint32_t pin_disparo;
	uint8_t flanco; // 0: subida, 1: bajada, 2: nada
	
	// config protocolo
	config_protocolo protocolo;
}Config_Analizador_t;


extern Config_Analizador_t config_global;

// mapeo dinamico de disparo
void Configurar_Disparo_Protocolo(void);

#endif
