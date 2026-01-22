/*
 * charge_can.c
 *
 *  Created on: 2024年8月26日
 *      Author: Bono
 */
#include <APP/pdu_can/pdu_can.h>
#include <APP/relay_ctrl/relay_ctrl.h>
#include <APP/data_sample/data_sample.h>
#include <HAL/factory_test/factory_test.h>
#include "fsl_flexcan.h"

#include "hal_can_IF.h"
#include "hal_sys_IF.h"
#include "SignalManage.h"
#include "PublicDefine.h"
#include "boot.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/
TaskHandle_t PduCanMonitorTask_Handler = NULL;
TaskHandle_t PduCanParseTask_Handler = NULL;
TaskHandle_t PduCanSendTask_Handler = NULL;

U16 g_u16PcuOfflineCnt = 0;
U16 u16AddrRepeatClearCnt = 0;
static BOOL g_bFirstRelayCmdTriggered  = TRUE;							// 首次收到继电器控制指令是立即发送版本信息标志

/*******************************************************************************
 * Tables
 ******************************************************************************/
PDU_MSG_CAN_ST g_PduMsgCanMap[] =
{
	{PDU_MSG_1501,		PDU_CAN_250MS_TIMER,	 	TRUE,		0	, FillPduMsFrame_1501},
	{PDU_MSG_1502,		PDU_CAN_250MS_TIMER,	 	TRUE,		0	, FillPduMsFrame_1502},
	{PDU_MSG_1401,		PDU_CAN_1S_TIMER,	 		TRUE,		0	, FillPduMsFrame_1401},
	{PDU_MSG_1301,		PDU_CAN_500MS_TIMER,	 	TRUE,		0	, FillPduMsFrame_1301},
	{PDU_MSG_1302,		PDU_CAN_500MS_TIMER,	 	TRUE,		0	, FillPduMsFrame_1302},
	{PDU_MSG_13A0,		PDU_CAN_500MS_TIMER,	 	TRUE,		0	, FillPduMsFrame_13A0},
	{PDU_MSG_13A1,		PDU_CAN_1S_TIMER,			TRUE,		0	, FillPduMsFrame_13A1},
};
#define PDU_MSG_CAN_TABLE_SIZE	(sizeof(g_PduMsgCanMap)/sizeof(PDU_MSG_CAN_ST))

#define MONITOR_TASK_TIMER			(100)								// 监测任务延时周期，单位ms
#define LED_SYNC_30S_TIMER			(30000 / MONITOR_TASK_TIMER) 		// LED首次同频时间30S
#define LED_SYNC_3600S_TIMER		(3600000 / MONITOR_TASK_TIMER) 		// LED同频监测周期3600S
/*******************************************************************************
 * Code
 ******************************************************************************/
void FillPduMsFrame_1501(U32 u32MsgId)
{
	flexcan_frame_t Frame = {0};
	S32 s32PduId = 0;
	S32 s32SigVal = 0;
	U8 data[8] = {0};
	U8 i = 0;

	//获取板号，填充CAN_ID
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);

	for(i = 0; i < MAX_RELAY_NUM; i++)
	{
		(void)GetSigVal(SIGNAL_STATUS_K1_FB+i, &s32SigVal);
		if(s32SigVal == TRUE)
		{
			data[i/8] |= TRUE << (i%8);
		}
	}

	CAN_8_DATA_COPY(data, &Frame);

	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT((u32MsgId<<16) | (PCU_ADDR_ID<<8) | (s32PduId));
	Frame.length = GCAN_FRAME_LEN;

	PduCanSendFrame(&Frame);
}

void FillPduMsFrame_1502(U32 u32MsgId)
{
	flexcan_frame_t Frame = {0};
	S32 s32PduId = 0;
	S32 s32SigVal = 0;
	U8 data[8] = {0};
	U8 i = 0;

	//获取板号，填充CAN_ID
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);

	for(i = 0; i < MAX_RELAY_NUM; i++)
	{
		(void)GetSigVal(SIGNAL_STATUS_K1+i, &s32SigVal);
		if(s32SigVal == TRUE)
		{
			data[i/8] |= TRUE << (i%8);
		}
	}

	CAN_8_DATA_COPY(data, &Frame);
	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT((u32MsgId<<16) | (PCU_ADDR_ID<<8) | (s32PduId));
	Frame.length = GCAN_FRAME_LEN;

	PduCanSendFrame(&Frame);
}

