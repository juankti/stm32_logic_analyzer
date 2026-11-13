#ifndef INTERFACE_CONF_H
#define INTERFACE_CONF_H

#include "main.h"

#define PROTOCOL_COUNT 4
#define FREQUENCY_COUNT 7
#include "config_analizador.h"

typedef enum {
	kHz20,
	kHz100,
	kHz400,
    MHz1,
    MHz2,
    MHz4,
    MHz10_5
} Freq_t;
typedef enum {
    RADIX_HEX,
    RADIX_DEC,
    RADIX_ASCII
} Radix_t;

typedef enum {
    SIZE_0_5KB,
    SIZE_1KB,
    SIZE_2KB,
    SIZE_4KB
} BufferSize_t;

const char * Config_ObtenerTextoProtocolo(Protocolo_t protocolo);
const char * Config_ObtenerTextoFrec(Freq_t frec);
const char * Config_ObtenerTextoRadix(Radix_t radix);
const char * Config_ObtenerTextoTamaño(BufferSize_t tamaño);
uint32_t Config_ObtenerValorTamaño(BufferSize_t tamaño);

typedef struct {
	Protocolo_t protocolo;
	Freq_t frecuencia;
    Radix_t radix;
    BufferSize_t tamaño_buffer;
} Configuration_t;

extern Configuration_t conf;

#define LOGIC_NUM_CHANNELS 6

typedef struct {
	const char *etiqueta;
	GPIO_TypeDef *puerto;
	uint16_t pin;
} PinChannel_t;

typedef struct {
	Protocolo_t protocolo;
	uint8_t num_canales_activos;
	PinChannel_t canales[LOGIC_NUM_CHANNELS];
} ProtocolPinConfig_t;

const ProtocolPinConfig_t* Config_ObtenerConfigPin(Protocolo_t protocolo);

#endif
