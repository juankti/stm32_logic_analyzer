/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @archivo           : main.h
  * @brief          : Header for main.c archivo.
  *                   This archivo contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE archivo
  * in the root directory of this software component.
  * If no LICENSE archivo comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

// definir para prevenir inclusion recursiva
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

// inclusiones
#include "stm32f4xx_hal.h"

// inclusiones privadas
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <stdbool.h>

/* USER CODE END Includes */

// tipos exportados
/* USER CODE BEGIN ET */

/* USER CODE END ET */

// constantes exportadas
/* USER CODE BEGIN EC */

/* USER CODE END EC */

// macro exportada
/* USER CODE BEGIN EM */
#define MAX_NUM_MUESTRAS 4096
#define TIMER_FREQ_HZ (84000000/(TIM2->PSC+1))
/* USER CODE END EM */

// prototipos funciones exportadas
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

// definiciones privadas
#define TOUCH_CS_Pin GPIO_PIN_0
#define TOUCH_CS_GPIO_Port GPIOA
#define TOUCH_INT_Pin GPIO_PIN_1
#define TOUCH_INT_GPIO_Port GPIOA
#define TOUCH_INT_EXTI_IRQn EXTI1_IRQn
#define DISP_CS_Pin GPIO_PIN_2
#define DISP_CS_GPIO_Port GPIOA
#define DISP_RST_Pin GPIO_PIN_3
#define DISP_RST_GPIO_Port GPIOA
#define DISP_DC_Pin GPIO_PIN_4
#define DISP_DC_GPIO_Port GPIOA
#define DISP_DCA9_Pin GPIO_PIN_9
#define DISP_DCA9_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */
#define ENCODER_SW_Pin GPIO_PIN_0
#define ENCODER_SW_GPIO_Port GPIOB



bool Usuario_Presiono_Inicio(void);
void Hardware_Configurar_Disparo(void);
void Actualizar_Pantalla_Y_Registro(void);
void Guardar_Caracter_Decodificado(uint8_t c, uint32_t indice_inicio, uint32_t indice_fin);
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif // __main_h
