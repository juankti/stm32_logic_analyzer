/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "maquina_estado.h"
#include "uart.h"
#include "captura.h"
#include "config_analizador.h"
#include "disp_driver.h"
#include "touch_driver.h"
#include "interface.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */


/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
DMA_HandleTypeDef hdma_tim1_ch1;
DMA_HandleTypeDef hdma_tim2_ch2_ch4;

/* USER CODE BEGIN PV */

volatile uint32_t buffer_timestamps[MAX_NUM_MUESTRAS];
volatile uint16_t buffer_states[MAX_NUM_MUESTRAS];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);

static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


bool User_Pressed_Start(void) {
    extern volatile bool start_capture_flag;
    if (start_capture_flag) {
        start_capture_flag = false;
        return true;
    }
    return false;
}

void Hardware_Config_Trigger(void) {
    if (config_global.flanco == 2) return; // NONE
    
    // Software Trigger ultrarrápido por polling
    uint32_t mask = (1 << config_global.trigger_pin);
    if (config_global.flanco == 1) {
        while((GPIOA->IDR & mask) != 0) { } // FALLING
    } else {
        while((GPIOA->IDR & mask) == 0) { } // RISING
    }
}

volatile uint8_t touch_event_occurred = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == TOUCH_INT_Pin) {
        touch_event_occurred = 1;
    }
    if (GPIO_Pin == ENCODER_SW_Pin) {
        static uint32_t press_time = 0;
        uint32_t current_time = HAL_GetTick();
        if (HAL_GPIO_ReadPin(ENCODER_SW_GPIO_Port, ENCODER_SW_Pin) == GPIO_PIN_RESET) {
            press_time = current_time;
        } else {
              if (current_time - press_time > 1000) {
                  extern void OnEncoderButtonLongPress(void);
                  OnEncoderButtonLongPress();
              } else if (current_time - press_time > 50) {
                  extern void OnEncoderButtonPress(void);
                  OnEncoderButtonPress();
              }
        }
    }
}

void Update_Display_And_Log(void) {
    // 1. Configuramos la estructura de captura para la interfaz
    capture.active_channels = 6;
    capture.total_samples = Config_GetSizeValue(conf.buffer_size);

    // Obtener los labels de la configuración actual
    const ProtocolPinConfig_t *cfg = Config_GetPinConfig(conf.protocol);
    for (uint8_t ch = 0; ch < LOGIC_NUM_CHANNELS; ch++) {
        capture.channel_labels[ch] = cfg->channels[ch].label;
    }

    // 2. Extraer los datos de buffer_states (16 bits) al arreglo de la interfaz (1 byte por canal)
    for (uint16_t i = 0; i < capture.total_samples; i++) {
        uint16_t state = buffer_states[i];
        for (uint8_t ch = 0; ch < LOGIC_NUM_CHANNELS; ch++) {
            // Suponemos que los canales corresponden a los primeros pines configurados
            // Si están en orden distinto, se puede usar cfg->channels[ch].pin como máscara
            uint16_t pin_mask = cfg->channels[ch].pin;
            capture.samples[ch][i] = (state & pin_mask) ? 1 : 0;
        }
    }

    // 3. Forzar el refresco de la vista de ondas
    extern uint32_t viewer_t_scroll;
    viewer_t_scroll = 0;
    viewer_needs_full_redraw = 1;
    extern bool has_captured;
    has_captured = true;
    current_state = STATE_MAIN_MENU;
    needs_refresh = 1;
}

#define MAX_DECODED_CHARS 256
volatile uint8_t decoded_buffer[MAX_DECODED_CHARS];
volatile uint32_t decoded_idx = 0;

volatile uint16_t decoded_start_indices[MAX_DECODED_CHARS];
volatile uint16_t decoded_end_indices[MAX_DECODED_CHARS];

volatile uint32_t decoded_start_timestamps[MAX_DECODED_CHARS];
volatile uint32_t decoded_end_timestamps[MAX_DECODED_CHARS];

void Save_Decoded_Char(uint8_t c, uint32_t start_idx, uint32_t end_idx) {
    if (decoded_idx < MAX_DECODED_CHARS) {
        decoded_buffer[decoded_idx] = c;
        decoded_start_indices[decoded_idx] = start_idx;
        decoded_end_indices[decoded_idx] = end_idx;
        decoded_start_timestamps[decoded_idx] = buffer_timestamps[start_idx];
        decoded_end_timestamps[decoded_idx] = buffer_timestamps[end_idx];
        decoded_idx++;
    }
}

