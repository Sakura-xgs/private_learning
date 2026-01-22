#ifndef __HAL_ADC_H
#define __HAL_ADC_H

#include "PublicDefine.h"

typedef enum
{
    ADC0_IN10_PCB_TEM1  =0 ,
    ADC0_IN11_A_CC1,
    ADC0_IN12_B_CC1,
    ADC0_IN13_PCB_TEM2,
    ADC_CHAN_NUM, // Number of ADC channels
}enumADC_ChanNo;

typedef struct hal_adc
{
    enumADC_ChanNo AdcRankNo;
	uint32_t u32AdcGpioPort;
	uint32_t u32AdcGpioPin;
	uint32_t u32AdcChanNo;
}strADC_PARA;


void HAL_ADC_INIT(void);
U16 HAL_ADC_GET_CHAN_AD(enumADC_ChanNo ChanNo);

#endif


