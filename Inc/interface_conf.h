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

const char * Config_GetProtocolText(Protocolo_t protocol);
const char * Config_GetFreqText(Freq_t freq);
const char * Config_GetRadixText(Radix_t radix);
const char * Config_GetSizeText(BufferSize_t size);
uint32_t Config_GetSizeValue(BufferSize_t size);

typedef struct {
	Protocolo_t protocol;
	Freq_t frequency;
    Radix_t radix;
    BufferSize_t buffer_size;
} Configuration_t;

extern Configuration_t conf;

#define LOGIC_NUM_CHANNELS 6

typedef struct {
	const char *label;
	GPIO_TypeDef *port;
	uint16_t pin;
} PinChannel_t;

typedef struct {
	Protocolo_t protocol;
	uint8_t num_active_channels;
	PinChannel_t channels[LOGIC_NUM_CHANNELS];
} ProtocolPinConfig_t;

const ProtocolPinConfig_t* Config_GetPinConfig(Protocolo_t protocol);

#endif
