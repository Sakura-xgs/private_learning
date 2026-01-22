/*
 * uart_boot.c
 *
 *  Created on: 2024年10月25日
 *      Author: Bono
 */

#include "uart_boot.h"
#include "SignalManage.h"
#include "PublicDefine.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/
static USART_BOOT_STEP_CMD enumUsartPreUpdataStep = USART_UPDATE_UNKNOW_CMD;
BOOL g_blUsartUpdataFuncEnFg = FALSE;

/*******************************************************************************
 * Code
 ******************************************************************************/

BOOL blDetcetUsartUpdataStep(USART_BOOT_STEP_CMD RecStep)
{
    BOOL blRet = TRUE;

    switch (enumUsartPreUpdataStep)
    {
        case USART_UPDATE_UNKNOW_CMD:
            if((USART_UPDATE_REQUEST_CMD != RecStep)
			|| (GetMsgVal(PDU_SAM_SIG_ID_UPDATE_STATE) != NO_UPGRADE))
            {
                blRet = FALSE;
            }
            break;

        case USART_UPDATE_REQUEST_CMD:
            if (USART_UPDATE_READY_CMD != RecStep)
            {
                blRet = FALSE;
            }
            break;

        case USART_UPDATE_READY_CMD:
            if (USART_UPDATE_BEGIN_CMD != RecStep)
            {
                blRet = FALSE;
            }
            break;

        case USART_UPDATE_BEGIN_CMD:
            if (USART_UPDATE_MIDDLE_CMD != RecStep)
            {
                blRet = FALSE;
            }
            break;

        case USART_UPDATE_MIDDLE_CMD:
            if ((USART_UPDATE_MIDDLE_CMD != RecStep)
                && (USART_UPDATE_END_CMD != RecStep))
            {
                blRet = FALSE;
            }
            break;

        case USART_UPDATE_END_CMD:
            if (USART_UPDATE_CHECKSUM_CMD != RecStep)
            {
                blRet = FALSE;
            }
            break;

        default:
            blRet = FALSE;
            break;
    }

    return blRet;
}

void UsartUpdataTimeOutFunc(void)
{
	(void)SetSigVal(PDU_SAM_SIG_ID_UPDATE_STATE, NO_UPGRADE);
	enumUsartPreUpdataStep = USART_UPDATE_UNKNOW_CMD;
	g_blUsartUpdataFuncEnFg = FALSE;
	(void)SetSigVal(SIGNAL_STATUS_UPDATA, FALSE);
}

/******************************************************************************
* 名  	称:  BOOT_ENTER_REPLY
* 功  	能:  进入BOOT应答
* 入口参数:  无
* 出口参数:  无
*******************************************************************************/
void BOOT_ENTER_REPLY(void)
{
    U16 i = 0;
    U16 j = 0;
    U16 u16Crc = 0;
	U8 u8Sendbuf[UART_COMM_BUF_LEN] = {0};

	//发送回应数据包
	u8Sendbuf[i++] = 0xAA;
	u8Sendbuf[i++] = 0x55;
	u8Sendbuf[i++] = GetMsgVal(PDU_SAM_SIG_ID_BOARD_ID);			//发送本机设备地址
	u8Sendbuf[i++] = 0x50;     			//发送功能码
	u8Sendbuf[i++] = 0x07;				//返回字节个数(寄存器长度为2字节)
	u8Sendbuf[i++] = 0x01;
	u8Sendbuf[i++] = 0x00;

	for(j = 0; j < 5; j++)				//取版本信息前5个byte
	{
		u8Sendbuf[i++] = g_boot_version[j];
	}

	u16Crc = crc16_ccitt_xmodem(&u8Sendbuf[3], (i-3), 0);

	u8Sendbuf[i++] = (U8)u16Crc;
	u8Sendbuf[i++] = (U8)(u16Crc>>8);

	u8Sendbuf[i++] = 0x0D;
	u8Sendbuf[i++] = 0x0A;

	Uart_Dma_Send((GetMsgVal(PDU_SAM_SIG_ID_UPDATE_STATE) - UPGRADE_BY_UART1), i , u8Sendbuf);
}


