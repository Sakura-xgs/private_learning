/*
 * Copyright (c) 2013 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2021, 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "poll_adc.h"
#include "hal_cd4052b_IF.h"
#include "tcp_client_IF.h"
#include "datasample.h"
#include "uart_comm.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
__BSS(SRAM_OC) static Adc_RawDataValue sTemperature_GUN_T[6][ADC_POLLING_SAMPLESNUM]={0};

static U8 MuxChannelSelect = (U8)e_0X0Y;

BOOL g_blAdcConversionCompletionFlag = 0;

/*******************************************************************************
 * Code
 ******************************************************************************/

/**
 * 功能:中位值平均滤波
 * ch - ADC通道号
 * NumOfSamples-采样值的数量
 * 返回值:滤波后的中位平均数
 */
static U32 Median_average_filter(U8 ch, U8 NumOfSamples)
{
	if (0U == NumOfSamples)
	{
		return 0U;
	}

    U32 Value_buf[ADC_POLLING_SAMPLESNUM] = {0};  // 创建一个数组，用于存储采样值
    U32 sum = 0;  // 初始化总和为0

    // 获取N个采样值
    for(U8 count = 0; count < NumOfSamples; count++)
    {
    	Value_buf[count] = sTemperature_GUN_T[ch][count];
    }

    // 冒泡排序对采样值进行排序，以便于找到中位数
    for(U8 j = 0; j < (NumOfSamples - 1U); j++)
    {
        for(U8 i = 0; i < (NumOfSamples - j - 1U); i++)
        {
            if(Value_buf[i] > Value_buf[i + 1U])
            {
                U32 temp = Value_buf[i];

                Value_buf[i] = Value_buf[i + 1U];
                Value_buf[i + 1U] = temp;
            }
        }
    }

    // 去掉最大最小值，计算中间值的平均
    // 遍历排序后的数组，跳过最大和最小的值，累加中间的值
    for(U8 count = 18; count < (NumOfSamples - 18U); count++)
    {
        sum += Value_buf[count];
    }

    // 返回中位平均值，即中间值的平均数
    if ((NumOfSamples - 36U) > 0U)
    {
    	return  sum / (U16)(NumOfSamples - 36U);
    }
    else
    {
    	return 0U;
    }
}

/*!
 * @brief Main function
 */
static void ADC_POLLING_TASK(void * pvParameters)
{
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
	volatile UBaseType_t uxHighWaterMark;
#endif
	const static Adc_RawDataValue Adc_ChannelGroupSelBuffer[ADC_POLLING_CHANNEL_NUM] =
	{
	    ADC_POLLING_CONVERT_GROUP0,
	    ADC_POLLING_CONVERT_GROUP1,
	    ADC_POLLING_CONVERT_GROUP2,
	    ADC_POLLING_CONVERT_GROUP3,
	    ADC_POLLING_CONVERT_GROUP4,
	    ADC_POLLING_CONVERT_GROUP5
	};

	static Adc_RawDataValue Adc_RawDataValueBuffer[ADC_POLLING_CHANNEL_NUM] = {0U};
    uint8_t channelCount = 0U;
    Adc_RawDataValue Sum_Adc_RawDataValueBuffer[6]={0};
    Adc_RawDataValue Sum_Adc_RawDataValueBufferLast[6]={0};
    U8 DataCount = 0;

    while(1)
    {
        hal_cd4052b_choose_channel((CD4052B_CHANNEL)MuxChannelSelect);

        if(DataCount < ADC_POLLING_SAMPLESNUM)
        {
        	for (channelCount = 0U; channelCount < (sizeof(ADC1_channels_config)/sizeof(adc_channel_config_t)); channelCount++)
        	{
				ADC_SetChannelConfig(ADC1, Adc_ChannelGroupSelBuffer[channelCount], &ADC1_channels_config[channelCount]);
				while (0U == ADC_GetChannelStatusFlags(ADC1, ADC_POLLING_CONVERSION_COMPLETE_REF_CHANNEL))
				{

				}

				Adc_RawDataValueBuffer[channelCount] = ADC_GetChannelConversionValue(ADC1, Adc_ChannelGroupSelBuffer[channelCount]);
            	sTemperature_GUN_T[channelCount][DataCount] = Adc_RawDataValueBuffer[channelCount];
        	}
        }
        else
        {
        	DataCount = 0;

        	for (channelCount = 0U; channelCount < (sizeof(ADC1_channels_config)/sizeof(adc_channel_config_t)); channelCount++)
        	{
				Sum_Adc_RawDataValueBuffer[channelCount] = Median_average_filter(channelCount,ADC_POLLING_SAMPLESNUM);

				Sum_Adc_RawDataValueBufferLast[channelCount] = Sum_Adc_RawDataValueBuffer[channelCount];
				if (TRUE == my_unsigned_abs(Sum_Adc_RawDataValueBufferLast[channelCount], Sum_Adc_RawDataValueBuffer[channelCount], 500))
				{
					my_printf(USER_ERROR,"%s %d %d\n",__func__, Sum_Adc_RawDataValueBufferLast[channelCount],Sum_Adc_RawDataValueBuffer[channelCount]);
				}

        		if ((channelCount == 0U) || (channelCount == 1U))
        		{
        			if (((MuxChannelSelect*2U) + 4U + channelCount) < TEMPER_NUM)
					{
        				TmpMap[(MuxChannelSelect*2U) + 4U + channelCount].u32AdValue = Sum_Adc_RawDataValueBuffer[channelCount];
					}
        		}
        		else
        		{
        			TmpMap[channelCount -2U].u32AdValue = Sum_Adc_RawDataValueBuffer[channelCount];
        		}
        	}

            if (MuxChannelSelect == (U8)e_3X3Y)
            {
                MuxChannelSelect = (U8)e_0X0Y;
            }
            else
            {
                MuxChannelSelect++;
            }

            g_blAdcConversionCompletionFlag = TRUE;
        }

        DataCount++;

        my_printf(USER_DEBUG, "%d %d %d %d\n",Adc_RawDataValueBuffer[2],Adc_RawDataValueBuffer[3],Adc_RawDataValueBuffer[4],Adc_RawDataValueBuffer[5]);

        vTaskDelay(50);

#ifdef FREE_STACK_SPACE_CHECK_ENABLE
		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
#endif
    }
}

void Poll_Adc_Init_Task(void * pvParameters)
{
#if !(defined(FSL_FEATURE_ADC_SUPPORT_HARDWARE_TRIGGER_REMOVE) && FSL_FEATURE_ADC_SUPPORT_HARDWARE_TRIGGER_REMOVE)
    ADC_EnableHardwareTrigger(ADC1, false);
#endif

    /* select cd4025b default channel */
    hal_cd4052b_choose_channel(MuxChannelSelect);

    /* Do auto hardware calibration. */
    while (1)
    {
        if (kStatus_Success == ADC_DoAutoCalibration(ADC1))
        {
            break;
        }
    }

    taskENTER_CRITICAL();

    (void)xTaskCreate(&ADC_POLLING_TASK,		"ADC_POLLING_TASK",			500U/4U,   NULL,   GENERAL_TASK_PRIO,		NULL);

    vTaskDelete(NULL);

    taskEXIT_CRITICAL();
}