void FillPduMsFrame_1401(U32 u32MsgId)
{
	flexcan_frame_t Frame = {0};
	S32 s32PduId = 0;
	S32 s32SigVal = 0;
	U8 data[8] = {0};
	U8 i = 0;

	//获取板号，填充CAN_ID
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);

	(void)GetSigVal(PDU_SAM_SIG_ID_PCB_TEMP_1, &s32SigVal);
	data[0] = (s32SigVal > 255) ? 255 : ((s32SigVal < 0) ? 0 : (U8)s32SigVal);
	(void)GetSigVal(PDU_SAM_SIG_ID_PCB_TEMP_2, &s32SigVal);
	data[1] = (s32SigVal > 255) ? 255 : ((s32SigVal < 0) ? 0 : (U8)s32SigVal);

	(void)GetSigVal(PDU_SAM_SIG_ID_DC_POS_TEMP, &s32SigVal);
	data[4] = (s32SigVal > 255) ? 255 : ((s32SigVal < 0) ? 0 : (U8)s32SigVal);
	(void)GetSigVal(PDU_SAM_SIG_ID_DC_NEG_TEMP, &s32SigVal); 
	data[5] = (s32SigVal > 255) ? 255 : ((s32SigVal < 0) ? 0 : (U8)s32SigVal);

	CAN_8_DATA_COPY(data, &Frame);
	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT((u32MsgId<<16) | (PCU_ADDR_ID<<8) | (s32PduId));
	Frame.length = GCAN_FRAME_LEN;

	PduCanSendFrame(&Frame);
}

void FillPduMsFrame_1301(U32 u32MsgId)
{
	flexcan_frame_t Frame = {0};
	S32 s32PduId = 0;
	S32 s32SigVal = 0;
	U8 data[8] = {0};
	U8 i = 0;

	//获取板号，填充CAN_ID
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);

	for(i = 0; i < MAX_RELAY_NUM; i++)
	{
		(void)GetSigVal(ALARM_ID_K1_ADHESION+i, &s32SigVal);
		if(s32SigVal == TRUE)
		{
			data[i/8] |= TRUE << (i%8);
		}
	}

	CAN_8_DATA_COPY(data, &Frame);
	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT((u32MsgId<<16) | (PCU_ADDR_ID<<8) | (s32PduId));
	Frame.length = GCAN_FRAME_LEN;

	PduCanSendFrame(&Frame);
}

void FillPduMsFrame_1302(U32 u32MsgId)
{
	flexcan_frame_t Frame = {0};
	S32 s32PduId = 0;
	S32 s32SigVal = 0;
	U8 data[8] = {0};
	U8 i = 0;

	//获取板号，填充CAN_ID
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);

	for(i = 0; i < MAX_RELAY_NUM; i++)
	{
		(void)GetSigVal(ALARM_ID_K1_DRIVE_FAILED+i, &s32SigVal);
		if(s32SigVal == TRUE)
		{
			data[i/8] |= TRUE << (i%8);
		}
	}

	CAN_8_DATA_COPY(data, &Frame);
	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT((u32MsgId<<16) | (PCU_ADDR_ID<<8) | (s32PduId));
	Frame.length = GCAN_FRAME_LEN;

	PduCanSendFrame(&Frame);
}

void FillPduMsFrame_13A0(U32 u32MsgId)
{
	flexcan_frame_t Frame = {0};
	S32 s32PduId = 0;
	S32 s32SigVal = 0;
	U8 data[8] = {0};
	U8 i = 0;
	U16 base = ALARM_ID_SYS_ERROR_LEV_1;
	U16 end = ALARM_ID_SYS_ERROR_DIVIDE_SIGNAL;

	//获取板号，填充CAN_ID
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);

	for(i = 0; i < (end - base); i++)
	{
		(void)GetSigVal(base + i, &s32SigVal);
		if(s32SigVal == TRUE)
		{
			data[i/8] |= TRUE << (i%8);
		}
	}

	CAN_8_DATA_COPY(data, &Frame);
	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT((u32MsgId<<16) | (PCU_ADDR_ID<<8) | (s32PduId));
	Frame.length = GCAN_FRAME_LEN;

	PduCanSendFrame(&Frame);
}

