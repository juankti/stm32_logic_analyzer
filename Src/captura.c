#include "captura.h"
#include "config_analizador.h"

void Conf_Captura(void){
	// detener timers y dma
	DMA1_Stream6->CR &= ~DMA_SxCR_EN;
	DMA2_Stream1->CR &= ~DMA_SxCR_EN;
	TIM1->CR1 &= ~TIM_CR1_CEN;
	TIM2->CR1 &= ~TIM_CR1_CEN;

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
}


void Init_Captura() {
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
