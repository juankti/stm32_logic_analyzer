#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H
#include "main.h"

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t esta_presionado;
    uint8_t recien_soltado;
} TouchState_t;

extern TouchState_t TS_State;
extern volatile uint8_t ocurrio_evento_toque;

void TOUCH_Inicializar(void);
void TOUCH_Actualizar(void);
#endif