void FillPduMsFrame_13A1(U32 u32MsgId)				//发给自身id用于检测地址重复异常，周期小于AddrRepeatMonitor
{
	flexcan_frame_t Frame = {0};
	S32 s32PduId = 0;
	U8 data[8] = {0};

	//获取板号，填充CAN_ID
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);
	CAN_8_DATA_COPY(data, &Frame);
	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT((u32MsgId<<16) | (s32PduId<<8) | (s32PduId));
	Frame.length = GCAN_FRAME_LEN;

	PduCanSendFrame(&Frame);
}

void ParseFactoryTestMsg(flexcan_frame_t *Frame)
{
	if(pdPASS == xQueueSend(g_Factory_Test_Msg_xQueue, Frame, (TickType_t)0))
	{
		return;
	}
}

void ParseLedSyncMsg(flexcan_frame_t *Frame)
{
	if((SYS_STATUS_SHUTTING_DOWN != GetSysRunModeFg()) && (FALSE == GetMsgVal(SIGNAL_STATUS_UPDATA)) && (NULL != g_LedTaskSync_Binary_Semaphore)) //倍频阶段不同频LED
	{
		if(Frame->dataByte0 == 0xAAu)
		{
			AllLedOn();
			xSemaphoreGive(g_LedTaskSync_Binary_Semaphore);
		}
		else if(Frame->dataByte0 == 0x55u)
		{
			AllLedOff();
			xSemaphoreGive(g_LedTaskSync_Binary_Semaphore);
		}
	}
}

void SoftwareInfoSend(void)
{
	flexcan_frame_t Frame = {0};
	S32 s32PduId,s32SigVal = 0;

	//获取板号，填充CAN_ID
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);
	(void)GetSigVal(PDU_SAM_SIG_ID_SW_VERSION_1_2_3_4, &s32SigVal);
	Frame.dataByte0 = GET_HIGH_4_BYTE(s32SigVal);
	Frame.dataByte1 = GET_MIDDLE_HIGH_4_BYTE(s32SigVal);
	Frame.dataByte2 = GET_MIDDLE_LOW_4_BYTE(s32SigVal);
	Frame.dataByte3 = GET_LOW_4_BYTE(s32SigVal);
	(void)GetSigVal(PDU_SAM_SIG_ID_SW_VERSION_5_6_7_8, &s32SigVal);
	Frame.dataByte4 = GET_HIGH_4_BYTE(s32SigVal);
	Frame.dataByte5 = GET_MIDDLE_HIGH_4_BYTE(s32SigVal);
	Frame.dataByte6 = GET_MIDDLE_LOW_4_BYTE(s32SigVal);
	Frame.dataByte7 = GET_LOW_4_BYTE(s32SigVal);

	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT((PDU_MSG_1201<<16) | (PCU_ADDR_ID<<8) | (s32PduId));
	Frame.length = GCAN_FRAME_LEN;
	PduCanSendFrame(&Frame);

	vTaskDelay(1);

	(void)GetSigVal(PDU_SAM_SIG_ID_SW_VERSION_9_10_11_12, &s32SigVal);
	Frame.dataByte0 = GET_HIGH_4_BYTE(s32SigVal);
	Frame.dataByte1 = GET_MIDDLE_HIGH_4_BYTE(s32SigVal);
	Frame.dataByte2 = GET_MIDDLE_LOW_4_BYTE(s32SigVal);
	Frame.dataByte3 = GET_LOW_4_BYTE(s32SigVal);
	(void)GetSigVal(PDU_SAM_SIG_ID_SW_VERSION_13_14_15_16, &s32SigVal);
	Frame.dataByte4 = GET_HIGH_4_BYTE(s32SigVal);
	Frame.dataByte5 = GET_MIDDLE_HIGH_4_BYTE(s32SigVal);
	Frame.dataByte6 = GET_MIDDLE_LOW_4_BYTE(s32SigVal);
	Frame.dataByte7 = GET_LOW_4_BYTE(s32SigVal);
	Frame.id = FLEXCAN_ID_EXT((PDU_MSG_1202<<16) | (PCU_ADDR_ID<<8) | (s32PduId));
	PduCanSendFrame(&Frame);
}

