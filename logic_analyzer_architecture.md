# Arquitectura y Diseño del Analizador Lógico basado en STM32

Este documento detalla la arquitectura, el diseño de hardware y el funcionamiento lógico de un analizador lógico implementado sobre un microcontrolador STM32F401CCUX. Sirve como base técnica exhaustiva para la redacción de un artículo de investigación.

## 1. Visión General del Sistema

El proyecto consiste en un analizador lógico portátil con interfaz gráfica integrada, capaz de capturar y decodificar señales digitales en tiempo real. Soporta protocolos de comunicación estándar como UART, SPI e I2C. La captura se basa en un esquema de Acceso Directo a Memoria (DMA) accionado por temporizadores (Timers) para garantizar un muestreo determinista y de alta precisión sin sobrecarga de la CPU.

## 2. Hardware y Periféricos

### 2.1. Microcontrolador Central
El sistema está basado en el **STM32F401CCUX** (núcleo ARM Cortex-M4), operando a una frecuencia máxima de reloj (configurada por PLL mediante HSI) para ofrecer la velocidad requerida en el muestreo de señales.

### 2.2. Interfaz de Captura (Sondas)
Las sondas lógicas se mapean directamente al puerto `GPIOA`, aprovechando su registro de entrada (`IDR`). Los pines asignados y sus roles según el protocolo son:

*   **PA8:** SCK (SPI)
*   **PA9:** TX (UART) / MOSI (SPI) / SDA (I2C)
*   **PA10:** RX (UART) / MISO (SPI) / SCL (I2C)
*   **PA11:** CS (SPI)
*   **PA12 & PA15:** Canales no conectados por defecto o reservas.

### 2.3. Interfaz de Usuario (Display y Táctil)
Se emplea una pantalla TFT y un controlador táctil. La comunicación se realiza a través de **SPI1** configurado en modo Master. 
*   **Control del Display:** `PA2` (CS), `PA3` (RST), `PA4` (DC).
*   **Control Táctil:** `PA0` (CS). Adicionalmente, cuenta con una interrupción externa en `PA1` (EXTI1) para detectar eventos de pulsación (`TOUCH_INT`).

### 2.4. Subsistema de Captura (DMA + Timers)
El núcleo de la adquisición de datos delega el muestreo a dos flujos de DMA coordinados por dos Timers, lo cual es vital para capturas a alta velocidad sin jitter por software:

1.  **Captura de Estados Lógicos (DMA2 Stream 1):** 
    *   Disparado por: `TIM1` (Canal 1).
    *   Origen: `GPIOA->IDR` (16 bits, capturando el estado de todos los pines de captura en paralelo).
    *   Destino: `buffer_states` en memoria RAM.
2.  **Captura de Tiempos / Timestamps (DMA1 Stream 6):**
    *   Disparado por: `TIM2` (Canal 2).
    *   Origen: `TIM2->CCR2` (32 bits, que contiene el valor del contador del timer al momento del trigger).
    *   Destino: `buffer_timestamps` en memoria RAM.

Ambos buffers operan con una profundidad fija de `1024` muestras (`NUM_MUESTRAS`).

## 3. Arquitectura del Software

El flujo de ejecución del analizador lógico está gobernado por una Máquina de Estados Finitos (FSM) no bloqueante alojada en el bucle principal (`main.c` -> `maquina_estado.c`).

### 3.1. Máquina de Estados Global (`Estado_FSM`)
*   **`ESTADO_READY`**: El sistema está en reposo, dibujando la interfaz de usuario e interaccionando con el panel táctil. Espera a que el usuario active la captura.
*   **`ESTADO_CONFIG`**: Detiene cualquier captura en curso y configura los punteros, tamaño de los buffers y habilitaciones de los flujos DMA y los triggers de los Timers.
*   **`ESTADO_CAPTURA`**: Habilita los flujos DMA y los Timers. El sistema monitorea la bandera de interrupción de transferencia completa de DMA (`DMA_LISR_TCIF1`). Al completarse la recolección de las 1024 muestras, se detienen los periféricos de captura y transiciona al siguiente estado.
*   **`ESTADO_DECOD`**: Llama a la sub-máquina de estados específica del protocolo seleccionado (UART, SPI, I2C) para interpretar los datos de `buffer_states` usando los tiempos de `buffer_timestamps`. Luego actualiza los buffers de pantalla e ingresa a `ESTADO_READY`.

