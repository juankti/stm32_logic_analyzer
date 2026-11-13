#include <disp_ui.h>

#define COLOR_WHITE    0xFFFF
#define COLOR_BG_DARK  0x10A2
#define COLOR_UI_BTN_ACT 0x07FF

int enAreaBoton(Button_t * boton, uint16_t toqueX, uint16_t toqueY){
	uint8_t segX = (toqueX >= boton->x && toqueX <= boton->x + boton->w);
   	uint8_t segY = (toqueY >= boton->y && toqueY <= boton->y + boton->h);
    return (segX && segY);
}

void UI_DibujarBoton(Button_t * boton){
	uint16_t color_texto = boton->estaActivo ? COLOR_BG_DARK : COLOR_WHITE;
	uint16_t color_fondo = boton->estaActivo ? COLOR_UI_BTN_ACT : boton->color;
	DISP_DibujarTextoCentrado(boton->x, boton->y, boton->w, boton->h, boton->texto, color_texto, color_fondo);
}


void UI_ManejarClic(Button_t * boton, uint16_t toqueX, uint16_t toqueY){
	uint8_t chequeoArea = enAreaBoton(boton, toqueX, toqueY);
	if(chequeoArea && boton->alClic != NULL){
		boton->alClic();
	}
}

void UI_ManejarActivo(Button_t * boton, uint16_t toqueX, uint16_t toqueY){
		uint8_t chequeoArea = enAreaBoton(boton, toqueX, toqueY);
		if(chequeoArea){
			boton->estaActivo = 1;
			return;
		}
		boton->estaActivo = 0;
}
