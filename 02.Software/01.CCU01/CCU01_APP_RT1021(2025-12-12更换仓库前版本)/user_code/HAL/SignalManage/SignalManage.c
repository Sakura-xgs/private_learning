/*
 * SignalManage.c
 *
 *  Created on: 2024年8月19日
 *      Author: Bono
 */
#include "hal_eeprom_IF.h"
#include "SignalManage.h"
#include "Signal.h"
#include "Signal_IF.h"
#include "uart_comm.h"
#include "PublicDefine.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "flash_data_IF.h"

#define CCU_CONFIG_NUM          (CCU_SET_SIG_ID_END_FLAG - CCU_SET_SIG_ID_BEGIN_FLAG)

SemaphoreHandle_t g_CtlExtEeprom_MutexSemaphore = NULL;
static QueueHandle_t g_ExtEeprom_xQueue = NULL;

static S32 s32EepromSignal[CCU_CONFIG_NUM] = {0};
static S32 s32RamSignal[CCU_SAM_SIG_ID_END_FLAG - CCU_SAM_SIG_ID_BEGIN_FLAG] = {0};
static S32 s32AlarmRam[ALARM_SIG_NUM] = {0};
static S32 s32BoolRam[BOOL_SIG_NUM] = {0};
static BOOL g_blParaInitFinishFgBuf[CCU_SET_SIG_ID_END_FLAG - CCU_SET_SIG_ID_BEGIN_FLAG] = {0};

BOOL g_blExtEepromErrFg = 0;	//禁止递归调用，EEPROM故障此处不能使用信号表信号

static BOOL SaveSignal2EepromFunc(U32 u32SigId, S32 s32SigVal);
static void SetSignalValBuf(U32 u32SigId, S32 s32SigVal);

//参数设置接口
BOOL SetSigVal(U32 u32SigId, S32 SignalValue)
{
	static U32 g_32InitParaErrCnt = 0;

    S32 s32MaxValue = 0;
    S32 s32MinValue = 0;
    S32 s32GetSigVal = 0;
	U8 i = 0;

	//寄存器表（参数）
	if ((u32SigId > CCU_SET_SIG_ID_BEGIN_FLAG)
		&& (u32SigId < CCU_SET_SIG_ID_END_FLAG))
	{
        s32MaxValue = SetSignalArry[u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG].m_u32MaxVal;
        s32MaxValue = s32MaxValue + SetSignalArry[u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG].m_s16OffSet;
        s32MinValue = SetSignalArry[u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG].m_u32MinVal;
        s32MinValue = s32MinValue + SetSignalArry[u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG].m_s16OffSet;

        //参数设置范围判断
	    if ((SignalValue > s32MaxValue)
        	|| (SignalValue < s32MinValue))
	    {
            return (FALSE);
		}

		//参数未初始化
		if (FALSE == g_blParaInitFinishFgBuf[u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG])
		{
			for(i = 0; i < 3U; i++)
			{
				if(TRUE == ExtEepromReadParameter(u32SigId, &s32GetSigVal))
				{
					SetSignalValBuf(u32SigId, s32GetSigVal);
					break;
				}
			}

			if(SignalValue != s32EepromSignal[u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG])
			{
				if(TRUE != ExtEepromSaveParameter(u32SigId, SignalValue))
				{
					g_32InitParaErrCnt++;
				}
				else
                {
                    SetSignalValBuf(u32SigId, SignalValue);
                }
			}

			g_blParaInitFinishFgBuf[u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG] = TRUE;

			return (TRUE);
		}

	    //参数值一样返回成功
		if(SignalValue != s32EepromSignal[u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG])
		{
			if(TRUE == SaveSignal2EepromFunc(u32SigId, SignalValue))
			{
				SaveSignal2FlashFunc(u32SigId, SignalValue);
            	return (TRUE);
			}
			else
			{
            	return (FALSE);
			}
		}
        else
        {
            return (TRUE);
        }
	}
	//寄存器表（实时寄存器数据）
	else if ((u32SigId > CCU_SAM_SIG_ID_BEGIN_FLAG)
		&& (u32SigId < CCU_SAM_SIG_ID_END_FLAG))
	{
		if(SignalValue != s32RamSignal[u32SigId - CCU_SAM_SIG_ID_BEGIN_FLAG])
		{
			SaveSignal2FlashFunc(u32SigId, SignalValue);

			s32RamSignal[u32SigId - CCU_SAM_SIG_ID_BEGIN_FLAG] = SignalValue;
		}
	}
	//开关量表（常规告警、特殊告警）
	else if ((u32SigId > ALARM_ID_PILE_BEGIN_FLAG)
		&& (u32SigId < ALARM_ID_PILE_END_FLAG))
	{
		if((0 != SignalValue) && (1 != SignalValue))
		{
			return (FALSE);
		}

		U8 u8bitPos = (U8)(u32SigId - ALARM_ID_PILE_BEGIN_FLAG)%SIG_BIT;

		if((U32)SignalValue != (((U32)s32AlarmRam[(u32SigId - ALARM_ID_PILE_BEGIN_FLAG)/SIG_BIT] >> u8bitPos) & 0x01U))
		{
			if(SignalValue == 1)
			{
				s32AlarmRam[(u32SigId - ALARM_ID_PILE_BEGIN_FLAG)/SIG_BIT] |= (1U << u8bitPos);
			}
			else
			{
				s32AlarmRam[(u32SigId - ALARM_ID_PILE_BEGIN_FLAG)/SIG_BIT] &= ~(1U << u8bitPos);
			}

			SaveSignal2FlashFunc(u32SigId, SignalValue);
		}
	}
	//开关量表（信号状态）
	else if ((u32SigId > SIGNAL_STATUS_BEGIN_FLAG)
		&& (u32SigId < SIGNAL_STATUS_END_FLAG))
	{
		if((0 != SignalValue) && (1 != SignalValue))
		{
			return (FALSE);
		}

		U8 u8bitPos = (U8)(u32SigId - SIGNAL_STATUS_BEGIN_FLAG)%SIG_BIT;

		if((U32)SignalValue != (((U32)s32BoolRam[(u32SigId - SIGNAL_STATUS_BEGIN_FLAG)/SIG_BIT] >> u8bitPos) & 0x01U))
		{
			if(SignalValue == 1)
			{
				s32BoolRam[(u32SigId - SIGNAL_STATUS_BEGIN_FLAG)/SIG_BIT] |= (1UL << u8bitPos);
			}
			else
			{
				s32BoolRam[(u32SigId - SIGNAL_STATUS_BEGIN_FLAG)/SIG_BIT] &= ~(1UL << u8bitPos);
			}

			SaveSignal2FlashFunc(u32SigId, SignalValue);
		}
	}
	else
	{
		return (FALSE);
	}

	return (TRUE);
}

