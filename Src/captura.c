#include "captura.h"
#include "config_analizador.h"
#include "interface_conf.h"

void Conf_Captura(void){
	// detener timers y dma
	DMA1_Stream6->CR &= ~DMA_SxCR_EN;
	DMA2_Stream1->CR &= ~DMA_SxCR_EN;
	TIM1->CR1 &= ~TIM_CR1_CEN;
	TIM2->CR1 &= ~TIM_CR1_CEN;

    uint32_t prescaler = 83; // 1MHz default (84MHz/84)
    switch(conf.frequency) {
        case kHz100: prescaler = 839; break; // 100KHz
        case kHz400: prescaler = 209; break; // 400KHz
        case kHz20:  prescaler = 4199; break; // 20KHz
        case MHz1:   prescaler = 83; break;  // 1MHz
    }
    TIM1->PSC = prescaler;
    TIM2->PSC = prescaler;
    
    // Forzar actualización para que el nuevo prescaler aplique INMEDIATAMENTE.
    TIM1->EGR = TIM_EGR_UG;
    TIM2->EGR = TIM_EGR_UG;

	// dma1 (timestamps)
	DMA1_Stream6->PAR = (uint32_t)&TIM2->CCR2;
	DMA1_Stream6->M0AR = (uint32_t)buffer_timestamps;
	DMA1_Stream6->NDTR = NUM_MUESTRAS;
	DMA1_Stream6->CR = DMA_SxCR_CHSEL_1 | DMA_SxCR_CHSEL_0 |
	                       DMA_SxCR_MINC |
	                       DMA_SxCR_MSIZE_1 |
	                       DMA_SxCR_PSIZE_1;

	// dma2 (gpioa)
	DMA2_Stream1->PAR = (uint32_t)&GPIOA->IDR;
	DMA2_Stream1->M0AR = (uint32_t)buffer_states;
	DMA2_Stream1->NDTR = NUM_MUESTRAS;
	DMA2_Stream1->CR = DMA_SxCR_CHSEL_1 | DMA_SxCR_CHSEL_2 |
	                       DMA_SxCR_MINC |
	                       DMA_SxCR_MSIZE_0 |
	                       DMA_SxCR_PSIZE_0;

	// trigger de timer a dma
	TIM1->DIER |= TIM_DIER_CC1DE;
	TIM2->DIER |= TIM_DIER_CC2DE;

    // Configurar ambos timers para capturar en AMBOS flancos (RISING y FALLING)
    // Esto es vital para UART (transiciones asíncronas). 
    TIM1->CCER |= (TIM_CCER_CC1P | TIM_CCER_CC1NP);
    TIM2->CCER |= (TIM_CCER_CC2P | TIM_CCER_CC2NP);
}


void Init_Captura() {
    TIM1->CNT = 0;
    TIM2->CNT = 0;
    DMA1->HIFCR = DMA_HIFCR_CTCIF6; // Clear TCIF6
    DMA2->LIFCR = DMA_LIFCR_CTCIF1; // Clear TCIF1

	DMA1_Stream6->CR |= DMA_SxCR_EN;
	DMA2_Stream1->CR |= DMA_SxCR_EN;
    
    // Habilitar los canales de captura
    TIM1->CCER |= TIM_CCER_CC1E;
    TIM2->CCER |= TIM_CCER_CC2E;
    
	TIM1->CR1 |= TIM_CR1_CEN;
	TIM2->CR1 |= TIM_CR1_CEN;
}

void Stop_Captura(){
	DMA1_Stream6->CR &= ~DMA_SxCR_EN;
	DMA2_Stream1->CR &= ~DMA_SxCR_EN;
	TIM1->CR1 &= ~TIM_CR1_CEN;
	TIM2->CR1 &= ~TIM_CR1_CEN;
    
    // Deshabilitar canales de captura
    TIM1->CCER &= ~TIM_CCER_CC1E;
    TIM2->CCER &= ~TIM_CCER_CC2E;
    
    // Limpiar flags de interrupción
    DMA1->HIFCR = DMA_HIFCR_CTCIF6; // Clear Transfer Complete Int Flag for Stream 6
    DMA2->LIFCR = DMA_LIFCR_CTCIF1; // Clear Transfer Complete Int Flag for Stream 1
}


bool Capture_IsFinished(void) {

      if (DMA2->LISR & DMA_LISR_TCIF1) { //transfer complete int flag
        return true;
    }
    return false;
}
