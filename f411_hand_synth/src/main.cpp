#include <stm32f411xe.h>
#include "oscillators.hpp"

bool change;

uint16_t tmpSinAmp0 = 0;
uint16_t tmpSquAmp0 = 0;
uint16_t tmpSawAmp0 = 0;

float_t freq0 = 440.0f;
float_t freq = 440.0f;
float_t sampFreq = 66852.0f;
float_t alpha = 1.0f;

uint32_t idx = 0;
uint32_t isLeft = 0;
uint32_t bounds = 0;
uint32_t cur_conv = 0;

int16_t filtered_sample = 0.0f;
int16_t wavetable[512] = {0};
WaveAdder *wa;


float_t miditable[128] = {
    8.18f, 8.66f, 9.18f, 9.72f, 10.30f, 10.91f, 11.56f, 12.25f, 12.98f, 13.75f, 14.57f, 15.43f, 
    16.35f, 17.32f, 18.35f, 19.45f, 20.60f, 21.83f, 23.12f, 24.50f, 25.96f, 27.50f, 29.14f, 30.87f, 
    32.70f, 34.65f, 36.71f, 38.89f, 41.20f, 43.65f, 46.25f, 49.00f, 51.91f, 55.00f, 58.27f, 61.74f, 
    65.41f, 69.30f, 73.42f, 77.78f, 82.41f, 87.31f, 92.50f, 98.00f, 103.83f, 110.00f, 116.54f, 123.47f, 
    130.81f, 138.59f, 146.83f, 155.56f, 164.81f, 174.61f, 185.00f, 196.00f, 207.65f, 220.00f, 233.08f, 
    246.94f, 261.63f, 277.18f, 293.66f, 311.13f, 329.63f, 349.23f, 369.99f, 392.00f, 415.30f, 440.00f,
    466.16f, 493.88f, 523.25f, 554.37f, 587.33f, 622.25f, 659.26f, 698.46f, 739.99f, 783.99f, 830.61f, 
    880.00f, 932.33f, 987.77f, 1046.50f, 1108.73f, 1174.66f, 1244.51f, 1318.51f, 1396.91f, 1479.98f, 1567.98f,
    1661.22f, 1760.00f, 1864.66f, 1975.53f, 2093.00f, 2217.46f, 2349.32f, 2489.02f, 2637.02f, 2793.83f, 2959.96f, 
    3135.96f, 3322.44f, 3520.00f, 3729.31f, 3951.07f, 4186.01f, 4434.92f, 4698.64f, 4978.03f, 5274.04f, 5587.65f, 
    5919.91f, 6271.93f, 6644.88f, 7040.00f, 7458.62f, 7902.13f, 8372.02f, 8869.84f, 9397.27f, 9956.06f, 10548.08f,
    11175.30f, 11839.82f, 12543.85f
};

void wav_recalc() {
    if(freq == freq0)
        return;

    bounds = (uint32_t)sampFreq/freq;
    if(bounds >= 512)
        bounds = 511;

    for(uint32_t i = 0; i < bounds; i++) {
        filtered_sample = wa->get_next_value()*alpha + (int16_t)((1.0f - alpha)*filtered_sample);

        wavetable[i] = filtered_sample;  
    }
}

void I2S_Cfg(void) {
    RCC->PLLI2SCFGR |= (302 << RCC_PLLI2SCFGR_PLLI2SN_Pos);
    RCC->PLLI2SCFGR |= (2   << RCC_PLLI2SCFGR_PLLI2SR_Pos);

    RCC->CR |= RCC_CR_PLLI2SON;
    
    while(RCC->CR & RCC_CR_PLLI2SRDY);

    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    SPI1->I2SCFGR &= ~SPI_I2SCFGR_I2SCFG_0;
    SPI1->I2SCFGR |= SPI_I2SCFGR_I2SCFG_1;

    SPI1->I2SCFGR |= SPI_I2SCFGR_I2SSTD_0;
    SPI1->I2SCFGR &= ~SPI_I2SCFGR_I2SSTD_1;

    SPI1->I2SCFGR &= ~SPI_I2SCFGR_DATLEN_0;
    SPI1->I2SCFGR &= ~SPI_I2SCFGR_DATLEN_1;

    SPI1->I2SCFGR &= ~SPI_I2SCFGR_CHLEN;

    SPI1->I2SPR |= SPI_I2SPR_ODD;
    SPI1->I2SPR |= (53 << SPI_I2SPR_I2SDIV_Pos);

    SPI1->I2SPR &= ~SPI_I2SPR_MCKOE;

    SPI1->CR2 |= SPI_CR2_TXEIE;

    NVIC_SetPriority(SPI1_IRQn, 1);
    NVIC_EnableIRQ(SPI1_IRQn);

    SPI1->I2SCFGR |= SPI_I2SCFGR_I2SMOD;
    SPI1->I2SCFGR |= SPI_I2SCFGR_I2SE; 
}

