#ifndef CAPTURA_H
#define CAPTURA_H

#include "main.h"
#include "uart.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum{
	CAPTURA_CK_INTERNO,
	CAPTURA_CK_EXTERNO
}Captura_t;

typedef struct{
	Captura_t modo;
	uint32_t freq_muestreo;
	uint32_t trigger_pin;
	bool flanco; // 0 : subida , 1 : bajada
}config_captura;


void Conf_Captura(config_captura* config);
void Init_Captura();
void Stop_Captura();
bool Captura_Full();


#endif
