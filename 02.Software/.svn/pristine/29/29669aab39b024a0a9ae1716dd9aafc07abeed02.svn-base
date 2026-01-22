#include "hal_adc.h"
#include "gd32f4xx.h"

const static strADC_PARA g_ADC_PARA[] =
{
    {ADC0_IN10_PCB_TEM1,    GPIOA,  GPIO_PIN_4,     ADC_CHANNEL_4},
    {ADC0_IN11_A_CC1,       GPIOA,  GPIO_PIN_5,     ADC_CHANNEL_5},
    {ADC0_IN12_B_CC1,       GPIOA,  GPIO_PIN_6,     ADC_CHANNEL_6},
    {ADC0_IN13_PCB_TEM2,    GPIOA,  GPIO_PIN_7,     ADC_CHANNEL_7},
};

static volatile U16 g_ADC_Buf[ADC_CHAN_NUM] = {0};  


/*!
    \brief      configure the DMA peripheral
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void dma_adc_config(void)
{
    /* ADC_DMA_channel configuration */
    dma_single_data_parameter_struct dma_data_parameter;
    
    /* ADC DMA_channel configuration */
    dma_deinit(DMA1, DMA_CH0);
    
    /* initialize DMA single data mode */
    dma_data_parameter.periph_addr  = (uint32_t)(&ADC_RDATA(ADC0));
    dma_data_parameter.periph_inc   = DMA_PERIPH_INCREASE_DISABLE;
    dma_data_parameter.memory0_addr  = (uint32_t)(g_ADC_Buf);
    dma_data_parameter.memory_inc   = DMA_MEMORY_INCREASE_ENABLE;
    dma_data_parameter.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
    dma_data_parameter.circular_mode  = DMA_CIRCULAR_MODE_ENABLE;
    dma_data_parameter.direction    = DMA_PERIPH_TO_MEMORY;
    dma_data_parameter.number       = ADC_CHAN_NUM;
    dma_data_parameter.priority     = DMA_PRIORITY_HIGH;

    dma_single_data_mode_init(DMA1, DMA_CH0, &dma_data_parameter);

    dma_circulation_enable(DMA1, DMA_CH0);
  
    /* enable DMA channel */
    dma_channel_enable(DMA1, DMA_CH0);
}

/*!
    \brief      configure the ADC peripheral
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void adc_config(void)
{
    U8 u8AdcChanCnt = 0;

    /* enable ADC clock */
    rcu_periph_clock_enable(RCU_ADC0);
    /* config ADC clock */
    adc_clock_config(ADC_ADCCK_PCLK2_DIV4);
    
    /* ADC continuous function enable */
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);
    /* ADC scan function enable */
    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);

    /* ADC data alignment config */
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);

    /* ADC channel length config */
    adc_channel_length_config(ADC0, ADC_ROUTINE_CHANNEL, ADC_CHAN_NUM);

    /* ADC regular channel config */ 
    for(u8AdcChanCnt = 0; u8AdcChanCnt < ADC_CHAN_NUM; u8AdcChanCnt++)
    {
        adc_routine_channel_config(ADC0, g_ADC_PARA[u8AdcChanCnt].AdcRankNo, g_ADC_PARA[u8AdcChanCnt].u32AdcChanNo, ADC_SAMPLETIME_15);   
    }

    /* ADC trigger config */
    adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);

    /* ADC DMA function enable */
	adc_dma_request_after_last_enable(ADC0);
    adc_dma_mode_enable(ADC0);
    
    /* enable ADC interface */
    adc_enable(ADC0);
    
    /* ADC calibration and reset calibration */
    adc_calibration_enable(ADC0);

    /* ADC software trigger enable */
    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);    
}

static void adc_gpio_config(void)
{
    U8 u8AdcChanCnt = 0;
    for(u8AdcChanCnt = 0; u8AdcChanCnt < ADC_CHAN_NUM; u8AdcChanCnt++)
    {
        gpio_mode_set(g_ADC_PARA[u8AdcChanCnt].u32AdcGpioPort, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, g_ADC_PARA[u8AdcChanCnt].u32AdcGpioPin);
    }    
}

/*!
    \brief      ADC模块初始化
    \param[in]  none
    \param[out] none
    \retval     none
*/
void HAL_ADC_INIT(void)
{
    adc_gpio_config();

    dma_adc_config();

    adc_config();
}

/******************************************************************************
* 名  	称:  HAL_ADC_GET_CHAN_AD
* 功  	能:	 获取通道ADC值
* 入口参数:  无
* 出口参数:  无
*******************************************************************************/
U16 HAL_ADC_GET_CHAN_AD(enumADC_ChanNo ChanNo)
{
	U16 u16RetVal = 0;

	u16RetVal = g_ADC_Buf[ChanNo];

	return u16RetVal;
}



