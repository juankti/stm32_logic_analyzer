#include "disp_driver.h"
#include "7x12_horizontal_LSB_1.h"
#include "spi_manager.h"
extern SPI_HandleTypeDef hspi1;

void DISP_WriteCommand(uint8_t cmd) {
    // 1. Tomar el bus (Asegura velocidad de Display y activa DISP_CS en LOW)
    SPI_Bus_Select(SPI_DEV_DISPLAY);

    // 2. Definir modo Comando (DC LOW)
    HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, GPIO_PIN_RESET);

    // 3. Transmitir el byte
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);

    // 4. Liberar el bus (Espera que BSY = 0 y pasa CS a HIGH)
    SPI_Bus_Deselect();
}


void DISP_WriteData(uint8_t data) {
    // 1. Tomar el bus
    SPI_Bus_Select(SPI_DEV_DISPLAY);

    // 2. Definir modo Dato (DC HIGH)
    HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, GPIO_PIN_SET);

    // 3. Transmitir el byte
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);

    // 4. Liberar el bus
    SPI_Bus_Deselect();
}


void DISP_WriteData16(uint16_t data) {
    // Preparar el búfer MSB primero (Big Endian que pide el ILI9341)
    uint8_t buffer[2] = {
        (uint8_t)(data >> 8),
        (uint8_t)(data & 0xFF)
    };

    // 1. Tomar el bus
    SPI_Bus_Select(SPI_DEV_DISPLAY);

    // 2. Definir modo Dato (DC HIGH)
    HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, GPIO_PIN_SET);

    // 3. Transmitir ambos bytes en una sola ráfaga
    HAL_SPI_Transmit(&hspi1, buffer, 2, HAL_MAX_DELAY);

    // 4. Liberar el bus
    SPI_Bus_Deselect();
}

void DISP_Reset(void) {
    HAL_GPIO_WritePin(DISP_RST_GPIO_Port, DISP_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(50); // Hold in reset state
    HAL_GPIO_WritePin(DISP_RST_GPIO_Port, DISP_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(150); // Wait for the display controller to boot up
}


void DISP_Init(void) {
    // 1. Hard reset the hardware
    DISP_Reset();

    // 2. Software Reset
    DISP_WriteCommand(0x01);
    HAL_Delay(5); // Wait for reset to finish

    // 3. Power Control Settings
    DISP_WriteCommand(0xC0); // Power control 1
    DISP_WriteData(0x23);    // VRH[5:0]

    DISP_WriteCommand(0xC1); // Power control 2
    DISP_WriteData(0x10);    // SAP[2:0];BT[3:0]

    DISP_WriteCommand(0xC5); // VCM control 1
    DISP_WriteData(0x3e);
    DISP_WriteData(0x28);

    DISP_WriteCommand(0xC7); // VCM control 2
    DISP_WriteData(0x86);

    // 4. Memory Access Control (Orientation & Color Order)
    DISP_WriteCommand(0x36);
    DISP_WriteData(0x28);    // 0x28 = Landscape mode (MV=1, BGR=1)

    // 5. Pixel Format Set
    DISP_WriteCommand(0x3A);
    DISP_WriteData(0x55);    // 0x55 = 16-bit pixel format (RGB565)

    // 6. Frame Rate Control
    DISP_WriteCommand(0xB1);
    DISP_WriteData(0x00);
    DISP_WriteData(0x18);

    // 7. Exit Sleep Mode
    DISP_WriteCommand(0x11);
    HAL_Delay(120); // The datasheet requires a 120ms wait after sleep out

    // 8. Turn on Display
    DISP_WriteCommand(0x29);
}


void DISP_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    DISP_WriteCommand(0x2A);
    DISP_WriteData(x0 >> 8);
    DISP_WriteData(x0 & 0xFF);
    DISP_WriteData(x1 >> 8);
    DISP_WriteData(x1 & 0xFF);

    DISP_WriteCommand(0x2B);
    DISP_WriteData(y0 >> 8);
    DISP_WriteData(y0 & 0xFF);
    DISP_WriteData(y1 >> 8);
    DISP_WriteData(y1 & 0xFF);

    DISP_WriteCommand(0x2C); // Prepare RAM write
}
void DISP_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
    if (width == 0 || height == 0) return;

    DISP_SetAddressWindow(x, y, x + width - 1, y + height - 1);

    uint32_t total_pixels = (uint32_t)width * height;

    static uint8_t line_buffer[480];
    uint16_t buffer_pixels = (width > 240) ? 240 : width;

    uint8_t color_high = color >> 8;
    uint8_t color_low  = color & 0xFF;
    for (uint16_t i = 0; i < buffer_pixels; i++) {
        line_buffer[i * 2]     = color_high;
        line_buffer[i * 2 + 1] = color_low;
    }

    SPI_Bus_Select(SPI_DEV_DISPLAY);
    HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, GPIO_PIN_SET);

    uint32_t pixels_sent = 0;
    while (pixels_sent < total_pixels) {
        uint32_t remaining = total_pixels - pixels_sent;
        uint32_t to_send = (remaining > buffer_pixels) ? buffer_pixels : remaining;

        HAL_SPI_Transmit_DMA(&hspi1, line_buffer, to_send * 2);
        while (HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY);

        pixels_sent += to_send;
    }

    SPI_Bus_Deselect();
}

void DISP_FillScreen(uint16_t color) {
    DISP_FillRect(0, 0, DISP_WIDTH, DISP_HEIGHT, color);
}