void HardwareInfoSend(void)
{
	flexcan_frame_t Frame = {0};
	S32 s32PduId,s32SigVal = 0;

	//获取板号，填充CAN_ID
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);

	(void)GetSigVal(PDU_SAM_SIG_ID_HW_VERSION_1_2_3_4, &s32SigVal);
	Frame.dataByte0 = GET_HIGH_4_BYTE(s32SigVal);
	Frame.dataByte1 = GET_MIDDLE_HIGH_4_BYTE(s32SigVal);
	Frame.dataByte2 = GET_MIDDLE_LOW_4_BYTE(s32SigVal);
	Frame.dataByte3 = GET_LOW_4_BYTE(s32SigVal);
	(void)GetSigVal(PDU_SAM_SIG_ID_HW_VERSION_5_6_7_8, &s32SigVal);
	Frame.dataByte4 = GET_HIGH_4_BYTE(s32SigVal);
	Frame.dataByte5 = GET_MIDDLE_HIGH_4_BYTE(s32SigVal);
	Frame.dataByte6 = GET_MIDDLE_LOW_4_BYTE(s32SigVal);
	Frame.dataByte7 = GET_LOW_4_BYTE(s32SigVal);

	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT((PDU_MSG_1203<<16) | (PCU_ADDR_ID<<8) | (s32PduId));
	Frame.length = GCAN_FRAME_LEN;
	PduCanSendFrame(&Frame);

	vTaskDelay(1);

	(void)GetSigVal(PDU_SAM_SIG_ID_HW_VERSION_9_10_11_12, &s32SigVal);
	Frame.dataByte0 = GET_HIGH_4_BYTE(s32SigVal);
	Frame.dataByte1 = GET_MIDDLE_HIGH_4_BYTE(s32SigVal);
	Frame.dataByte2 = GET_MIDDLE_LOW_4_BYTE(s32SigVal);
	Frame.dataByte3 = GET_LOW_4_BYTE(s32SigVal);
	(void)GetSigVal(PDU_SAM_SIG_ID_HW_VERSION_13_14_15_16, &s32SigVal);
	Frame.dataByte4 = GET_HIGH_4_BYTE(s32SigVal);
	Frame.dataByte5 = GET_MIDDLE_HIGH_4_BYTE(s32SigVal);
	Frame.dataByte6 = GET_MIDDLE_LOW_4_BYTE(s32SigVal);
	Frame.dataByte7 = GET_LOW_4_BYTE(s32SigVal);
	Frame.id = FLEXCAN_ID_EXT((PDU_MSG_1204<<16) | (PCU_ADDR_ID<<8) | (s32PduId));
	PduCanSendFrame(&Frame);
}

void Pdu_Can_Parse(flexcan_frame_t *Frame)
{
    U8 u8ScrId, u8TargetId = 0;
	U16 u16FuncId = 0;
	S32 s32PduId = 0;

	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);
	u8ScrId = (U8)((Frame->id) & 0xFF);
	u8TargetId = (U8)((Frame->id>>8) & 0xFF);
	u16FuncId = (U16)((Frame->id>>16) & 0xFFFF);

	if(u8ScrId == s32PduId)
	{
		u16AddrRepeatClearCnt = 0;
		(void)SetSigVal(ALARM_ID_PDU_ID_REPEAT, EVENT_HAPPEN);
	}

	if(((u8TargetId != s32PduId) && (u8TargetId != BOARDCAST_ADDR))
	|| (TRUE == GetMsgVal(SIGNAL_STATUS_UPDATA)))
	{
		return;
	}

	if((u16FuncId == FACTORY_TEST_FUNCID) || (u16FuncId == SN_READ_FUNCID) || (u16FuncId == SN_WRITE_FUNCID) || (u16FuncId == REMOTE_CTRL_FUNCID))	/* 耗时的任务统一放入生产测试队列处理 */
	{
		ParseFactoryTestMsg(Frame);
	}
	else if(u16FuncId == RELAY_CTRL_FUNCID)
	{
		g_u16PcuOfflineCnt = 0;
		(void)SetSigVal(ALARM_ID_PCU_OFFLINE, EVENT_CANCEL);
		(void)ParseRelayCtrlMsg(Frame);
		if(TRUE == g_bFirstRelayCmdTriggered)				// 首次收到继电器控制指令时立即发送版本信息
		{
			g_bFirstRelayCmdTriggered = FALSE;
			SoftwareInfoSend();
			HardwareInfoSend();
		}
	}
	else if(u16FuncId == LED_SYNC_FUNCID)
	{
		ParseLedSyncMsg(Frame);
	}
}

