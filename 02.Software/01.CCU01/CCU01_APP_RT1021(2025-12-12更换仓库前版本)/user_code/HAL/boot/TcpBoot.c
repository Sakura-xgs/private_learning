#include "FreeRTOS.h"
#include "semphr.h"
#include "TcpBoot.h"
#include "boot.h"
#include "SignalManage.h"
#include "string.h"
#include "portmacro.h"
#include "hal_eth.h"
#include "queue.h"
#include "task.h"
#include "sockets.h"
#include "flash_data_IF.h"
#include "tcp_client.h"
#include "uart_comm.h"
#include "secc_logandupgrade.h"
#include "uart_boot.h"
#include "hmi_IF.h"

//#define TCP_UPDATA_BUF_LEN 256

static TCP_BOOT_STEP_CMD g_enumTcpPreUpdataStep = TCP_UPDATE_UNKNOW_CMD;
SemaphoreHandle_t TCP_Updata_Binary_Semaphore = NULL;
UpdateTypeDef g_Updata_CommBuf;
static BOOL g_blTcpUpdataFuncEnFg = FALSE;

static BOOL blDetcetTCPUpdataStep(TCP_BOOT_STEP_CMD RecStep);
static void TCP_BOOT_ENTER_REPLY(BOOL blFg);
static void TCP_BOOT_UPDATE_ACK(BOOL blFg);
/**
 * @brief 判断CCU升级步骤是否按照正确流程
 * @param RecStep:接收到的PCU传来的当前流程
 */
