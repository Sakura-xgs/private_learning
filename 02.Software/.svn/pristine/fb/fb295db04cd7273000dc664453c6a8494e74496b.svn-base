/*
 * factory_test.c
 *
 *  Created on: 2024年12月3日
 *      Author: Bono
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "fsl_flexcan.h"
#include "data_sample.h"
#include "hal_can_IF.h"
#include "hal_sys_IF.h"
#include "SignalManage.h"
#include "factory_test.h"
#include "boot.h"
#include "pdu_can.h"

QueueHandle_t g_Factory_Test_Msg_xQueue = NULL;

const TEST_PARAM_T TestParamMap[] =
{
	{0x00,	FactoryModeCtlFunc		},
	{0x01,	RelayCtlTestFunc		},
	{0x06,  LedTestFunc 			},
	{0x07,  SwVersionTestFunc 		},
	{0x08,  EepromTestFunc 			},
	{0x09,  WdgTestFunc 			},
	{0x0A,  GpioDITestFunc 			},
};
#define PDU_CALIB_MSG_CAN_FRAME_NUM sizeof(TestParamMap)/sizeof(TEST_PARAM_T)

void ParseSNReadMsg(flexcan_frame_t *Frame)
{
	S32 s32PduId;
	S32 snSignals[8] = {0};
	uint8_t snBytes[32]; 									// 存储32个SN字节

	if((Frame->dataByte0 != 0x55) || (Frame->dataByte1 != 0xAA))
	{
		return;
	}
	
	// 一次性获取所有SN数据并拆解为字节数组
	for (U8 i = 0u; i < 8u; i++)
	{
		(void)GetSigVal(PDU_SET_SIG_ID_SN_NUMBER_1_2_3_4 + i, &snSignals[i]);
		snBytes[i*4]   = (snSignals[i] >> 24) & 0xFF;  		// 最高字节
		snBytes[i*4+1] = (snSignals[i] >> 16) & 0xFF;  		// 次高字节
		snBytes[i*4+2] = (snSignals[i] >> 8)  & 0xFF;  		// 次低字节
		snBytes[i*4+3] = snSignals[i] & 0xFF;          		// 最低字节
	}

	// 获取板号配置通用CAN帧参数
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);
	Frame->format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame->type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame->id     = FLEXCAN_ID_EXT((SN_READ_FUNCID<<16) | (PCU_ADDR_ID<<8) | (s32PduId));
	Frame->length = GCAN_FRAME_LEN;

	// 分为三帧数据发送
	for (U8 group = 1u; group <= 5u; group++)
	{
		Frame->dataByte0 = group;  							// Group ID
		// 根据组号填充数据
		switch(group) 
		{
			case 1u: 										// SN1-SN7
				Frame->dataByte1 = snBytes[0];
				Frame->dataByte2 = snBytes[1];
				Frame->dataByte3 = snBytes[2];
				Frame->dataByte4 = snBytes[3];
				Frame->dataByte5 = snBytes[4];
				Frame->dataByte6 = snBytes[5];
				Frame->dataByte7 = snBytes[6];
				break;
				
			case 2u: 										// SN8-SN14
				Frame->dataByte1 = snBytes[7];
				Frame->dataByte2 = snBytes[8];
				Frame->dataByte3 = snBytes[9];
				Frame->dataByte4 = snBytes[10];
				Frame->dataByte5 = snBytes[11];
				Frame->dataByte6 = snBytes[12];
				Frame->dataByte7 = snBytes[13];
				break;
				
			case 3u: 										// SN15-SN21
				Frame->dataByte1 = snBytes[14];
				Frame->dataByte2 = snBytes[15];
				Frame->dataByte3 = snBytes[16];
				Frame->dataByte4 = snBytes[17];
				Frame->dataByte5 = snBytes[18];
				Frame->dataByte6 = snBytes[19];
				Frame->dataByte7 = snBytes[20];
				break;

			case 4u: 										// SN22-SN28
				Frame->dataByte1 = snBytes[21];
				Frame->dataByte2 = snBytes[22];
				Frame->dataByte3 = snBytes[23];
				Frame->dataByte4 = snBytes[24];
				Frame->dataByte5 = snBytes[25];
				Frame->dataByte6 = snBytes[26];
				Frame->dataByte7 = snBytes[27];
				break;

			case 5u: 										// SN29-SN32 + 填充0
				Frame->dataByte1 = snBytes[28];
				Frame->dataByte2 = snBytes[29];
				Frame->dataByte3 = snBytes[30];
				Frame->dataByte4 = snBytes[31];
				Frame->dataByte5 = 0;						// 预留
				Frame->dataByte6 = 0;						// 预留
				Frame->dataByte7 = 0;						// 预留
				break;
		}
		
		PduCanSendFrame(Frame);
		if (group < 5u)
		{
			vTaskDelay(1); 									// 帧间延时，最后一帧不需要
		}
	}
}

void ParseSNWriteMsg(flexcan_frame_t *Frame)
{
	U8 u8Ret = 0x55u;
	S32 s32PduId = 0;
	U32 u32TempSigVal = 0;

	if(Frame->dataByte0 == 1u)
	{
		// 构建 SN1-4 的32位值 (直接覆盖)
        u32TempSigVal = (Frame->dataByte1 << 24) | 
                        (Frame->dataByte2 << 16) | 
                        (Frame->dataByte3 << 8)  | 
                        Frame->dataByte4;
        (void)SetSigVal(PDU_SET_SIG_ID_SN_NUMBER_1_2_3_4, (S32)u32TempSigVal);

        // 构建 SN5-7 的32位值 (保留第4字节不变)
        (void)GetSigVal(PDU_SET_SIG_ID_SN_NUMBER_5_6_7_8, (S32*)&u32TempSigVal);
        u32TempSigVal = (Frame->dataByte5 << 24) | 
                        (Frame->dataByte6 << 16) | 
                        (Frame->dataByte7 << 8)  | 
                        (u32TempSigVal & 0xFF);  						// 保留原始SN8
        (void)SetSigVal(PDU_SET_SIG_ID_SN_NUMBER_5_6_7_8, (S32)u32TempSigVal);

        u8Ret = 0xAAu;
	}
	else if(Frame->dataByte0 == 2u)
	{
		// 更新 SN8 (信号2的第4字节)
        (void)GetSigVal(PDU_SET_SIG_ID_SN_NUMBER_5_6_7_8, (S32*)&u32TempSigVal);
        u32TempSigVal = (u32TempSigVal & 0xFFFFFF00) | Frame->dataByte1; // 只更新最低字节
        (void)SetSigVal(PDU_SET_SIG_ID_SN_NUMBER_5_6_7_8, (S32)u32TempSigVal);

        // 构建 SN9-12 的32位值 (直接覆盖)
        u32TempSigVal = (Frame->dataByte2 << 24) | 
                        (Frame->dataByte3 << 16) | 
                        (Frame->dataByte4 << 8)  | 
                        Frame->dataByte5;
        (void)SetSigVal(PDU_SET_SIG_ID_SN_NUMBER_9_10_11_12, (S32)u32TempSigVal);

        // 更新 SN13-14 (信号4的高2字节)
        (void)GetSigVal(PDU_SET_SIG_ID_SN_NUMBER_13_14_15_16, (S32*)&u32TempSigVal);
        u32TempSigVal = (Frame->dataByte6 << 24) | 
                        (Frame->dataByte7 << 16) | 
                        (u32TempSigVal & 0xFFFF);  						// 保留原始SN15-16
        (void)SetSigVal(PDU_SET_SIG_ID_SN_NUMBER_13_14_15_16, (S32)u32TempSigVal);

        u8Ret = 0xAAu;
	}
	else if(Frame->dataByte0 == 3u)
	{
		// 更新 SN15-16 (信号4的低2字节)
        (void)GetSigVal(PDU_SET_SIG_ID_SN_NUMBER_13_14_15_16, (S32*)&u32TempSigVal);
        u32TempSigVal = (u32TempSigVal & 0xFFFF0000) | 
                        (Frame->dataByte1 << 8) | 
                        Frame->dataByte2;
        (void)SetSigVal(PDU_SET_SIG_ID_SN_NUMBER_13_14_15_16, (S32)u32TempSigVal);

		// 构建 SN17-20 的32位值 (直接覆盖)
        u32TempSigVal = (Frame->dataByte3 << 24) | 
                        (Frame->dataByte4 << 16) | 
                        (Frame->dataByte5 << 8)  | 
                        Frame->dataByte6;
        (void)SetSigVal(PDU_SET_SIG_ID_SN_NUMBER_17_18_19_20, (S32)u32TempSigVal);

		// 更新 SN21 (信号6的高1字节)
		(void)GetSigVal(PDU_SET_SIG_ID_SN_NUMBER_21_22_23_24, (S32*)&u32TempSigVal);
		u32TempSigVal = (Frame->dataByte7 << 24) |
						(u32TempSigVal & 0x00FFFFFF);  					// 保留原始SN22-24
		(void)SetSigVal(PDU_SET_SIG_ID_SN_NUMBER_21_22_23_24, (S32)u32TempSigVal);

        u8Ret = 0xAAu;
	}
	else if(Frame->dataByte0 == 4u)
	{
		// 更新 SN22-24 (信号6的低3字节)
		(void)GetSigVal(PDU_SET_SIG_ID_SN_NUMBER_21_22_23_24, (S32*)&u32TempSigVal);
		u32TempSigVal = (u32TempSigVal & 0xFF000000) |
						(Frame->dataByte1 << 16) |
						(Frame->dataByte2 << 8)  |
						Frame->dataByte3;
		(void)SetSigVal(PDU_SET_SIG_ID_SN_NUMBER_21_22_23_24, (S32)u32TempSigVal);

		// 构建 SN25-28 的32位值 (直接覆盖)
		u32TempSigVal = (Frame->dataByte4 << 24) | 
						(Frame->dataByte5 << 16) | 
						(Frame->dataByte6 << 8)  | 
						Frame->dataByte7;
		(void)SetSigVal(PDU_SET_SIG_ID_SN_NUMBER_25_26_27_28, (S32)u32TempSigVal);

		u8Ret = 0xAAu;
	}
	else if(Frame->dataByte0 == 5u)
	{
		// 更新 SN29-32 (信号8的全部4字节)
		u32TempSigVal = (Frame->dataByte1 << 24) | 
						(Frame->dataByte2 << 16) | 
						(Frame->dataByte3 << 8)  | 
						Frame->dataByte4;
		(void)SetSigVal(PDU_SET_SIG_ID_SN_NUMBER_29_30_31_32, (S32)u32TempSigVal);
		
		u8Ret = 0xAAu;
	}


	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);				//获取板号，填充CAN_ID
	Frame->dataByte1 = u8Ret;
	Frame->format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame->type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame->id = FLEXCAN_ID_EXT((SN_WRITE_FUNCID<<16) | (PCU_ADDR_ID<<8) | (s32PduId));
	Frame->length = GCAN_FRAME_LEN;

	PduCanSendFrame(Frame);
}

BOOL ClearPduSaveEepromData(void)
{
	U8 i = 0;
	for(i = 0; i < MAX_RELAY_NUM; i++)
	{
		s32RelayCtrlTimes[i] = 0;
		(void)SetSigVal(PDU_SET_RELAY_1_OPERATE_TIMES+i, 0);
		vTaskDelay(2);
	}
	(void)SetSigVal(PDU_SET_SIG_ID_RESET_TIMES, 0);

	for(i = 0; i < MAX_RELAY_NUM; i++)
	{
		if(GetMsgVal(PDU_SET_RELAY_1_OPERATE_TIMES+i) != 0)
		{
			return FALSE;
		}
	}

	if(GetMsgVal(PDU_SET_SIG_ID_RESET_TIMES) != 0)
	{
		return FALSE;
	}

	return TRUE;
}

void PduMsgResponse(U16 u16FuncId, BOOL ret)
{
	flexcan_frame_t Frame = {0};
	S32 s32PduId = 0;
	U8 data[8] = {0};

	//获取板号，填充CAN_ID
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);

	if(ret) {
		data[0] = 0xAA;
	} else {
		data[0] = 0x55;
	}

	CAN_8_DATA_COPY(data, &Frame);
	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT((u16FuncId<<16) | (PCU_ADDR_ID<<8) | (s32PduId));
	Frame.length = GCAN_FRAME_LEN;

	PduCanSendFrame(&Frame);
}

void ParseRemoteCtrlMsg(flexcan_frame_t *Frame)
{
	BOOL blRet = FALSE;

	switch(Frame->dataByte0)
	{
		case PCU_CMD_CLEAR_HISTORY:
			blRet = ClearPduSaveEepromData();
			PduMsgResponse(REMOTE_CTRL_FUNCID, blRet);
			break;

		case PCU_CMD_SOFT_RESET:
			PduMsgResponse(REMOTE_CTRL_FUNCID, TRUE);
			PduSoftReset();
			break;

		default:
			break;
	}
}

void FactoryTestParse(flexcan_frame_t *Frame)
{
    U8 i = 0;
    U8 data[8] = {0};
	U16 u16FuncId = 0;

	u16FuncId = (U16)((Frame->id>>16) & 0xFFFF);

    data[0] = Frame->dataByte0;
    data[1] = Frame->dataByte1;
    data[2] = Frame->dataByte2;
    data[3] = Frame->dataByte3;
    data[4] = Frame->dataByte4;
    data[5] = Frame->dataByte5;
    data[6] = Frame->dataByte6;
    data[7] = Frame->dataByte7;

	if((Frame->dataByte0 == 0x00) && (u16FuncId == FACTORY_TEST_FUNCID))	/* 0x10010180 */
	{
		FactoryModeCtlFunc(data);
	}
	else if((TRUE == GetMsgVal(SIGNAL_STATUS_FACTORY_TEST))  && (u16FuncId == FACTORY_TEST_FUNCID))
	{
		for(i = 1; i < PDU_CALIB_MSG_CAN_FRAME_NUM; i++)
		{
			if(TestParamMap[i].cmd == data[0])
			{
				TestParamMap[i].Func(data);
				break;
			}
		}
	}
	else
	{
		if(u16FuncId == SN_READ_FUNCID)
		{
			ParseSNReadMsg(Frame);
		}
		else if(u16FuncId == SN_WRITE_FUNCID)
		{
			ParseSNWriteMsg(Frame);
		}
		else if(u16FuncId == REMOTE_CTRL_FUNCID)
		{
			ParseRemoteCtrlMsg(Frame);
		}
	}
}

