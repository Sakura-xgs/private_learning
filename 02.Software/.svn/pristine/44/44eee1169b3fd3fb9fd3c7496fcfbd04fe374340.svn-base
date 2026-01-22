/*
 * Copyright (c) 2013 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2021, 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rgb_pwm.h"
#include "rgb_pwm_IF.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

typedef struct 
{
    TMR_Type * peripheral;
    qtmr_channel_selection_t timerChannelSel;
    bool Polarity;
    qtmr_counting_mode_t countMode;
    U32  srcClockPeriod;
}Rgb_Pwm_ChannelInfoType;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

Rgb_Pwm_DataSetType Rgb_Pwm_LED_A[3] = {0};
Rgb_Pwm_DataSetType Rgb_Pwm_LED_B[3] = {0};
SemaphoreHandle_t g_sLed_change_sem = NULL;

static Rgb_Pwm_ChannelInfoType Rgb_Pwm_ChannelInfoBuffer[RGB_PWM_CHANNEL_SUM_NUM] =
{
    {
        TMR1_PERIPHERAL,
        RGB_PWM_CHANNEL1,
        false,
        kQTMR_PriSrcRiseEdge,
        RGB_PWM_CHANNEL_CLOCK_PERIOD
    },
    {
        TMR1_PERIPHERAL,
        RGB_PWM_CHANNEL2,
        false,
        kQTMR_PriSrcRiseEdge,
        RGB_PWM_CHANNEL_CLOCK_PERIOD
    },
    {
        TMR1_PERIPHERAL,
        RGB_PWM_CHANNEL3,
        false,
        kQTMR_PriSrcRiseEdge,
        RGB_PWM_CHANNEL_CLOCK_PERIOD
    },
    {
        TMR1_PERIPHERAL,
        RGB_PWM_CHANNEL4,
        false,
        kQTMR_PriSrcRiseEdge,
        RGB_PWM_CHANNEL_CLOCK_PERIOD
    },
    {
        TMR2_PERIPHERAL,
        RGB_PWM_CHANNEL5,
        false,
        kQTMR_PriSrcRiseEdge,
        RGB_PWM_CHANNEL_CLOCK_PERIOD
    },
    {
        TMR2_PERIPHERAL,
        RGB_PWM_CHANNEL6,
        false,
        kQTMR_PriSrcRiseEdge,
        RGB_PWM_CHANNEL_CLOCK_PERIOD
    }
};
/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Main function
 */
static void Set_PeriodAndDuty_Task(void * pvParameters)
{
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
	volatile UBaseType_t uxHighWaterMark;
#endif
    U8 channelCount = 0U;

	for( ;; )
	{
		if (pdTRUE == xSemaphoreTake(g_sLed_change_sem, portMAX_DELAY))
		{
			for (channelCount = 0U; channelCount < RGB_PWM_CHANNEL_SUM_NUM; channelCount++)
			{
				//A枪LED通道
				if (channelCount < 3U)
				{
					if (Rgb_Pwm_LED_A[channelCount].period != 0U)
					{
						(void)QTMR_SetupPwm(Rgb_Pwm_ChannelInfoBuffer[channelCount].peripheral,
						Rgb_Pwm_ChannelInfoBuffer[channelCount].timerChannelSel,
						Rgb_Pwm_LED_A[channelCount].period,
						Rgb_Pwm_LED_A[channelCount].dutycycle,
						Rgb_Pwm_ChannelInfoBuffer[channelCount].Polarity,
						RGB_PWM_CHANNEL_CLOCK_PERIOD);

						QTMR_StartTimer(Rgb_Pwm_ChannelInfoBuffer[channelCount].peripheral,
						Rgb_Pwm_ChannelInfoBuffer[channelCount].timerChannelSel,
						Rgb_Pwm_ChannelInfoBuffer[channelCount].countMode);
					}
				}
				//B枪LED通道
				else
				{
					if (Rgb_Pwm_LED_B[channelCount-3U].period != 0U)
					{
						(void)QTMR_SetupPwm(Rgb_Pwm_ChannelInfoBuffer[channelCount].peripheral,
						Rgb_Pwm_ChannelInfoBuffer[channelCount].timerChannelSel,
						Rgb_Pwm_LED_B[channelCount-3U].period,
						Rgb_Pwm_LED_B[channelCount-3U].dutycycle,
						Rgb_Pwm_ChannelInfoBuffer[channelCount].Polarity,
						RGB_PWM_CHANNEL_CLOCK_PERIOD);

						QTMR_StartTimer(Rgb_Pwm_ChannelInfoBuffer[channelCount].peripheral,
						Rgb_Pwm_ChannelInfoBuffer[channelCount].timerChannelSel,
						Rgb_Pwm_ChannelInfoBuffer[channelCount].countMode);
					}
				}
			}
		}
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
#endif
	}
}

void Rgb_Pwm_Init_Task(void * pvParameters)
{
    (void)pvParameters;
    U8 channelCount = 0U;

    g_sLed_change_sem = xSemaphoreCreateBinary();
    if (NULL == g_sLed_change_sem)
    {
    	vTaskSuspend(NULL);
    }

    //初始化占空比为0
    for (channelCount = 0; channelCount < RGB_PWM_CHANNEL_SUM_NUM; channelCount++)
    {
    	(void)QTMR_SetupPwm(Rgb_Pwm_ChannelInfoBuffer[channelCount].peripheral,
		Rgb_Pwm_ChannelInfoBuffer[channelCount].timerChannelSel,
		1000,
		0,
		Rgb_Pwm_ChannelInfoBuffer[channelCount].Polarity,
		RGB_PWM_CHANNEL_CLOCK_PERIOD);

        QTMR_StartTimer(Rgb_Pwm_ChannelInfoBuffer[channelCount].peripheral,
        Rgb_Pwm_ChannelInfoBuffer[channelCount].timerChannelSel,
        Rgb_Pwm_ChannelInfoBuffer[channelCount].countMode);
    }

    taskENTER_CRITICAL();

    (void)xTaskCreate(&Set_PeriodAndDuty_Task,     "SET_PERIODANDDUTY_TASK",	400U/4U,   NULL,   SET_PERIODANDDUTY_TASK_PRIO,		NULL);

    vTaskDelete(NULL);

    taskEXIT_CRITICAL();
}
