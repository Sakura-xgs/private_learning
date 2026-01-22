/*
 * data_sample.h
 *
 *  Created on: 2024年12月13日
 *      Author: Bono
 */

#ifndef APP_DATA_SAMPLE_DATA_SAMPLE_H_
#define APP_DATA_SAMPLE_DATA_SAMPLE_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "PublicDefine.h"

#define DATASAMPLE_TASK_PERIOD_10MS	(10)
#define TIME_2S						(2000/DATASAMPLE_TASK_PERIOD_10MS)

typedef struct
{
	U32 u32SingalId;
	GPIO_Type *base;
	uint32_t pin;
}relay_input_t;

typedef struct
{
	U32 u32AdhAlarmId;
	U32 u32DrvFailAlarmId;
	U32 u32CtrlStSingalId;
	U32 u32DiFbSingalId;
	U32 u32HappenVal;
	U32 u32CancelVal;
}relay_alarm_t;

typedef struct
{
	U32 u32AdValue;
	U32 u32SingalId;
	U16 (*func)(U16 AdValue);
}tmp_sample_t;


extern tmp_sample_t TmpMap[];

extern void DataSample_Init_task(void * pvParameters);
extern void PduSoftReset(void);

#endif /* APP_DATA_SAMPLE_DATA_SAMPLE_H_ */
