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

#define CAN1_SEND_MB_BEG_NO	(1U)
#define CAN1_SEND_MB_END_NO	(15U)

#define CAN1_REC_MB_BEG_NO	(16U)
#define CAN1_REC_MB_END_NO	(31U)
#define CAN1_REC_MB_NUM		(CAN1_REC_MB_END_NO - CAN1_REC_MB_BEG_NO + 1U)

#define CAN2_SEND_MB_BEG_NO	(32U)
#define CAN2_SEND_MB_END_NO	(47U)

#define CAN2_REC_MB_BEG_NO	(48U)
#define CAN2_REC_MB_END_NO	(63U)
#define CAN2_REC_MB_NUM		(CAN2_REC_MB_END_NO - CAN2_REC_MB_BEG_NO + 1U)


/*******************************************************************************
 * Prototypes
 ******************************************************************************/
status_t SendCANData(CAN_Type *base, flexcan_frame_t *frame);
extern status_t ChgACanSendFrame(flexcan_frame_t *frame);
extern status_t ChgBCanSendFrame(flexcan_frame_t *frame);

#endif /* HAL_HAL_CAN_HAL_CAN_H_ */
