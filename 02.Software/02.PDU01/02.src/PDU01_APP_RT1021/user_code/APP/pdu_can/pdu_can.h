/*
 * charge_can.h
 *
 *  Created on: 2024年8月26日
 *      Author: Bono
 */

#ifndef APP_PDU_CAN_PDU_CAN_H_
#define APP_PDU_CAN_PDU_CAN_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "PublicDefine.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define PDU_CAN_COMM_TASK_PRIO		(4)

#define PDU_CAN_10MS_TIMER			(5)
#define PDU_CAN_50MS_TIMER			(PDU_CAN_10MS_TIMER*5)
#define PDU_CAN_250MS_TIMER			(PDU_CAN_10MS_TIMER*25)
#define PDU_CAN_500MS_TIMER			(PDU_CAN_10MS_TIMER*50)
#define PDU_CAN_1S_TIMER			(PDU_CAN_500MS_TIMER*2)
#define PDU_CAN_9S_TIMER			(PDU_CAN_1S_TIMER*9)
#define ADDR_REPEAT_10S_TIMER		(100)
#define PCU_OFFLINE_3S_TIMER		(30)
#define PDU_INFO_CAN_10S			(10000/100)

#define GCAN_FRAME_LEN				(8)


#define PDU_MSG_1501				(0x1501)
#define PDU_MSG_1502				(0x1502)
#define PDU_MSG_1401				(0x1401)
#define PDU_MSG_1301				(0x1301)
#define PDU_MSG_1302				(0x1302)
#define PDU_MSG_13A0				(0x13A0)
#define PDU_MSG_13A1				(0x13A1)

#define PDU_MSG_1201				(0x1201)
#define PDU_MSG_1202				(0x1202)
#define PDU_MSG_1203				(0x1203)
#define PDU_MSG_1204				(0x1204)
#define PDU_MSG_1205				(0x1205)
#define PDU_MSG_1101				(0x1101)

#define FACTORY_TEST_FUNCID			(0x1001)
#define RELAY_CTRL_FUNCID			(0x1601)
#define LED_SYNC_FUNCID				(0x1602)
#define SN_READ_FUNCID				(0x0901)
#define SN_WRITE_FUNCID				(0x0902)
#define REMOTE_CTRL_FUNCID			(0x0903)

#define PCU_ADDR_ID					(0x80)
#define BOARDCAST_ADDR				(0xFF)

/* PCU Command */
typedef enum
{
	PCU_CMD_CLEAR_HISTORY = 0xAA,
	PCU_CMD_SOFT_RESET = 0xAB,
}PCU_COMMAND_ENUM;

typedef struct
{
	U32	u32MsgId;			// 消息ID
	U16	u16MsgPeriod;		// 消息发送周期
	U8	u8Flag;				// 使能标识
	U16 u16MsgTimer;          //计时器
	void (*Func)(const U32);// 函数指针
} PDU_MSG_CAN_ST;

void FillPduMsFrame_1501(U32 u32MsgId);
void FillPduMsFrame_1502(U32 u32MsgId);
void FillPduMsFrame_1401(U32 u32MsgId);
void FillPduMsFrame_1301(U32 u32MsgId);
void FillPduMsFrame_1302(U32 u32MsgId);
void FillPduMsFrame_13A0(U32 u32MsgId);
void FillPduMsFrame_13A1(U32 u32MsgId);

extern void Pdu_Can_Comm_Init_task(void * pvParameters);
extern void SuspendPduCanSendTaskFunc(void);
extern void ResumePduCanSendTaskFunc(void);

extern void LedSyncMsgSend(U16 u16FuncId, BOOL ret);

#endif /* APP_PDU_CAN_PDU_CAN_H_ */
