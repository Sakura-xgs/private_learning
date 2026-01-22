/*
 * Copyright (c) 2013 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2021, 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "poll_adc.h"
#include "hal_adc.h"
#include "data_sample.h"
#include "PublicDefine.h"
#include "hal_sys_IF.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
Adc_RawDataValue Adc1_RawDataValueBuffer[ADC1_POLLING_CHANNEL_NUM] = {0U};
Adc_RawDataValue Adc2_RawDataValueBuffer[ADC2_POLLING_CHANNEL_NUM] = {0U};
const static Adc_RawDataValue Adc1_ChannelGroupSelBuffer[ADC1_POLLING_CHANNEL_NUM] =
{
    ADC1_POLLING_CONVERT_GROUP0,
    ADC1_POLLING_CONVERT_GROUP1,
};

const static Adc_RawDataValue Adc2_ChannelGroupSelBuffer[ADC2_POLLING_CHANNEL_NUM] =
{
    ADC2_POLLING_CONVERT_GROUP0,
    ADC2_POLLING_CONVERT_GROUP1,
};

const uint32_t g_Adc_12bitFullRange = 4096U;

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Main function
 */
void ADC_POLLING_TASK(void * pvParameters)
{
    uint8_t channelCount = 0U;
    S32 s32Temp = 0;
    for( ;; )
    {
		if(SYS_STATUS_NORMAL != GetSysRunModeFg())
		{
			vTaskDelete(NULL);
		}

        for (channelCount = 0U; channelCount < ADC1_POLLING_CHANNEL_NUM; channelCount++)
        {
            ADC_SetChannelConfig(ADC1, Adc1_ChannelGroupSelBuffer[channelCount], &ADC1_channels_config[channelCount]);
            while (0U == ADC_GetChannelStatusFlags(ADC1, ADC_POLLING_CONVERSION_COMPLETE_REF_CHANNEL))
            {

            }

            Adc1_RawDataValueBuffer[channelCount] = ADC_GetChannelConversionValue(ADC1, Adc1_ChannelGroupSelBuffer[channelCount]);
        }

        for (channelCount = 0U; channelCount < ADC2_POLLING_CHANNEL_NUM; channelCount++)
        {
            ADC_SetChannelConfig(ADC2, Adc2_ChannelGroupSelBuffer[channelCount], &ADC2_channels_config[channelCount]);
            while (0U == ADC_GetChannelStatusFlags(ADC2, ADC_POLLING_CONVERSION_COMPLETE_REF_CHANNEL))
            {

            }

            Adc2_RawDataValueBuffer[channelCount] = ADC_GetChannelConversionValue(ADC2, Adc2_ChannelGroupSelBuffer[channelCount]);
        }

        TmpMap[0].u32AdValue = Adc1_RawDataValueBuffer[0];
        TmpMap[1].u32AdValue = Adc1_RawDataValueBuffer[1];
        TmpMap[2].u32AdValue = Adc2_RawDataValueBuffer[0];
        TmpMap[3].u32AdValue = Adc2_RawDataValueBuffer[1];

        vTaskDelay(1000);
    }
}

void Poll_Adc_Init_Task(void * pvParameters)
{
#if !(defined(FSL_FEATURE_ADC_SUPPORT_HARDWARE_TRIGGER_REMOVE) && FSL_FEATURE_ADC_SUPPORT_HARDWARE_TRIGGER_REMOVE)
    ADC_EnableHardwareTrigger(ADC1, false);
    ADC_EnableHardwareTrigger(ADC2, false);
#endif

    /* Do auto hardware calibration. */
    while (1) {
        if (kStatus_Success == ADC_DoAutoCalibration(ADC1)) {
        	uPRINTF("ADC1 AutoCalibration failed");
        	vTaskDelay(100);
            break;
        }
    }

    while (1) {
        if (kStatus_Success == ADC_DoAutoCalibration(ADC2)) {
        	uPRINTF("ADC2 AutoCalibration failed");
        	vTaskDelay(100);
            break;
        }
    }

    taskENTER_CRITICAL();

    xTaskCreate(ADC_POLLING_TASK,		"ADC_POLLING_TASK",			configMINIMAL_STACK_SIZE*2,   NULL,   ADC_POLLING_TASK_TASK_PRIO,		NULL);

    taskEXIT_CRITICAL();

    vTaskDelete(NULL);

}
