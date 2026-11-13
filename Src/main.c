/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @archivo           : main.c
  * @brief          : Main program body
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
// inclusiones
#include "main.h"

// inclusiones privadas
/* USER CODE BEGIN Includes */

#include "maquina_estado.h"
#include "uart.h"
#include "captura.h"
#include "config_analizador.h"
#include "disp_driver.h"
#include "touch_driver.h"
#include "interface.h"


/* USER CODE END Includes */

// typedef privado
/* USER CODE BEGIN PTD */


/* USER CODE END PTD */

// definicion privada
/* USER CODE BEGIN PD */

/* USER CODE END PD */

// macro privada
/* USER CODE BEGIN PM */

/* USER CODE END PM */

// variables privadas
SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
DMA_HandleTypeDef hdma_tim1_ch1;
DMA_HandleTypeDef hdma_tim2_ch2_ch4;

/* USER CODE BEGIN PV */

volatile uint32_t tiempos_buffer[MAX_NUM_MUESTRAS];
volatile uint16_t estados_buffer[MAX_NUM_MUESTRAS];
/* USER CODE END PV */

// prototipos funciones privadas
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);

static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */


/* USER CODE END PFP */

// codigo de usuario privado
/* USER CODE BEGIN 0 */

void Ejecutar_POST(void) {
    // 1. Mostrar pantalla de inicio
    DISP_LlenarRect(0, 0, DISP_WIDTH, DISP_HEIGHT, COLOR_BG_DARK);
    DISP_DibujarTextoCentrado(0, 40, DISP_WIDTH, 50, "ANALIZADOR LOGICO", COLOR_UI_ACCENT, COLOR_BG_DARK);
    DISP_DibujarTextoCentrado(0, 80, DISP_WIDTH, 20, "TDII - 2026", COLOR_SIG_LABEL, COLOR_BG_DARK);
    DISP_DibujarTextoCentrado(0, 110, DISP_WIDTH, 20, "Desarrollado por:", COLOR_SIG_LABEL, COLOR_BG_DARK);
    DISP_DibujarTextoCentrado(0, 130, DISP_WIDTH, 20, "Juan Ignacio Cati", COLOR_SIG_LABEL, COLOR_BG_DARK);
    DISP_DibujarTextoCentrado(0, 150, DISP_WIDTH, 20, "Facundo Gimenez", COLOR_SIG_LABEL, COLOR_BG_DARK);
    
    DISP_DibujarTextoCentrado(0, 200, DISP_WIDTH, 20, "Ejecutando POST...", COLOR_UI_BTN_ACT, COLOR_BG_DARK);

    HAL_Delay(1000); // Mostrar pantalla inicialmente

    uint8_t post_fallo = 0;

    // --- PUNTO 1: Verificaciones del Nucleo ---
    if (SystemCoreClock != 84000000) {
        post_fallo = 1;
    }
    // Leer causa de reset (para debug o log, opcional en pantalla)
    uint32_t reset_cause = RCC->CSR;
    RCC->CSR |= RCC_CSR_RMVF; // Limpiar banderas de reset
    HAL_Delay(500);

    // --- PUNTO 4: Perifericos ---
    // Verificar que los relojes de DMA esten activos
    if ((RCC->AHB1ENR & RCC_AHB1ENR_DMA1EN) == 0 || (RCC->AHB1ENR & RCC_AHB1ENR_DMA2EN) == 0) {
        post_fallo = 1;
    }
    // SPI1 (Display usa SPI1)
    if ((RCC->APB2ENR & RCC_APB2ENR_SPI1EN) == 0) {
        post_fallo = 1;
    }
    HAL_Delay(500);

    // --- PUNTO 5: Especificos del Analizador ---
    // Verificar Timers 1, 2, 3
    if ((RCC->APB2ENR & RCC_APB2ENR_TIM1EN) == 0 || 
        (RCC->APB1ENR & RCC_APB1ENR_TIM2EN) == 0 || 
        (RCC->APB1ENR & RCC_APB1ENR_TIM3EN) == 0) {
        post_fallo = 1;
    }
    HAL_Delay(500);

    // Resultado final
    DISP_LlenarRect(0, 200, DISP_WIDTH, 30, COLOR_BG_DARK);
    if (post_fallo) {
        DISP_DibujarTextoCentrado(0, 200, DISP_WIDTH, 20, "POST ERROR!", COLOR_UI_REC, COLOR_BG_DARK);
        while(1); // Detener el sistema si falla el hardware critico
    } else {
        DISP_DibujarTextoCentrado(0, 200, DISP_WIDTH, 20, "POST OK!", COLOR_SIG_HIGH, COLOR_BG_DARK);
    }
    
    HAL_Delay(1000); // Dar tiempo a leer antes de ir al menu
}