void Save_Decoded_Char_With_Times(uint8_t c, uint32_t start_idx, uint32_t end_idx, uint32_t t_start, uint32_t t_end) {
    if (decoded_idx < MAX_DECODED_CHARS) {
        decoded_buffer[decoded_idx] = c;
        decoded_start_indices[decoded_idx] = start_idx;
        decoded_end_indices[decoded_idx] = end_idx;
        decoded_start_timestamps[decoded_idx] = t_start;
        decoded_end_timestamps[decoded_idx] = t_end;
        decoded_idx++;
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  config_global.protocolo.protocolo_activo = UART;
  config_global.protocolo.settings.uart.baudrate = 9600;
  config_global.protocolo.settings.uart.data_bits = 8;
  config_global.protocolo.settings.uart.paridad = UART_PARITY_SIN;
  config_global.protocolo.settings.uart.stop_bits = UART_STOPBITS_1;
  config_global.protocolo.settings.uart.lsb_first = 1;
  config_global.protocolo.settings.uart.inverted = 0;
  
  config_global.protocolo.settings.spi.mode = SPI_MODE_0;
  config_global.protocolo.settings.spi.data_size = 8;
  config_global.protocolo.settings.spi.bit_order = SPI_MSB_FIRST;
  // Suponiendo que el Trigger (flanco de bajada) ocurre en el pin RX
  config_global.trigger_pin = 8; // Pin 10 (PA10 = RX)


  Configurar_Disparo_Protocolo();
  DISP_Init();
  TOUCH_Init();
  DISP_FillRect(0, 0, 320, 240, COLOR_BG_DARK);
  // Clear stale RAM (STM32 does NOT zero RAM on warm reset)
  decoded_idx = 0;
  for (uint16_t i = 0; i < MAX_DECODED_CHARS; i++) {
      decoded_buffer[i] = 0;
      decoded_start_indices[i] = 0;
      decoded_end_indices[i] = 0;
  }
  for (uint16_t i = 0; i < MAX_NUM_MUESTRAS; i++) buffer_states[i] = 0;
  Update_Display_And_Log();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    static uint32_t last_encoder_cnt = 0;
    uint32_t current_encoder_cnt = TIM3->CNT;
    if (current_encoder_cnt != last_encoder_cnt) {
        int16_t diff = (int16_t)current_encoder_cnt - (int16_t)last_encoder_cnt;
        if (diff >= 4) {
             extern void OnEncoderScrollRight(void);
             OnEncoderScrollRight();
             last_encoder_cnt = current_encoder_cnt;
        } else if (diff <= -4) {
             extern void OnEncoderScrollLeft(void);
             OnEncoderScrollLeft();
             last_encoder_cnt = current_encoder_cnt;
        }
    }

    Estado_FSM();

    // --- Manejo del renderizado UI ---
    if (needs_refresh) {
        switch (current_state) {
            case STATE_MAIN_MENU:
                Draw_MainMenu();
                break;
            case STATE_VIEWER:
                Draw_Viewer();
                break;
            case STATE_SETTINGS:
                Draw_Settings();
                break;
            case STATE_PROTOCOL_CONFIG:
                Draw_ProtocolConfig();
                break;
            case STATE_TABLE_VIEW:
                Draw_TableView();
                break;
        }
        needs_refresh = 0;
    }

    // --- Manejo de la lógica táctil ---
    if (touch_event_occurred || TS_State.is_pressed || TS_State.just_released) {
        TOUCH_Update();

        uint16_t px = TS_State.x;
        uint16_t py = TS_State.y;
        View_t *active_view = &views[current_state];

        if (active_view->buttons != NULL && active_view->buttonCount > 0) {
            if (TS_State.is_pressed) {
                for (uint8_t i = 0; i < active_view->buttonCount; i++) {
                    Button_t *btn = &active_view->buttons[i];
                    uint8_t was_active = btn->isActive;

                    UI_HandleActive(btn, px, py);

                    if (btn->isActive != was_active) {
                        UI_DrawButton(btn);
                    }
                }
            }
            
            if (TS_State.just_released) {
                for (uint8_t i = 0; i < active_view->buttonCount; i++) {
                    Button_t *btn = &active_view->buttons[i];
                    if (btn->isActive) {
                        btn->isActive = 0;
                        UI_DrawButton(btn);
                        if (btn->onClick != NULL) {
                            btn->onClick();
                        }
                    }
                }
                touch_event_occurred = 0;
            }
        }
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 83;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_BOTHEDGE;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */

static void MX_TIM3_Init(void)
{
  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 15;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 15;
  if (HAL_TIM_Encoder_Init(&htim3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
}

static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, TOUCH_CS_Pin|DISP_CS_Pin|DISP_RST_Pin|DISP_DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : TOUCH_CS_Pin DISP_CS_Pin DISP_RST_Pin DISP_DC_Pin */
  GPIO_InitStruct.Pin = TOUCH_CS_Pin|DISP_CS_Pin|DISP_RST_Pin|DISP_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : TOUCH_INT_Pin */
  GPIO_InitStruct.Pin = TOUCH_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(TOUCH_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : DISP_DCA9_Pin PA10 PA11 PA12
                           PA15 */
  GPIO_InitStruct.Pin = DISP_DCA9_Pin|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  __HAL_RCC_GPIOB_CLK_ENABLE();
  /*Configure GPIO pin : ENCODER_SW_Pin */
  GPIO_InitStruct.Pin = ENCODER_SW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(ENCODER_SW_GPIO_Port, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);


  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
    static uint32_t last_encoder_cnt = 0;
    uint32_t current_encoder_cnt = TIM3->CNT;
    if (current_encoder_cnt != last_encoder_cnt) {
        int16_t diff = (int16_t)current_encoder_cnt - (int16_t)last_encoder_cnt;
        if (diff >= 4) {
             extern void OnEncoderScrollRight(void);
             OnEncoderScrollRight();
             last_encoder_cnt = current_encoder_cnt;
        } else if (diff <= -4) {
             extern void OnEncoderScrollLeft(void);
             OnEncoderScrollLeft();
             last_encoder_cnt = current_encoder_cnt;
        }
    }

  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
