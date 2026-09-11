#ifndef INTERFACE_CONF_H
#define INTERFACE_CONF_H

#include "main.h"

#define PROTOCOL_COUNT 4
#define FREQUENCY_COUNT 3
#include "config_analizador.h"

typedef enum {
	kHz100,
	kHz400,
	kHz20
} Freq_t;
const char * Config_GetProtocolText(Protocolo_t protocol);
const char * Config_GetFreqText(Freq_t freq);

typedef struct {
	Protocolo_t protocol;
	Freq_t frequency;
} Configuration_t;

extern Configuration_t conf;

#define LOGIC_NUM_CHANNELS 6
#define LOGIC_BUFFER_SIZE 1000

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