void AddrRepeatMonitor(void)
{
	if(EVENT_HAPPEN == GetMsgVal(ALARM_ID_PDU_ID_REPEAT))
	{
		u16AddrRepeatClearCnt++;
		if(u16AddrRepeatClearCnt >= ADDR_REPEAT_10S_TIMER)
		{
			u16AddrRepeatClearCnt = 0;
			(void)SetSigVal(ALARM_ID_PDU_ID_REPEAT, EVENT_CANCEL);
		}
	}
	else
	{
		u16AddrRepeatClearCnt = 0;
	}
}

void PcuOfflineMonitor(void)
{
	if(EVENT_CANCEL == GetMsgVal(ALARM_ID_PCU_OFFLINE))
	{
		g_u16PcuOfflineCnt++;
		if(g_u16PcuOfflineCnt >= PCU_OFFLINE_3S_TIMER)
		{
			g_u16PcuOfflineCnt = 0;
			(void)SetSigVal(ALARM_ID_PCU_OFFLINE, EVENT_HAPPEN);
			CutOffAllRelaysCmd();
		}
	}
	else
	{
		g_u16PcuOfflineCnt = 0;
	}
}

void PduSumMsgCanSendFunc(void)
{
	static U8 i = 0;

	if(i >= PDU_MSG_CAN_TABLE_SIZE)
	{
		i = 0;
	}

	for ( ; i < PDU_MSG_CAN_TABLE_SIZE; i++)
	{
		if((++g_PduMsgCanMap[i].u16MsgTimer >= g_PduMsgCanMap[i].u16MsgPeriod)
		&& (TRUE == g_PduMsgCanMap[i].u8Flag))
		{
			g_PduMsgCanMap[i].Func(g_PduMsgCanMap[i].u32MsgId);

			g_PduMsgCanMap[i].u16MsgTimer = 0;

			i++;

			break;
		}
		else if (g_PduMsgCanMap[i].u16MsgTimer > 65000)
		{
			g_PduMsgCanMap[i].u16MsgTimer = 65000;
		}
	}
}

void PeriodicSoftwareInfoSend(void)
{
	static U32 u32SendCnt = 0;

	u32SendCnt++;
	if(u32SendCnt >= PDU_INFO_CAN_10S)
	{
		u32SendCnt = 0;
	}
	else
	{
		return;
	}

	SoftwareInfoSend();
}

void PeriodicHardwareInfoSend(void)
{
	static U32 u32SendCnt = 0;

	u32SendCnt++;
	if(u32SendCnt >= PDU_INFO_CAN_10S)
	{
		u32SendCnt = 0;
	}
	else
	{
		return;
	}

	HardwareInfoSend();
}

void ConfigInfoSend(void)
{
	static U32 u32SendCnt = 0;

	u32SendCnt++;
	if(u32SendCnt >= PDU_INFO_CAN_10S)
	{
		u32SendCnt = 0;
	}
	else
	{
		return;
	}

	flexcan_frame_t Frame = {0};
	S32 s32PduId = 0;

	//获取板号，填充CAN_ID
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);

	Frame.dataByte0 = MAX_RELAY_NUM;

	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT((PDU_MSG_1205<<16) | (PCU_ADDR_ID<<8) | (s32PduId));
	Frame.length = GCAN_FRAME_LEN;
	PduCanSendFrame(&Frame);
}

