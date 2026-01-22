/*
 * hal_can.h
 *
 *  Created on: 2024年8月23日
 *      Author: Bono
 */

#ifndef HAL_HAL_CAN_HAL_CAN_H_
#define HAL_HAL_CAN_HAL_CAN_H_

#include "fsl_flexcan.h"



/*CAN相关宏定义*/
#define CHARGE_A_CAN (CAN1)								//定义使用的CAN
#define CHARGE_B_CAN (CAN2)								//定义使用的CAN

#define CHARGE_A_CAN_QUEUE	(g_Can1_RecData_xQueue)
#define CHARGE_B_CAN_QUEUE	(g_Can2_RecData_xQueue)

#define CAN1_SEND_MB_BEG_NO	(1)
#define CAN1_SEND_MB_END_NO	(15)

#define CAN1_REC_MB_BEG_NO	(16)
#define CAN1_REC_MB_END_NO	(31)
#define CAN1_REC_MB_NUM		(CAN1_REC_MB_END_NO - CAN1_REC_MB_BEG_NO + 1)

#define CAN2_SEND_MB_BEG_NO	(32)
#define CAN2_SEND_MB_END_NO	(47)

#define CAN2_REC_MB_BEG_NO	(48)
#define CAN2_REC_MB_END_NO	(63)
#define CAN2_REC_MB_NUM		(CAN2_REC_MB_END_NO - CAN2_REC_MB_BEG_NO + 1)


/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void hal_can_init(void);
status_t SendCANData(CAN_Type *base, flexcan_frame_t *frame);
status_t ChgACanSendFrame(flexcan_frame_t *frame);
status_t ChgBCanSendFrame(flexcan_frame_t *frame);

#endif /* HAL_HAL_CAN_HAL_CAN_H_ */