bool Usuario_Presiono_Inicio(void) {
    extern volatile bool bandera_inicio_captura;
    if (bandera_inicio_captura) {
        bandera_inicio_captura = false;
        return true;
    }
    return false;
}

void Hardware_Configurar_Disparo(void) {
    if (config_global.flanco == 2) return; // ninguno
    
    // disparo software ultrarrapido por polling
    uint32_t mascara = (1 << config_global.pin_disparo);
    if (config_global.flanco == 1) {
        while((GPIOA->IDR & mascara) != 0) { } // bajada
    } else {
        while((GPIOA->IDR & mascara) == 0) { } // subida
    }
}

volatile uint8_t ocurrio_evento_toque = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == TOUCH_INT_Pin) {
        ocurrio_evento_toque = 1;
    }
    if (GPIO_Pin == ENCODER_SW_Pin) {
        static uint32_t tiempo_presion = 0;
        uint32_t tiempo_actual = HAL_GetTick();
        if (HAL_GPIO_ReadPin(ENCODER_SW_GPIO_Port, ENCODER_SW_Pin) == GPIO_PIN_RESET) {
            tiempo_presion = tiempo_actual;
        } else {
              if (tiempo_actual - tiempo_presion > 1000) {
                  extern void AlPresionarLargoBotonEncoder(void);
                  AlPresionarLargoBotonEncoder();
              } else if (tiempo_actual - tiempo_presion > 50) {
                  extern void AlPresionarBotonEncoder(void);
                  AlPresionarBotonEncoder();
              }
        }
    }
}

void Actualizar_Pantalla_Y_Registro(void) {
    // 1. configuramos estructura de captura
    captura.canales_activos = 6;
    captura.muestras_totales = Config_ObtenerValorTamaño(conf.tamaño_buffer);

    // obtener etiquetas de la configuracion actual
    const ProtocolPinConfig_t *cfg = Config_ObtenerConfigPin(conf.protocolo);
    for (uint8_t ch = 0; ch < LOGIC_NUM_CHANNELS; ch++) {
        captura.etiquetas_canal[ch] = cfg->canales[ch].etiqueta;
    }

    // 2. extraer datos de estados_buffer al arreglo de interfaz
    for (uint16_t i = 0; i < captura.muestras_totales; i++) {
        uint16_t estado = estados_buffer[i];
        for (uint8_t ch = 0; ch < LOGIC_NUM_CHANNELS; ch++) {
            // suponemos que los canales corresponden a los primeros pines configurados
            // si estan en orden distinto, usar pin como mascara
            uint16_t mascara_pin = cfg->canales[ch].pin;
            captura.muestras[ch][i] = (estado & mascara_pin) ? 1 : 0;
        }
    }

    // 3. forzar refresco de vista de ondas
    extern uint32_t desplazamiento_t_visor;
    desplazamiento_t_visor = 0;
    visor_necesita_redibujo_completo = 1;
    extern bool ha_capturado;
    ha_capturado = true;
    estado_actual = STATE_MAIN_MENU;
    necesita_refresco = 1;
}

