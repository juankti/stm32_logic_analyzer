#include "disp_driver.h"
#include "7x12_horizontal_LSB_1.h"
#include "spi_manager.h"
extern SPI_HandleTypeDef hspi1;

void DISP_EscribirComando(uint8_t cmd) {
    // 1. tomar bus
    SPI_Bus_Select(SPI_DEV_DISPLAY);

    // 2. definir modo comando (dc low)
    HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, GPIO_PIN_RESET);

    // 3. transmitir byte
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);

    // 4. liberar el bus
    SPI_Bus_Deselect();
}


void DISP_EscribirDatos(uint8_t datos) {
    // 1. tomar bus
    SPI_Bus_Select(SPI_DEV_DISPLAY);

    // 2. definir modo dato (dc high)
    HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, GPIO_PIN_SET);

    // 3. transmitir byte
    HAL_SPI_Transmit(&hspi1, &datos, 1, HAL_MAX_DELAY);

    // 4. liberar bus
    SPI_Bus_Deselect();
}


void DISP_EscribirDatos16(uint16_t datos) {
    // preparar el bufer msb primero (big endian que pide el ili9341)
    uint8_t buffer[2] = {
        (uint8_t)(datos >> 8),
        (uint8_t)(datos & 0xFF)
    };

    // 1. tomar bus
    SPI_Bus_Select(SPI_DEV_DISPLAY);

    // 2. definir modo dato (dc high)
    HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, GPIO_PIN_SET);

    // 3. transmitir ambos bytes en una rafaga
    HAL_SPI_Transmit(&hspi1, buffer, 2, HAL_MAX_DELAY);

    // 4. liberar bus
    SPI_Bus_Deselect();
}

void DISP_Reiniciar(void) {
    HAL_GPIO_WritePin(DISP_RST_GPIO_Port, DISP_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(50); // mantener en estado de reinicio
    HAL_GPIO_WritePin(DISP_RST_GPIO_Port, DISP_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(150); // espera a que el controlador de pantalla arranque
}


void DISP_Inicializar(void) {
    // 1. reinicio duro del hardware
    DISP_Reiniciar();

    // 2. reinicio por software
    DISP_EscribirComando(0x01);
    HAL_Delay(5); // esperar a que termine reinicio

    // 3. ajustes de energia
    DISP_EscribirComando(0xC0); // control energia 1
    DISP_EscribirDatos(0x23);    // vrh[5:0]

    DISP_EscribirComando(0xC1); // control poder 2
    DISP_EscribirDatos(0x10);    // sap[2:0];bt[3:0]

    DISP_EscribirComando(0xC5); // control vcm 1
    DISP_EscribirDatos(0x3e);
    DISP_EscribirDatos(0x28);

    DISP_EscribirComando(0xC7); // control vcm 2
    DISP_EscribirDatos(0x86);

    // 4. control acceso memoria
    DISP_EscribirComando(0x36);
    DISP_EscribirDatos(0x28);    // 0x28 = modo apaisado

    // 5. fijar formato pixel
    DISP_EscribirComando(0x3A);
    DISP_EscribirDatos(0x55);    // 0x55 = formato pixel 16-bit

    // 6. control tasa de cuadros
    DISP_EscribirComando(0xB1);
    DISP_EscribirDatos(0x00);
    DISP_EscribirDatos(0x18);

    // 7. salir modo suspension
    DISP_EscribirComando(0x11);
    HAL_Delay(120); // el datasheet requiere una espera de 120ms tras salir de suspension

    // 8. encender pantalla
    DISP_EscribirComando(0x29);
}


void DISP_FijarVentanaDireccion(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    DISP_EscribirComando(0x2A);
    DISP_EscribirDatos(x0 >> 8);
    DISP_EscribirDatos(x0 & 0xFF);
    DISP_EscribirDatos(x1 >> 8);
    DISP_EscribirDatos(x1 & 0xFF);

    DISP_EscribirComando(0x2B);
    DISP_EscribirDatos(y0 >> 8);
    DISP_EscribirDatos(y0 & 0xFF);
    DISP_EscribirDatos(y1 >> 8);
    DISP_EscribirDatos(y1 & 0xFF);

    DISP_EscribirComando(0x2C); // preparar escritura ram
}
void DISP_LlenarRect(uint16_t x, uint16_t y, uint16_t ancho, uint16_t alto, uint16_t color) {
    if (ancho == 0 || alto == 0) return;

    DISP_FijarVentanaDireccion(x, y, x + ancho - 1, y + alto - 1);

    uint32_t pixeles_totales = (uint32_t)ancho * alto;

    static uint8_t buffer_linea[480];
    uint16_t pixeles_buffer = (ancho > 240) ? 240 : ancho;

    uint8_t color_alto = color >> 8;
    uint8_t color_bajo  = color & 0xFF;
    for (uint16_t i = 0; i < pixeles_buffer; i++) {
        buffer_linea[i * 2]     = color_alto;
        buffer_linea[i * 2 + 1] = color_bajo;
    }

    SPI_Bus_Select(SPI_DEV_DISPLAY);
    HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, GPIO_PIN_SET);

    uint32_t pixeles_enviados = 0;
    while (pixeles_enviados < pixeles_totales) {
        uint32_t restante = pixeles_totales - pixeles_enviados;
        uint32_t para_enviar = (restante > pixeles_buffer) ? pixeles_buffer : restante;

        HAL_SPI_Transmit_DMA(&hspi1, buffer_linea, para_enviar * 2);
        while (HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY);

        pixeles_enviados += para_enviar;
    }

    SPI_Bus_Deselect();
}


void DISP_DibujarCaracter(uint16_t x, uint16_t y, char c, uint16_t colorTexto, uint16_t colorFondo) {
    const char* letra = fuente[(uint8_t)c];
    uint8_t buffer_caracteres[8 * 12 * 2]; // 8x12 pixeles * 2 bytes por pixel
    uint16_t indice_buffer = 0;

    uint8_t alto_texto = colorTexto >> 8, texto_l = colorTexto & 0xFF;
    uint8_t fondo_h   = colorFondo >> 8,   fondo_l   = colorFondo & 0xFF;

    // 1. armar el bufer en ram
    for (uint8_t fila = 0; fila < 12; fila++) {
        uint8_t linea = (uint8_t)letra[fila];
        for (uint8_t columna = 0; columna < 8; columna++) {
            if (linea & (1 << columna)) {
                buffer_caracteres[indice_buffer++] = alto_texto;
                buffer_caracteres[indice_buffer++] = texto_l;
            } else {
                buffer_caracteres[indice_buffer++] = fondo_h;
                buffer_caracteres[indice_buffer++] = fondo_l;
            }
        }
    }

    // 2. definir la ventana exacta para el caracter
    DISP_FijarVentanaDireccion(x, y, x + 7, y + 11);

    // 3. enviar todo el caracter en un solo disparo spi
    SPI_Bus_Select(SPI_DEV_DISPLAY);
    HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, GPIO_PIN_SET);

    HAL_SPI_Transmit(&hspi1, buffer_caracteres, sizeof(buffer_caracteres), HAL_MAX_DELAY);

    SPI_Bus_Deselect();
}

