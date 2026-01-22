/*
 * SignalManage.h
 *
 *  Created on: 2024年8月19日
 *      Author: Bono
 */

#ifndef HAL_SIGNALMANAGE_SIGNALMANAGE_H_
#define HAL_SIGNALMANAGE_SIGNALMANAGE_H_

#include "PublicDefine.h"
#include "Signal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


#define SIG_BIT			(32U)
#define ALARM_SIG_NUM	(((ALARM_ID_PILE_END_FLAG - ALARM_ID_PILE_BEGIN_FLAG)/SIG_BIT) + 1U)
#define BOOL_SIG_NUM	(((SIGNAL_STATUS_END_FLAG - SIGNAL_STATUS_BEGIN_FLAG)/SIG_BIT) + 1U)

extern BOOL g_blExtEepromErrFg;	//禁止递归调用，EEPROM故障此处不能使用信号表信号
extern SemaphoreHandle_t g_CtlExtEeprom_MutexSemaphore;

extern void InitSigVal(void);
extern BOOL SetSigVal(U32 u32SigId, S32 SignalValue);
extern BOOL GetSigVal(U32 u32SigId, S32 *SignalValue);
extern S32 GetMsgVal(U32 u32SigId);
extern void SetSignalValInit_Task(void * pvParameters);
extern void Save_Eeprom_Task(void * pvParameters);
extern void EepromDelay(void);

#endif /* HAL_SIGNALMANAGE_SIGNALMANAGE_H_ */
