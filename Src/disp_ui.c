#include <disp_ui.h>

#define COLOR_WHITE    0xFFFF
#define COLOR_BG_DARK  0x10A2
#define COLOR_UI_BTN_ACT 0x07FF

int inButtonArea(Button_t * btn, uint16_t touchX, uint16_t touchY){
	uint8_t segX = (touchX >= btn->x && touchX <= btn->x + btn->w);
   	uint8_t segY = (touchY >= btn->y && touchY <= btn->y + btn->h);
    return (segX && segY);
}

void UI_DrawButton(Button_t * btn){
	uint16_t text_color = btn->isActive ? COLOR_BG_DARK : COLOR_WHITE;
	uint16_t bg_color = btn->isActive ? COLOR_UI_BTN_ACT : btn->color;
	DISP_DrawCenteredText(btn->x, btn->y, btn->w, btn->h, btn->text, text_color, bg_color);
}


void UI_HandleClick(Button_t * btn, uint16_t touchX, uint16_t touchY){
	uint8_t areaCheck = inButtonArea(btn, touchX, touchY);
	if(areaCheck && btn->onClick != NULL){
		btn->onClick();
	}
}

void UI_HandleActive(Button_t * btn, uint16_t touchX, uint16_t touchY){
		uint8_t areaCheck = inButtonArea(btn, touchX, touchY);
		if(areaCheck){
			btn->isActive = 1;
			return;
		}
		btn->isActive = 0;
}