#define MAX_DECODED_CHARS 256
volatile uint8_t buffer_decodificado[MAX_DECODED_CHARS];
volatile uint32_t indice_decodificado = 0;

volatile uint32_t tiempos_inicio_decodificados[MAX_DECODED_CHARS];
volatile uint32_t tiempos_fin_decodificados[MAX_DECODED_CHARS];

void Guardar_Caracter_Decodificado(uint8_t c, uint32_t indice_inicio, uint32_t indice_fin) {
    if (indice_decodificado < MAX_DECODED_CHARS) {
        buffer_decodificado[indice_decodificado] = c;
        tiempos_inicio_decodificados[indice_decodificado] = tiempos_buffer[indice_inicio];
        tiempos_fin_decodificados[indice_decodificado] = tiempos_buffer[indice_fin];
        indice_decodificado++;
    }
}

void Guardar_Caracter_Decodificado_Con_Tiempos(uint8_t c, uint32_t indice_inicio, uint32_t indice_fin, uint32_t inicio_t, uint32_t fin_t) {
    if (indice_decodificado < MAX_DECODED_CHARS) {
        buffer_decodificado[indice_decodificado] = c;
        tiempos_inicio_decodificados[indice_decodificado] = inicio_t;
        tiempos_fin_decodificados[indice_decodificado] = fin_t;
        indice_decodificado++;
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

  // configuracion mcu

  // reinicio perifericos, inicializa flash y systick
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  // configurar reloj del sistema
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  // inicializa todos los perifericos configurados
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  config_global.protocolo.protocolo_activo = UART;
  config_global.protocolo.ajustes.uart.tasa_baudios = 9600;
  config_global.protocolo.ajustes.uart.bits_datos = 8;
  config_global.protocolo.ajustes.uart.paridad = UART_PARITY_SIN;
  config_global.protocolo.ajustes.uart.bits_parada = UART_STOPBITS_1;
  config_global.protocolo.ajustes.uart.lsb_primero = 1;
  config_global.protocolo.ajustes.uart.invertido = 0;
  
  config_global.protocolo.ajustes.spi.modo = SPI_MODE_0;
  config_global.protocolo.ajustes.spi.tamaño_datos = 8;
  config_global.protocolo.ajustes.spi.orden_bits = SPI_MSB_FIRST;
  // suponiendo que el disparo ocurre en el pin rx
  config_global.pin_disparo = 8; // pin 10 (pa10 = rx)


  Configurar_Disparo_Protocolo();
  DISP_Inicializar();
  TOUCH_Inicializar();
  
  // Ejecutar el POST (verificara nucleo, perifericos y timers de captura)
  Ejecutar_POST();
  
  DISP_LlenarRect(0, 0, 320, 240, COLOR_BG_DARK);
  // limpiar ram obsoleta
  indice_decodificado = 0;
  for (uint16_t i = 0; i < MAX_DECODED_CHARS; i++) {
      buffer_decodificado[i] = 0;
  }
  for (uint16_t i = 0; i < MAX_NUM_MUESTRAS; i++) estados_buffer[i] = 0;
  Actualizar_Pantalla_Y_Registro();
  /* USER CODE END 2 */

  // bucle infinito
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    static uint32_t ultimo_conteo_encoder = 0;
    uint32_t conteo_encoder_actual = TIM3->CNT;
    if (conteo_encoder_actual != ultimo_conteo_encoder) {
        int16_t diferencia = (int16_t)conteo_encoder_actual - (int16_t)ultimo_conteo_encoder;
        if (diferencia >= 4) {
             extern void AlDesplazarEncoderDer(void);
             AlDesplazarEncoderDer();
             ultimo_conteo_encoder = conteo_encoder_actual;
        } else if (diferencia <= -4) {
             extern void AlDesplazarEncoderIzq(void);
             AlDesplazarEncoderIzq();
             ultimo_conteo_encoder = conteo_encoder_actual;
        }
    }

    Estado_FSM();

    // --- manejo del renderizado ui ---
    if (necesita_refresco) {
        switch (estado_actual) {
            case STATE_MAIN_MENU:
                Dibujar_MenuPrincipal();
                break;
            case STATE_VIEWER:
                Dibujar_Visor();
                break;
            case STATE_SETTINGS:
                Dibujar_Ajustes();
                break;
            case STATE_PROTOCOL_CONFIG:
                Dibujar_ConfigProtocolo();
                break;
            case STATE_TABLE_VIEW:
                Dibujar_VistaTabla();
                break;
        }
        necesita_refresco = 0;
    }

    // --- manejo logica tactil ---
    if (ocurrio_evento_toque || TS_State.esta_presionado || TS_State.recien_soltado) {
        TOUCH_Actualizar();

        uint16_t px = TS_State.x;
        uint16_t py = TS_State.y;
        View_t *vista_activa = &vistas[estado_actual];

        if (vista_activa->botones != NULL && vista_activa->conteoBotones > 0) {
            if (TS_State.esta_presionado) {
                for (uint8_t i = 0; i < vista_activa->conteoBotones; i++) {
                    Button_t *boton = &vista_activa->botones[i];
                    uint8_t estaba_activo = boton->estaActivo;

                    UI_ManejarActivo(boton, px, py);

                    if (boton->estaActivo != estaba_activo) {
                        UI_DibujarBoton(boton);
                    }
                }
            }
            
            if (TS_State.recien_soltado) {
                for (uint8_t i = 0; i < vista_activa->conteoBotones; i++) {
                    Button_t *boton = &vista_activa->botones[i];
                    if (boton->estaActivo) {
                        boton->estaActivo = 0;
                        UI_DibujarBoton(boton);
                        if (boton->alClic != NULL) {
                            boton->alClic();
                        }
                    }
                }
                ocurrio_evento_toque = 0;
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

  // configurar voltaje de salida del regulador interno main
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

  // inicializa los relojes de buses cpu, ahb y apb
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

// habilitar reloj controlador dma
static void MX_DMA_Init(void)
{

  // habilitacion reloj controlador dma
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  // init interrupcion dma
  // configuracion interrupcion dma1_stream6_irqn
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
  // configuracion interrupcion dma2_stream1_irqn
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
  // configuracion interrupcion dma2_stream3_irqn
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

  // habilitacion reloj puertos gpio
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  // configurar nivel de salida de pin gpio
  HAL_GPIO_WritePin(GPIOA, TOUCH_CS_Pin|DISP_CS_Pin|DISP_RST_Pin|DISP_DC_Pin, GPIO_PIN_RESET);

  // configurar pines gpio : touch_cs_pin disp_cs_pin disp_rst_pin disp_dc_pin
  GPIO_InitStruct.Pin = TOUCH_CS_Pin|DISP_CS_Pin|DISP_RST_Pin|DISP_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // configurar pin gpio : touch_int_pin
  GPIO_InitStruct.Pin = TOUCH_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(TOUCH_INT_GPIO_Port, &GPIO_InitStruct);

  // configurar pines gpio : disp_dca9_pin pa10 pa11 pa12 pa15
  GPIO_InitStruct.Pin = DISP_DCA9_Pin|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // inicializacion interrupcion exti
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  __HAL_RCC_GPIOB_CLK_ENABLE();
  // configurar pin gpio : encoder_sw_pin
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
  // el usuario puede agregar implementacion de reporte error
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source archivo and the source linea number
  *         where the assert_param error has occurred.
  * @param  archivo: pointer to the source archivo name
  * @param  linea: assert_param error linea source number
  * @retval None
  */
void assert_failed(uint8_t *archivo, uint32_t linea)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the archivo name and linea number,
     ex: printf("Wrong parameters value: archivo %s on linea %d\r\n", archivo, linea) */
  /* USER CODE END 6 */
}
#endif // use_full_assert
