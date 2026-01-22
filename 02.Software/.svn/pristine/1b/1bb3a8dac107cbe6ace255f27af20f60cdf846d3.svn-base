/*
 * data_sample.c
 *
 *  Created on: 2024年12月13日
 *      Author: Bono
 */
#include <APP/data_sample/data_sample.h>
#include <APP/relay_ctrl/relay_ctrl.h>
#include <HAL/factory_test/factory_test.h>
#include "fsl_flexcan.h"

#include "boot.h"
#include "hal_sys_IF.h"
#include "hal_can_IF.h"
#include "hal_adc_IF.h"
#include "SignalManage.h"
#include "PublicDefine.h"
#include "APP/average_filter/average_filter.h"

tmp_sample_t TmpMap[] =
{
	{0, PDU_SAM_SIG_ID_DC_NEG_TEMP, 	HAL_TEMP_AdToTemp_PT1000},
	{0, PDU_SAM_SIG_ID_PCB_TEMP_1, 		HAL_TEMP_AdToTemp_3435},
	{0, PDU_SAM_SIG_ID_DC_POS_TEMP, 	HAL_TEMP_AdToTemp_PT1000},
	{0, PDU_SAM_SIG_ID_PCB_TEMP_2, 		HAL_TEMP_AdToTemp_3435},
};
#define TEMP_SAMPLE_TABLE_SIZE (sizeof(TmpMap)/sizeof(tmp_sample_t))

relay_input_t RelayDiMap[MAX_RELAY_NUM] =
{
	{SIGNAL_STATUS_K1_FB, BOARD_INITPINS_DI_K1_GPIO, BOARD_INITPINS_DI_K1_GPIO_PIN},
	{SIGNAL_STATUS_K2_FB, BOARD_INITPINS_DI_K2_GPIO, BOARD_INITPINS_DI_K2_GPIO_PIN},
	{SIGNAL_STATUS_K3_FB, BOARD_INITPINS_DI_K3_GPIO, BOARD_INITPINS_DI_K3_GPIO_PIN},
	{SIGNAL_STATUS_K4_FB, BOARD_INITPINS_DI_K4_GPIO, BOARD_INITPINS_DI_K4_GPIO_PIN},
	{SIGNAL_STATUS_K5_FB, BOARD_INITPINS_DI_K5_GPIO, BOARD_INITPINS_DI_K5_GPIO_PIN},
	{SIGNAL_STATUS_K6_FB, BOARD_INITPINS_DI_K6_GPIO, BOARD_INITPINS_DI_K6_GPIO_PIN},
	{SIGNAL_STATUS_K7_FB, BOARD_INITPINS_DI_K7_GPIO, BOARD_INITPINS_DI_K7_GPIO_PIN},
	{SIGNAL_STATUS_K8_FB, BOARD_INITPINS_DI_K8_GPIO, BOARD_INITPINS_DI_K8_GPIO_PIN},
	{SIGNAL_STATUS_K9_FB, BOARD_INITPINS_DI_K9_GPIO, BOARD_INITPINS_DI_K9_GPIO_PIN},
	{SIGNAL_STATUS_K10_FB, BOARD_INITPINS_DI_K10_GPIO, BOARD_INITPINS_DI_K10_GPIO_PIN},
	{SIGNAL_STATUS_K11_FB, BOARD_INITPINS_DI_K11_GPIO, BOARD_INITPINS_DI_K11_GPIO_PIN},
	{SIGNAL_STATUS_K12_FB, BOARD_INITPINS_DI_K12_GPIO, BOARD_INITPINS_DI_K12_GPIO_PIN},
	{SIGNAL_STATUS_K13_FB, BOARD_INITPINS_DI_K13_GPIO, BOARD_INITPINS_DI_K13_GPIO_PIN},
	{SIGNAL_STATUS_K14_FB, BOARD_INITPINS_DI_K14_GPIO, BOARD_INITPINS_DI_K14_GPIO_PIN},
	{SIGNAL_STATUS_K15_FB, BOARD_INITPINS_DI_K15_GPIO, BOARD_INITPINS_DI_K15_GPIO_PIN},
	{SIGNAL_STATUS_K16_FB, BOARD_INITPINS_DI_K16_GPIO, BOARD_INITPINS_DI_K16_GPIO_PIN},
};

