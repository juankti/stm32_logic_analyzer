#include "interface_conf.h"

Configuration_t conf = {
	I2C,
	kHz20,
};

static const char * const PROTOCOLO_TEXT[PROTOCOL_COUNT] = {
		[UART] = "UART",
		[SPI]  = "SPI",
		[I2C]  = "I2C"
};

static const char * const FREQ_TEXT[FREQUENCY_COUNT] = {
		[kHz100] = "100 kHz",
		[kHz400]  = "400 kHz",
		[kHz20]  = "20 kHz"
};

const char * Config_GetProtocolText(Protocolo_t protocol){
	if(protocol < PROTOCOL_COUNT){
		return PROTOCOLO_TEXT[protocol];
	}
	return "ERROR";
}


const char * Config_GetFreqText(Freq_t freq){
	if(freq < FREQUENCY_COUNT){
		return FREQ_TEXT[freq];
	}
	return "ERROR";
}

static const ProtocolPinConfig_t pin_configs[PROTOCOL_COUNT] = {
	[UART] = {
		.protocol = UART,
		.num_active_channels = 2,
		.channels = {
			{.label = "N/C", .port = GPIOA, .pin = GPIO_PIN_8},
			{.label = "TX",  .port = GPIOA, .pin = GPIO_PIN_9},
			{.label = "RX",  .port = GPIOA, .pin = GPIO_PIN_10},
			{.label = "N/C", .port = GPIOA, .pin = GPIO_PIN_11},
			{.label = "N/C", .port = GPIOA, .pin = GPIO_PIN_12},
			{.label = "N/C", .port = GPIOA, .pin = GPIO_PIN_15},
		}
	},
	[SPI] = {
		.protocol = SPI,
		.num_active_channels = 4,
		.channels = {
			{.label = "SCK",  .port = GPIOA, .pin = GPIO_PIN_8},
			{.label = "MOSI", .port = GPIOA, .pin = GPIO_PIN_9},
			{.label = "MISO", .port = GPIOA, .pin = GPIO_PIN_10},
			{.label = "CS",   .port = GPIOA, .pin = GPIO_PIN_11},
			{.label = "N/C",  .port = GPIOA, .pin = GPIO_PIN_12},
			{.label = "N/C",  .port = GPIOA, .pin = GPIO_PIN_15},
		}
	},
	[I2C] = {
		.protocol = I2C,
		.num_active_channels = 2,
		.channels = {
			{.label = "N/C", .port = GPIOA, .pin = GPIO_PIN_8},
			{.label = "SDA", .port = GPIOA, .pin = GPIO_PIN_9},
			{.label = "SCL", .port = GPIOA, .pin = GPIO_PIN_10},
			{.label = "N/C", .port = GPIOA, .pin = GPIO_PIN_11},
			{.label = "N/C", .port = GPIOA, .pin = GPIO_PIN_12},
			{.label = "N/C", .port = GPIOA, .pin = GPIO_PIN_15},
		}
	},
};

const ProtocolPinConfig_t* Config_GetPinConfig(Protocolo_t protocol) {
	if (protocol < PROTOCOL_COUNT) {
		return &pin_configs[protocol];
	}
	return &pin_configs[I2C];
}