static BOOL blDetcetTCPUpdataStep(TCP_BOOT_STEP_CMD RecStep)
{
    BOOL blRet = TRUE;

    switch (g_enumTcpPreUpdataStep)
    {
        case TCP_UPDATE_UNKNOW_CMD:
            if ((TCP_UPDATE_REQUEST_CMD < RecStep)
			|| (g_Mbms_sam_update_state != NO_UPGRADE))
            {
                blRet = FALSE;
            }
            break;

        case TCP_UPDATE_REQUEST_CMD:
            if (TCP_UPDATE_READY_CMD < RecStep)
            {
                blRet = FALSE;
            }
            break;

        case TCP_UPDATE_READY_CMD:
            if (TCP_UPDATE_BEGIN_CMD < RecStep)
            {
                blRet = FALSE;
            }
            break;

        case TCP_UPDATE_BEGIN_CMD:
            if (TCP_UPDATE_MIDDLE_CMD < RecStep)
            {
                blRet = FALSE;
            }
            break;

        case TCP_UPDATE_MIDDLE_CMD:
            if (TCP_UPDATE_END_CMD < RecStep)
            {
                blRet = FALSE;
            }
            break;

        case TCP_UPDATE_END_CMD:
            if (TCP_UPDATE_CHECKSUM_CMD < RecStep)
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

BOOL blTcpCheckUpdateMsg(void)
{
	BOOL blRet = FALSE;
	U16 u16DataType = 0;
	S32 s32UpdateType = 0;

	s32UpdateType = g_Mbms_sam_update_state;
	//(void)GetSigVal(CCU_SAM_SIG_ID_UPDATE_STATE, &s32UpdateType);
	if((s32UpdateType != NO_UPGRADE) && (s32UpdateType != CCU_TCP_UPGRADE))
	{
		return blRet;
	}


	u16DataType = (U16)g_Updata_CommBuf.u8Recvbuf[10] | (((U16)g_Updata_CommBuf.u8Recvbuf[9])<<8);

	//判断升级报文是否按照升级流程要求
	if(TRUE != blDetcetTCPUpdataStep((TCP_BOOT_STEP_CMD)u16DataType))
	{
		return blRet;
	}

	blRet = TRUE;

	if(FALSE == g_blTcpUpdataFuncEnFg)
	{
		if ((FALSE == CheckChargingStatus(GUN_A)) && (FALSE == CheckChargingStatus(GUN_B)))
		{
			g_blTcpUpdataFuncEnFg = TRUE;

			//(void)SetSigVal(CCU_SAM_SIG_ID_UPDATE_STATE, CCU_TCP_UPGRADE);
			g_Mbms_sam_update_state = CCU_TCP_UPGRADE;

			g_sGun_data[GUN_A].eGun_special_status = STA_UPDATE;//通知充电逻辑部分
			g_sGun_data[GUN_B].eGun_special_status = STA_UPDATE;//通知充电逻辑部分

			ResumeNormalUpdataTaskFunc();
		}
		else
		{
			blRet = FALSE;
		}
	}

	return blRet;
}

static void TCP_BOOT_ENTER_REPLY(BOOL blFg)
{
	//S32 SendReturnLen = 0;
	U16 i = 0, j = 0;
	U8 u8Sendbuf[16] = {0xAA,0x55,0x65,0x80,0x28,0x00,0x00,0x00,0x06,0x43,0x43,0x55,0x48,0x49,0, 0};//CCU01

	u8Sendbuf[i] = 0xAA;
	i++;
	u8Sendbuf[i] = 0x55;
	i++;
	u8Sendbuf[i] = g_sStorage_data.ucPile_num;//桩编号;
	i++;
	u8Sendbuf[i] = 0x80;
	i++;
	u8Sendbuf[i] = 0x28;
	i++;
	u8Sendbuf[i] = g_Updata_CommBuf.u8Recvbuf[5];
	i++;
	u8Sendbuf[i] = g_Updata_CommBuf.u8Recvbuf[6];
	i++;
	u8Sendbuf[i] = 0x00;
	i++;
	u8Sendbuf[i] = 0x06;
	i++;
	//发送回应数据包
	for(j = 0; j < 5U; j++)//取版本信息前5个byte
	{
		u8Sendbuf[i] = g_boot_version[j];
		i++;
	}

	u8Sendbuf[i] = (U8)blFg;
	i++;

	if (-1 != sockfd)
	{
		//SendReturnLen = send(sockfd,u8Sendbuf,i,0);
		(void)TcpSend(u8Sendbuf, i);
//		if(SendReturnLen != i)
//		{
//			my_printf(USER_ERROR,"update send fail!/n");
//		}
	}
}

static void TCP_BOOT_UPDATE_ACK(BOOL blFg)
{
	//S32 SendReturnLen = 0;
	U16 i = 0, j = 0;
	U8 u8Sendbuf[15] = {0xAA,0x55,0x65,0x80,0x28,0x00,0x00,0x00,0x06,0x43,0x43,0x55,0x48,0x49,0x00};//CCU01

	u8Sendbuf[i] = 0xAA;
	i++;
	u8Sendbuf[i] = 0x55;
	i++;
	u8Sendbuf[i] = g_sStorage_data.ucPile_num;//桩编号;
	i++;
	u8Sendbuf[i] = 0x80;
	i++;
	u8Sendbuf[i] = 0x28;
	i++;
	u8Sendbuf[i] = g_Updata_CommBuf.u8Recvbuf[5];
	i++;
	u8Sendbuf[i] = g_Updata_CommBuf.u8Recvbuf[6];
	i++;
	u8Sendbuf[i] = 0x00;
	i++;
	u8Sendbuf[i] = 0x06;
	i++;
	//发送回应数据包
	for(j = 0; j < 5U; j++)//版本信息置0
	{
		u8Sendbuf[i] = 0;
		i++;
	}

	u8Sendbuf[i] = (U8)blFg;
	i++;

	if (-1 != sockfd)
	{
//		SendReturnLen = send(sockfd,u8Sendbuf,i,0);
//		if(SendReturnLen != i)
//		{
//			my_printf(USER_ERROR,"update send fail!/n");
//		}
		(void)TcpSend(u8Sendbuf, i);
	}
}

/******************************************************************************
* 名  	称:  TCPUpdataFunc
* 功  	能:  升级主程序
* 入口参数:  无
* 出口参数:  无
*******************************************************************************/
void TcpUpdataFunc(void)
{
	BOOL blRet = FALSE;
	U16 u16DataType = 0;
	U16 u16DataCnt = 0;
	U16 u16DataLen = 0;
	U16 u16MsgNo = 0;
	U8 u8DataBuf[256] = {0};
	U32 u32GetTotalPackCrc = 0;
	static U32 u32CalTotalPackCrc = 0;
	//static U32 u32CalTotalPackCrcNot = 0;
	//BaseType_t err = pdFALSE;
	static U32 u32CalTotalBytes = 0;

	u16DataType = (U16)g_Updata_CommBuf.u8Recvbuf[10] | (((U16)g_Updata_CommBuf.u8Recvbuf[9])<<8);

	//进行升级步骤判断，一次升级过程只能依次处理完整的升级流程
	if (u16DataType < g_enumTcpPreUpdataStep)
	{
		return;
	}

	g_enumTcpPreUpdataStep = (TCP_BOOT_STEP_CMD)u16DataType;//更新升级步骤

	switch(u16DataType)
	{
		case TCP_UPDATE_REQUEST_CMD:	//升级请求
			if ((g_Updata_CommBuf.u8Recvbuf[13] == g_boot_version[0])
				&& (g_Updata_CommBuf.u8Recvbuf[14] == g_boot_version[1])
				&& (g_Updata_CommBuf.u8Recvbuf[15] == g_boot_version[2])
				&& (g_Updata_CommBuf.u8Recvbuf[16] == g_boot_version[3])
				&& (g_Updata_CommBuf.u8Recvbuf[17] == g_boot_version[4]))
			{
				TCP_BOOT_ENTER_REPLY(TRUE);
			}
			else
			{
				TCP_BOOT_ENTER_REPLY(FALSE);
			}
		break;

		case TCP_UPDATE_READY_CMD:	//升级准备

			//blRet = Hal_Norflash_Init();
			//if(blRet == TRUE)
			//{
				//blRet = ExtFlashClrBackupApp();
				//if(blRet != TRUE)
				//{
				//	break;
				//}
			//}
			//else
			//{
			//	break;
			//}

			blRet = TRUE;
			vTaskDelay(100);
			u32CalTotalPackCrc = 0xFFFFFFFFU;//CRC32校验初始值
			u32CalTotalBytes = 0;
		break;

		case TCP_UPDATE_BEGIN_CMD:		//起始包
		case TCP_UPDATE_MIDDLE_CMD:		//中间包
		case TCP_UPDATE_END_CMD:		//结束包
			//更新数据字节数 = 上报的长度 - 协议数据（9字节） - 数据包标识（4字节）
			u16DataLen = g_Updata_CommBuf.u16RecvDataCnt -9U - 4U;

			u16MsgNo = (U16)g_Updata_CommBuf.u8Recvbuf[12] | (((U16)g_Updata_CommBuf.u8Recvbuf[11])<<8);

			for(u16DataCnt = 0; u16DataCnt < u16DataLen; u16DataCnt++)
			{
				/*+13是因为在数据包前段含有13个其他信息：
				帧头标志（2字节）、目标地址（1字节）、命令（2字节）、帧id(2字节)
				数据长度（2字节）、升级包类型（2字节）、升级包计数器（2字节）*/
				u8DataBuf[u16DataCnt] = g_Updata_CommBuf.u8Recvbuf[u16DataCnt+13U];
			}

			if((u16MsgNo > 0U) && (u16DataLen <= 256U))
			{
				blRet = UpdataFlashFunc(u8DataBuf, u16DataLen, u16MsgNo-1U);

				u32CalTotalBytes += u16DataLen;//累计收到的固件包大小

				g_sMsg_control.sComm_Unans.ucHeart_unansNum = 0U;//收到一帧数据给心跳清0 ，防止重新登录

				//计算累计数据包的校验值
				u32CalTotalPackCrc = compute_crc32(u8DataBuf, u16DataLen, u32CalTotalPackCrc);
			}
			else
			{
				blRet = FALSE;
			}

		break;

		case TCP_UPDATE_CHECKSUM_CMD:		//固件文件校验码包
			u32GetTotalPackCrc = (((U32)g_Updata_CommBuf.u8Recvbuf[13])<<24)
									| (((U32)g_Updata_CommBuf.u8Recvbuf[14])<<16)
									| (((U32)g_Updata_CommBuf.u8Recvbuf[15])<<8)
									| ((U32)g_Updata_CommBuf.u8Recvbuf[16]);

			if((~u32CalTotalPackCrc) == u32GetTotalPackCrc)
			{
				//g_strM2SUpdataType.u32RecTotalPackCrc = u32CalTotalPackCrc;
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
		break;
	}

	if(TCP_UPDATE_REQUEST_CMD != (TCP_BOOT_STEP_CMD)u16DataType)
	{
		TCP_BOOT_UPDATE_ACK(blRet);

		if((TCP_UPDATE_CHECKSUM_CMD == (TCP_BOOT_STEP_CMD)u16DataType) && (blRet == TRUE))
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

void TcpUpdataTimeOutFunc(void)
{
	(void)SetSigVal(CCU_SAM_SIG_ID_UPDATE_STATE, NO_UPGRADE);
	(void)SetSigVal(SIGNAL_STATUS_UPDATA, FALSE);

	g_enumTcpPreUpdataStep = TCP_UPDATE_UNKNOW_CMD;
	g_blTcpUpdataFuncEnFg = FALSE;
	g_Mbms_sam_update_state = NO_UPGRADE;

	g_sGun_data[GUN_A].eGun_special_status = STA_NORMAL;//通知充电逻辑部分
	g_sGun_data[GUN_B].eGun_special_status = STA_NORMAL;//通知充电逻辑部分

}