static const relay_alarm_t RelayAlarmMap[MAX_RELAY_NUM] =
{
	{ALARM_ID_K1_ADHESION, 	ALARM_ID_K1_DRIVE_FAILED, 	SIGNAL_STATUS_K1,	SIGNAL_STATUS_K1_FB,	TIME_2S, TIME_2S},
	{ALARM_ID_K2_ADHESION, 	ALARM_ID_K2_DRIVE_FAILED, 	SIGNAL_STATUS_K2,	SIGNAL_STATUS_K2_FB,	TIME_2S, TIME_2S},
	{ALARM_ID_K3_ADHESION, 	ALARM_ID_K3_DRIVE_FAILED, 	SIGNAL_STATUS_K3,	SIGNAL_STATUS_K3_FB,	TIME_2S, TIME_2S},
	{ALARM_ID_K4_ADHESION, 	ALARM_ID_K4_DRIVE_FAILED, 	SIGNAL_STATUS_K4,	SIGNAL_STATUS_K4_FB,	TIME_2S, TIME_2S},
	{ALARM_ID_K5_ADHESION, 	ALARM_ID_K5_DRIVE_FAILED, 	SIGNAL_STATUS_K5,	SIGNAL_STATUS_K5_FB,	TIME_2S, TIME_2S},
	{ALARM_ID_K6_ADHESION, 	ALARM_ID_K6_DRIVE_FAILED, 	SIGNAL_STATUS_K6,	SIGNAL_STATUS_K6_FB,	TIME_2S, TIME_2S},
	{ALARM_ID_K7_ADHESION, 	ALARM_ID_K7_DRIVE_FAILED, 	SIGNAL_STATUS_K7,	SIGNAL_STATUS_K7_FB,	TIME_2S, TIME_2S},
	{ALARM_ID_K8_ADHESION, 	ALARM_ID_K8_DRIVE_FAILED, 	SIGNAL_STATUS_K8,	SIGNAL_STATUS_K8_FB,	TIME_2S, TIME_2S},
	{ALARM_ID_K9_ADHESION, 	ALARM_ID_K9_DRIVE_FAILED, 	SIGNAL_STATUS_K9,	SIGNAL_STATUS_K9_FB,	TIME_2S, TIME_2S},
	{ALARM_ID_K10_ADHESION, ALARM_ID_K10_DRIVE_FAILED, 	SIGNAL_STATUS_K10,	SIGNAL_STATUS_K10_FB,	TIME_2S, TIME_2S},
	{ALARM_ID_K11_ADHESION, ALARM_ID_K11_DRIVE_FAILED, 	SIGNAL_STATUS_K11,	SIGNAL_STATUS_K11_FB,	TIME_2S, TIME_2S},
	{ALARM_ID_K12_ADHESION, ALARM_ID_K12_DRIVE_FAILED, 	SIGNAL_STATUS_K12,	SIGNAL_STATUS_K12_FB,	TIME_2S, TIME_2S},
	{ALARM_ID_K13_ADHESION, ALARM_ID_K13_DRIVE_FAILED, 	SIGNAL_STATUS_K13,	SIGNAL_STATUS_K13_FB,	TIME_2S, TIME_2S},
	{ALARM_ID_K14_ADHESION, ALARM_ID_K14_DRIVE_FAILED, 	SIGNAL_STATUS_K14,	SIGNAL_STATUS_K14_FB,	TIME_2S, TIME_2S},
	{ALARM_ID_K15_ADHESION, ALARM_ID_K15_DRIVE_FAILED, 	SIGNAL_STATUS_K15,	SIGNAL_STATUS_K15_FB,	TIME_2S, TIME_2S},
	{ALARM_ID_K16_ADHESION, ALARM_ID_K16_DRIVE_FAILED, 	SIGNAL_STATUS_K16,	SIGNAL_STATUS_K16_FB,	TIME_2S, TIME_2S},
};

static const AverageFilterType channelMap[TEMP_SAMPLE_TABLE_SIZE] =
{
    TEMP_DC_NEG_TYPE,
    TEMP_PCB_1_TYPE,
    TEMP_DC_POS_TYPE,
    TEMP_PCB_2_TYPE
};

void RelayStateSample(void)
{
	U8 i = 0;
	S32 s32State = 0;

	for(i = 0; i < MAX_RELAY_NUM; i++)
	{
		s32State = GPIO_ReadPinInput(RelayDiMap[i].base, RelayDiMap[i].pin);
		s32State = (~s32State) & 0x01;	/*根据电气定义，高电平为断开，低电平为闭合*/
		(void)SetSigVal(RelayDiMap[i].u32SingalId, s32State);
	}
}

void TemperatureSample(void)
{
	U8 i = 0;
	U8 j = 0;
	S32 s32temp = 0;
	S32 s32sum = 0;
	S32 s32filteredTemp = 0;
	
	for(i = 0; i < TEMP_SAMPLE_TABLE_SIZE; i++)
	{
		s32temp = TmpMap[i].func(TmpMap[i].u32AdValue);						// 获取当前温度值

		if(TRUE == GetMsgVal(SIGNAL_STATUS_FACTORY_TEST))
		{
			(void)SetSigVal(TmpMap[i].u32SingalId, s32temp);				// 生产测试模式下不进行温度滤波
		}
		else
		{
			s32filteredTemp = AverageFilter_Update(channelMap[i], s32temp);				// 进行温度滤波	
			(void)SetSigVal(TmpMap[i].u32SingalId, s32filteredTemp);		// 更新信号值
		}
	}
}