void DISP_DrawChar(uint16_t x, uint16_t y, char c, uint16_t textColor, uint16_t bgColor) {
    const char* glyph = font[(uint8_t)c];
    uint8_t char_buffer[8 * 12 * 2]; // 8x12 pixels * 2 bytes por pixel
    uint16_t buf_idx = 0;

    uint8_t text_h = textColor >> 8, text_l = textColor & 0xFF;
    uint8_t bg_h   = bgColor >> 8,   bg_l   = bgColor & 0xFF;

    // 1. Armar el búfer completo en RAM
    for (uint8_t row = 0; row < 12; row++) {
        uint8_t line = (uint8_t)glyph[row];
        for (uint8_t col = 0; col < 8; col++) {
            if (line & (1 << col)) {
                char_buffer[buf_idx++] = text_h;
                char_buffer[buf_idx++] = text_l;
            } else {
                char_buffer[buf_idx++] = bg_h;
                char_buffer[buf_idx++] = bg_l;
            }
        }
    }

    // 2. Definir la ventana exacta para el carácter
    DISP_SetAddressWindow(x, y, x + 7, y + 11);

    // 3. Enviar todo el carácter en un SOLO disparo SPI
    SPI_Bus_Select(SPI_DEV_DISPLAY);
    HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, GPIO_PIN_SET);

    HAL_SPI_Transmit(&hspi1, char_buffer, sizeof(char_buffer), HAL_MAX_DELAY);

    SPI_Bus_Deselect();
}

void DISP_DrawCenteredText(uint16_t rectX, uint16_t rectY, uint16_t rectW, uint16_t rectH,
                        const char * text, uint16_t textColor, uint16_t rectColor) {

    // 1. Dibujar fondo del rectángulo
    DISP_FillRect(rectX, rectY, rectW, rectH, rectColor);

    // 2. Calcular dimensiones del texto
    uint16_t charW = 8;  // Ajustado a 8 píxeles de ancho real
    uint16_t charH = 12;

    uint16_t textLen = 0;
    while(text[textLen] != '\0') textLen++;

    uint16_t textW = (textLen * charW);
    uint16_t textH = charH;

    // 3. Centrar
    uint16_t startX = rectX + (rectW - textW) / 2;
    uint16_t startY = rectY + (rectH - textH) / 2;

    // 4. Dibujar caracteres rápidamente
    for(uint16_t i = 0; i < textLen; i++) {
        DISP_DrawChar(startX + (i * charW), startY, text[i], textColor, rectColor);
    }
}

void DISP_DrawHLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
    DISP_FillRect(x, y, length, 1, color);
}

void DISP_DrawVLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
    DISP_FillRect(x, y, 1, length, color);
}

static uint8_t stream_line_buffer[480];
static uint8_t stream_color_h, stream_color_l;
static uint16_t stream_buf_pixels;

void DISP_OpenStream(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    DISP_SetAddressWindow(x0, y0, x1, y1);
    SPI_Bus_Select(SPI_DEV_DISPLAY);
    HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, GPIO_PIN_SET);
    stream_buf_pixels = 240;
}

void DISP_StreamFill(uint32_t pixel_count, uint16_t color) {
    stream_color_h = color >> 8;
    stream_color_l = color & 0xFF;

    for (uint16_t i = 0; i < stream_buf_pixels; i++) {
        stream_line_buffer[i * 2]     = stream_color_h;
        stream_line_buffer[i * 2 + 1] = stream_color_l;
    }

    uint32_t pixels_sent = 0;
    while (pixels_sent < pixel_count) {
        uint32_t remaining = pixel_count - pixels_sent;
        uint32_t to_send = (remaining > stream_buf_pixels) ? stream_buf_pixels : remaining;

        HAL_SPI_Transmit_DMA(&hspi1, stream_line_buffer, to_send * 2);
        while (HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY);

        pixels_sent += to_send;
    }
}

void DISP_CloseStream(void) {
    while (__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_BSY) != RESET);
    SPI_Bus_Deselect();
}

void DISP_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg) {
    uint16_t len = 0;
    while (str[len] != '\0') len++;
    if (len == 0) return;

    uint16_t total_w = len * 8;
    DISP_SetAddressWindow(x, y, x + total_w - 1, y + 11);

    SPI_Bus_Select(SPI_DEV_DISPLAY);
    HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, GPIO_PIN_SET);

    uint8_t fg_h = fg >> 8, fg_l = fg & 0xFF;
    uint8_t bg_h = bg >> 8, bg_l = bg & 0xFF;

    uint8_t char_buffer[8 * 12 * 2];

    for (uint16_t c = 0; c < len; c++) {
        const char* glyph = font[(uint8_t)str[c]];
        uint16_t buf_idx = 0;

        for (uint8_t row = 0; row < 12; row++) {
            uint8_t line = (uint8_t)glyph[row];
            for (uint8_t col = 0; col < 8; col++) {
                if (line & (1 << col)) {
                    char_buffer[buf_idx++] = fg_h;
                    char_buffer[buf_idx++] = fg_l;
                } else {
                    char_buffer[buf_idx++] = bg_h;
                    char_buffer[buf_idx++] = bg_l;
                }
            }
        }

        HAL_SPI_Transmit(&hspi1, char_buffer, sizeof(char_buffer), HAL_MAX_DELAY);
    }

    SPI_Bus_Deselect();
}

