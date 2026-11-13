#ifndef DISP_DRIVER_H
#define DISP_DRIVER_H
#include "main.h" // trae todas las macros disp_ y definiciones hal
// dimensiones ili9341
#define DISP_WIDTH  320
#define DISP_HEIGHT 240
// abstraccion de hardware
void DISP_EscribirComando(uint8_t cmd);
void DISP_EscribirDatos(uint8_t datos);
void DISP_EscribirDatos16(uint16_t datos);
void DISP_Reiniciar(void);

void SPI1_SetPrescaler(uint32_t preescalador);
// control pantalla
void DISP_Inicializar(void);
void DISP_FijarVentanaDireccion(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

void DISP_LlenarRect(uint16_t x, uint16_t y, uint16_t ancho, uint16_t alto, uint16_t color);
void DISP_DibujarCaracter(uint16_t x, uint16_t y, char c, uint16_t colorTexto, uint16_t colorFondo);
void DISP_DibujarTextoCentrado(uint16_t rectX, uint16_t rectY, uint16_t anchoRect, uint16_t altoRect, const char * texto, uint16_t colorTexto, uint16_t colorRect);

void DISP_DibujarLineaH(uint16_t x, uint16_t y, uint16_t longitud, uint16_t color);
void DISP_DibujarLineaV(uint16_t x, uint16_t y, uint16_t longitud, uint16_t color);

void DISP_DibujarCadena(uint16_t x, uint16_t y, const char *cad, uint16_t fg, uint16_t bg);
#endif