//参数设置接口
BOOL GetSigVal(U32 u32SigId,S32 *SignalValue)
{
	//非法地址
	if ((NULL == SignalValue)
		|| ((void*)0 == SignalValue)
		|| (0 == SignalValue))
	{
		return (FALSE);
	}

	//寄存器表（参数）
	if ((u32SigId > CCU_SET_SIG_ID_BEGIN_FLAG)
		&& (u32SigId < CCU_SET_SIG_ID_END_FLAG))
	{
	    S32 s32GetSigVal = 0;
		U8 i = 0;
		//参数未初始化
		if (FALSE == g_blParaInitFinishFgBuf[u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG])
		{
			for(i = 0; i < 3U; i++)
			{
				if(TRUE == ExtEepromReadParameter(u32SigId, &s32GetSigVal))
				{
					SetSignalValBuf(u32SigId, s32GetSigVal);
					break;
				}
			}

			g_blParaInitFinishFgBuf[u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG] = TRUE;
		}

		*SignalValue = s32EepromSignal[u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG];
	}
	//寄存器表（实时寄存器数据）
	else if ((u32SigId > CCU_SAM_SIG_ID_BEGIN_FLAG)
		&& (u32SigId < CCU_SAM_SIG_ID_END_FLAG))
	{
		*SignalValue = s32RamSignal[u32SigId - CCU_SAM_SIG_ID_BEGIN_FLAG];
	}
	//开关量表（常规告警、特殊告警）
	else if ((u32SigId > ALARM_ID_PILE_BEGIN_FLAG)
		&& (u32SigId < ALARM_ID_PILE_END_FLAG))
	{
		U8 u8bitPos = (U8)((u32SigId - ALARM_ID_PILE_BEGIN_FLAG)%SIG_BIT);
		if (((U32)s32AlarmRam[(u32SigId - ALARM_ID_PILE_BEGIN_FLAG)/SIG_BIT] >> u8bitPos) & 0x01U)
		{
			*SignalValue = 1;
		}
		else
		{
			*SignalValue = 0;
		}
	}
	//开关量表（信号状态）
	else if ((u32SigId > SIGNAL_STATUS_BEGIN_FLAG)
		&& (u32SigId < SIGNAL_STATUS_END_FLAG))
	{
		U8 u8bitPos = (u32SigId - SIGNAL_STATUS_BEGIN_FLAG)%SIG_BIT;
		if (((U32)s32BoolRam[(u32SigId - SIGNAL_STATUS_BEGIN_FLAG)/SIG_BIT] >> u8bitPos) & 0x01U)
		{
			*SignalValue = 1;
		}
		else
		{
			*SignalValue = 0;
		}
	}
	else
	{
		return (FALSE);
	}

	return (TRUE);
}

