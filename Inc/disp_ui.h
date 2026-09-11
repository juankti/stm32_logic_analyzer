#ifndef DISP_UI_H
#define DISP_UI_H
#include "disp_driver.h"

typedef void (*ButtonCallback)(void);

typedef struct {
	uint16_t x,y,w,h;
	uint16_t color;
	const char * text;
	ButtonCallback onClick;
	uint8_t isActive;
} Button_t;

void UI_DrawButton(Button_t * btn);
void UI_HandleClick(Button_t * btn, uint16_t touchX, uint16_t touchY);
void UI_HandleActive(Button_t * btn, uint16_t touchX, uint16_t touchY);
#endif