void DISP_DibujarTextoCentrado(uint16_t rectX, uint16_t rectY, uint16_t anchoRect, uint16_t altoRect,
                        const char * texto, uint16_t colorTexto, uint16_t colorRect) {

    // 1. dibujar fondo del rectangulo
    DISP_LlenarRect(rectX, rectY, anchoRect, altoRect, colorRect);

    // 2. calcular dimensiones del texto
    uint16_t ancho_caracter = 8;  // ajustado a 8 pixeles de ancho real
    uint16_t altura_caracter = 12;

    uint16_t longitud_texto = 0;
    while(texto[longitud_texto] != '\0') longitud_texto++;

    uint16_t ancho_texto = (longitud_texto * ancho_caracter);
    uint16_t alto_texto = altura_caracter;

    // 3. centrar
    uint16_t inicioX = rectX + (anchoRect - ancho_texto) / 2;
    uint16_t inicioY = rectY + (altoRect - alto_texto) / 2;

    // 4. dibujar caracteres rapidamente
    for(uint16_t i = 0; i < longitud_texto; i++) {
        DISP_DibujarCaracter(inicioX + (i * ancho_caracter), inicioY, texto[i], colorTexto, colorRect);
    }
}

void DISP_DibujarLineaH(uint16_t x, uint16_t y, uint16_t longitud, uint16_t color) {
    DISP_LlenarRect(x, y, longitud, 1, color);
}

void DISP_DibujarLineaV(uint16_t x, uint16_t y, uint16_t longitud, uint16_t color) {
    DISP_LlenarRect(x, y, 1, longitud, color);
}





void DISP_DibujarCadena(uint16_t x, uint16_t y, const char *cad, uint16_t fg, uint16_t bg) {
    uint16_t longitud = 0;
    while (cad[longitud] != '\0') {
        DISP_DibujarCaracter(x + (longitud * 8), y, cad[longitud], fg, bg);
        longitud++;
    }
}