//系统参数初始化
void InitSigVal(void)
{
    U16 i = 0, j = 0;
    S32 s32GetSigVal = 0;

    ///MBMS 参数初始化
    for(i = 0; i < CCU_CONFIG_NUM; i++)
    {
        //s32EepromSignal[i] = (SetSignalArry[i].m_u32DefaultVal+SetSignalArry[i].m_s16OffSet);
    	//参数未初始化
    	if (FALSE == g_blParaInitFinishFgBuf[i])
    	{
    		for(j = 0; j < 3U; j++)
    		{
    			vTaskDelay(10);
    			if(TRUE == ExtEepromReadParameter(SetSignalArry[i].m_u16Signal, &s32GetSigVal))
    			{
    	    		g_blParaInitFinishFgBuf[i] = TRUE;
    				SetSignalValBuf(SetSignalArry[i].m_u16Signal, s32GetSigVal);
    				break;
    			}
    		}
    	}
    }
}

static void SetSignalValBuf(U32 u32SigId, S32 s32SigVal)
{
    S32 s32MaxValue = 0;
    S32 s32MinValue = 0;

	//寄存器表（参数）
	if ((u32SigId > CCU_SET_SIG_ID_BEGIN_FLAG)
		&& (u32SigId < CCU_SET_SIG_ID_END_FLAG))
	{
        s32MaxValue = SetSignalArry[u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG].m_u32MaxVal;
        s32MaxValue = s32MaxValue + SetSignalArry[u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG].m_s16OffSet;
        s32MinValue = SetSignalArry[u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG].m_u32MinVal;
        s32MinValue = s32MinValue + SetSignalArry[u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG].m_s16OffSet;

        //参数设置范围判断
		if ((s32SigVal > s32MaxValue)
		|| (s32SigVal < s32MinValue))
		{
            return;
		}

	    //参数值一样返回成功
		if(s32SigVal != s32EepromSignal[u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG])
		{
			s32EepromSignal[u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG] = s32SigVal;
		}
	}
}

void SetSignalValInit_Task(void * pvParameters)
{
	static S32 s32EEPROMCheck = 0;

	U8 u8EECheckCnt = 0;
	S32 s32ResetNum = 0;//,s32EEPROMCheck = 0;

	//延时200ms，等待写eeprom的任务完成创建
	vTaskDelay(200);

	//记录复位次数
	(void)GetSigVal(CCU_SET_SIG_ID_RESET_TIMES, &s32ResetNum);
	s32ResetNum++;
	uPRINTF("Power On, This CCU has started up for %d times\n", s32ResetNum);

	//EEPROM自检
	(void)SetSigVal(CCU_SET_SIG_ID_RESET_TIMES, s32ResetNum);
	vTaskDelay(500);
	(void)GetSigVal(CCU_SET_SIG_ID_RESET_TIMES, &s32EEPROMCheck);
	if(s32ResetNum == s32EEPROMCheck)
	{
		u8EECheckCnt++;
	}
	(void)SetSigVal(CCU_SET_SIG_ID_EXT_EEPROM_TEST_VAL, s32ResetNum);
	vTaskDelay(500);
	(void)GetSigVal(CCU_SET_SIG_ID_EXT_EEPROM_TEST_VAL, &s32EEPROMCheck);
	if(s32ResetNum == s32EEPROMCheck)
	{
		u8EECheckCnt++;
	}

	if(u8EECheckCnt == 0U)
	{
		g_blExtEepromErrFg = TRUE;
	}

	vTaskDelete(NULL);
}

S32 GetMsgVal(U32 u32SigId)
{
	S32 s32SigVal = 0;
	if(TRUE == GetSigVal(u32SigId, &s32SigVal))
	{
		return s32SigVal;
	}
	return (S32)FALSE;
}

static BOOL SaveSignal2EepromFunc(U32 u32SigId, S32 s32SigVal)
{
	static EEP_MSG eepMessage = {0};

	//外部EEPROM无故障才进行写入操作
	if(TRUE == g_blExtEepromErrFg)
	{
		return FALSE;
	}

	eepMessage.u32SigId = u32SigId;
	eepMessage.s32SigVal = s32SigVal;

	if(pdPASS == xQueueSend(g_ExtEeprom_xQueue, (void *) &eepMessage, (TickType_t)500))
	{
		return TRUE;
	}

	return FALSE;
}

/*!
    \brief      Save Eeprom task
    \param[in]  pvParameters not used
    \param[out] none
    \retval     none
*/
void Save_Eeprom_Task(void * pvParameters)
{
    BaseType_t err = pdFALSE;
	volatile UBaseType_t uxHighWaterMark;
	EEP_MSG ptMsg = {0};

	(void)pvParameters;

	g_ExtEeprom_xQueue = xQueueCreate(20, sizeof(EEP_MSG));
	g_CtlExtEeprom_MutexSemaphore = xSemaphoreCreateMutex();

    for( ;; )
    {
		if(NULL != g_ExtEeprom_xQueue)
		{
			err = xQueueReceive(g_ExtEeprom_xQueue, (void *)&ptMsg, (TickType_t)portMAX_DELAY);
            if(pdTRUE == err)
            {
				if(TRUE == ExtEepromSaveParameter(ptMsg.u32SigId, ptMsg.s32SigVal))
				{
					s32EepromSignal[ptMsg.u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG] = ptMsg.s32SigVal;
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
/************************************/
