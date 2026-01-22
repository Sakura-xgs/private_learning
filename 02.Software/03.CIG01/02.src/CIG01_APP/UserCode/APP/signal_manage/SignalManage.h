#ifndef  SIGNALMANAGE_H_
#define  SIGNALMANAGE_H_
#include "PublicDefine.h"
#include "MbmsSignal.h"
#include "MbmsSignal_IF.h"
#include "MbmsSignal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"




void InitSigVal(void);
// void SetSignalValInit_Task(void * pvParameters);
void Save_Eeprom_Task(void * pvParameters);


bl SetSigVal(U32 u32SigId, S32 SignalValue);
bl GetSigVal(U32 u32SigId,S32 *SignalValue);
S32 GetMsgVal(U32 u32SigId);




#endif
