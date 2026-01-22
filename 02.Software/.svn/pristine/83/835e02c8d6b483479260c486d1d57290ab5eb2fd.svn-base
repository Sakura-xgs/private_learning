/*
 * datasample.h
 *
 *  Created on: 2024年11月4日
 *      Author: Bono
 */

#ifndef APP_DATASAMPLE_DATASAMPLE_H_
#define APP_DATASAMPLE_DATASAMPLE_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "PublicDefine.h"
#include "semphr.h"

#define TEMPER_NUM 12U

typedef struct
{
	U32 u32AdValue;
	U32 u32SingalId;
	U16 (*func)(U16 AdValue);
}tmp_sample_t;

extern tmp_sample_t TmpMap[TEMPER_NUM];
extern U8 g_cTemperatureHMI[2];

void DataSample_Init_task(void * pvParameters);

#endif /* APP_DATASAMPLE_DATASAMPLE_H_ */
