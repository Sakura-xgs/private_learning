#include "SignalManage.h"
#include "SignalManageDefine.h"
#include "MbmsSignal_IF.h"
#include "ext_eeprom.h"



#define MBMS_CONFIG_NUM          (MBMS_SET_SIG_ID_END_FLAG - MBMS_SET_SIG_ID_BEGIN_FLAG)



BOOL SaveSignal2EepromFunc(U32 u32SigId, S32 s32SigVal);
BOOL g_blParaInitFinishFgBuf[MBMS_SET_SIG_ID_END_FLAG - MBMS_SET_SIG_ID_BEGIN_FLAG];


static QueueHandle_t g_ExtEeprom_xQueue = NULL;

S32  s32MbmsSet[MBMS_CONFIG_NUM] = {0};
S32  s32MbmsRam[MBMS_SAM_SIG_ID_END_FLAG - MBMS_SAM_SIG_ID_BEGIN_FLAG] = {0};
S32  s32MbmsAlarmRam[ALARM_ID_EVENT_END_FLAG - ALARM_ID_EVENT_BEGIN_FLAG] = {0};
S32  s32MbmsTotalAlarmRam[SIGNAL_STATUS_END_FLAG - SIGNAL_STATUS_BEGIN_FLAG] = {0};


BOOL g_blParaInitFinishFgBuf[MBMS_SET_SIG_ID_END_FLAG - MBMS_SET_SIG_ID_BEGIN_FLAG] = {0};

EEP_MSG eepMessage = {0};
U32 g_32InitParaErrCnt = 0;

static void SetSignalValBuf(U32 u32SigId, S32 s32SigVal)
{
	S32 s32MaxValue = 0;
	S32 s32MinValue = 0;

	//寄存器表（参数）
	if(u32SigId > MBMS_SET_SIG_ID_BEGIN_FLAG
		&& u32SigId < MBMS_SET_SIG_ID_END_FLAG)
	{
		s32MaxValue = MBMSetSignalArry[u32SigId - MBMS_SET_SIG_ID_BEGIN_FLAG].m_u32MaxVal;
		s32MaxValue = s32MaxValue + MBMSetSignalArry[u32SigId - MBMS_SET_SIG_ID_BEGIN_FLAG].m_s16OffSet;
		s32MinValue = MBMSetSignalArry[u32SigId - MBMS_SET_SIG_ID_BEGIN_FLAG].m_u32MinVal;
		s32MinValue = s32MinValue + MBMSetSignalArry[u32SigId - MBMS_SET_SIG_ID_BEGIN_FLAG].m_s16OffSet;

		//参数设置范围判断
		if(s32SigVal > s32MaxValue
			|| s32SigVal < s32MinValue)
		{
			return;
		}

		//参数值一样返回成功
		if(s32SigVal != s32MbmsSet[u32SigId - MBMS_SET_SIG_ID_BEGIN_FLAG])
		{
			s32MbmsSet[u32SigId - MBMS_SET_SIG_ID_BEGIN_FLAG] = s32SigVal;
		}
	}
}


static BOOL EepromSelfCheck(void)
{
	U8 u8EECheckCnt = 0;
	S32 s32EEPROMRead = 0,s32EEPROMCheck = 0;
	BOOL ret = FALSE;

	//EEPROM自检
	vTaskDelay(10);
	if(ExtEepromReadParameter(MBMS_SET_SIG_ID_EXT_EEPROM_TEST_VAL, &s32EEPROMCheck) != TRUE)
	{
		return FALSE;
	}
	vTaskDelay(10);
	if(ExtEepromSaveParameter(MBMS_SET_SIG_ID_EXT_EEPROM_TEST_VAL, s32EEPROMCheck+1) != TRUE)
	{
		return FALSE;
	}
	vTaskDelay(10);
	if(ExtEepromReadParameter(MBMS_SET_SIG_ID_EXT_EEPROM_TEST_VAL, &s32EEPROMRead) != TRUE)
	{
		return FALSE;
	}
	if(s32EEPROMRead == (s32EEPROMCheck+1))
	{
		u8EECheckCnt++;
	}

	if(ExtEepromReadParameter(MBMS_SET_SIG_ID_EXT_EEPROM_TEST_HIGH_FREQ_VAL, &s32EEPROMCheck) != TRUE)
	{
		return FALSE;
	}
	vTaskDelay(10);
	if(ExtEepromSaveParameter(MBMS_SET_SIG_ID_EXT_EEPROM_TEST_HIGH_FREQ_VAL, s32EEPROMCheck+1) != TRUE)
	{
		return FALSE;
	}
	vTaskDelay(10);
	if(ExtEepromReadParameter(MBMS_SET_SIG_ID_EXT_EEPROM_TEST_HIGH_FREQ_VAL, &s32EEPROMRead) != TRUE)
	{
		return FALSE;
	}
	if(s32EEPROMRead == (s32EEPROMCheck+1))
	{
		u8EECheckCnt++;
	}	
	
	if(u8EECheckCnt == 0)
	{
		ret = FALSE;
	}
	else
	{
		ret = TRUE;
	}

	return ret;
}

