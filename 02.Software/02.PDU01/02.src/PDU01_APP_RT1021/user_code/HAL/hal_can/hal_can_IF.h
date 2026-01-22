/*
 * hal_can_IF.h
 *
 *  Created on: 2024年8月23日
 *      Author: Bono
 */

#ifndef HAL_HAL_CAN_HAL_CAN_IF_H_
#define HAL_HAL_CAN_HAL_CAN_IF_H_

#include "PublicDefine.h"


extern SemaphoreHandle_t g_CanUpdata_Binary_Semaphore;
extern QueueHandle_t g_Can1_RecData_xQueue;
extern QueueHandle_t g_Can2_RecData_xQueue;
extern SemaphoreHandle_t g_Can1_SendData_MutexSemaphore;
extern SemaphoreHandle_t g_Can2_SendData_MutexSemaphore;

extern void Hal_Can_Init(void);
extern status_t PduCanSendFrame(flexcan_frame_t *frame);
extern void CAN_8_DATA_COPY(const U8 data[8], flexcan_frame_t *Frame);

#endif /* HAL_HAL_CAN_HAL_CAN_IF_H_ */
