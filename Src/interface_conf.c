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

const char * Config_ObtenerTextoProtocolo(Protocolo_t protocolo){
	if (protocolo < PROTOCOL_COUNT) return PROTOCOLO_TEXT[protocolo];
	return "UNKNOWN";
}

const char * Config_ObtenerTextoFrec(Freq_t frec){
	if (frec < FREQUENCY_COUNT) return FREQ_TEXT[frec];
	return "UNKNOWN";
}

const char * Config_ObtenerTextoRadix(Radix_t radix){
	if (radix < 3) return RADIX_TEXT[radix];
	return "UNKNOWN";
}

static const char * BUFFER_SIZE_TEXT[] = {
    "512",
    "1024",
    "2048",
    "4096"
};

const char * Config_ObtenerTextoTamaño(BufferSize_t tamaño) {
    if (tamaño < 4) return BUFFER_SIZE_TEXT[tamaño];
    return "UNKNOWN";
}

uint32_t Config_ObtenerValorTamaño(BufferSize_t tamaño) {
    switch (tamaño) {
        case SIZE_0_5KB: return 512;
        case SIZE_1KB: return 1024;
        case SIZE_2KB: return 2048;
        case SIZE_4KB: return 4096;
        default: return 1024;
    }
}

ProtocolPinConfig_t configs_pin[PROTOCOL_COUNT] = {
    [NONE] = {
		.protocolo = NONE,
		.num_canales_activos = 6,
		.canales = {
			{.etiqueta = "CH1", .puerto = GPIOA, .pin = GPIO_PIN_8},
			{.etiqueta = "CH2", .puerto = GPIOA, .pin = GPIO_PIN_9},
			{.etiqueta = "CH3", .puerto = GPIOA, .pin = GPIO_PIN_10},
			{.etiqueta = "CH4", .puerto = GPIOA, .pin = GPIO_PIN_11},
			{.etiqueta = "CH5", .puerto = GPIOA, .pin = GPIO_PIN_12},
			{.etiqueta = "CH6", .puerto = GPIOA, .pin = GPIO_PIN_15},
		}
	},
	[UART] = {
		.protocolo = UART,
		.num_canales_activos = 1,
		.canales = {
			{.etiqueta = "RX",  .puerto = GPIOA, .pin = GPIO_PIN_8},
			{.etiqueta = "CH2", .puerto = GPIOA, .pin = GPIO_PIN_9},
			{.etiqueta = "CH3", .puerto = GPIOA, .pin = GPIO_PIN_10},
			{.etiqueta = "CH4", .puerto = GPIOA, .pin = GPIO_PIN_11},
			{.etiqueta = "CH5", .puerto = GPIOA, .pin = GPIO_PIN_12},
			{.etiqueta = "CH6", .puerto = GPIOA, .pin = GPIO_PIN_15},
		}
	},
	[SPI] = {
		.protocolo = SPI,
		.num_canales_activos = 4,
		.canales = {
			{.etiqueta = "SCLK", .puerto = GPIOA, .pin = GPIO_PIN_8},
			{.etiqueta = "CS",   .puerto = GPIOA, .pin = GPIO_PIN_9},
			{.etiqueta = "MOSI", .puerto = GPIOA, .pin = GPIO_PIN_10},
			{.etiqueta = "MISO", .puerto = GPIOA, .pin = GPIO_PIN_11},
			{.etiqueta = "CH5",  .puerto = GPIOA, .pin = GPIO_PIN_12},
			{.etiqueta = "CH6",  .puerto = GPIOA, .pin = GPIO_PIN_15},
		}
	},
	[I2C] = {
		.protocolo = I2C,
		.num_canales_activos = 2,
		.canales = {
			{.etiqueta = "SDA", .puerto = GPIOA, .pin = GPIO_PIN_8},
			{.etiqueta = "SCL", .puerto = GPIOA, .pin = GPIO_PIN_9},
			{.etiqueta = "CH3", .puerto = GPIOA, .pin = GPIO_PIN_10},
			{.etiqueta = "CH4", .puerto = GPIOA, .pin = GPIO_PIN_11},
			{.etiqueta = "CH5", .puerto = GPIOA, .pin = GPIO_PIN_12},
			{.etiqueta = "CH6", .puerto = GPIOA, .pin = GPIO_PIN_15},
		}
	}
};

const ProtocolPinConfig_t* Config_ObtenerConfigPin(Protocolo_t protocolo) {
	if (protocolo < PROTOCOL_COUNT) {
		return &configs_pin[protocolo];
	}
	return &configs_pin[NONE];
}
