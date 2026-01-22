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


#define SIG_BIT			(32)
#define ALARM_SIG_NUM	(((ALARM_ID_EVENT_END_FLAG - ALARM_ID_EVENT_BEGIN_FLAG)/SIG_BIT) + 1)
#define BOOL_SIG_NUM	(((SIGNAL_STATUS_END_FLAG - SIGNAL_STATUS_BEGIN_FLAG)/SIG_BIT) + 1)


extern SemaphoreHandle_t g_CtlExtEeprom_MutexSemaphore;
extern QueueHandle_t g_ExtEeprom_xQueue;
extern BOOL g_blParaInitFinishFgBuf[PDU_SET_SIG_ID_END_FLAG - PDU_SET_SIG_ID_BEGIN_FLAG];
extern S32 s32RelayCtrlTimes[MAX_RELAY_NUM];

extern void InitSigVal(void);
extern bl SetSigVal(U32 u32SigId, S32 SignalValue);
extern bl GetSigVal(U32 u32SigId,S32 *SignalValue);
extern S32 GetMsgVal(U32 u32SigId);
extern void SomeSigValInit(void);
extern void SetSignalValBuf(U32 u32SigId, S32 s32SigVal);
extern void SetSignalValInit_Task(void * pvParameters);
extern void Save_Eeprom_Task(void * pvParameters);
extern BOOL SaveSignal2EepromFunc(U32 u32SigId, S32 s32SigVal);

#endif /* HAL_SIGNALMANAGE_SIGNALMANAGE_H_ */
