#include "interface_conf.h"

Configuration_t conf = {
	NONE,
	kHz100,
    RADIX_HEX,
    SIZE_1KB,
};

static const char * const PROTOCOLO_TEXT[PROTOCOL_COUNT] = {
        [NONE] = "NONE",
		[UART] = "UART",
		[SPI]  = "SPI",
		[I2C]  = "I2C"
};

static const char * const FREQ_TEXT[FREQUENCY_COUNT] = {
		[kHz20]  = "20 kHz",
		[kHz100] = "100 kHz",
		[kHz400] = "400 kHz",
        [MHz1]   = "1 MHz",
        [MHz2]   = "2 MHz",
        [MHz4]   = "4 MHz",
        [MHz10_5]= "10.5 MHz"
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

static const char * BUFFER_SIZE_TEXT[] = {
    "512",
    "1024",
    "2048",
    "4096"
};

const char * Config_GetSizeText(BufferSize_t size) {
    if (size < 4) return BUFFER_SIZE_TEXT[size];
    return "UNKNOWN";
}

uint32_t Config_GetSizeValue(BufferSize_t size) {
    switch (size) {
        case SIZE_0_5KB: return 512;
        case SIZE_1KB: return 1024;
        case SIZE_2KB: return 2048;
        case SIZE_4KB: return 4096;
        default: return 1024;
    }
}

ProtocolPinConfig_t pin_configs[PROTOCOL_COUNT] = {
    [NONE] = {
		.protocol = NONE,
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
	[UART] = {
		.protocol = UART,
		.num_active_channels = 1,
		.channels = {
			{.label = "RX",  .port = GPIOA, .pin = GPIO_PIN_8},
			{.label = "CH2", .port = GPIOA, .pin = GPIO_PIN_9},
			{.label = "CH3", .port = GPIOA, .pin = GPIO_PIN_10},
			{.label = "CH4", .port = GPIOA, .pin = GPIO_PIN_11},
			{.label = "CH5", .port = GPIOA, .pin = GPIO_PIN_12},
			{.label = "CH6", .port = GPIOA, .pin = GPIO_PIN_15},
		}
	},
	[SPI] = {
		.protocol = SPI,
		.num_active_channels = 4,
		.channels = {
			{.label = "SCLK", .port = GPIOA, .pin = GPIO_PIN_8},
			{.label = "CS",   .port = GPIOA, .pin = GPIO_PIN_9},
			{.label = "MOSI", .port = GPIOA, .pin = GPIO_PIN_10},
			{.label = "MISO", .port = GPIOA, .pin = GPIO_PIN_11},
			{.label = "CH5",  .port = GPIOA, .pin = GPIO_PIN_12},
			{.label = "CH6",  .port = GPIOA, .pin = GPIO_PIN_15},
		}
	},
	[I2C] = {
		.protocol = I2C,
		.num_active_channels = 2,
		.channels = {
			{.label = "SDA", .port = GPIOA, .pin = GPIO_PIN_8},
			{.label = "SCL", .port = GPIOA, .pin = GPIO_PIN_9},
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
	return &pin_configs[NONE];
}
