typedef enum {
    SPI_DEV_DISPLAY,
    SPI_DEV_TOUCH
} SPI_Device_t;

// Display
#define SPI_PRESCALER_DISPLAY   SPI_BAUDRATEPRESCALER_2
// Touch XPT2046
#define SPI_PRESCALER_TOUCH     SPI_BAUDRATEPRESCALER_64

void SPI_Bus_Select(SPI_Device_t dev);
void SPI_Bus_Deselect(void);
