/*
 * relay_ctrl.c
 *
 *  Created on: 2024年12月5日
 *      Author: Bono
 */
#include <APP/relay_ctrl/relay_ctrl.h>

#include "hal_sys_IF.h"
#include "hal_can_IF.h"
#include "SignalManage.h"

static uint8_t g_RelayCtrlData[RELAY_CMD_ARRAY_SIZE] = {0};
static SemaphoreHandle_t g_RelayCtrl_Binary_Semaphore = NULL;

BOOL ParseRelayCtrlMsg(flexcan_frame_t *Frame)
{
    if((Frame == NULL) || (Frame->length != RELAY_CMD_ARRAY_SIZE))		// 检查继电器控制命令
    {
        return FALSE;
    }

    g_RelayCtrlData[0] = Frame->dataByte0;								// 将控制命令复制到全局变量
    g_RelayCtrlData[1] = Frame->dataByte1;
    g_RelayCtrlData[2] = Frame->dataByte2;
    g_RelayCtrlData[3] = Frame->dataByte3;
    g_RelayCtrlData[4] = Frame->dataByte4;
    g_RelayCtrlData[5] = Frame->dataByte5;
    g_RelayCtrlData[6] = Frame->dataByte6;
    g_RelayCtrlData[7] = Frame->dataByte7;

   	if (xSemaphoreGive(g_RelayCtrl_Binary_Semaphore) == pdTRUE)			// 释放信号量通知任务处理
    {
        return TRUE;
    }
    
    return FALSE;
}

void CutOffAllRelaysCmd(void)
{
	flexcan_frame_t Frame = {0};
	Frame.length = RELAY_CMD_ARRAY_SIZE;
	(void)ParseRelayCtrlMsg(&Frame);
}

BOOL IsAllRelayCutOff(void)
{
	U8 i = 0;
	S32 s32SigVal = 0;

	for(i = 0; i < MAX_RELAY_NUM; i++)
	{
		(void)GetSigVal(SIGNAL_STATUS_K1+i, &s32SigVal);
		if(s32SigVal == TRUE)
		{
			return FALSE;
		}
	}

	return TRUE;
}

void RelayCtrlFunc(const uint8_t control_array[RELAY_CMD_ARRAY_SIZE])
{
	U8 i, j = 0;
	U8 u8Byte = 0;
	U8 u8RelayIndex = 0;
	BOOL blRelayCmd = false;

    for (i = 0; i < RELAY_CMD_ARRAY_SIZE; i++)
    {
    	u8Byte = control_array[i];
        for (j = 0; j < 8; j++)
        {
        	blRelayCmd = (u8Byte >> j) & 0x01;
        	u8RelayIndex = (i * 8) + j;
        	if(u8RelayIndex >= MAX_RELAY_NUM)
        	{
        		return;
        	}
            // 处理当前位的控制命令
            ctrl_relay_cmd(u8RelayIndex, blRelayCmd);
        }
    }
}

void CutOffAllRelay(void)
{
	U8 RelayCmd[RELAY_CMD_ARRAY_SIZE] = {0};
	RelayCtrlFunc(RelayCmd);
}

void Relay_Ctrl_Task(void * pvParameters)
{
    BaseType_t err = pdFALSE;
	volatile UBaseType_t uxHighWaterMark;

	(void)pvParameters;

	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

    for( ;; )
    {
		if((SYS_STATUS_NORMAL != GetSysRunModeFg())
		|| (TRUE == GetMsgVal(SIGNAL_STATUS_FACTORY_TEST)))
		{
			CutOffAllRelay();
			vTaskDelete(NULL);
		}

		if(NULL != g_RelayCtrl_Binary_Semaphore)
		{
			err = xSemaphoreTake(g_RelayCtrl_Binary_Semaphore, (TickType_t)RELAY_CTRL_TIME_OUT_VAL);

            if((pdTRUE == err) && (FALSE == GetMsgVal(SIGNAL_STATUS_FACTORY_TEST)))
            {
				RelayCtrlFunc(g_RelayCtrlData);
            }
            else if(err == pdFALSE)
            {
            	if(EVENT_HAPPEN == GetMsgVal(ALARM_ID_PCU_OFFLINE))
            	{
            		CutOffAllRelay();
            	}
            }
		}
		else
		{
			vTaskDelay(300);
		}

		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    }
}

void Relay_Ctrl_Init_task(void * pvParameters)
{
	volatile UBaseType_t uxHighWaterMark = 0;

	taskENTER_CRITICAL();

	g_RelayCtrl_Binary_Semaphore = xSemaphoreCreateBinary();

	xTaskCreate(Relay_Ctrl_Task,	"RELAY_CTRL",		configMINIMAL_STACK_SIZE,   NULL,   GENERAL_TASK_PRIO,		NULL);

	vTaskDelete(NULL);

	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

	taskEXIT_CRITICAL();
}
