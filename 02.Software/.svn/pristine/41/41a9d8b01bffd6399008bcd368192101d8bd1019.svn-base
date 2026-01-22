/*
 * Copyright (c) 2013 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2021, 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fan_pwm.h"
#include "fan_pwm_IF.h"
#include "hal_sys_IF.h"
#include "tcp_client_IF.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

Fan_Pwm_DataSetType Fan_Pwm = {
		.period = 20000,
		.dutycycle = 50,
};

typedef struct
{
    TMR_Type * peripheral;
    qtmr_channel_selection_t timerChannelSel;
    bool Polarity;
    qtmr_counting_mode_t countMode;
    uint32_t  srcClockPeriod;
}Fan_Pwm_ChannelInfoType;


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

static Fan_Pwm_ChannelInfoType Fan_Pwm_ChannelInfoBuffer[1] =
{
    {
        TMR2_PERIPHERAL,
		FAN_PWM_CHANNEL1,
        false,
        kQTMR_PriSrcRiseEdge,
        FAN_PWM_CHANNEL_CLOCK_PERIOD
    }
};
/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Main function
 */
static void SetFan_PeriodAndDuty_Task(void * pvParameters)
{
	U8 channelCount = 0U;
    U8 LastDutycycle = 0;
    U8 FanControlStatus = 0;

	for( ;; )
	{
		if(LastDutycycle != Fan_Pwm.dutycycle)
		{
			(void)QTMR_SetupPwm(Fan_Pwm_ChannelInfoBuffer[channelCount].peripheral,
			Fan_Pwm_ChannelInfoBuffer[channelCount].timerChannelSel,
			Fan_Pwm.period,
			Fan_Pwm.dutycycle,
			Fan_Pwm_ChannelInfoBuffer[channelCount].Polarity,
			FAN_PWM_CHANNEL_CLOCK_PERIOD);

			QTMR_StartTimer(Fan_Pwm_ChannelInfoBuffer[channelCount].peripheral,
			Fan_Pwm_ChannelInfoBuffer[channelCount].timerChannelSel,
			Fan_Pwm_ChannelInfoBuffer[channelCount].countMode);

			//上传web反向，数值对应真实转速
			g_sPile_data.ucFan_pwm = 100U - Fan_Pwm.dutycycle;

			LastDutycycle = Fan_Pwm.dutycycle;
		}

		vTaskDelay(5000);

		if(Fan_Pwm.dutycycle == 100U)//控制风扇停,风扇停止的状态是高电平
		{
			FanControlStatus = ~GET_DI_HIT_STATUS();
		}
		else//控制风扇运转
		{
			FanControlStatus = GET_DI_HIT_STATUS();//风扇运转的状态是低电平
		}
	}
}

void Fan_Pwm_Init_Task(void * pvParameters)
{
    (void)pvParameters;
    uint8_t channelCount = 0U;

    //初始化占空比为0
	(void)QTMR_SetupPwm(Fan_Pwm_ChannelInfoBuffer[channelCount].peripheral,
	Fan_Pwm_ChannelInfoBuffer[channelCount].timerChannelSel,
	20000,
	0,
	Fan_Pwm_ChannelInfoBuffer[channelCount].Polarity,
	FAN_PWM_CHANNEL_CLOCK_PERIOD);

	QTMR_StartTimer(Fan_Pwm_ChannelInfoBuffer[channelCount].peripheral,
	Fan_Pwm_ChannelInfoBuffer[channelCount].timerChannelSel,
	Fan_Pwm_ChannelInfoBuffer[channelCount].countMode);

    taskENTER_CRITICAL();

    (void)xTaskCreate(&SetFan_PeriodAndDuty_Task,     "SETFAN_PERIODANDDUTY_TASK",	400U/4U,   NULL,   5,		NULL);

    vTaskDelete(NULL);

    taskEXIT_CRITICAL();
}
