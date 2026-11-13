#include "captura.h"
#include "config_analizador.h"
#include "interface_conf.h"

void Conf_Captura(void){
	// detener timers y dma
	DMA1_Stream6->CR &= ~DMA_SxCR_EN;
	DMA2_Stream1->CR &= ~DMA_SxCR_EN;
	TIM1->CR1 &= ~TIM_CR1_CEN;
	TIM2->CR1 &= ~TIM_CR1_CEN;

    uint32_t preescalador = 83; // 1mhz default (84MHz/84)
    switch(conf.frecuencia) {
        case kHz20:  preescalador = 4199; break; // 20khz
        case kHz100: preescalador = 839; break; // 100khz
        case kHz400: preescalador = 209; break; // 400khz
        case MHz1:   preescalador = 83; break;  // 1mhz
        case MHz2:   preescalador = 41; break;  // 2mhz
        case MHz4:   preescalador = 20; break;  // 4mhz
        case MHz10_5: preescalador = 7; break;  // 10.5mhz
    }
    TIM1->PSC = preescalador;
    TIM2->PSC = preescalador;
    
    // forzar actualizacion de preescalador
    TIM1->EGR = TIM_EGR_UG;
    TIM2->EGR = TIM_EGR_UG;

	// dma1 (marcas de tiempo)
	DMA1_Stream6->PAR = (uint32_t)&TIM2->CCR2;
	DMA1_Stream6->M0AR = (uint32_t)tiempos_buffer;
	DMA1_Stream6->NDTR = Config_ObtenerValorTamaño(conf.tamaño_buffer);
	DMA1_Stream6->CR = DMA_SxCR_CHSEL_1 | DMA_SxCR_CHSEL_0 |
	                       DMA_SxCR_MINC |
	                       DMA_SxCR_MSIZE_1 |
	                       DMA_SxCR_PSIZE_1;

	// dma2 (gpioa)
	DMA2_Stream1->PAR = (uint32_t)&GPIOA->IDR;
	DMA2_Stream1->M0AR = (uint32_t)estados_buffer;
	DMA2_Stream1->NDTR = Config_ObtenerValorTamaño(conf.tamaño_buffer);
	DMA2_Stream1->CR = DMA_SxCR_CHSEL_1 | DMA_SxCR_CHSEL_2 |
	                       DMA_SxCR_MINC |
	                       DMA_SxCR_MSIZE_0 |
	                       DMA_SxCR_PSIZE_0;

	// trigger de timer a dma
	TIM1->DIER |= TIM_DIER_CC1DE;
	TIM2->DIER |= TIM_DIER_CC2DE;

    // configurar ambos timers para capturar en ambos flancos
    // vital para uart (transiciones asincronas)
    TIM1->CCER |= (TIM_CCER_CC1P | TIM_CCER_CC1NP);
    TIM2->CCER |= (TIM_CCER_CC2P | TIM_CCER_CC2NP);
}


void Inicializar_Captura() {
    TIM1->CNT = 0;
    TIM2->CNT = 0;
    DMA1->HIFCR = DMA_HIFCR_CTCIF6; // limpiar tcif6
    DMA2->LIFCR = DMA_LIFCR_CTCIF1; // limpiar tcif1

	DMA1_Stream6->CR |= DMA_SxCR_EN;
	DMA2_Stream1->CR |= DMA_SxCR_EN;
    
    // habilitar canales captura
    TIM1->CCER |= TIM_CCER_CC1E;
    TIM2->CCER |= TIM_CCER_CC2E;
    
	TIM1->CR1 |= TIM_CR1_CEN;
	TIM2->CR1 |= TIM_CR1_CEN;
}

void Detener_Captura(){
	DMA1_Stream6->CR &= ~DMA_SxCR_EN;
	DMA2_Stream1->CR &= ~DMA_SxCR_EN;
	TIM1->CR1 &= ~TIM_CR1_CEN;
	TIM2->CR1 &= ~TIM_CR1_CEN;
    
    // deshabilitar canales de captura
    TIM1->CCER &= ~TIM_CCER_CC1E;
    TIM2->CCER &= ~TIM_CCER_CC2E;
    
    // limpiar flags interrupcion
    DMA1->HIFCR = DMA_HIFCR_CTCIF6; // limpiar tcif flujo 6
    DMA2->LIFCR = DMA_LIFCR_CTCIF1; // limpiar bandera int de transferencia completa flujo 1
}


bool Captura_Terminada(void) {

      if (DMA2->LISR & DMA_LISR_TCIF1) { // bandera int de transferencia completa
        return true;
    }
    return false;
}