static BOOL SaveSignal2EepromFunc(U32 u32SigId, S32 s32SigVal)
{
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
		
	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

	(void)EepromSelfCheck();

    for( ;; )
    {
		if(NULL != g_ExtEeprom_xQueue)
		{
			err = xQueueReceive(g_ExtEeprom_xQueue, (void *)&ptMsg, (TickType_t)portMAX_DELAY);
            if(pdTRUE == err)
            {
				if(TRUE == ExtEepromSaveParameter(ptMsg.u32SigId, ptMsg.s32SigVal))
				{
					s32MbmsSet[ptMsg.u32SigId - MBMS_SET_SIG_ID_BEGIN_FLAG] = ptMsg.s32SigVal;
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

//参数设置接口
bl SetSigVal(U32 u32SigId, S32 SignalValue)
{
    S32 s32MaxValue = 0;
    S32 s32MinValue = 0;
    S32 s32GetSigVal = 0;
	U8 i = 0;

	//寄存器表（参数）
	if(u32SigId > MBMS_SET_SIG_ID_BEGIN_FLAG
		&& u32SigId < MBMS_SET_SIG_ID_END_FLAG)
	{
        s32MaxValue = MBMSetSignalArry[u32SigId - MBMS_SET_SIG_ID_BEGIN_FLAG].m_u32MaxVal;
        s32MaxValue = s32MaxValue + MBMSetSignalArry[u32SigId - MBMS_SET_SIG_ID_BEGIN_FLAG].m_s16OffSet;
        s32MinValue = MBMSetSignalArry[u32SigId - MBMS_SET_SIG_ID_BEGIN_FLAG].m_u32MinVal;
        s32MinValue = s32MinValue + MBMSetSignalArry[u32SigId - MBMS_SET_SIG_ID_BEGIN_FLAG].m_s16OffSet;

        //参数设置范围判断
		if(SignalValue > s32MaxValue
		|| SignalValue < s32MinValue)
		{
            return (FALSE);
		}
		
		//参数未初始化
		if (FALSE == g_blParaInitFinishFgBuf[u32SigId - MBMS_SET_SIG_ID_BEGIN_FLAG])
		{
			for(i = 0; i < 3; i++)
			{
				if(TRUE == ExtEepromReadParameter(u32SigId, &s32GetSigVal))
				{
					SetSignalValBuf(u32SigId, s32GetSigVal);
					break;
				}
			}
			
			if(SignalValue != s32MbmsSet[u32SigId - MBMS_SET_SIG_ID_BEGIN_FLAG])
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
			
			g_blParaInitFinishFgBuf[u32SigId - MBMS_SET_SIG_ID_BEGIN_FLAG] = TRUE;
			
			return (TRUE);	
		}

		//参数值一样返回成功
		if(SignalValue != s32MbmsSet[u32SigId - MBMS_SET_SIG_ID_BEGIN_FLAG])
		{
			if(TRUE == SaveSignal2EepromFunc(u32SigId, SignalValue))
			{
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
	else if(u32SigId > MBMS_SAM_SIG_ID_BEGIN_FLAG
		&& u32SigId < MBMS_SAM_SIG_ID_END_FLAG)
	{
		if(SignalValue != s32MbmsRam[u32SigId - MBMS_SAM_SIG_ID_BEGIN_FLAG])
		{
			s32MbmsRam[u32SigId - MBMS_SAM_SIG_ID_BEGIN_FLAG] = SignalValue;
		}
	}
	//开关量表（常规告警、特殊告警）
	else if(u32SigId > ALARM_ID_EVENT_BEGIN_FLAG
		&& u32SigId < ALARM_ID_EVENT_END_FLAG)
	{
		if(SignalValue != s32MbmsAlarmRam[u32SigId - ALARM_ID_EVENT_BEGIN_FLAG])
		{
			s32MbmsAlarmRam[u32SigId - ALARM_ID_EVENT_BEGIN_FLAG] = SignalValue;
		}
	}
	//开关量表（信号状态）
	else if(u32SigId > SIGNAL_STATUS_BEGIN_FLAG
		&& u32SigId < SIGNAL_STATUS_END_FLAG)
	{
		if(SignalValue != s32MbmsTotalAlarmRam[u32SigId - SIGNAL_STATUS_BEGIN_FLAG])
		{
			s32MbmsTotalAlarmRam[u32SigId - SIGNAL_STATUS_BEGIN_FLAG] = SignalValue;
		}
	}
	else
	{
		return (FALSE);
	}

	return (TRUE);
}

//参数设置接口
bl GetSigVal(U32 u32SigId,S32 *SignalValue)
{
    S32 s32GetSigVal = 0;
	U8 i = 0;

	//非法地址
	if(NULL == SignalValue
		|| (void*)0 == SignalValue
		|| 0 == SignalValue)
	{
		return (FALSE);
	}
	
	//寄存器表（参数）
	if(u32SigId > MBMS_SET_SIG_ID_BEGIN_FLAG
		&& u32SigId < MBMS_SET_SIG_ID_END_FLAG)
	{
		//参数未初始化
		if (FALSE == g_blParaInitFinishFgBuf[u32SigId - MBMS_SET_SIG_ID_BEGIN_FLAG])
		{
			for(i = 0; i < 3; i++)
			{
				if(TRUE == ExtEepromReadParameter(u32SigId, &s32GetSigVal))
				{
					SetSignalValBuf(u32SigId, s32GetSigVal);
					break;
				}
			}
			
			g_blParaInitFinishFgBuf[u32SigId - MBMS_SET_SIG_ID_BEGIN_FLAG] = TRUE;
		}

		*SignalValue = s32MbmsSet[u32SigId - MBMS_SET_SIG_ID_BEGIN_FLAG];
	}
	//寄存器表（实时寄存器数据）
	else if(u32SigId > MBMS_SAM_SIG_ID_BEGIN_FLAG
		&& u32SigId < MBMS_SAM_SIG_ID_END_FLAG)
	{
		*SignalValue = s32MbmsRam[u32SigId - MBMS_SAM_SIG_ID_BEGIN_FLAG];
	}
	//开关量表（常规告警、特殊告警）
	else if(u32SigId > ALARM_ID_EVENT_BEGIN_FLAG
		&& u32SigId < ALARM_ID_EVENT_END_FLAG)
	{
		*SignalValue = s32MbmsAlarmRam[u32SigId - ALARM_ID_EVENT_BEGIN_FLAG];
	}
	//开关量表（信号状态）
	else if(u32SigId > SIGNAL_STATUS_BEGIN_FLAG
		&& u32SigId < SIGNAL_STATUS_END_FLAG)
	{
		*SignalValue = s32MbmsTotalAlarmRam[u32SigId - SIGNAL_STATUS_BEGIN_FLAG];
	}
	else
	{
		return (FALSE);
	}

	return (TRUE);
}


S32 GetMsgVal(U32 u32SigId)
{
	S32 s32SigVal = 0;
	if(TRUE == GetSigVal(u32SigId, &s32SigVal))
	{
		return s32SigVal;
	}
	return FALSE;
}


static U32 u32EepNoChangeCnt = 0;
/// @brief 上电从eeprom中加载set sig参数
/// @param  
static void LoadEepromParameter(void)   
{
	U32 u32ParaCnt = 0;
	S32 s32GetSigVal = 0;

	for (u32ParaCnt = MBMS_SET_SIG_ID_BEGIN_FLAG; u32ParaCnt < MBMS_SET_SIG_ID_END_FLAG; u32ParaCnt++)
	{
		if((TRUE == ExtEepromReadParameter(u32ParaCnt, &s32GetSigVal)))
		{
			SetSignalValBuf(u32ParaCnt, s32GetSigVal);
		}
        else
        {
            u32EepNoChangeCnt++;
        }

		g_blParaInitFinishFgBuf[u32ParaCnt - MBMS_SET_SIG_ID_BEGIN_FLAG] = TRUE;
	}
}


//系统参数初始化
void InitSigVal(void)
{
	g_ExtEeprom_xQueue = xQueueCreate(10, sizeof(EEP_MSG)); 

	LoadEepromParameter();   //上电加载eeprom（set signal）参数
}




// void SetSignalValInit_Task(void * pvParameters)
// {
// 	// U8 u8EECheckCnt = 0;
// 	// S32 s32BoardNum = 0,s32SigVal = 0;
// 	// S32 s32ResetNum = 0,s32EEPROMCheck = 0;
	
// 	// (void)pvParameters;

// 	// //延时20ms，等待写eeprom的任务完成创建
// 	// vTaskDelay(200);

// 	// (void)SetSigVal(MBMS_SAM_SIG_ID_RGB_SHUTDOWN_ENABLE, GetMsgVal(MBMS_SET_SIG_ID_RGB_SHUTDOWN_ENABLE));
// 	// ReadAlarmRecordFromEEprom();
// 	// (void)SetSigVal(MBMS_SAM_SIG_ID_CELL_LOW_VOLT_LEV_2, GetMsgVal(MBMS_SET_SIG_ID_CELL_LOW_VOLT_LEV_2));
// 	// (void)SetSigVal(MBMS_SAM_SIG_ID_CELL_LOW_VOLT_LEV_2_REC_VAL, GetMsgVal(MBMS_SET_SIG_ID_CELL_LOW_VOLT_LEV_2_REC_VAL));

// 	// //电池系统
// 	// #if (BMU_SERIES_CONFIG == BMU_SERIES_12S)
// 	// (void)SetSigVal(MBMS_SAM_SIG_ID_BAT_SERIAL_NUM, BMU_SERIES_12S);
// 	// (void)SetSigVal(MBMS_SAM_SIG_ID_TEMP_SERIAL_NUM, 15);
// 	// #endif
// 	// //电池额定总电压
//     // (void)GetSigVal(MBMS_SAM_SIG_ID_BAT_SERIAL_NUM,&s32SigVal);
//     // s32SigVal *= 32;   //0.1V unit
//     // s32SigVal *= GetMsgVal(MBMS_SET_SIG_ID_BMU_NUM);
//     // (void)SetSigVal(MBMS_SAM_SIG_ID_BAT_RATE_VOLT,s32SigVal);

// 	// //记录复位次数
// 	// (void)GetSigVal(MBMS_SET_SIG_ID_RECORD_RESET_NUM, &s32ResetNum);
// 	// s32ResetNum++;

// 	// //EEPROM自检
// 	// (void)SetSigVal(MBMS_SET_SIG_ID_RECORD_RESET_NUM, s32ResetNum);
// 	// vTaskDelay(500);
// 	// (void)GetSigVal(MBMS_SET_SIG_ID_RECORD_RESET_NUM, &s32EEPROMCheck);
// 	// if(s32ResetNum == s32EEPROMCheck)
// 	// {
// 	// 	u8EECheckCnt++;
// 	// }
// 	// (void)SetSigVal(MBMS_SET_SIG_ID_EXT_EEPROM_TEST_VAL, s32ResetNum);
// 	// vTaskDelay(500);
// 	// (void)GetSigVal(MBMS_SET_SIG_ID_EXT_EEPROM_TEST_VAL, &s32EEPROMCheck);
// 	// if(s32ResetNum == s32EEPROMCheck)
// 	// {
// 	// 	u8EECheckCnt++;
// 	// }
// 	// (void)SetSigVal(MBMS_SET_SIG_ID_EXT_EEPROM_TEST_HIGH_FREQ_VAL, s32ResetNum);
// 	// vTaskDelay(500);
// 	// (void)GetSigVal(MBMS_SET_SIG_ID_EXT_EEPROM_TEST_HIGH_FREQ_VAL, &s32EEPROMCheck);
// 	// if(s32ResetNum == s32EEPROMCheck)
// 	// {
// 	// 	u8EECheckCnt++;
// 	// }
	
// 	// if(u8EECheckCnt == 0)
// 	// {
// 	// 	(void)SetSigVal(ALARM_ID_EEPROM_ERROR,EVENT_HAPPEN);
// 	// }

// 	vTaskDelete(NULL);
// }








