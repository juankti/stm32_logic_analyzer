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
	                       DMA_SxCR_PSIZE_1 |
	                       DMA_SxCR_CIRC;

	// dma2 (gpiob)
	DMA2_Stream1->PAR = (uint32_t)&GPIOB->IDR;
	DMA2_Stream1->M0AR = (uint32_t)buffer_states;
	DMA2_Stream1->NDTR = NUM_MUESTRAS;
	DMA2_Stream1->CR = DMA_SxCR_CHSEL_1 | DMA_SxCR_CHSEL_2 |
	                       DMA_SxCR_MINC |
	                       0x00000000 |
	                       0x00000000 |
	                       DMA_SxCR_CIRC;

	// trigger de timer a dma
	TIM1->DIER |= TIM_DIER_CC1DE;
	TIM2->DIER |= TIM_DIER_CC2DE;
}


void Init_Captura() {
	DMA1_Stream6->CR != DMA_SxCR_EN;
	DMA2_Stream1->CR != DMA_SxCR_EN;
	TIM1->CR1 |= TIM_CR1_CEN;
	TIM2->CR1 |= TIM_CR1_CEN;
}

void Stop_Captura(){
	DMA1_Stream6->CR &= ~DMA_SxCR_EN;
	DMA2_Stream1->CR &= ~DMA_SxCR_EN;
	TIM1->CR1 &= ~TIM_CR1_CEN;
	TIM2->CR1 &= ~TIM_CR1_CEN;
}


bool Capture_IsFinished(void) {

      if (DMA2->LISR & DMA_LISR_TCIF1) { //transfer complete int flag
        return true;
    }
    return false;
}