void RelayInfoSend(void)
{
	static U32 u32SendCnt = 0;

	u32SendCnt++;
	if(u32SendCnt >= PDU_INFO_CAN_10S)
	{
		u32SendCnt = 0;
	}
	else
	{
		return;
	}

	flexcan_frame_t Frame = {0};
	S32 s32PduId = 0;
	S32 s32SigVal = 0;
	U32 u32MsgId = 0;
	U8 data[8] = {0};
	U8 i,j = 0;

	//获取板号，填充CAN_ID
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);
	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.length = GCAN_FRAME_LEN;
	u32MsgId = PDU_MSG_1101;

	for(i = 0; i < MAX_RELAY_NUM; i++)
	{
		j = (i%2)*4;
		s32SigVal = s32RelayCtrlTimes[i];
		data[j] = GET_HIGH_4_BYTE(s32SigVal);
		data[j+1] = GET_MIDDLE_HIGH_4_BYTE(s32SigVal);
		data[j+2] = GET_MIDDLE_LOW_4_BYTE(s32SigVal);
		data[j+3] = GET_LOW_4_BYTE(s32SigVal);

		if(((i + 1) % 2) == 0)
		{
			vTaskDelay(1);

			CAN_8_DATA_COPY(data, &Frame);
			Frame.id = FLEXCAN_ID_EXT((u32MsgId<<16) | (PCU_ADDR_ID<<8) | (s32PduId));
			u32MsgId++;
			PduCanSendFrame(&Frame);
		}
	}
}

void LedSyncMsgSend(U16 u16FuncId, BOOL ret)
{
	flexcan_frame_t Frame = {0};
	S32 s32PduId = 0;
	U8 data[8] = {0};

	if(TRUE == GetMsgVal(SIGNAL_STATUS_UPDATA))
	{
		return;
	}

	//获取板号，填充CAN_ID
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);

	if(ret) {
		data[0] = 0x55u;			//ret为TRUE时灯的状态熄灭
	} else {
		data[0] = 0xAAu;			//ret为FALSE时灯的状态点亮
	}

	CAN_8_DATA_COPY(data, &Frame);
	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT((u16FuncId<<16) | (BOARDCAST_ADDR<<8) | (s32PduId));
	Frame.length = GCAN_FRAME_LEN;

	PduCanSendFrame(&Frame);
}

void PduInfoCanSendMonitor(void)
{
	PeriodicSoftwareInfoSend();
	PeriodicHardwareInfoSend();
	ConfigInfoSend();
	RelayInfoSend();
}

void LedSyncMonitor(U16 *pu16Count, BOOL *pblFirstSyncDone)
{
	if((NULL == pu16Count) || (NULL == pblFirstSyncDone))
	{
		return;
	}

	(*pu16Count)++;

	if(((FALSE == *pblFirstSyncDone) && (*pu16Count >= LED_SYNC_30S_TIMER)) || (*pu16Count >= LED_SYNC_3600S_TIMER))     // 开机30s时同频一次，之后按照1个小时周期发送同频指令
    {
		*pu16Count = 0u;
		*pblFirstSyncDone = TRUE;
		LedSyncMsgSend(LED_SYNC_FUNCID, GET_USERLED_STATUS());
		xSemaphoreGive(g_LedTaskSync_Binary_Semaphore);
	}
}

void Pdu_Can_Monitor_Task(void * pvParameters)
{
    BaseType_t err = pdFALSE;
	volatile UBaseType_t uxHighWaterMark;
	U16 u16Count = 0u;
	S32 s32PduId = 0;
	BOOL blFirstSyncDone = FALSE;                                               	// 首次同频完成标志

	(void)pvParameters;

	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);

    for( ;; )
    {

		if(SYS_STATUS_NORMAL != GetSysRunModeFg())
		{
			vTaskDelete(NULL);
		}

		if(s32PduId == 1)															// 仅PDU01进行LED同频通知
		{
			LedSyncMonitor(&u16Count, &blFirstSyncDone);                            // LED同频监测
		}

		AddrRepeatMonitor();

		PcuOfflineMonitor();

		PduInfoCanSendMonitor();

		vTaskDelay(MONITOR_TASK_TIMER);

		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    }
}

void Pdu_Can_Send_Task(void * pvParameters)
{
    BaseType_t err = pdFALSE;
	volatile UBaseType_t uxHighWaterMark;

	(void)pvParameters;

	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

    for( ;; )
    {
		if(SYS_STATUS_NORMAL != GetSysRunModeFg())
		{
			vTaskDelete(NULL);
		}

    	PduSumMsgCanSendFunc();

		vTaskDelay(2);

		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    }
}

