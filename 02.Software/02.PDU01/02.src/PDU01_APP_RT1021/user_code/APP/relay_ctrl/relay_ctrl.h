/*
 * relay_ctrl.h
 *
 *  Created on: 2024年12月5日
 *      Author: Bono
 */

#ifndef APP_RELAY_CTRL_RELAY_CTRL_H_
#define APP_RELAY_CTRL_RELAY_CTRL_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "fsl_flexcan.h"
#include "PublicDefine.h"


#define RELAY_CMD_ARRAY_SIZE		(8)
#define RELAY_CTRL_TIME_OUT_VAL		(100)			//100毫秒超时


extern void Relay_Ctrl_Init_task(void * pvParameters);
extern BOOL IsAllRelayCutOff(void);
extern void CutOffAllRelaysCmd(void);
extern BOOL ParseRelayCtrlMsg(flexcan_frame_t *Frame);

#endif /* APP_RELAY_CTRL_RELAY_CTRL_H_ */