/******************************************************************************
* 名  	称:  BOOT_DISENTER_REPLY
* 功  	能:  BMS退出BOOT应答
* 入口参数:  无
* 出口参数:  无
*******************************************************************************/
void BOOT_DISENTER_REPLY(void)
{
    U16 i = 0;
    U16 j = 0;
    U16 u16Crc = 0;
	U8 u8Sendbuf[UART_COMM_BUF_LEN] = {0};

	//发送回应数据包
	u8Sendbuf[i++] = 0xAA;
	u8Sendbuf[i++] = 0x55;
	u8Sendbuf[i++] = GetMsgVal(PDU_SAM_SIG_ID_BOARD_ID);			//发送本机设备地址
	u8Sendbuf[i++] = 0x50;     			//发送功能码
	u8Sendbuf[i++] = 0x07;				//返回字节个数(寄存器长度为2字节)
	u8Sendbuf[i++] = 0x01;
	u8Sendbuf[i++] = 0x00;

	for(j = 0; j < 5; j++)				//取版本信息前5个byte
	{
		u8Sendbuf[i++] = 0xFF;
	}

	u16Crc = crc16_ccitt_xmodem(&u8Sendbuf[3], (i-3), 0);

	u8Sendbuf[i++] = (U8)u16Crc;
	u8Sendbuf[i++] = (U8)(u16Crc>>8);

	u8Sendbuf[i++] = 0x0D;
	u8Sendbuf[i++] = 0x0A;

	Uart_Dma_Send((GetMsgVal(PDU_SAM_SIG_ID_UPDATE_STATE) - UPGRADE_BY_UART1), i , u8Sendbuf);
}

void BOOT_UPDATE_ACK(BOOL blFg)
{
    U16 i = 0;
    U16 u16Crc = 0;
	U8 u8Sendbuf[UART_COMM_BUF_LEN] = {0};

	//发送回应数据包
	u8Sendbuf[i++] = 0xAA;
	u8Sendbuf[i++] = 0x55;
	u8Sendbuf[i++] = GetMsgVal(PDU_SAM_SIG_ID_BOARD_ID);			//发送本机设备地址
	u8Sendbuf[i++] = 0x50;     			//发送功能码
	u8Sendbuf[i++] = 0x01;				//返回字节个数(寄存器长度为2字节)
	u8Sendbuf[i++] = blFg;

	u16Crc = crc16_ccitt_xmodem(&u8Sendbuf[3], (i-3), 0);

	u8Sendbuf[i++] = (U8)u16Crc;
	u8Sendbuf[i++] = (U8)(u16Crc>>8);

	u8Sendbuf[i++] = 0x0D;
	u8Sendbuf[i++] = 0x0A;

	Uart_Dma_Send((GetMsgVal(PDU_SAM_SIG_ID_UPDATE_STATE) - UPGRADE_BY_UART1), i , u8Sendbuf);
}


