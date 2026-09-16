#include "interface_conf.h"

Configuration_t conf = {
	UART,
	kHz100,
    RADIX_HEX,
};

static const char * const PROTOCOLO_TEXT[PROTOCOL_COUNT] = {
		[UART] = "UART",
		[SPI]  = "SPI",
		[I2C]  = "I2C"
};

static const char * const FREQ_TEXT[FREQUENCY_COUNT] = {
		[kHz100] = "100 kHz",
		[kHz400] = "400 kHz",
		[kHz20]  = "20 kHz",
        [MHz1]   = "1 MHz"
};

static const char * const RADIX_TEXT[3] = {
        [RADIX_HEX] = "HEX",
        [RADIX_DEC] = "DEC",
        [RADIX_ASCII] = "ASCII"
};

const char * Config_GetProtocolText(Protocolo_t protocol){
	if (protocol < PROTOCOL_COUNT) return PROTOCOLO_TEXT[protocol];
	return "UNKNOWN";
}

const char * Config_GetFreqText(Freq_t freq){
	if (freq < FREQUENCY_COUNT) return FREQ_TEXT[freq];
	return "UNKNOWN";
}

const char * Config_GetRadixText(Radix_t radix){
	if (radix < 3) return RADIX_TEXT[radix];
	return "UNKNOWN";
}

ProtocolPinConfig_t pin_configs[PROTOCOL_COUNT] = {
	[UART] = {
		.protocol = UART,
		.num_active_channels = 6,
		.channels = {
			{.label = "CH1", .port = GPIOA, .pin = GPIO_PIN_8},
			{.label = "CH2", .port = GPIOA, .pin = GPIO_PIN_9},
			{.label = "CH3", .port = GPIOA, .pin = GPIO_PIN_10},
			{.label = "CH4", .port = GPIOA, .pin = GPIO_PIN_11},
			{.label = "CH5", .port = GPIOA, .pin = GPIO_PIN_12},
			{.label = "CH6", .port = GPIOA, .pin = GPIO_PIN_15},
		}
	},
	[SPI] = {
		.protocol = SPI,
		.num_active_channels = 6,
		.channels = {
			{.label = "CH1", .port = GPIOA, .pin = GPIO_PIN_8},
			{.label = "CH2", .port = GPIOA, .pin = GPIO_PIN_9},
			{.label = "CH3", .port = GPIOA, .pin = GPIO_PIN_10},
			{.label = "CH4", .port = GPIOA, .pin = GPIO_PIN_11},
			{.label = "CH5", .port = GPIOA, .pin = GPIO_PIN_12},
			{.label = "CH6", .port = GPIOA, .pin = GPIO_PIN_15},
		}
	},
	[I2C] = {
		.protocol = I2C,
		.num_active_channels = 6,
		.channels = {
			{.label = "CH1", .port = GPIOA, .pin = GPIO_PIN_8},
			{.label = "CH2", .port = GPIOA, .pin = GPIO_PIN_9},
			{.label = "CH3", .port = GPIOA, .pin = GPIO_PIN_10},
			{.label = "CH4", .port = GPIOA, .pin = GPIO_PIN_11},
			{.label = "CH5", .port = GPIOA, .pin = GPIO_PIN_12},
			{.label = "CH6", .port = GPIOA, .pin = GPIO_PIN_15},
		}
	}
};

const ProtocolPinConfig_t* Config_GetPinConfig(Protocolo_t protocol) {
	if (protocol < PROTOCOL_COUNT) {
		return &pin_configs[protocol];
	}
	return &pin_configs[I2C];
}