void FactoryModeCtlFunc(U8 *data)
{
	flexcan_frame_t Frame = {0};
	S32 s32PduId = 0;

	//开启生产测试模式
	if(0xAA == data[1])
	{
		if(TRUE == GetMsgVal(SIGNAL_STATUS_FACTORY_TEST))
		{
			data[1] = 0;										//已在生产测试模式
		}
		else
		{
			USER_LED_ON();
			LED1_ON();											//转接板灯与运行灯同步闪烁
			(void)SetSigVal(SIGNAL_STATUS_FACTORY_TEST, TRUE);
		}
	}

	//填充数据段
	CAN_8_DATA_COPY(data, &Frame);
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);		//获取板号，填充CAN_ID
	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT(FACTORY_REPLY_ID | s32PduId);
	Frame.length = 8;

	PduCanSendFrame(&Frame);

	if(0x55 == data[1])
	{
		//记录关机事件
		(void)SetSigVal(SIGNAL_STATUS_POWER_OFF, TRUE);

		vTaskDelay(50);

		SystemResetFunc();
	}
}

void RelayCtlTestFunc(U8 *data)
{
	flexcan_frame_t Frame = {0};
	U8 u8Ret = 0x55;
	S32 s32PduId = 0;

	//开启生产测试模式
	if((MAX_RELAY_NUM >= data[1]) && (data[2] <= 1))
	{
		ctrl_relay_cmd(data[1]-1, data[2]);
		u8Ret = 0xAA;
	}
	data[3] = u8Ret;

	//填充数据段
	CAN_8_DATA_COPY(data, &Frame);
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);		//获取板号，填充CAN_ID
	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT(FACTORY_REPLY_ID | s32PduId);
	Frame.length = 8;

	PduCanSendFrame(&Frame);
}