/******************************************************************************
* 名  	称:  UsartUpdataFunc
* 功  	能:  升级主程序
* 入口参数:  无
* 出口参数:  无
*******************************************************************************/
void UsartUpdataFunc(void)
{
	__BSS(SRAM_OC) static U8 u8DataBuf[UART_COMM_BUF_LEN] = {0};
	BOOL blRet = FALSE;
	U16 u16DataType = 0;
	U16 u16DataCnt = 0;
	U16 u16DataLen = 0;
	U16 u16MsgNo = 0;
	U32 u32GetTotalPackCrc = 0;
	static U32 u32CalTotalPackCrc = 0;
	S32 s32BorNo = 0;
	S32 s32BmsNum = 0;
	S32 s32UpdateType = 0;

	(void)GetSigVal(PDU_SAM_SIG_ID_UPDATE_STATE, &s32UpdateType);

//	u16DataType = RecUpgradeData();

	switch(u16DataType)
	{
		case USART_UPDATE_REQUEST_CMD:		//升级请求
//			if((WIFI_RecBuf.u8Recvbuf[9] == g_boot_version[0])
//			&& (WIFI_RecBuf.u8Recvbuf[10] == g_boot_version[1])
//			&& (WIFI_RecBuf.u8Recvbuf[11] == g_boot_version[2])
//			&& (WIFI_RecBuf.u8Recvbuf[12] == g_boot_version[3])
//			&& (WIFI_RecBuf.u8Recvbuf[13] == g_boot_version[4])
//			&& (s32UpdateType == UART2_BMS_UPGRADE))
//			{
//				SuspendLowPowerDectTaskFunc();
//				SuspendFlashTaskFunc();
//				BOOT_ENTER_REPLY();
//				AllLedOff();
//				g_strM2SUpdataType.u32AppTotalByteNum = 0;
//				g_strM2SUpdataType.u16AppTotalBytePackNum = 0;
//			}
//			else if((g_Usart_0_CommBuf.u8Recvbuf[9] == g_boot_version[0])
//			&& (g_Usart_0_CommBuf.u8Recvbuf[10] == g_boot_version[1])
//			&& (g_Usart_0_CommBuf.u8Recvbuf[11] == g_boot_version[2])
//			&& (g_Usart_0_CommBuf.u8Recvbuf[12] == g_boot_version[3])
//			&& (g_Usart_0_CommBuf.u8Recvbuf[13] == g_boot_version[4])
//			&& (s32UpdateType == UART0_BMS_UPGRADE))
//			{
//				SuspendLowPowerDectTaskFunc();
//				SuspendFlashTaskFunc();
//				BOOT_ENTER_REPLY();
//				AllLedOff();
//				g_strM2SUpdataType.u32AppTotalByteNum = 0;
//				g_strM2SUpdataType.u16AppTotalBytePackNum = 0;
//			}
//			else
//			{
//				BOOT_DISENTER_REPLY();
//				g_strM2SUpdataType.u32AppTotalByteNum = 0;
//				g_strM2SUpdataType.u16AppTotalBytePackNum = 0;
//			}
		break;

		case USART_UPDATE_READY_CMD:			//升级准备

//			blRet = ExtFlashClrBackupApp();

			vTaskDelay(100);

			u32CalTotalPackCrc = 0xFFFFFFFF;
		break;

		case USART_UPDATE_BEGIN_CMD:			//起始包
		case USART_UPDATE_MIDDLE_CMD:			//中间包
		case USART_UPDATE_END_CMD:				//结束包
//			if(s32UpdateType == UART2_BMS_UPGRADE)
//			{
//				u16DataLen = WIFI_RecBuf.u8Recvbuf[4] - 4;	//更新数据字节数 = 上报的长度 - CRC（2字节） - 帧尾标志（2字节）
//				u16MsgNo = (U16)WIFI_RecBuf.u8Recvbuf[7] | ((U16)WIFI_RecBuf.u8Recvbuf[8])<<8;
//
//				for(u16DataCnt = 0; u16DataCnt < u16DataLen; u16DataCnt++)
//				{
//					/*+9是因为在数据包前段含有9个其他信息：
//					帧头标志（2字节）、目标地址（1字节）、命令（1字节）、
//					数据长度（1字节）、升级包类型（2字节）、升级包计数器（2字节）*/
//					u8DataBuf[u16DataCnt] = WIFI_RecBuf.u8Recvbuf[u16DataCnt+9];
//				}
//			}
//			else if(s32UpdateType == UART0_BMS_UPGRADE)
//			{
//				u16DataLen = g_Usart_0_CommBuf.u8Recvbuf[4] - 4;	//更新数据字节数 = 上报的长度 - CRC（2字节） - 帧尾标志（2字节）
//				u16MsgNo = (U16)g_Usart_0_CommBuf.u8Recvbuf[7] | ((U16)g_Usart_0_CommBuf.u8Recvbuf[8])<<8;
//
//				for(u16DataCnt = 0; u16DataCnt < u16DataLen; u16DataCnt++)
//				{
//					/*+9是因为在数据包前段含有9个其他信息：
//					帧头标志（2字节）、目标地址（1字节）、命令（1字节）、
//					数据长度（1字节）、升级包类型（2字节）、升级包计数器（2字节）*/
//					u8DataBuf[u16DataCnt] = g_Usart_0_CommBuf.u8Recvbuf[u16DataCnt+9];
//				}
//			}

//			blRet = UpdataFlashFunc(u8DataBuf, u16DataLen, u16MsgNo-1);
//
//			//计算累计数据包的校验值
//			u32CalTotalPackCrc = crc32(u8DataBuf, u16DataLen, u32CalTotalPackCrc);
//
//			g_strM2SUpdataType.u32AppTotalByteNum += u16DataLen;
//			g_strM2SUpdataType.u16AppTotalBytePackNum = u16MsgNo-1;
		break;

		case USART_UPDATE_CHECKSUM_CMD:		//固件文件校验码包
//			if(s32UpdateType == UART2_BMS_UPGRADE)
//			{
//				u32GetTotalPackCrc = (U32)WIFI_RecBuf.u8Recvbuf[9]
//										| ((U32)WIFI_RecBuf.u8Recvbuf[10])<<8
//										| ((U32)WIFI_RecBuf.u8Recvbuf[11])<<16
//										| ((U32)WIFI_RecBuf.u8Recvbuf[12])<<24;
//			}
//			else if(s32UpdateType == UART0_BMS_UPGRADE)
//			{
//				u32GetTotalPackCrc = (U32)g_Usart_0_CommBuf.u8Recvbuf[9]
//										| ((U32)g_Usart_0_CommBuf.u8Recvbuf[10])<<8
//										| ((U32)g_Usart_0_CommBuf.u8Recvbuf[11])<<16
//										| ((U32)g_Usart_0_CommBuf.u8Recvbuf[12])<<24;
//			}

			if((~u32CalTotalPackCrc) == u32GetTotalPackCrc)
			{
//				g_strM2SUpdataType.u32RecTotalPackCrc = u32CalTotalPackCrc;

				//升级成功后，写入APP完整的标识
//				if(TRUE == WriteAppInterFg2ExtFlash())
//				{
//					if (TRUE == CheckExtFlashAppIntegFg())
//					{
//						blRet = TRUE;
//					}
//				}
			}
		break;

		default:
		break;
	}

	enumUsartPreUpdataStep = (USART_BOOT_STEP_CMD)u16DataType;		//更新升级步骤

//	if(USART_UPDATE_REQUEST_CMD != (BOOT_STEP_CMD)u16DataType)
//	{
//		BOOT_UPDATE_ACK(blRet);
//
//		if((USART_UPDATE_CHECKSUM_CMD == (BOOT_STEP_CMD)u16DataType)
//			&& (blRet == TRUE))
//		{
//			(void)GetSigVal(MBMS_SAM_SIG_ID_GROUP_NO, &s32BorNo);
//
//			//获取已成功编址BMS个数
//			(void)GetSigVal(MBMS_SAM_SIG_ID_AUTOENCODE_BMS_NUM, &s32BmsNum);
//			g_strM2SUpdataType.u8SysBmsNum = (U8)s32BmsNum;
//
//			//当前BMS为主机，且系统内BMS个数不止1个板时，且使能主机升级从机功能，则启动主机升级从机流程
//			if((1 == s32BorNo)
//				&& (1 < g_strM2SUpdataType.u8SysBmsNum))
//			{
//				//进行主机升级从机时，不再接收升级指令
//				(void)SetSigVal(PDU_SAM_SIG_ID_UPDATE_STATE, CAN_M2S_BMS_UPGRADE);
//
//				//进行主升从策略
//				Master_Updata_Slave_Func();
//			}
//
//			WriteBmsUpdataBoardNo(s32BorNo);
//
//			//关闭所有MOS
//			DelFetTaskCtlFetOff();
//
//			vTaskDelay(500);
//
//			WriteCanUpdataFg();
//
//			SystemResetFunc();//重启
//		}
//	}
}
