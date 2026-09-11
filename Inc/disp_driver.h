#ifndef DISP_DRIVER_H
#define DISP_DRIVER_H
#include "main.h" // Pulls in all your DISP_ macros and HAL definitions
// ILI9341 Dimensions
#define DISP_WIDTH  320
#define DISP_HEIGHT 240
/*Hardware Abstraction */
void DISP_WriteCommand(uint8_t cmd);
void DISP_WriteData(uint8_t data);
void DISP_WriteData16(uint16_t data);
void DISP_Reset(void);

void SPI1_SetPrescaler(uint32_t prescaler);
/*Display Control */
void DISP_Init(void);
void DISP_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

void DISP_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void DISP_FillScreen(uint16_t color);

void DISP_DrawChar(uint16_t x, uint16_t y, char c, uint16_t textColor, uint16_t bgColor);
void DISP_DrawCenteredText(uint16_t rectX, uint16_t rectY, uint16_t rectW, uint16_t rectH, const char * text, uint16_t textColor, uint16_t rectColor);

void DISP_DrawHLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color);
void DISP_DrawVLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color);

void DISP_OpenStream(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void DISP_StreamFill(uint32_t pixel_count, uint16_t color);
void DISP_CloseStream(void);

void DISP_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg);
#endif
