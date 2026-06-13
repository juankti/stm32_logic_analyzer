#ifndef MAQUINA_ESTADO_H
#define MAQUINA_ESTADO_H

#include <stdint.h>
#include "main.h"
#include "uart.h"
#include "captura.h"
#include "config_analizador.h"

typedef enum{
	ESTADO_IDLE,
	ESTADO_CONFIG,
	ESTADO_READY,
	ESTADO_CAPTURA,
	ESTADO_DECOD,
	ESTADO_EXPORT
}Estado_t;

void Estado_FSM();

#endif