void RelayAlarmSample(void)
{
	static U32 u32RelayAdhCnt[MAX_RELAY_NUM] = {0};
	static U32 u32RelayDrvFailCnt[MAX_RELAY_NUM] = {0};
	U8 i = 0;

	for(i = 0; i < MAX_RELAY_NUM; i++)
	{
		if(EVENT_CANCEL == GetMsgVal(RelayAlarmMap[i].u32AdhAlarmId))
		{
			if(true == GetMsgVal(RelayAlarmMap[i].u32DiFbSingalId)
			&& false == GetMsgVal(RelayAlarmMap[i].u32CtrlStSingalId))
			{
				u32RelayAdhCnt[i]++;
				if(u32RelayAdhCnt[i] >= TIME_2S)
				{
					u32RelayAdhCnt[i] = 0;
					(void)SetSigVal(RelayAlarmMap[i].u32AdhAlarmId, EVENT_HAPPEN);
				}
			}
			else
			{
				u32RelayAdhCnt[i] = 0;
			}
		}
		else
		{
			if(false == GetMsgVal(RelayAlarmMap[i].u32DiFbSingalId))
			{
				u32RelayAdhCnt[i]++;
				if(u32RelayAdhCnt[i] >= TIME_2S)
				{
					u32RelayAdhCnt[i] = 0;
					(void)SetSigVal(RelayAlarmMap[i].u32AdhAlarmId, EVENT_CANCEL);
				}
			}
		}


		if(EVENT_CANCEL == GetMsgVal(RelayAlarmMap[i].u32DrvFailAlarmId))
		{
			if(false == GetMsgVal(RelayAlarmMap[i].u32DiFbSingalId)
			&& true == GetMsgVal(RelayAlarmMap[i].u32CtrlStSingalId))
			{
				u32RelayDrvFailCnt[i]++;
				if(u32RelayDrvFailCnt[i] >= TIME_2S)
				{
					u32RelayDrvFailCnt[i] = 0;
					(void)SetSigVal(RelayAlarmMap[i].u32DrvFailAlarmId, EVENT_HAPPEN);
				}
			}
			else
			{
				u32RelayDrvFailCnt[i] = 0;
			}
		}
		else
		{
			if(true == GetMsgVal(RelayAlarmMap[i].u32DiFbSingalId))
			{
				u32RelayDrvFailCnt[i]++;
				if(u32RelayDrvFailCnt[i] >= TIME_2S)
				{
					u32RelayDrvFailCnt[i] = 0;
					(void)SetSigVal(RelayAlarmMap[i].u32DrvFailAlarmId, EVENT_CANCEL);
				}
			}
		}
	}
}

void PduSoftReset(void)
{
	(void)SetSigVal(SIGNAL_STATUS_POWER_OFF, TRUE);
	SetSysShuttingDownModeFg();
	CutOffAllRelaysCmd();
	vTaskDelay(100);							// 防止切断继电器操作未完成就进行数据保存
	SaveRealyCtrlTimes();
	uPRINTF("Soft Shutting down! all massage has been saved.");
	SetSysPowerOffModeFg();

	vTaskDelay(1000);
	SystemResetFunc();
}

void PduShutDown(void)
{
	U16 u16Cnt = 0;

	(void)SetSigVal(SIGNAL_STATUS_POWER_OFF, TRUE);
	SetSysShuttingDownModeFg();
	CutOffAllRelaysCmd();
	vTaskDelay(100);							// 防止切断继电器操作未完成就进行数据保存
	SaveRealyCtrlTimes();
	uPRINTF("Power off! all massage has been saved.");
	SetSysPowerOffModeFg();

	vTaskDelay(5000);
	while(u16Cnt < 5) {
		vTaskDelay(30);
		if(GET_WDIPFO_STATUS() == TRUE) {
			u16Cnt++;
		} else {
			u16Cnt = 0;
		}
	}
	uPRINTF("Power on recover, reset!");
	SystemResetFunc();
}

void PowerOffSample(void)
{
	static U16 u16Cnt = 0;
	static BOOL blPwOffFg = FALSE;

	if(GET_WDIPFO_STATUS() == FALSE
	|| blPwOffFg == TRUE)
	{
		u16Cnt++;
		if(u16Cnt >= 3)
		{
			u16Cnt = 0;
			blPwOffFg = TRUE;
			PduShutDown();
		}
	}
	else
	{
		u16Cnt = 0;
	}
}

/*!
    \brief      DataSample task
    \param[in]  pvParameters not used
    \param[out] none
    \retval     none
*/
void DataSample_task(void * pvParameters)
{
	volatile UBaseType_t uxHighWaterMark;
	portTickType xLastWakeTime;

	(void)pvParameters;

	xLastWakeTime = xTaskGetTickCount();

	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

    for( ;; )
    {
        RelayStateSample();

        TemperatureSample();

        RelayAlarmSample();

        PowerOffSample();

		vTaskDelay(DATASAMPLE_TASK_PERIOD_10MS);

		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    }
}

/*!
    \brief      DATA SAMPLE INIT task
    \param[in]  pvParameters not used
    \param[out] none
    \retval     none
*/
void DataSample_Init_task(void * pvParameters)
{
	volatile UBaseType_t uxHighWaterMark;

	taskENTER_CRITICAL();

    xTaskCreate(DataSample_task,  "SYS_DATA_SAMPLE",  configMINIMAL_STACK_SIZE*2, NULL,   GENERAL_TASK_PRIO,      NULL);

	taskEXIT_CRITICAL();

	vTaskDelete(NULL);
}
