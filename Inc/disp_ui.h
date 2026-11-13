#ifndef DISP_UI_H
#define DISP_UI_H
#include "disp_driver.h"

typedef void (*ButtonCallback)(void);

typedef struct {
	uint16_t x,y,w,h;
	uint16_t color;
	const char * texto;
	ButtonCallback alClic;
	uint8_t estaActivo;
} Button_t;

void UI_DibujarBoton(Button_t * boton);
void UI_ManejarClic(Button_t * boton, uint16_t toqueX, uint16_t toqueY);
void UI_ManejarActivo(Button_t * boton, uint16_t toqueX, uint16_t toqueY);
#endif