extern "C" void SPI1_IRQHandler(void) {
    if(SPI1->SR & SPI_SR_TXE) {    
        if(isLeft == 0) {
            idx = idx + 1;
            isLeft = 1;
        } else {
            isLeft = 0;
        }
        
        if(idx >= bounds) {
            idx = idx - bounds;
            GPIOA->ODR |= GPIO_ODR_OD0;
        }

        int16_t data = wavetable[idx];

        SPI1->DR = data;
    }
}

void GPIOA_Cfg(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    GPIOC->MODER |= GPIO_MODER_MODE13_0;
    GPIOC->MODER &= ~GPIO_MODER_MODE13_1;
    GPIOC->OTYPER &= ~GPIO_OTYPER_OT13;
    GPIOC->PUPDR &= ~GPIO_PUPDR_PUPD13_0 & ~ GPIO_PUPDR_PUPD13_1;

    RCC->AHB1ENR  |= RCC_AHB1ENR_GPIOAEN;

    // I2S
    GPIOA->MODER  &= ~GPIO_MODER_MODE4_0;
    GPIOA->MODER  |= GPIO_MODER_MODE4_1;
    GPIOA->OTYPER &= ~GPIO_OTYPER_OT4;
    GPIOA->PUPDR  &= ~GPIO_PUPDR_PUPD4_0 & ~GPIO_PUPDR_PUPD4_1;

    GPIOA->AFR[0] |= GPIO_AFRL_AFRL4_0;
    GPIOA->AFR[0] &= ~GPIO_AFRL_AFRL4_1;
    GPIOA->AFR[0] |= GPIO_AFRL_AFRL4_2;
    GPIOA->AFR[0] &= ~GPIO_AFRL_AFRL4_3;

    GPIOA->MODER  &= ~GPIO_MODER_MODE5_0;
    GPIOA->MODER  |= GPIO_MODER_MODE5_1;
    GPIOA->OTYPER &= ~GPIO_OTYPER_OT5;
    GPIOA->PUPDR  &= ~GPIO_PUPDR_PUPD5_0 & ~GPIO_PUPDR_PUPD5_1;

    GPIOA->AFR[0] |= GPIO_AFRL_AFRL5_0;
    GPIOA->AFR[0] &= ~GPIO_AFRL_AFRL5_1;
    GPIOA->AFR[0] |= GPIO_AFRL_AFRL5_2;
    GPIOA->AFR[0] &= ~GPIO_AFRL_AFRL5_3;

    GPIOA->MODER  &= ~GPIO_MODER_MODE7_0;
    GPIOA->MODER  |= GPIO_MODER_MODE7_1;
    GPIOA->OTYPER &= ~GPIO_OTYPER_OT7;
    GPIOA->PUPDR  &= ~GPIO_PUPDR_PUPD7_0 & ~GPIO_PUPDR_PUPD7_1;

    GPIOA->AFR[0] |= GPIO_AFRL_AFRL7_0;
    GPIOA->AFR[0] &= ~GPIO_AFRL_AFRL7_1;
    GPIOA->AFR[0] |= GPIO_AFRL_AFRL7_2;
    GPIOA->AFR[0] &= ~GPIO_AFRL_AFRL7_3;

    //ADC channel 0
    GPIOA->MODER  |= GPIO_MODER_MODE0_0;
    GPIOA->MODER  |= GPIO_MODER_MODE0_1;
    GPIOA->PUPDR  &= ~GPIO_PUPDR_PUPD0_0;
    GPIOA->PUPDR  &= ~GPIO_PUPDR_PUPD0_1;

    //ADC channel 1
    GPIOA->MODER  |= GPIO_MODER_MODE1_0;
    GPIOA->MODER  |= GPIO_MODER_MODE1_1;
    GPIOA->PUPDR  &= ~GPIO_PUPDR_PUPD1_0;
    GPIOA->PUPDR  &= ~GPIO_PUPDR_PUPD1_1;

    //ADC channel 2
    GPIOA->MODER  |= GPIO_MODER_MODE2_0;
    GPIOA->MODER  |= GPIO_MODER_MODE2_1;
    GPIOA->PUPDR  &= ~GPIO_PUPDR_PUPD2_0;
    GPIOA->PUPDR  &= ~GPIO_PUPDR_PUPD2_1;

    //ADC channel 3
    GPIOA->MODER  |= GPIO_MODER_MODE3_0;
    GPIOA->MODER  |= GPIO_MODER_MODE3_1;
    GPIOA->PUPDR  &= ~GPIO_PUPDR_PUPD3_0;
    GPIOA->PUPDR  &= ~GPIO_PUPDR_PUPD3_1;

    //ADC channel 6
    GPIOA->MODER  |= GPIO_MODER_MODE6_0;
    GPIOA->MODER  |= GPIO_MODER_MODE6_1;
    GPIOA->PUPDR  &= ~GPIO_PUPDR_PUPD6_0;
    GPIOA->PUPDR  &= ~GPIO_PUPDR_PUPD6_1;
}