void LedTestFunc(U8 *data)
{
	flexcan_frame_t Frame = {0};
	U8 u8Ret = 0x55;
	S32 s32PduId = 0;

	//开启生产测试模式
	if((MAX_PWM_LED_NUM >= data[1]) && (data[2] <= 1))
	{
		ctrl_led_cmd(data[1]-1, data[2]);
		u8Ret = 0xAA;
	}
	data[3] = u8Ret;

	//填充数据段
	CAN_8_DATA_COPY(data, &Frame);
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);		//获取板号，填充CAN_ID
	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT(FACTORY_REPLY_ID | s32PduId);
	Frame.length = 8;

	PduCanSendFrame(&Frame);
}

void SwVersionTestFunc(U8 *data)
{
	flexcan_frame_t Frame = {0};
	S32 s32PduId,s32SigVal = 0;

	/*回复软件版本*/
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
	Frame.length = 8;
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

void EepromTestFunc(U8 *data)
{
	flexcan_frame_t Frame = {0};
	U8 u8Ret = 0x55;
	S32 s32PduId = 0;

	if(EVENT_CANCEL == GetMsgVal(ALARM_ID_EEPROM_ERROR))
	{
		u8Ret = 0xAA;
	}
	data[1] = u8Ret;

	//填充数据段
	CAN_8_DATA_COPY(data, &Frame);
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);		//获取板号，填充CAN_ID
	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT(FACTORY_REPLY_ID | s32PduId);
	Frame.length = 8;

	PduCanSendFrame(&Frame);
}

