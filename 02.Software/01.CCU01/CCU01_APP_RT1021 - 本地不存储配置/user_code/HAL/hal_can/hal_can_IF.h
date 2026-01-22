/*
 * hal_can_IF.h
 *
 *  Created on: 2024年8月23日
 *      Author: Bono
 */

#ifndef HAL_HAL_CAN_HAL_CAN_IF_H_
#define HAL_HAL_CAN_HAL_CAN_IF_H_

extern QueueHandle_t g_Can1_RecData_xQueue;
extern QueueHandle_t g_Can2_RecData_xQueue;

extern void Hal_Can_Init(void);

#endif /* HAL_HAL_CAN_HAL_CAN_IF_H_ */