void Pdu_Can_Parse_Task(void * pvParameters)
{
    BaseType_t err = pdFALSE;
	volatile UBaseType_t uxHighWaterMark;
	flexcan_frame_t Frame = {0};

	(void)pvParameters;

	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

    for( ;; )
    {
		if(NULL != g_Can1_RecData_xQueue)
		{
			err = xQueueReceive(g_Can1_RecData_xQueue, (void *)&Frame, (TickType_t)portMAX_DELAY);

            if(pdTRUE == err)
            {
            	Pdu_Can_Parse(&Frame);
            }
		}
		else
		{
			vTaskDelay(300);
		}

		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    }
}

void SuspendPduCanSendTaskFunc(void)
{
	xSemaphoreTake(g_Can1_SendData_MutexSemaphore, portMAX_DELAY);
	xSemaphoreTake(g_Can2_SendData_MutexSemaphore, portMAX_DELAY);

	if((PduCanSendTask_Handler != NULL)
	&& (eSuspended != eTaskGetState(PduCanSendTask_Handler)))
	{
		vTaskSuspend(PduCanSendTask_Handler);
	}

	if((PduCanMonitorTask_Handler != NULL)
	&& (eSuspended != eTaskGetState(PduCanMonitorTask_Handler)))
	{
		vTaskSuspend(PduCanMonitorTask_Handler);
	}

	xSemaphoreGive(g_Can1_SendData_MutexSemaphore);
	xSemaphoreGive(g_Can2_SendData_MutexSemaphore);
}

void ResumePduCanSendTaskFunc(void)
{
	if((PduCanSendTask_Handler != NULL)
	&& (eSuspended == eTaskGetState(PduCanSendTask_Handler)))
	{
		vTaskResume(PduCanSendTask_Handler);
	}

	if((PduCanMonitorTask_Handler != NULL)
	&& (eSuspended == eTaskGetState(PduCanMonitorTask_Handler)))
	{
		vTaskResume(PduCanMonitorTask_Handler);
	}
}

void SendResetCountMsg(void)
{
    flexcan_frame_t Frame = {0};
    S32 s32PduId = 0;
	S32 s32ResetNum = 0;
    
    // 获取板号，填充CAN_ID, 填充复位次数数据
    (void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);
	(void)GetSigVal(PDU_SET_SIG_ID_RESET_TIMES, &s32ResetNum);
	Frame.dataByte0 = 0xFFu;									// 功能码：0xFF表示发送复位次数
    Frame.dataByte1 = GET_HIGH_4_BYTE(s32ResetNum);
	Frame.dataByte2 = GET_MIDDLE_HIGH_4_BYTE(s32ResetNum);
	Frame.dataByte3 = GET_MIDDLE_LOW_4_BYTE(s32ResetNum);
	Frame.dataByte4 = GET_LOW_4_BYTE(s32ResetNum);

    Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
    Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
    Frame.id = FLEXCAN_ID_EXT((FACTORY_TEST_FUNCID << 16) | (PCU_ADDR_ID << 8) | (s32PduId));
    Frame.length = GCAN_FRAME_LEN;
    
    PduCanSendFrame(&Frame);
}

void Pdu_Can_Comm_Init_task(void * pvParameters)
{
	volatile UBaseType_t uxHighWaterMark = 0;

	vTaskDelay(GetMsgVal(PDU_SAM_SIG_ID_BOARD_ID)*2);

    SendResetCountMsg();          								// 设备开机发送一帧复位次数

	taskENTER_CRITICAL();

	xTaskCreate(Pdu_Can_Monitor_Task,	"PDU_CAN_MONITOR",	configMINIMAL_STACK_SIZE,   NULL,   PDU_CAN_COMM_TASK_PRIO,		&PduCanMonitorTask_Handler);

	xTaskCreate(Pdu_Can_Send_Task,		"PDU_CAN_SEND",		configMINIMAL_STACK_SIZE,   NULL,   PDU_CAN_COMM_TASK_PRIO,		&PduCanSendTask_Handler);

	xTaskCreate(Pdu_Can_Parse_Task,    	"PDU_CAN_PARSE",	configMINIMAL_STACK_SIZE,   NULL,   PDU_CAN_COMM_TASK_PRIO,		&PduCanParseTask_Handler);

	vTaskDelete(NULL);

	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

	taskEXIT_CRITICAL();
}
