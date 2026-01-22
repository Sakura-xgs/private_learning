#ifndef POLL_ADC_H_
#define POLL_ADC_H_

#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_adc.h"
#include "MIMXRT1021.h"
#include "peripherals.h"

#define ADC_POLLING_CHANNEL_NUM                         (sizeof(ADC1_channels_config)/sizeof(adc_channel_config_t))
#define ADC_POLLING_CONVERSION_COMPLETE_REF_CHANNEL     (0)

#define ADC_POLLING_CONVERT_GROUP0                      (ADC1_CH0_CONTROL_GROUP)
#define ADC_POLLING_CONVERT_GROUP1                      (ADC1_CH1_CONTROL_GROUP)
#define ADC_POLLING_CONVERT_GROUP2                      (ADC1_CH2_CONTROL_GROUP)
#define ADC_POLLING_CONVERT_GROUP3                      (ADC1_CH3_CONTROL_GROUP)
#define ADC_POLLING_CONVERT_GROUP4                      (ADC1_CH4_CONTROL_GROUP)
#define ADC_POLLING_CONVERT_GROUP5                      (ADC1_CH5_CONTROL_GROUP)

#define ADC_POLLING_SAMPLESNUM       40U

typedef uint32_t Adc_RawDataValue;

extern BOOL g_blAdcConversionCompletionFlag;

extern void Poll_Adc_Init_Task(void * pvParameters);


#endif