void TIM2_Cfg(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2->CNT = 0;
    TIM2->PSC = 2000;
    TIM2->ARR = 4000;
    TIM2->DIER |= TIM_DIER_UIE; 

    TIM2->CR1 &= ~TIM_CR1_DIR;
    TIM2->CR1 |= TIM_CR1_CEN;

    NVIC_SetPriority(TIM2_IRQn, 0);

    NVIC_EnableIRQ(TIM2_IRQn);
}

extern "C" void TIM2_IRQHandler(void) {
    TIM2->SR  &= ~TIM_SR_UIF;
    cur_conv = 0;
    
    ADC1->CR2 |= ADC_CR2_ADON;
    ADC1->CR2 |= ADC_CR2_SWSTART;
}

void ADC_Cfg() {
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    ADC1->SQR1   |= (0b0100  << ADC_SQR1_L_Pos);
    
    ADC1->SQR3   |= (0b00000 << ADC_SQR3_SQ1_Pos);
    ADC1->SQR3   |= (0b00001 << ADC_SQR3_SQ2_Pos);
    ADC1->SQR3   |= (0b00010 << ADC_SQR3_SQ3_Pos);
    ADC1->SQR3   |= (0b00011 << ADC_SQR3_SQ4_Pos);
    ADC1->SQR3   |= (0b00110 << ADC_SQR3_SQ5_Pos);

    ADC1->SMPR2  |= (0b111   << ADC_SMPR2_SMP0_Pos);
    ADC1->SMPR2  |= (0b111   << ADC_SMPR2_SMP1_Pos);
    ADC1->SMPR2  |= (0b111   << ADC_SMPR2_SMP2_Pos);
    ADC1->SMPR2  |= (0b111   << ADC_SMPR2_SMP3_Pos);
    ADC1->SMPR2  |= (0b111   << ADC_SMPR2_SMP6_Pos);

    ADC1->CR1    |= ADC_CR1_EOCIE;
    ADC1->CR1    |= ADC_CR1_SCAN;

    ADC1->CR2    |= ADC_CR2_EOCS;
    ADC1->CR2    |= ADC_CR2_ADON;

    ADC1->CR2    |= ADC_CR2_SWSTART;

    NVIC_EnableIRQ(ADC_IRQn);
}

extern "C" void ADC_IRQHandler() {
    if(ADC1->SR & ADC_SR_OVR) {
        ADC1->SR &= ~ADC_SR_OVR;
        GPIOC->ODR |= GPIO_ODR_OD13;
    }

    if(ADC1->SR & ADC_SR_EOC) {
        bool change = 0;
        switch(cur_conv) {
            case 0:
            {
                uint16_t tmpSinAmp = (uint16_t)(ADC1->DR & 0x0000FFFF);

                if(tmpSinAmp > 500)
                    GPIOC->ODR |= GPIO_ODR_OD13; 
            

                if(tmpSinAmp - tmpSinAmp0 > 5 || tmpSinAmp - tmpSinAmp0 < 5) {
                    wa->change_sin_amp((float_t)tmpSinAmp);
                    change = 1;
                    tmpSinAmp0 = tmpSinAmp;
                }

                break;
            }
            case 1:
            {
                uint16_t tmpSquAmp = (uint16_t)(ADC1->DR & 0x0000FFFF);

                if(tmpSquAmp - tmpSquAmp0 > 5 || tmpSquAmp - tmpSquAmp0 < 5) {
                    wa->change_squ_amp((float_t)tmpSquAmp);
                    change = 1;
                    tmpSquAmp0 = tmpSquAmp;
                }
                break;
            }
            case 2:
            {
                uint16_t tmpSawAmp = (uint16_t)(ADC1->DR & 0x0000FFFF);

                if(tmpSawAmp - tmpSawAmp0 > 5 || tmpSawAmp - tmpSawAmp0 < 5) {
                    wa->change_saw_amp((float_t)tmpSawAmp);
                    change = 1;
                    tmpSawAmp = tmpSawAmp0;
                }

                break;
            }
            case 3:
            {
                alpha = (float_t)((ADC1->DR & 0x0000FFFF)/4096.0f);

                break;
            }
            case 4:
            {
                uint16_t midiNote = (uint16_t)(48*(ADC1->DR & 0x0000FFFF)/4096) + 48;

                freq0 = freq;
                freq = miditable[midiNote];
               
                if((freq - freq0) > 10.0f || (freq - freq0) < 10.0f) {
                    wa->change_freq(freq);
                    change = 1;
                }
    
                break;
            }
            default:
                break;
        }
        cur_conv++;

        if(cur_conv == 5) {
            cur_conv = 0;

            if(change) {
                change = 0;
                wav_recalc();
            }
        }
    }

}

int main(void) {
    wa = new WaveAdder(freq, 0.0f, 0.0f, sampFreq, 32768.0f, -32768.0f);

    wav_recalc();

    ADC_Cfg();
    GPIOA_Cfg();
    TIM2_Cfg();
    I2S_Cfg();

    for(;;) {

    }

    return 0;
}