### 3.2. Mecanismo de Interfaz Gráfica (UI)
La UI emplea un modelo de retención de estado (`disp_ui.c`, `interface.c`). El sistema consta de tres vistas:
1.  **Main Menu**: Botones para acceder a configuración ("Options") o iniciar captura ("REC").
2.  **Settings**: Permite seleccionar protocolo (UART/SPI/I2C), frecuencia de muestreo e instigador (Trigger: RISING/FALLING).
3.  **Viewer (Visor de Señales)**: Dibuja las formas de onda (Waveforms) extrayendo la información del buffer de estados. Permite desplazarse (Scroll `<`/`>`) y hacer zoom (`Z+`/`Z-`). Implementa optimizaciones de renderizado para evitar repintados completos de pantalla; redibuja únicamente las transiciones o líneas estables.

La gestión táctil funciona por interrupciones (EXTI1 activa un flag `touch_event_occurred`) procesado asíncronamente en el bucle principal, actualizando los estados visuales del botón presionado y ejecutando callbacks.

## 4. Lógica de Decodificación de Protocolos

Las rutinas de decodificación (`uart.c`, `spi.c`, `i2c.c`) procesan los arrays de captura offline (una vez terminada la captura en RAM), iterando a través de las 1024 muestras.

### 4.1. Decodificador UART
*   Calcula el ancho nominal en ticks de un bit (`ticks_per_bit = TIMER_FREQ_HZ / baudrate`).
*   **FSM Interna**: 
    1.  `UART_SEARCH_START`: Inspecciona las muestras hasta detectar un pin RX en estado bajo. Al encontrarlo, establece el tiempo objetivo (`t_target`) en la muestra actual más 1.5 veces `ticks_per_bit` (para caer en el centro del primer bit de datos).
    2.  `UART_SAMPLE_DATA`: Avanza por el buffer de timestamps comparando con `t_target`. Al alcanzar el instante de tiempo, toma el valor lógico del pin RX, conformando el byte. Incrementa el objetivo en `ticks_per_bit` por cada bit de datos.
    3.  `UART_VERIFY_STOP`: Verifica el bit de parada en el último `t_target`. Si es correcto, guarda el byte en `decoded_buffer`.

### 4.2. Decodificador SPI
*   Se basa estrictamente en la detección de flancos en los canales CS, SCK y MOSI/MISO.
*   Ignora datos si el pin CS (Chip Select) está inactivo (Alto).
*   Al estar CS bajo, detecta un flanco de subida en SCK (actualmente configurado para CPHA=0). 
*   Agrupa los bits capturados de la línea MOSI según el `bit_order` (MSB_FIRST / LSB_FIRST) hasta alcanzar el tamaño de la trama (`data_size`, usualmente 8 bits).

### 4.3. Decodificador I2C
*   Identifica el reloj SCL y la línea de datos SDA.
*   **Detección de Trama (Start/Stop):** Debido a que I2C es asíncrono entre ráfagas, el código asume que una pausa en la comunicación (evaluada observando diferencias de tiempo en `buffer_timestamps` mayores a 1000 microsegundos) implica un reseteo del paquete (nueva trama).
*   **Lectura de Datos:** Busca flancos de subida en el pin SCL y lee el pin SDA asumiendo formato MSB First. Al noveno bit, descarta el bit de ACK/NACK y guarda el byte decodificado.

## 5. Áreas de Mejora Futuras (Contexto)
Aunque no se menciona de forma explícita como "fallo" en la redacción científica, este diseño presenta oportunidades de mejora como:
*   **Disparador por Hardware:** Actualmente usa un trigger de software por polling, lo que puede causar bloqueos. Migrar a un Trigger de hardware mediante las unidades esclavas del Timer u otro pin EXTI.
*   **Rendimiento del DMA Circular/Doble Buffer:** En un futuro se podría operar en modo continuo para evitar la limitación de 1024 muestras mediante compresión o streaming de datos.
*   **Mejoras en el I2C:** Detectar de forma estricta las condiciones reales de START/STOP lógicas (transición en SDA con SCL en alto) en vez de basarse únicamente en el tiempo de inactividad de la señal. 
*   **Escalabilidad UI:** La lógica del visor de pantalla tiene cuellos de botella al recalcular las líneas. Implementar un buffer virtual de línea permitiría formas de onda más fluidas.
