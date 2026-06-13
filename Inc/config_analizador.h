#ifndef CONFIG_ANALIZADOR_H
#define CONFIG_ANALIZADOR_H

#include "captura.h"
#include "uart.h"

typedef enum{
	NONE,
	UART
	// SPI
	// I2C
}Protocolo_t;

typedef struct{
	Protocolo_t protocolo_activo;
	union{
		UART_Config_T uart;
		//SPI_Config_T spi;...
	}settings;
}config_protocolo;

typedef struct{
	config_captura captura;
	config_protocolo protocolo;
}Config_Analizador_t;


extern Config_Analizador_t config_global;


#endif
