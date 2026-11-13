typedef enum {
    SPI_DEV_DISPLAY,
    SPI_DEV_TOUCH
} SPI_Device_t;

// pantalla
#define SPI_PRESCALER_DISPLAY   SPI_BAUDRATEPRESCALER_2
// tactil xpt2046
#define SPI_PRESCALER_TOUCH     SPI_BAUDRATEPRESCALER_64

void SPI_Bus_Select(SPI_Device_t dispositivo);
void SPI_Bus_Deselect(void);
