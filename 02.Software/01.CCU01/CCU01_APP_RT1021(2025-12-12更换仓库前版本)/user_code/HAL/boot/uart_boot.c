/*
 * uart_boot.c
 *
 *  Created on: 2024年10月25日
 *      Author: Bono
 */

#include "uart_boot.h"
#include "SignalManage.h"
#include "PublicDefine.h"
#include "hal_uart_IF.h"
#include "tcp_client_IF.h"
#include "uart_comm.h"
#include "hmi_IF.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/
static USART_BOOT_STEP_CMD enumUsartPreUpdataStep = USART_UPDATE_UNKNOW_CMD;
BOOL g_blUsartUpdataFuncEnFg = FALSE;
__BSS(SRAM_OC) UpdateTypeDef g_Usart_2_CommBuf;

/*******************************************************************************
 * Code
 ******************************************************************************/
static void BOOT_ENTER_REPLY(void);
static void BOOT_DISENTER_REPLY(void);
static void BOOT_UPDATE_ACK(BOOL blFg);

/**
 * @brief 判断CCU升级步骤是否按照正确流程
 * @param RecStep:485接收到的上位机传来的当前流程
 */
BOOL blDetcetUsartUpdataStep(USART_BOOT_STEP_CMD RecStep)
{
    BOOL blRet = TRUE;

    switch (enumUsartPreUpdataStep)
    {
        case USART_UPDATE_UNKNOW_CMD:
            if((USART_UPDATE_REQUEST_CMD != RecStep)
			|| (NO_UPGRADE != g_Mbms_sam_update_state))
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
	(void)SetSigVal(CCU_SAM_SIG_ID_UPDATE_STATE, NO_UPGRADE);
	g_Mbms_sam_update_state = NO_UPGRADE;
	enumUsartPreUpdataStep = USART_UPDATE_UNKNOW_CMD;

	g_sGun_data[GUN_A].eGun_special_status = STA_NORMAL;//通知充电逻辑部分
	g_sGun_data[GUN_B].eGun_special_status = STA_NORMAL;//通知充电逻辑部分

	g_blUsartUpdataFuncEnFg = FALSE;
	(void)SetSigVal(SIGNAL_STATUS_UPDATA, FALSE);
}

/******************************************************************************
* 名  	称:  BOOT_ENTER_REPLY
* 功  	能:  进入BOOT应答
* 入口参数:  无
* 出口参数:  无
*******************************************************************************/
static void BOOT_ENTER_REPLY(void)
{
    U16 i = 0,j = 0;
    U16 u16Crc = 0;
	U8 u8Sendbuf[UART_COMM_BUF_LEN] = {0};

	//发送回应数据包
	u8Sendbuf[i] = 0xAAU;
	i++;
	u8Sendbuf[i] = 0x55U;
	i++;
	u8Sendbuf[i] = CCU_ADDR;			//发送本机设备地址
	i++;
	u8Sendbuf[i] = 0x50U;     			//发送功能码
	i++;
	u8Sendbuf[i] = 0x07U;				//返回字节个数(寄存器长度为2字节)
	i++;
	u8Sendbuf[i] = 0x00U;
	i++;
	u8Sendbuf[i] = 0x01U;
	i++;
	u8Sendbuf[i] = 0x00U;
	i++;

	for(j = 0; j < 5U; j++)				//取版本信息前5个byte
	{
		u8Sendbuf[i] = g_boot_version[j];
		i++;
	}

	u16Crc = crc16_ccitt_xmodem(&u8Sendbuf[3], (i-3U), 0);

	u8Sendbuf[i] = (U8)(u16Crc & 0xFFU);
	i++;
	u8Sendbuf[i] = (U8)(u16Crc>>8);
	i++;

	u8Sendbuf[i] = 0x0D;
	i++;
	u8Sendbuf[i] = 0x0A;
	i++;

	//Uart_Dma_Send((GetMsgVal(MBMS_SAM_SIG_ID_UPDATE_STATE) - CCU_UPGRADE_BY_UART1), i , u8Sendbuf);
	Uart_Dma_Send(DBG_UART, i , u8Sendbuf);
}

/******************************************************************************
* 名  	称:  BOOT_DISENTER_REPLY
* 功  	能:  BMS退出BOOT应答
* 入口参数:  无
* 出口参数:  无
*******************************************************************************/
static void BOOT_DISENTER_REPLY(void)
{
    U16 i = 0,j = 0;
    U16 u16Crc = 0;
	U8 u8Sendbuf[UART_COMM_BUF_LEN] = {0};

	//发送回应数据包
	u8Sendbuf[i] = 0xAAU;
	i++;
	u8Sendbuf[i] = 0x55U;
	i++;
	u8Sendbuf[i] = CCU_ADDR;			//发送本机设备地址
	i++;
	u8Sendbuf[i] = 0x50U;     			//发送功能码
	i++;
	u8Sendbuf[i] = 0x07U;				//返回字节个数(寄存器长度为2字节)
	i++;
	u8Sendbuf[i] = 0x00U;
	i++;
	u8Sendbuf[i] = 0x01U;
	i++;
	u8Sendbuf[i] = 0x00U;
	i++;

	for(j = 0; j < 5U; j++)				//取版本信息前5个byte
	{
		u8Sendbuf[i] = 0xFF;
		i++;
	}

	u16Crc = crc16_ccitt_xmodem(&u8Sendbuf[3], (i-3U), 0);

	u8Sendbuf[i] = (U8)(u16Crc & 0xFFU);
	i++;
	u8Sendbuf[i] = (U8)(u16Crc>>8);
	i++;

	u8Sendbuf[i] = 0x0DU;
	i++;
	u8Sendbuf[i] = 0x0AU;
	i++;

	//Uart_Dma_Send((GetMsgVal(MBMS_SAM_SIG_ID_UPDATE_STATE) - CCU_UPGRADE_BY_DBG8_UART), i , u8Sendbuf);
	Uart_Dma_Send(DBG_UART, i , u8Sendbuf);
}

static void BOOT_UPDATE_ACK(BOOL blFg)
{
    U16 i = 0;
    U16 u16Crc = 0;
	U8 u8Sendbuf[UART_COMM_BUF_LEN] = {0};

	//发送回应数据包
	u8Sendbuf[i] = 0xAAU;
	i++;
	u8Sendbuf[i] = 0x55U;
	i++;
	u8Sendbuf[i] = CCU_ADDR;					//发送本机设备地址
	i++;
	u8Sendbuf[i] = 0x50U;     			//发送功能码
	i++;
	u8Sendbuf[i] = 0x01U;				//返回字节个数(寄存器长度为2字节)
	i++;
	u8Sendbuf[i] = 0x00U;				//返回字节个数(寄存器长度为2字节)
	i++;
	u8Sendbuf[i] = (U8)blFg;
	i++;

	u16Crc = crc16_ccitt_xmodem(&u8Sendbuf[3], (i-3U), 0);

	u8Sendbuf[i] = (U8)(u16Crc & 0xFFU);
	i++;
	u8Sendbuf[i] = (U8)(u16Crc>>8);
	i++;

	u8Sendbuf[i] = 0x0DU;
	i++;
	u8Sendbuf[i] = 0x0AU;
	i++;

	//Uart_Dma_Send((GetMsgVal(MBMS_SAM_SIG_ID_UPDATE_STATE) - CCU_UPGRADE_BY_DBG8_UART), i , u8Sendbuf);
	Uart_Dma_Send(DBG_UART, i , u8Sendbuf);
}

/******************************************************************************
* 名  	称:  UsartUpdataFunc
* 功  	能:  升级主程序
* 入口参数:  无
* 出口参数:  无
*******************************************************************************/

void UsartUpdataFunc(void)
{
	__BSS(SRAM_OC) static U8 u8DataBuf[UPDATE_MAX_DATALEN] = {0};
	BOOL blRet = FALSE;
	U16 u16DataType = 0;
	U16 u16DataCnt = 0;
	U16 u16DataLen = 0;
	U16 u16MsgNo = 0;
	U32 u32GetTotalPackCrc = 0;
	static U32 u32CalTotalPackCrc = 0;
	static U32 u32CalTotalBytes = 0;
	//S32 s32BorNo = 0;
	//S32 s32BmsNum = 0;

	u16DataType = (U16)g_Usart_2_CommBuf.u8Recvbuf[6] | (((U16)g_Usart_2_CommBuf.u8Recvbuf[7])<<8);

	//进行升级步骤判断，一次升级过程只能依次处理完整的升级流程
	if (u16DataType < enumUsartPreUpdataStep)
	{
		return;
	}

	enumUsartPreUpdataStep = (USART_BOOT_STEP_CMD)u16DataType;//更新升级步骤

	switch(u16DataType)
	{
		case USART_UPDATE_REQUEST_CMD:		//升级请求
			if((g_Usart_2_CommBuf.u8Recvbuf[10] == g_boot_version[0])
			&& (g_Usart_2_CommBuf.u8Recvbuf[11] == g_boot_version[1])
			&& (g_Usart_2_CommBuf.u8Recvbuf[12] == g_boot_version[2])
			&& (g_Usart_2_CommBuf.u8Recvbuf[13] == g_boot_version[3])
			&& (g_Usart_2_CommBuf.u8Recvbuf[14] == g_boot_version[4]))
			{
				BOOT_ENTER_REPLY();
			}
			else
			{
				BOOT_DISENTER_REPLY();
			}
		break;

		case USART_UPDATE_READY_CMD:			//升级准备
//			blRet = Hal_Norflash_Init();

//			if(blRet == TRUE)
//			{
//				blRet = ExtFlashClrBackupApp();
//				if(blRet != TRUE)
//				{
//					break;
//				}
//			}
//			else
//			{
//				break;
//			}

			blRet = TRUE;

			vTaskDelay(100);

			u32CalTotalPackCrc = 0xFFFFFFFFU;
			u32CalTotalBytes = 0;

		break;

		case USART_UPDATE_BEGIN_CMD:			//起始包
		case USART_UPDATE_MIDDLE_CMD:			//中间包
		case USART_UPDATE_END_CMD:				//结束包

			u16DataLen = ((U16)g_Usart_2_CommBuf.u8Recvbuf[4] | (((U16)g_Usart_2_CommBuf.u8Recvbuf[5])<<8)) - 4U;

			u16MsgNo = (U16)g_Usart_2_CommBuf.u8Recvbuf[8] | (((U16)g_Usart_2_CommBuf.u8Recvbuf[9])<<8);

			if(u16DataLen <= 256U)
			{
				for(u16DataCnt = 0; u16DataCnt < u16DataLen; u16DataCnt++)
				{
					/*+10是因为在数据包前段含有10个其他信息：
					帧头标志（2字节）、目标地址（1字节）、命令（1字节）、
					数据长度（2字节）、升级包类型（2字节）、升级包计数器（2字节）*/
					u8DataBuf[u16DataCnt] = g_Usart_2_CommBuf.u8Recvbuf[u16DataCnt+10U];
				}
			}

			blRet = UpdataFlashFunc(u8DataBuf, u16DataLen, u16MsgNo-1U);

			u32CalTotalBytes += u16DataLen;//累计收到的固件包大小

			//计算累计数据包的校验值
			u32CalTotalPackCrc = compute_crc32(u8DataBuf, u16DataLen, u32CalTotalPackCrc);
		break;

		case USART_UPDATE_CHECKSUM_CMD:		//固件文件校验码包

			u32GetTotalPackCrc = (((U32)g_Usart_2_CommBuf.u8Recvbuf[10])<<24)
								| (((U32)g_Usart_2_CommBuf.u8Recvbuf[11])<<16)
								| (((U32)g_Usart_2_CommBuf.u8Recvbuf[12])<<8)
								| ((U32)g_Usart_2_CommBuf.u8Recvbuf[13]);

			if((~u32CalTotalPackCrc) == u32GetTotalPackCrc)
			{
				//升级成功后，写入APP完整的标识
				if(TRUE == WriteAppInterFg2Flash(str_APP_INTEG_FLAG_ADDR, u32CalTotalBytes))
				{
					if (TRUE == CheckExtFlashAppIntegFg())
					{
						blRet = TRUE;
					}
				}
			}

		break;

		default:
			//
		break;
	}

	if((U16)USART_UPDATE_REQUEST_CMD != u16DataType)
	{
		BOOT_UPDATE_ACK(blRet);

		if(((U16)USART_UPDATE_CHECKSUM_CMD == u16DataType)
			&& (blRet == TRUE))
		{
			TcpClose();
			if (NULL != g_psHMI_data)
			{
				g_psHMI_data->bRestore_flag = TRUE;
			}
			vTaskDelay(2000);
			SystemResetFunc();//重启
		}
	}
}
