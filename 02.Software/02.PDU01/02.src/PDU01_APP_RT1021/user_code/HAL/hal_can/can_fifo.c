/*
 * can_fifo.c
 *
 *  Created on: 2024年8月23日
 *      Author: Bono
 */
#include "boot.h"
#include "SignalManage.h"
#include "hal_sys_IF.h"
#include "hal_can_IF.h"

strCAN_REC_BUF g_Can1RecBuf = {0};
BOOL g_blUpdataRecFinishFg = FALSE;
BOOL g_blUpdataFuncEnFg = FALSE;
U32  g_u32TotalPackCount = 0;

BOOL AddFrameToCan1UpdataRxBuf(flexcan_frame_t *Frame)
{
	BOOL blRet = FALSE;
    U8 u8MsgDataCnt = 0;
	U16 u16BufPos = 0;
	S32 s32UpdateType = 0;

	(void)GetSigVal(PDU_SAM_SIG_ID_UPDATE_STATE, &s32UpdateType);

	if(s32UpdateType != PDU_CAN1_UPGRADE
	&& s32UpdateType != NO_UPGRADE)
	{
		return blRet;
	}

    if((GetMsgVal(PDU_SAM_SIG_ID_BOARD_ID) != (U8)((Frame->id)>>8))
    && (0xFF != (U8)((Frame->id)>>8)))
    {
        return blRet;
    }

    if((UPDATE_REQUEST_CMD != (BOOT_STEP_CMD)((Frame->id)>>16))
    && (UPDATE_READY_CMD != (BOOT_STEP_CMD)((Frame->id)>>16))
    && (UPDATE_MIDDLE_CMD != (BOOT_STEP_CMD)((Frame->id)>>16))
    && (UPDATE_CHECKSUM_CMD != (BOOT_STEP_CMD)((Frame->id)>>16)))
    {
		return blRet;
    }

	g_Can1RecBuf.u8MsgSourceAddr = (Frame->id)&0xFF;
	g_Can1RecBuf.u8MsgTargetAddr = (Frame->id>>8)&0xFF;
	g_Can1RecBuf.enumUpdataStep = (BOOT_STEP_CMD)((Frame->id)>>16);

    //判断升级报文是否按照升级流程要求
    if(TRUE != blDetcetUpdataStep(g_Can1RecBuf.enumUpdataStep))
	{
		return blRet;
	}

	switch (g_Can1RecBuf.enumUpdataStep)
	{
		case UPDATE_REQUEST_CMD:
			if((Frame->dataByte0 == g_boot_version[0])
            && (Frame->dataByte1 == g_boot_version[1])
            && (Frame->dataByte2 == g_boot_version[2])
            && (Frame->dataByte3 == g_boot_version[3])
            && (Frame->dataByte4 == g_boot_version[4])
            && (Frame->dataByte5 == g_boot_version[5])
            && (Frame->dataByte6 == g_boot_version[6])
			&& (Frame->dataByte7 == g_boot_version[7])
            && GetMsgVal(PDU_SAM_SIG_ID_UPDATE_STATE) == NO_UPGRADE)
			{
                g_blUpdataRecFinishFg = TRUE;
				blRet = TRUE;

                if(FALSE == g_blUpdataFuncEnFg)
                {
                    g_blUpdataFuncEnFg = TRUE;
                    (void)SetSigVal(PDU_SAM_SIG_ID_UPDATE_STATE, PDU_CAN1_UPGRADE);
                    xTaskResumeFromISR(NormalUpdataTask_Handler);
                }
			}
			break;

		case UPDATE_READY_CMD:
			if((0xAA == Frame->dataByte0)
			&& (0x55 == Frame->dataByte1))
			{
				g_Can1RecBuf.u32FileByteNums = (((U32)Frame->dataByte4)<<24)
												| (U32)Frame->dataByte5<<16
												| (U32)Frame->dataByte6<<8
												| (U32)Frame->dataByte7;
				g_u32TotalPackCount = g_Can1RecBuf.u32FileByteNums / MSG_UPDATE_MAX_DATALEN;	// 默认总包数大于1，且从0开始
                g_blUpdataRecFinishFg = TRUE;
				blRet = TRUE;
			}
			break;

		case UPDATE_MIDDLE_CMD:
			if(0xFF != Frame->dataByte0)
			{
				u16BufPos = Frame->dataByte0;
				u8MsgDataCnt = 1;
				g_Can1RecBuf.u8RxData[u16BufPos*7 + ((u8MsgDataCnt++)-1)] = Frame->dataByte1;
				g_Can1RecBuf.u8RxData[u16BufPos*7 + ((u8MsgDataCnt++)-1)] = Frame->dataByte2;
				g_Can1RecBuf.u8RxData[u16BufPos*7 + ((u8MsgDataCnt++)-1)] = Frame->dataByte3;
				g_Can1RecBuf.u8RxData[u16BufPos*7 + ((u8MsgDataCnt++)-1)] = Frame->dataByte4;
				g_Can1RecBuf.u8RxData[u16BufPos*7 + ((u8MsgDataCnt++)-1)] = Frame->dataByte5;
				g_Can1RecBuf.u8RxData[u16BufPos*7 + ((u8MsgDataCnt++)-1)] = Frame->dataByte6;
				g_Can1RecBuf.u8RxData[u16BufPos*7 + ((u8MsgDataCnt++)-1)] = Frame->dataByte7;

                g_blUpdataRecFinishFg = FALSE;
				blRet = TRUE;
			}
			else if(0xFF == Frame->dataByte0)
			{
				g_Can1RecBuf.u16PackDataNum = (((U16)Frame->dataByte1)<<8)|Frame->dataByte2;
				g_Can1RecBuf.u16PackNum = (((U16)Frame->dataByte3)<<8)|Frame->dataByte4;
				g_Can1RecBuf.u16PackCrc = (((U16)Frame->dataByte5)<<8)|Frame->dataByte6;

				if((g_Can1RecBuf.u16PackNum < g_u32TotalPackCount) && (g_Can1RecBuf.u16PackDataNum != MSG_UPDATE_MAX_DATALEN))	////非最后一包数据包长度错误
				{
					break;
				}

                g_blUpdataRecFinishFg = TRUE;
				blRet = TRUE;
			}
			break;


		case UPDATE_CHECKSUM_CMD:
				g_Can1RecBuf.u32FileCrc = (((U32)Frame->dataByte0)<<24)
											| (U32)Frame->dataByte1<<16
											| (U32)Frame->dataByte2<<8
											| (U32)Frame->dataByte3;

                g_blUpdataRecFinishFg = TRUE;
				blRet = TRUE;
			break;

		case UPDATE_UNKNOW_CMD:
		default:
                g_blUpdataRecFinishFg = FALSE;
				blRet = FALSE;
			break;
	}

	return blRet;
}