void WdgTestFunc(U8 *data)
{
	flexcan_frame_t Frame = {0};
	U8 u8Ret = 0x55;
	S32 s32PduId = 0;

	//开启生产测试模式
	if(data[1] == 0xAA)
	{
		(void)SetSigVal(SIGNAL_STATUS_WDG_TEST, TRUE);
		u8Ret = 0xAA;
	}
	data[1] = u8Ret;

	//填充数据段
	CAN_8_DATA_COPY(data, &Frame);
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);		//获取板号，填充CAN_ID
	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT(FACTORY_REPLY_ID | s32PduId);
	Frame.length = 8;

	PduCanSendFrame(&Frame);
}


void GpioDITestFunc(U8 *data)
{
	flexcan_frame_t Frame = {0};
	S32 s32PduId = 0;

	S32 addr = 0, hw_ver = 0;
	U8 addr1 = 0, addr2 = 0, addr3 = 0, addr4 = 0;
	U8 ver1 = 0, ver2 = 0;

	ver1 = GET_VER1_STATUS();
	ver2 = GET_VER2_STATUS();
	addr1 = GET_ADDR1_STATUS();
	addr2 = GET_ADDR2_STATUS();
	addr3 = GET_ADDR3_STATUS();
	addr4 = GET_ADDR4_STATUS();

	hw_ver |= (ver1 << 0);
	hw_ver |= (ver2 << 1);
	addr |= (addr1 << 0);
	addr |= (addr2 << 1);
	addr |= (addr3 << 2);
	addr |= (addr4 << 3);

	data[1] = addr;
	data[2] = hw_ver;
	//填充数据段
	CAN_8_DATA_COPY(data, &Frame);
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);		//获取板号，填充CAN_ID
	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT(FACTORY_REPLY_ID | s32PduId);
	Frame.length = 8;

	PduCanSendFrame(&Frame);
}

void Factory_Test_Task(void * pvParameters)
{
    BaseType_t err = pdFALSE;
	volatile UBaseType_t uxHighWaterMark;
	flexcan_frame_t Frame = {0};
	(void)pvParameters;

	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

    for( ;; )
    {
		if(SYS_STATUS_NORMAL != GetSysRunModeFg())
		{
			vTaskDelete(NULL);
		}

		if(NULL != g_Factory_Test_Msg_xQueue)
		{
			err = xQueueReceive(g_Factory_Test_Msg_xQueue, (void *)&Frame, portMAX_DELAY);

            if(pdTRUE == err)
            {
				FactoryTestParse(&Frame);
            }
			else
			{
				vTaskDelay(300);															// 避免密集错误循环
			}
		}
		else
		{
			vTaskDelay(300);
		}

		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    }
}

void Factory_Test_Init_task(void * pvParameters)
{
	volatile UBaseType_t uxHighWaterMark = 0;

	taskENTER_CRITICAL();

	xTaskCreate(Factory_Test_Task,	"FACTORY_TEST",		configMINIMAL_STACK_SIZE,   NULL,   GENERAL_TASK_PRIO,		NULL);

	vTaskDelete(NULL);

	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

	taskEXIT_CRITICAL();
}
