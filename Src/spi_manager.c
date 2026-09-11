#include "spi_manager.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;
static SPI_Device_t current_device = SPI_DEV_DISPLAY;


static void SPI_SetPrescaler(uint32_t prescaler) {
    if (hspi1.Init.BaudRatePrescaler != prescaler) {
        //Wait
        while (HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_BUSY_TX ||
               HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_BUSY_RX);

        __HAL_SPI_DISABLE(&hspi1);
        hspi1.Init.BaudRatePrescaler = prescaler;
        HAL_SPI_Init(&hspi1);
        __HAL_SPI_ENABLE(&hspi1);
    }
}

void SPI_Bus_Select(SPI_Device_t dev) {
    // Desactivar AMBOS Chip Selects
    HAL_GPIO_WritePin(DISP_CS_GPIO_Port, DISP_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_SET);

    // Ajustar la velocidad segun dispositivo
    if (dev == SPI_DEV_DISPLAY) {
        SPI_SetPrescaler(SPI_PRESCALER_DISPLAY);

        HAL_GPIO_WritePin(DISP_CS_GPIO_Port, DISP_CS_Pin, GPIO_PIN_RESET);
    }
    else if (dev == SPI_DEV_TOUCH) {
        SPI_SetPrescaler(SPI_PRESCALER_TOUCH);

        HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_RESET);
    }

    current_device = dev;
}

void SPI_Bus_Deselect(void) {
    // Wait until it finishes last task
    while (__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_BSY) != RESET);

    HAL_GPIO_WritePin(DISP_CS_GPIO_Port, DISP_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_SET);
}
