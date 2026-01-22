#include "fsl_flexcan.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "hal_can_IF.h"
#include "semphr.h"
#include "hal_eth.h"
#include "sockets.h"
#include "boot.h"
#include "secc_logandupgrade.h"
#include "hal_can.h"
#include "tcp_client.h"
#include "TcpBoot.h"
#include "uart_comm.h"
#include "event_groups.h"

__BSS(SRAM_OC) SECC_LogAndUpGrade_t g_Secc_LogAndUpgrade[GUN_MAX_NUM];
__BSS(SRAM_OC) UpdateTypeDef g_SeccUpgrade_CommBuf[GUN_MAX_NUM];
static __BSS(SRAM_OC) FirmwareHeader_t FirmWare_Header;

static U8 u8SeccSendRet[GUN_MAX_NUM] = {(U8)FALSE,(U8)FALSE};//这个类型是U8类型，不要改为BOOL
static SECC_PacketStepTypeDef SECC_PacketStep[GUN_MAX_NUM] = {CCU_UNKNOW_CMD};
static SECC_UpgradeCmd_t SECC_Upgrade_Cmd[GUN_MAX_NUM] = {Upgrade_Unknow};
static SECC_UPGRADE_STEP_CMD g_enumSeccPreUpgradeStep = SECC_UPGRADE_UNKNOW_CMD;
static SECC_ReagLog_t g_Secc_sam_ReadLog_state[GUN_MAX_NUM] = {NO_READLOG};
static BOOL g_blSeccUpdateFuncEnFg = FALSE;
static BOOL g_blSeccReadLogFuncEnFg[GUN_MAX_NUM] = {FALSE,FALSE};
static EventGroupHandle_t SECC_SendEventHandle = NULL;
static const U8 SECC_SEND_EVENT[GUN_MAX_NUM] = {0x01U << 0,0x01U << 1};//0x01 << 0,0x01 << 1
SECC_BinarySemaphore_t SECC_BinarySemaphore[GUN_MAX_NUM] = {NULL};
//static SECC_Version_t SECC_Version[GUN_MAX_NUM];
CCU_READ_LOG_CMD SECC_ReadLog_Cmd[GUN_MAX_NUM] = {Read_Log_Unknow};
UPGRADE_TYPE g_Secc_sam_upgrade_state = NO_UPGRADE;
TaskHandle_t SeccA_LogAndUpgradeTask_Handler = NULL,SeccB_LogAndUpgradeTask_Handler = NULL;

U8 SendMessageIdAndGunId[2][3] = {0};

//初始化两个CAN缓存
__BSS(SRAM_OC) CANBuffer_t CanBufferGun[2] = {0};

void CANBuffer_Init(CANBuffer_t *buffer)
{
    (void)memset(buffer->buffer, 0, sizeof(buffer->buffer));
    buffer->writeIndex = 0;
    buffer->readIndex = 0;
    buffer->count = 0;
}

//向指定缓存写入CAN数据
BaseType_t CANCache_Write(CANBuffer_t *buffer, const U8 *data, U32 id, U8 length)
{
    if (length > CAN_DATA_SIZE)
    {
        return pdFALSE; //数据长度超过缓存大小
    }

	if (buffer->count >= CAN_BUFFER_DEPTH)
	{
		return pdFALSE; //缓存已满
	}

	//填充CAN消息
	buffer->buffer[buffer->writeIndex].id = id;
	buffer->buffer[buffer->writeIndex].length = length;
	(void)memcpy(buffer->buffer[buffer->writeIndex].data, data, length);

	//更新索引
	buffer->writeIndex = (buffer->writeIndex + 1U) % CAN_BUFFER_DEPTH;
	buffer->count++;

	return pdTRUE;
}

//从指定缓存读取CAN数据
static BaseType_t CANCache_Read(CANBuffer_t *buffer, U8 *data, U32 *id, U8 *length)
{
	if (buffer->count == 0U)
	{
		return pdFALSE; //缓存为空
	}

	//读取CAN消息
	*id = buffer->buffer[buffer->readIndex].id;
	*length = buffer->buffer[buffer->readIndex].length;
	(void)memcpy(data, buffer->buffer[buffer->readIndex].data, *length);

	//更新索引
	buffer->readIndex = (buffer->readIndex + 1U) % CAN_BUFFER_DEPTH;
	buffer->count--;

	return pdTRUE;
}

//获取缓存中可用消息数量
static U8 CANCache_GetCount(const CANBuffer_t *buffer)
{
	U8 count = buffer->count;

	return count;
}

static SECC_UpgradeCmdFun_t SECC_UpgradeCmdFun[GUN_MAX_NUM] =
{
	{GUN_A, CAN1,&SeccUpgradeReq,&SeccUpgradeConfig,&SeccUpgradeFileInfo,&SeccUpgradeProgram,&SeccUpgradeVerify,&SeccUpgradeReboot},
	{GUN_B, CAN2,&SeccUpgradeReq,&SeccUpgradeConfig,&SeccUpgradeFileInfo,&SeccUpgradeProgram,&SeccUpgradeVerify,&SeccUpgradeReboot},
};
static SuspendSeccLogAndUpgradeFuncPtr SuspendSeccLogAndUpgradeFun[2] = {&SuspendSeccALogAndUpgradeTaskFunc, &SuspendSeccBLogAndUpgradeTaskFunc};
static ResumeSeccLogAndUpgradeFuncPtr ResumeSeccLogAndUpgradeFun[2] = {&ResumeSeccALogAndUpgradeTaskFunc, &ResumeSeccBLogAndUpgradeTaskFunc};

static U8 ReadSeccChecksum(const U8 *buf,U32 len)
{
	 U32 i;
	 U8 checksum=0xA5;
	 for(i=0;i<len;i++)
	 {
		 if(i != 3)//第3位是检验位，校验的时候把这一位去掉了
		 {
			 checksum^=buf[i];
		 }
	 }
	 return checksum;
}

static U32 FirmwareHeaderChecksum(const U32 *p,U32 len)
{
	U32 checksum = 0;

	while(len > 0U)
	{
		len-=4U;
		checksum^=*p++;
	}

	return ~checksum;
}

static void TCP_LogAndUpgradeReply(const U8 *SeccData,uint16_t index,U8 cmd,U16 messageid,U8 GUN_ID,U16 len)
{
	if((SeccData != NULL) && (len < 256U))
	{
		U16 i = 0;
		U8 Sendbuf[280];//log数据帧长度是240 + 7

		Sendbuf[i] = 0xAA;
		i++;
		Sendbuf[i] = 0x55;
		i++;
		Sendbuf[i] = g_sStorage_data.ucPile_num;//桩编号
		i++;
		Sendbuf[i] = 0x00;
		i++;
		Sendbuf[i] = cmd;
		i++;
		Sendbuf[i] = (U8)((messageid & 0xFF00U) >> 8);//message id
		i++;
		Sendbuf[i] = (U8)(messageid & 0xFFU);//message id
		i++;
		Sendbuf[i] = (U8)(((len+3U) & 0xFF00U) >> 8);
		i++;
		Sendbuf[i] = (U8)((len+3U) & 0xFFU);
		i++;
		Sendbuf[i] = (2U * g_sStorage_data.ucPile_num) - 1U + (U8)GUN_ID;//把CCU用的0-1的枪编号转换成PCU用的1-8的枪编号
		i++;
		if(index != 0U)
		{
			Sendbuf[i] = (U8)((index & 0xFF00U) >> 8);
			i++;
			Sendbuf[i] = (U8)(index & 0xFFU);
			i++;
		}

		for(U16 j = 0;j < len;j++)
		{
			Sendbuf[i] = SeccData[j];
			i++;
		}

		if(sockfd != -1)
		{
			if(i < 260U)
			{
				//(void)send(sockfd,Sendbuf,i,0);
				(void)TcpSend(Sendbuf, i);
			}
		}
	}
}

static void SECC_READLOG_ACK(BOOL blFg,U8 GUN_ID,U16 u16LogPageNum)
{
	U16 i = 0;
	U8 u8Sendbuf[15] = {0};

	u8Sendbuf[i] = 0xAA;
	i++;
	u8Sendbuf[i] = 0x55;
	i++;
	u8Sendbuf[i] = g_sStorage_data.ucPile_num;//桩编号;
	i++;
	u8Sendbuf[i] = 0x80;
	i++;
	u8Sendbuf[i] = 0x30;
	i++;
	u8Sendbuf[i] = g_Secc_LogAndUpgrade[GUN_ID].MessageId[0];
	i++;
	u8Sendbuf[i] = g_Secc_LogAndUpgrade[GUN_ID].MessageId[1];
	i++;
	u8Sendbuf[i] = 0x00;
	i++;
	u8Sendbuf[i] = 0x03;
	i++;

	u8Sendbuf[i] = (U8)blFg;
	i++;
	u8Sendbuf[i] = (U8)((u16LogPageNum & 0xFF00U) >> 8);
	i++;
	u8Sendbuf[i] = (U8)(u16LogPageNum & 0xFFU);
	i++;

	if (-1 != sockfd)
	{
		//(void)send(sockfd,u8Sendbuf,i,0);
		(void)TcpSend(u8Sendbuf, i);
	}
	else
	{
		vTaskDelay(10);
	}
}

static void SECC_UPDATE_ACK(BOOL blFg)
{
	U16 i = 0,j = 0;
	U8 u8Sendbuf[15] = {0};

	u8Sendbuf[i] = 0xAA;
	i++;
	u8Sendbuf[i] = 0x55;
	i++;
	u8Sendbuf[i] = g_sStorage_data.ucPile_num;//s32BoardIpAddr;
	i++;
	u8Sendbuf[i] = 0x80;
	i++;
	u8Sendbuf[i] = 0x29;
	i++;
	u8Sendbuf[i] = g_Secc_LogAndUpgrade[0].MessageId[0];
	i++;
	u8Sendbuf[i] = g_Secc_LogAndUpgrade[0].MessageId[1];
	i++;
	u8Sendbuf[i] = 0x00;
	i++;
	u8Sendbuf[i] = 0x06;
	i++;
	//发送回应数据包
	for(j = 0U; j < 5U; j++)//取版本信息前5个byte
	{
		u8Sendbuf[i] = 0;
		i++;
	}

	u8Sendbuf[i] = (U8)blFg;
	i++;

	if (-1 != sockfd)
	{
		//(void)send(sockfd,u8Sendbuf,i,0);
		(void)TcpSend(u8Sendbuf, i);
	}
	else
	{
		vTaskDelay(10);
	}
}

/**
  * @brief 与SECC的命令交互包
  * @param ReadLogReq:命令帧
  * @param GUN_ID:枪ID
  * @param base:can号
  */
static U8 SeccPacketAlternatelyProcess(const U8 * ReadLogReq,U8 GUN_ID,CAN_Type *base)
{
	if((ReadLogReq == NULL) && (base == NULL))
	{
		return (U8)FALSE;
	}

	BaseType_t err = pdTRUE;
	flexcan_frame_t Frame = {0};

	U8 CanData[CAN_DATA_SIZE]={0};
	U8 CanDataCount = 0;
	CANBuffer_t *bufferToProcess = NULL;
	U32 SeccCanId;
	U8 SeccCanDatalength;

	switch(SECC_PacketStep[GUN_ID])
	{
		case CCU_UNKNOW_CMD:

			break;
		case CCU_HANDSHAKE_REQ:

			Frame.id     = FLEXCAN_ID_EXT(CCU_TO_SECC_Cmd_ID);
			Frame.format = (U8)kFLEXCAN_FrameFormatExtend;
			Frame.type   = (U8)kFLEXCAN_FrameTypeData;
			Frame.length = (U8)8;
			Frame.dataByte0 = 0x10;//handshake req
			Frame.dataByte1 = ReadLogReq[1];//msglen
			Frame.dataByte2 = 0x00;
			Frame.dataByte3 = (ReadLogReq[1] + 6U)/7U;//pack num
			Frame.dataByte4 = 0xFF;
			Frame.dataByte5 = ReadLogReq[2];//msg cmd
			Frame.dataByte6 = 0x00;
			Frame.dataByte7 = 0x00;

			g_Secc_LogAndUpgrade[GUN_ID].CanRecFlag=0;

			CanBufferGun[GUN_ID].readIndex = 0;
			CanBufferGun[GUN_ID].writeIndex = 0;
			CanBufferGun[GUN_ID].count = 0;

			(void)SendCANData(base,&Frame);

			SECC_PacketStep[GUN_ID] = SECC_HANDSHAKE_ACK;

			break;
		case SECC_HANDSHAKE_ACK:
			err = xSemaphoreTake(SECC_BinarySemaphore[GUN_ID].ReqBinarySemaphore, SECC_PACKET_WAITTIME);//毫秒

			if(pdTRUE == err)
			{
				if(g_Secc_LogAndUpgrade[GUN_ID].CanRecFlag == 1U)
				{
					CanDataCount = CANCache_GetCount(&CanBufferGun[GUN_ID]);
					//检查缓存有数据
					if (CanDataCount > 0U)
					{
						bufferToProcess = &CanBufferGun[GUN_ID];

						if (bufferToProcess != NULL)
						{
							for(U8 i = 0;i < CanDataCount;i++)
							{
								if (pdTRUE == CANCache_Read(bufferToProcess, CanData, &SeccCanId, &SeccCanDatalength))
								{
									//处理CAN数据
									if(CanData[0] == 0x11U)
									{
										SECC_PacketStep[GUN_ID] = CCU_DATA_REQ;
									}
									else
									{
										my_printf(USER_ERROR, "SECC_HANDSHAKE_ACK %d: %x:\n",GUN_ID,g_Secc_LogAndUpgrade[GUN_ID].rec[0]);
										return (U8)FALSE;
									}
								}
							}
						}
					}
					else
					{
						my_printf(USER_ERROR, "SECC_HANDSHAKE_ACK timeout %d:\n",GUN_ID);
						return (U8)FALSE;
					}
				}
			}
			else
			{
				my_printf(USER_ERROR, "SECC_HANDSHAKE_ACK timeout %d:\n",GUN_ID);
				return (U8)FALSE;
			}
			break;
		case CCU_DATA_REQ:
			Frame.id     = FLEXCAN_ID_EXT(CCU_TO_SECC_Pack_ID);
			Frame.format = (U8)kFLEXCAN_FrameFormatExtend;
			Frame.type   = (U8)kFLEXCAN_FrameTypeData;
			Frame.length = (U8)8;

			g_Secc_LogAndUpgrade[GUN_ID].CCU_DataReq_Packnum = (ReadLogReq[1] + 6U)/7U;

			for(U8 i = 0;i < g_Secc_LogAndUpgrade[GUN_ID].CCU_DataReq_Packnum;i++)
			{
				Frame.dataByte0 = i + 1U;
				Frame.dataByte1 = ReadLogReq[i*7U];
				Frame.dataByte2 = ReadLogReq[((i*7U) + 1U)];
				Frame.dataByte3 = ReadLogReq[((i*7U) + 2U)];
				Frame.dataByte4 = ReadLogReq[((i*7U) + 3U)];
				Frame.dataByte5 = ReadLogReq[((i*7U) + 4U)];
				Frame.dataByte6 = ReadLogReq[((i*7U) + 5U)];
				Frame.dataByte7 = ReadLogReq[((i*7U) + 6U)];

				CanBufferGun[GUN_ID].readIndex = 0;
				CanBufferGun[GUN_ID].writeIndex = 0;
				CanBufferGun[GUN_ID].count = 0;

				(void)SendCANData(base,&Frame);
				vTaskDelay(3);
			}

			SECC_PacketStep[GUN_ID] = SECC_DATA_ACK;
			break;
		case SECC_DATA_ACK:
			err = xSemaphoreTake(SECC_BinarySemaphore[GUN_ID].ReqBinarySemaphore, 5100);//4200毫秒
			if(pdTRUE == err)
			{
				CanDataCount = CANCache_GetCount(&CanBufferGun[GUN_ID]);
				//检查缓存有数据
				if (CanDataCount > 0U)
				{
					bufferToProcess = &CanBufferGun[GUN_ID];

					if (bufferToProcess != NULL)
					{
						for(U8 i = 0;i < CanDataCount;i++)
						{
							if (pdTRUE == CANCache_Read(bufferToProcess, CanData, &SeccCanId, &SeccCanDatalength))
							{
								if((CanData[0] == 0x13U))//收到了secc的确认
								{
									SECC_PacketStep[GUN_ID] = SECC_HANDSHAKE_REQ;//原来SECC_HANDSHAKE_REQ
								}
								else if(CanData[0] == 0x10U)//收到了secc的握手
								{
									g_Secc_LogAndUpgrade[GUN_ID].SECC_DataAck_MsgLen_L = CanData[1];
									g_Secc_LogAndUpgrade[GUN_ID].SECC_DataAck_MsgLen_H = CanData[2];
									g_Secc_LogAndUpgrade[GUN_ID].SECC_DataAck_PackNum = CanData[3];

									SECC_PacketStep[GUN_ID] = CCU_HANDSHAKE_ACK;
								}
								else
								{
									my_printf(USER_ERROR, "%s:%d SECC_DATA_ACK %d:\n",__FILE__, __LINE__,GUN_ID);
									return (U8)FALSE;
								}
							}
						}
					}
				}
				else
				{
					my_printf(USER_ERROR, "%s:%d SECC_DATA_ACK %d:\n",__FILE__, __LINE__,GUN_ID);
					return (U8)FALSE;
				}
			}
			else
			{
				my_printf(USER_ERROR, "%s:%d SECC_DATA_ACK %d:\n",__FILE__, __LINE__,GUN_ID);
				return (U8)FALSE;
			}

			break;
		case SECC_HANDSHAKE_REQ:
			err = xSemaphoreTake(SECC_BinarySemaphore[GUN_ID].ReqBinarySemaphore, SECC_PACKET_WAITTIME);//毫秒

			if(pdTRUE == err)
			{
				CanDataCount = CANCache_GetCount(&CanBufferGun[GUN_ID]);
				//检查缓存有数据
				if (CanDataCount > 0U)
				{
					bufferToProcess = &CanBufferGun[GUN_ID];

					if (bufferToProcess != NULL)
					{
						for(U8 i = 0;i < CanDataCount;i++)
						{
							if (pdTRUE == CANCache_Read(bufferToProcess, CanData, &SeccCanId, &SeccCanDatalength))
							{
								if(CanData[0] == 0x10U)//握手命令
								{
									g_Secc_LogAndUpgrade[GUN_ID].SECC_DataAck_MsgLen_L = CanData[1];
									g_Secc_LogAndUpgrade[GUN_ID].SECC_DataAck_MsgLen_H = CanData[2];
									g_Secc_LogAndUpgrade[GUN_ID].SECC_DataAck_PackNum = CanData[3];
									g_Secc_LogAndUpgrade[GUN_ID].SECC_Cmd = CanData[5];

									SECC_PacketStep[GUN_ID] = CCU_HANDSHAKE_ACK;
								}
								else
								{
									my_printf(USER_ERROR, "SECC_HANDSHAKE_REQ %d:\n",GUN_ID);
									return (U8)FALSE;
								}
							}
						}
					}
				}
				else
				{
					my_printf(USER_ERROR, "SECC_HANDSHAKE_REQ %d:\n",GUN_ID);
					return (U8)FALSE;
				}

			}
			else
			{
				my_printf(USER_ERROR, "SECC_HANDSHAKE_REQ %d:\n",GUN_ID);
				return (U8)FALSE;
			}

			break;
		case CCU_HANDSHAKE_ACK:
			Frame.id     = FLEXCAN_ID_EXT(CCU_TO_SECC_Cmd_ID);
			Frame.format = (U8)kFLEXCAN_FrameFormatExtend;
			Frame.type   = (U8)kFLEXCAN_FrameTypeData;
			Frame.length = (U8)8;
			Frame.dataByte0 = 0x11;//handshake ack
			Frame.dataByte1 = 0xFF;
			Frame.dataByte2 = 0x01;
			Frame.dataByte3 = 0xFF;
			Frame.dataByte4 = 0xFF;
			Frame.dataByte5 = g_Secc_LogAndUpgrade[GUN_ID].SECC_Cmd;
			Frame.dataByte6 = 0x00;
			Frame.dataByte7 = 0x00;

			CanBufferGun[GUN_ID].readIndex = 0;
			CanBufferGun[GUN_ID].writeIndex = 0;
			CanBufferGun[GUN_ID].count = 0;

			(void)SendCANData(base,&Frame);

			g_Secc_LogAndUpgrade[GUN_ID].SECC_Cmd = 0;

			g_Secc_LogAndUpgrade[GUN_ID].RecPackIndex=0;
			SECC_PacketStep[GUN_ID] = SECC_DATA_REQ;
			break;
		case SECC_DATA_REQ:
			err = xSemaphoreTake(SECC_BinarySemaphore[GUN_ID].ReqBinarySemaphore, SECC_PACKET_WAITTIME);//毫秒

			if(pdTRUE == err)
			{
				CanDataCount = CANCache_GetCount(&CanBufferGun[GUN_ID]);
				//检查缓存有数据
				if (CanDataCount > 0U)
				{
					bufferToProcess = &CanBufferGun[GUN_ID];

					if (bufferToProcess != NULL)
					{
						for(U8 i = 0;i < CanDataCount;i++)
						{
							if (pdTRUE == CANCache_Read(bufferToProcess, CanData, &SeccCanId, &SeccCanDatalength))
							{
								// 处理CAN数据
								(void)memcpy(&g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[g_Secc_LogAndUpgrade[GUN_ID].RecPackIndex * 7U],&CanData[1],7);

								g_Secc_LogAndUpgrade[GUN_ID].RecPackIndex++;
							}
						}
					}
				}

				if(g_Secc_LogAndUpgrade[GUN_ID].RecPackIndex >= g_Secc_LogAndUpgrade[GUN_ID].SECC_DataAck_PackNum)//包接收够了
				{
					SECC_PacketStep[GUN_ID] = CCU_DATA_ACK;
				}
			}
			else
			{
				my_printf(USER_ERROR, "SECC_DATA_REQ %d:\n",GUN_ID);
				return (U8)FALSE;
			}

			break;
		case CCU_DATA_ACK:
			Frame.id     = FLEXCAN_ID_EXT(CCU_TO_SECC_Cmd_ID);
			Frame.format = (U8)kFLEXCAN_FrameFormatExtend;
			Frame.type   = (U8)kFLEXCAN_FrameTypeData;
			Frame.length = (U8)8;
			Frame.dataByte0 = 0x13;//data ack
			Frame.dataByte1 = 0xFF;
			Frame.dataByte2 = g_Secc_LogAndUpgrade[GUN_ID].SECC_DataAck_MsgLen_L;
			Frame.dataByte3 = g_Secc_LogAndUpgrade[GUN_ID].SECC_DataAck_MsgLen_H;
			Frame.dataByte4 = 0xFF;
			Frame.dataByte5 = ReadLogReq[2];
			Frame.dataByte6 = 0x00;
			Frame.dataByte7 = 0x00;

			CanBufferGun[GUN_ID].readIndex = 0;
			CanBufferGun[GUN_ID].writeIndex = 0;
			CanBufferGun[GUN_ID].count = 0;

			(void)SendCANData(base,&Frame);

			SECC_PacketStep[GUN_ID] = CCU_UNKNOW_CMD;

			return (U8)TRUE;

		default:

			break;
	}

	return WAITING;
}

static BOOL blDetcetSeccUpgradeStep(SECC_UPGRADE_STEP_CMD RecStep)
{
    BOOL blRet = TRUE;

    switch (g_enumSeccPreUpgradeStep)
    {
        case SECC_UPGRADE_UNKNOW_CMD:
            if ((SECC_UPGRADE_REQUEST_CMD < RecStep)
			|| (g_Secc_sam_upgrade_state != NO_UPGRADE))
            {
                blRet = FALSE;
            }
            break;

        case SECC_UPGRADE_REQUEST_CMD:
            if (SECC_UPGRADE_READY_CMD < RecStep)
            {
                blRet = FALSE;
            }
            break;

        case SECC_UPGRADE_READY_CMD:
            if (SECC_UPGRADE_BEGIN_CMD < RecStep)
            {
                blRet = FALSE;
            }
            break;

        case SECC_UPGRADE_BEGIN_CMD:
            if (SECC_UPGRADE_MIDDLE_CMD < RecStep)
            {
                blRet = FALSE;
            }
            break;

        case SECC_UPGRADE_MIDDLE_CMD:
            if (SECC_UPGRADE_END_CMD < RecStep)
            {
                blRet = FALSE;
            }
            break;

        case SECC_UPGRADE_END_CMD:
            if (SECC_UPGRADE_CHECKSUM_CMD < RecStep)
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

static void SuspendSeccLogAndUpgradeTaskFunc(void)
{
	if((SeccUpgradeTask_Handler != NULL)
        && (eSuspended != eTaskGetState(SeccUpgradeTask_Handler)))
	{
		vTaskSuspend(SeccUpgradeTask_Handler);
	}
}

static void ResumeSeccLogAndUpgradeTaskFunc(void)
{
	if((SeccUpgradeTask_Handler != NULL)
	&& (eSuspended == eTaskGetState(SeccUpgradeTask_Handler)))
	{
		vTaskResume(SeccUpgradeTask_Handler);
	}
}

static void SeccUpdateTimeOutFunc(void)
{
	g_enumSeccPreUpgradeStep = SECC_UPGRADE_UNKNOW_CMD;
	g_blSeccUpdateFuncEnFg = FALSE;
	(void)xEventGroupClearBits(SECC_SendEventHandle,SECC_SEND_EVENT[GUN_A]);
	(void)xEventGroupClearBits(SECC_SendEventHandle,SECC_SEND_EVENT[GUN_B]);

	g_sGun_data[GUN_A].eGun_special_status = STA_NORMAL;//通知充电逻辑部分
	g_sGun_data[GUN_B].eGun_special_status = STA_NORMAL;//通知充电逻辑部分
	g_Secc_sam_upgrade_state = NO_UPGRADE;
}
static void SeccReadLogTimeOutFunc(U8 GUN_ID)
{
	g_blSeccReadLogFuncEnFg[GUN_ID] = FALSE;
	g_sGun_data[GUN_ID].eGun_special_status = STA_NORMAL;//通知充电逻辑部分
	g_Secc_sam_ReadLog_state[GUN_ID] = (UPGRADE_TYPE)NO_READLOG;
}

BOOL blSeccCheckUpgradeMsg(U8 GUN_ID)
{
	BOOL blRet = FALSE;
	U16 u16DataType = 0;

	if((NO_UPGRADE != g_Secc_sam_upgrade_state)
	&& (SECC_UPGRADE != g_Secc_sam_upgrade_state))
	{
		return blRet;
	}

	u16DataType = (U16)g_SeccUpgrade_CommBuf[GUN_ID].u8Recvbuf[10] | (((U16)g_SeccUpgrade_CommBuf[GUN_ID].u8Recvbuf[9]) << 8U);

	//判断升级报文是否按照升级流程要求
	if(TRUE != blDetcetSeccUpgradeStep((SECC_UPGRADE_STEP_CMD)u16DataType))
	{
		return blRet;
	}

	blRet = TRUE;

	if(FALSE == g_blSeccUpdateFuncEnFg)
	{
		if ((FALSE == CheckChargingStatus(GUN_A)) && (FALSE == CheckChargingStatus(GUN_B)))
		{
			g_blSeccUpdateFuncEnFg = TRUE;
			g_Secc_sam_upgrade_state = SECC_UPGRADE;

			g_sGun_data[GUN_ID].eGun_special_status = STA_UPDATE;//通知充电逻辑部分

			ResumeSeccLogAndUpgradeTaskFunc();
			ResumeSeccLogAndUpgradeFun[GUN_A]();
			ResumeSeccLogAndUpgradeFun[GUN_B]();
		}
		else
		{
			blRet = FALSE;
		}
	}

	return blRet;
}

static U8 SeccUpgradeReq(U8 GUN_ID,CAN_Type *base)
{
	if (base == NULL)
	{
		return (U8)FALSE;
	}

	U8 u8Ret = WAITING;//等待
	U8 checksum = 0;
	U8 Req[10]={0x5A,0x0A,0x11,0xE8,0x47,0x71,0x54,0x65,0x63,0x68};

	while(SECC_Upgrade_Cmd[GUN_ID] != Upgrade_Unknow)
	{
		switch(SECC_Upgrade_Cmd[GUN_ID])
		{
			case Upgrade_Unknow:

				break;
			case Upgrade_Req_Ready:
				SECC_PacketStep[GUN_ID] = CCU_HANDSHAKE_REQ;
				SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Req;

				break;
			case Upgrade_Req:
				u8Ret = SeccPacketAlternatelyProcess(Req,GUN_ID,base);

				if(TRUE == u8Ret)
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Req_Ack;
				}
				else if(FALSE == u8Ret)
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
					u8Ret = (U8)FALSE;
				}
				else
				{}

				break;
			case Upgrade_Req_Ack:
				checksum = ReadSeccChecksum(g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec,g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[1]);
				if((0x11U == g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[2]) && (checksum == g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[3]))
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
				}
				else
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
					u8Ret = (U8)FALSE;
				}

				break;

			default:
				break;
		}
	}
	return u8Ret;
}

static U8 SeccUpgradeConfig(U8 GUN_ID,CAN_Type *base)
{
	if(base == NULL)
	{
		return (U8)FALSE;
	}

	U8 u8Ret = WAITING;//等待
	U8 checksum = 0;
	U8 Req[4]={0x5A,0x04,0x01,0xFA};

	while(SECC_Upgrade_Cmd[GUN_ID] != Upgrade_Unknow)
	{
		switch(SECC_Upgrade_Cmd[GUN_ID])
		{
			case Upgrade_Unknow:

				break;
			case Upgrade_Config_Req_Ready:
				SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Config_Req;
				SECC_PacketStep[GUN_ID] = CCU_HANDSHAKE_REQ;

				break;
			case Upgrade_Config_Req:
				u8Ret = SeccPacketAlternatelyProcess(Req,GUN_ID,base);
				if(TRUE == u8Ret)
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Config_Ack;
				}
				else if(FALSE == u8Ret)
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
					u8Ret = (U8)FALSE;
				}
				else
				{}
				break;
			case Upgrade_Config_Ack:
				checksum = ReadSeccChecksum(g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec,g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[1]);
				if((0x01U == g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[2]) && (checksum == g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[3]))
				{
					//(void)memcpy(SECC_Version[GUN_ID].version,&g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[4],6);
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
				}
				else
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
					u8Ret = (U8)FALSE;
				}
				break;

			default:
				break;
		}
	}
	return u8Ret;
}

static U8 SeccUpgradeFileInfo(U8 GUN_ID,CAN_Type *base)
{
	if(base == NULL)
	{
		return (U8)FALSE;
	}

	U8 u8Ret = WAITING;//等待
	U8 checksum = 0;
	U8 Req[16]={0};

	Req[0] = 0x5A;
	while(SECC_Upgrade_Cmd[GUN_ID] != Upgrade_Unknow)
	{
		switch(SECC_Upgrade_Cmd[GUN_ID])
		{
			case Upgrade_Unknow:

				break;

			case Upgrade_FileInfo_Req_Ready:
				Req[1] = 0x0C;
				Req[2] = 0x02;
				Req[4] = (U8)(FirmWare_Header.appCodeStartAddr & 0x000000FFU);
				Req[5] = (U8)((FirmWare_Header.appCodeStartAddr & 0x0000FF00U) >> 8);
				Req[6] = (U8)((FirmWare_Header.appCodeStartAddr & 0x00FF0000U) >> 16);
				Req[7] = (U8)((FirmWare_Header.appCodeStartAddr & 0xFF000000U) >> 24);

				Req[8] = (U8)((FirmWare_Header.appCodeSize + 0x4000U) & 0x000000FFU);
				Req[9] = (U8)(((FirmWare_Header.appCodeSize + 0x4000U) & 0x0000FF00U) >> 8) ;
				Req[10] = (U8)(((FirmWare_Header.appCodeSize + 0x4000U) & 0x00FF0000U) >> 16) ;
				Req[11] = (U8)(((FirmWare_Header.appCodeSize + 0x4000U) & 0xFF000000U) >> 24);

				checksum = ReadSeccChecksum(Req,0x0C);
				Req[3] = checksum;

				SECC_Upgrade_Cmd[GUN_ID] = Upgrade_FileInfo_Req;
				SECC_PacketStep[GUN_ID] = CCU_HANDSHAKE_REQ;
				break;
			case Upgrade_FileInfo_Req:
				u8Ret = SeccPacketAlternatelyProcess(Req,GUN_ID,base);
				if(TRUE == u8Ret)
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_FileInfo_Ack;
				}
				else if(FALSE == u8Ret)
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
					u8Ret = (U8)FALSE;
				}
				else
				{}
				break;
			case Upgrade_FileInfo_Ack:
				checksum = ReadSeccChecksum(g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec,g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[1]);

				if((0x02U == g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[2]) && (0x00 == g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[4]) && (checksum == g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[3]))
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
					u8Ret = (U8)TRUE;
				}
				else
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
					u8Ret = (U8)FALSE;
				}
				break;

			default:
				break;
		}
	}
	return u8Ret;
}

/**
 * @brief secc发送升级包指令
 * @param GUN_ID枪号:0-1. base CAN号:1-2
 */
static U8 SeccUpgradeProgram(U8 GUN_ID,CAN_Type *base)
{
	if(base == NULL)
	{
		return (U8)FALSE;
	}

	U16 u16MsgNo = 0;
	U8 checksum = 0;
	U8 u8Ret = WAITING;//等待
	U8 u8DataLen = 0;
	static U8 Req[260] = {0};

	Req[0] = 0x5A;

	while(SECC_Upgrade_Cmd[GUN_ID] != Upgrade_Unknow)
	{
		switch(SECC_Upgrade_Cmd[GUN_ID])
		{
			case Upgrade_Unknow:

				break;
			case Upgrade_Program_Req_Ready:
				u8DataLen = (U8)(g_SeccUpgrade_CommBuf[0].u16RecvDataCnt - 9U - 4U);//更新数据字节数 = 上报的长度 - 数据长度（2字节） - 数据包标识（2字节）
				u16MsgNo = (U16)g_SeccUpgrade_CommBuf[0].u8Recvbuf[12] | ((U16)(g_SeccUpgrade_CommBuf[0].u8Recvbuf[11])<<8);
				if((u8DataLen <= 0xF0U) && (u16MsgNo > 0U))
				{
					Req[1] = u8DataLen + 0x0CU;
					Req[8] = u8DataLen;
					Req[9] = 0x00;
					Req[10] = 0x00;
					Req[11] = 0x00;

					g_Secc_LogAndUpgrade[GUN_ID].SeccProgramIndex = (U32)(0xF0U*(u16MsgNo-1U)) + FirmWare_Header.appCodeStartAddr;//发给SECC的

					Req[2] = 0x03;
					Req[4] = (U8)(g_Secc_LogAndUpgrade[GUN_ID].SeccProgramIndex & 0x000000FFU);
					Req[5] = (U8)((g_Secc_LogAndUpgrade[GUN_ID].SeccProgramIndex & 0x0000FF00U) >> 8);
					Req[6] = (U8)((g_Secc_LogAndUpgrade[GUN_ID].SeccProgramIndex & 0x00FF0000U) >> 16);
					Req[7] = (U8)((g_Secc_LogAndUpgrade[GUN_ID].SeccProgramIndex & 0xFF000000U) >> 24);

					(void)memcpy(&Req[12],&g_SeccUpgrade_CommBuf[0].u8Recvbuf[13],u8DataLen);

					checksum = ReadSeccChecksum(Req,Req[1]);
					Req[3] = checksum;

					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Program_Req;
					SECC_PacketStep[GUN_ID] = CCU_HANDSHAKE_REQ;
				}
				else
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
					u8Ret = (U8)FALSE;
					my_printf(USER_ERROR, "Error:Upgrade_Program_Req_Ready %d:\n",GUN_ID);
				}

				break;
			case Upgrade_Program_Req:
				u8Ret = SeccPacketAlternatelyProcess(Req,GUN_ID,base);
				if(TRUE == u8Ret)
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Program_Ack;
				}
				else if(FALSE == u8Ret)
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
					u8Ret = (U8)FALSE;
					my_printf(USER_ERROR, "Error:Upgrade_Program_Req %d:\n",GUN_ID);
				}
				else
				{}
				break;
			case Upgrade_Program_Ack:
				checksum = ReadSeccChecksum(g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec,g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[1]);
				if((checksum == g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[3]) && (0x00U == g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[4]))//返回成功
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
					u8Ret = (U8)TRUE;
				}
				else
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
					u8Ret = (U8)FALSE;
					my_printf(USER_ERROR, "Error:Upgrade_Program_Ack %d:\n",GUN_ID);
				}
				break;

			default :
				break;
		}
	}
	return u8Ret;
}

/**
 * @brief bin文件发完后，询问secc校验码指令
 * @param GUN_ID枪号:0-1. base CAN号:1-2
 */
static U8 SeccUpgradeVerify(U8 GUN_ID,CAN_Type *base)
{
	if(base == NULL)
	{
		return (U8)FALSE;
	}

	U8 u8Ret = WAITING;
	U8 checksum=0;
	U8 Req[16]={0};

	Req[0] = 0x5A;

	while(SECC_Upgrade_Cmd[GUN_ID] != Upgrade_Unknow)
	{
		switch(SECC_Upgrade_Cmd[GUN_ID])
		{
			case Upgrade_Unknow:

				break;

			case Upgrade_Verify_Req_Ready:
				Req[1] = 0x0C;
				Req[2] = 0x05;
				//(void)memcpy(&Req[4],&FirmWare_Header.appCodeStartAddr,8);
				Req[4] = (U8)(FirmWare_Header.appCodeStartAddr & 0x000000FFU);
				Req[5] = (U8)((FirmWare_Header.appCodeStartAddr & 0x0000FF00U) >> 8);
				Req[6] = (U8)((FirmWare_Header.appCodeStartAddr & 0x00FF0000U) >> 16);
				Req[7] = (U8)((FirmWare_Header.appCodeStartAddr & 0xFF000000U) >> 24);

				Req[8] = (U8)(FirmWare_Header.appCodeSize & 0x000000FFU);
				Req[9] = (U8)((FirmWare_Header.appCodeSize & 0x0000FF00U) >> 8) ;
				Req[10] = (U8)((FirmWare_Header.appCodeSize & 0x00FF0000U) >> 16) ;
				Req[11] = (U8)((FirmWare_Header.appCodeSize & 0xFF000000U) >> 24);

				checksum = ReadSeccChecksum(Req,Req[1]);
				Req[3] = checksum;

				SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Verify_Req;
				SECC_PacketStep[GUN_ID] = CCU_HANDSHAKE_REQ;

				break;
			case Upgrade_Verify_Req:
				u8Ret = SeccPacketAlternatelyProcess(Req,GUN_ID,base);
				if(TRUE == u8Ret)
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Verify_Ack;
				}
				else if(FALSE == u8Ret)
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
					u8Ret = (U8)FALSE;
				}
				else
				{}
				break;
			case Upgrade_Verify_Ack:
				checksum = ReadSeccChecksum(g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec,g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[1]);
				if((0x05U == g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[2]) && (checksum == g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[3]))
				{
					//询问SECC检验码并和表头里存的校验码比对，一致就返回true
					g_Secc_LogAndUpgrade[GUN_ID].crc32 = ((U32)g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[7] << 24) | ((U32)g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[6] << 16) |
										  	  	  	  	 ((U32)g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[5] << 8) | (U32)g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[4];
					if(g_Secc_LogAndUpgrade[GUN_ID].crc32 == FirmWare_Header.crc32)
					{
						SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
						u8Ret = (U8)TRUE;
					}
					else
					{
						SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
						u8Ret = (U8)FALSE;
						my_printf(USER_ERROR, "Error:SECC_UPGRADE_CHECKSUM_CMD gun:%d crc32:%d\n",GUN_ID,g_Secc_LogAndUpgrade[GUN_ID].crc32);
					}
				}
				else
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
					u8Ret = (U8)FALSE;
					my_printf(USER_ERROR, "Error:Upgrade_Verify_Ack :%d\n",GUN_ID);
				}

				break;

			default :
				break;
		}
	}
	return u8Ret;
}

/**
 * @brief secc重启指令
 * @param GUN_ID枪号:0-1. base CAN号:1-2
 */
static U8 SeccUpgradeReboot(U8 GUN_ID,CAN_Type *base)
{
	if(base == NULL)
	{
		return (U8)FALSE;
	}

	U8 u8Ret = WAITING;
	U8 checksum=0;
	U8 Req[8]={0x5A,0x04,0x06,0xFD, 0, 0, 0, 0};

	while(SECC_Upgrade_Cmd[GUN_ID] != Upgrade_Unknow)
	{
		switch(SECC_Upgrade_Cmd[GUN_ID])
		{
			case Upgrade_Unknow:

				break;
			case Upgrade_Reboot_Req_Ready:
				SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Reboot_Req;
				SECC_PacketStep[GUN_ID] = CCU_HANDSHAKE_REQ;

				break;
			case Upgrade_Reboot_Req:
				u8Ret = SeccPacketAlternatelyProcess(Req,GUN_ID,base);
				if(TRUE == u8Ret)
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Reboot_Ack;
				}
				else if(FALSE == u8Ret)
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
					u8Ret = (U8)FALSE;
				}
				else
				{}
				break;
			case Upgrade_Reboot_Ack:
				checksum = ReadSeccChecksum(g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec,g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[1]);
				if((0x06U == g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[2]) && (checksum == g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[3]))
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
					u8Ret = (U8)TRUE;
				}
				else
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Unknow;
					u8Ret = (U8)FALSE;
				}

				break;

			default :
				break;
		}
	}
	return u8Ret;
}

/**
 * @brief secc升级步骤，给两个secc升级
 *
 * @param
 */
static void SeccUpgradeFunc(void)
{
	U32 u32Checksum = 0;
	SECC_UPGRADE_STEP_CMD u16DataType = 0;
	EventBits_t SeccSendEvent;

	u16DataType = (SECC_UPGRADE_STEP_CMD)((U16)g_SeccUpgrade_CommBuf[0].u8Recvbuf[10] | ((U16)g_SeccUpgrade_CommBuf[0].u8Recvbuf[9]<<8));

	//进行升级步骤判断，一次升级过程只能依次处理完整的升级流程
	if (u16DataType < g_enumSeccPreUpgradeStep)
	{
		return;
	}

	g_enumSeccPreUpgradeStep = u16DataType;//更新升级步骤

	if(u16DataType == SECC_UPGRADE_REQUEST_CMD)//升级请求
	{
		//表头校验，表头大小是64字节，包含4字节校验码
		u32Checksum = FirmwareHeaderChecksum((U32*)&g_SeccUpgrade_CommBuf[0].u8Recvbuf[13],60);
		//表头赋值，把接收到的表头数据赋值给表头结构体
		(void)memcpy(&FirmWare_Header,&g_SeccUpgrade_CommBuf[0].u8Recvbuf[13],sizeof(FirmWare_Header));

		if(u32Checksum == FirmWare_Header.firmwareHeaderChecksum)
		{
			SECC_UPDATE_ACK(TRUE);
		}
		else
		{
			SECC_UPDATE_ACK(FALSE);
			my_printf(USER_ERROR, "Error:SECC_UPGRADE_REQUEST_CMD\n");
		}
	}

	if(SECC_UPGRADE_REQUEST_CMD != u16DataType)
	{
		u8SeccSendRet[GUN_A] = (U8)FALSE;
		u8SeccSendRet[GUN_B] = (U8)FALSE;
		if(SECC_SendEventHandle != NULL)
		{
			for(U8 i = 0U;i < 2U;i++)
			{
				(void)xSemaphoreGive(SECC_BinarySemaphore[i].SeccUpgradeSend_Binary_Semaphore);
			}

			SeccSendEvent = xEventGroupWaitBits(SECC_SendEventHandle,
												SECC_SEND_EVENT[GUN_A]|SECC_SEND_EVENT[GUN_B],
												pdTRUE,
												pdTRUE,
												8000);
			if((SeccSendEvent & (SECC_SEND_EVENT[GUN_A]|SECC_SEND_EVENT[GUN_B])) == (SECC_SEND_EVENT[GUN_A]|SECC_SEND_EVENT[GUN_B]))
			{
				//两个secc都回复了
			}
			else
			{
				//事件错误
			}
		}

		if((SECC_UPGRADE_CHECKSUM_CMD == u16DataType) && (u8SeccSendRet[GUN_B] == TRUE) && (u8SeccSendRet[GUN_A] == TRUE))
		{
			vTaskDelay(500);
			for(U8 i = 0U;i < 2U;i++)
			{
				//如果校验码一致，就给SECC发送指令从bootloader跳转到app
				SECC_Upgrade_Cmd[i] = Upgrade_Reboot_Req_Ready;
				u8SeccSendRet[i] = SECC_UpgradeCmdFun[i].UpgradeReboot(SECC_UpgradeCmdFun[i].GunId,SECC_UpgradeCmdFun[i].CanBase);
				g_sGun_data[i].eGun_special_status = STA_NORMAL;//通知充电逻辑部分
			}
		}
		if ((u8SeccSendRet[GUN_A] == TRUE) && (u8SeccSendRet[GUN_B] == TRUE))
		{
			SECC_UPDATE_ACK(TRUE);
		}
		else
		{
			SECC_UPDATE_ACK(FALSE);

			my_printf(USER_ERROR, "%s:%d SECC_UPDATE_ACK GunA:%d GunB:%d\n",__FILE__, __LINE__,u8SeccSendRet[GUN_A],u8SeccSendRet[GUN_B]);

		}
	}

	return;
}

/**
 * @brief secc日志读取指令，设置secc进入日志模式
 * @param GUN_ID枪号:0-1. base CAN号:1-2
 */
static U8 SeccReadLogStart(U8 GUN_ID,CAN_Type *base)
{
	if(base == NULL)
	{
		return (U8)FALSE;
	}

	U8 u8Ret = WAITING;
	U8 checksum = 0;
	U8 ReadLogStartCmd[12] = {0x5A,0x0C,0x50,0x55,0x52,0x65,0x61,0x64,0x4C,0x6F,0x67,0x80};

	while(SECC_ReadLog_Cmd[GUN_ID] != Read_Log_Unknow)
	{
		switch(SECC_ReadLog_Cmd[GUN_ID])
		{
			case Read_Log_Unknow:

				break;
			case Read_Log_Start_Ready:
				SECC_PacketStep[GUN_ID] = CCU_HANDSHAKE_REQ;
				SECC_ReadLog_Cmd[GUN_ID] = Read_Log_Start;
				break;

			case Read_Log_Start:
				u8Ret = SeccPacketAlternatelyProcess(ReadLogStartCmd,GUN_ID,base);//请求开始读log，SECC返回g_SeccLogAndUpgradeType.SECC_DataRec[]

				if(TRUE == u8Ret)
				{
					SECC_ReadLog_Cmd[GUN_ID] = Read_Log_Start_Ack;
				}
				else if(FALSE == u8Ret)
				{
					SECC_ReadLog_Cmd[GUN_ID] = Read_Log_Unknow;
					my_printf(USER_ERROR, "Error:Read_Log_Start %d:\n",GUN_ID);
				}
				else
				{}
				break;
			case Read_Log_Start_Ack:
				checksum = ReadSeccChecksum(g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec,(U32)g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[1]);
				if(checksum == g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[3])
				{
					g_Secc_LogAndUpgrade[GUN_ID].Start_Ack_SizeL = g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[5];
					g_Secc_LogAndUpgrade[GUN_ID].Start_Ack_SizeH = g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[6];
					g_Secc_LogAndUpgrade[GUN_ID].Start_Ack_LogLength_Byte1 = g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[7];
					g_Secc_LogAndUpgrade[GUN_ID].Start_Ack_LogLength_Byte2 = g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[8];
					g_Secc_LogAndUpgrade[GUN_ID].Start_Ack_LogLength_Byte3 = g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[9];
					g_Secc_LogAndUpgrade[GUN_ID].Start_Ack_LogLength_Byte4 = g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[10];

					g_Secc_LogAndUpgrade[GUN_ID].LogSize = ((U16)g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[6] << 8) | (U16)g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[5];
					g_Secc_LogAndUpgrade[GUN_ID].LogLength = ((U32)g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[10] << 24) | ((U32)g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[9] << 16) |
															 ((U32)g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[8] << 8) | (U32)g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[7];

					vTaskDelay(100);

					g_Secc_LogAndUpgrade[GUN_ID].ReadPageIndex = 0;
					SECC_ReadLog_Cmd[GUN_ID] = Read_Log_Unknow;
				}
				else//如果校验不对就不读了，需要重新开始
				{
					SECC_ReadLog_Cmd[GUN_ID] = Read_Log_Unknow;
					u8Ret = (U8)FALSE;
					my_printf(USER_ERROR, "Error:Read_Log_Start_Ack %d:\n",GUN_ID);
				}

				break;

			default:
				break;
		}
	}
	return u8Ret;
}

/**
 * @brief 读取secc日志指令，每读一次，日志序号加1，直到全部读完
 * @param GUN_ID枪号:0-1. base CAN号:1-2
 */
static U8 SeccReadLogData(U8 GUN_ID,CAN_Type *base)
{
	if(base == NULL)
	{
		return (U8)FALSE;
	}

	U8 u8Ret = WAITING;
	U8 checksum = 0;
	U8 ReadLogDataFrame[9] = {0x5A,0x09,0x51,0x00,0x80,0x00,0x00,0x00,0x00};

	while(SECC_ReadLog_Cmd[GUN_ID] != Read_Log_Unknow)
	{
		switch(SECC_ReadLog_Cmd[GUN_ID])
		{
			case Read_Log_Unknow:

				break;
			case Read_Log_Data_Ready:
				ReadLogDataFrame[5] = (U8)(g_Secc_LogAndUpgrade[GUN_ID].ReadPageIndex & 0xFFU);
				ReadLogDataFrame[6] = (U8)((g_Secc_LogAndUpgrade[GUN_ID].ReadPageIndex & 0xFF00U) >> 8);
				ReadLogDataFrame[7] = g_Secc_LogAndUpgrade[GUN_ID].Start_Ack_SizeL;
				ReadLogDataFrame[8] = g_Secc_LogAndUpgrade[GUN_ID].Start_Ack_SizeH;

				checksum = ReadSeccChecksum(ReadLogDataFrame,(U32)ReadLogDataFrame[1]);
				ReadLogDataFrame[3] = checksum;

				SECC_PacketStep[GUN_ID] = CCU_HANDSHAKE_REQ;
				SECC_ReadLog_Cmd[GUN_ID] = Read_Log_Data;

				break;
			case Read_Log_Data:
				u8Ret = SeccPacketAlternatelyProcess(ReadLogDataFrame,GUN_ID,base);//read log data
				if(TRUE == u8Ret)
				{
					checksum = ReadSeccChecksum(g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec,(U32)g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[1]);

					if((g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[1] <= 0xF7U) && (checksum == g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[3]))//帧数据不长于F7才是正确数据
					{
						SECC_ReadLog_Cmd[GUN_ID] = Read_Log_Unknow;
					}
					else
					{
						u8Ret = (U8)FALSE;
						SECC_ReadLog_Cmd[GUN_ID] = Read_Log_Unknow;
						my_printf(USER_ERROR, "Error:Read_Log_Data1 %d:\n",GUN_ID);
					}
				}
				else if(FALSE == u8Ret)
				{
					SECC_ReadLog_Cmd[GUN_ID] = Read_Log_Unknow;
					my_printf(USER_ERROR, "Error:Read_Log_Data2 %d:\n",GUN_ID);
				}
				else
				{}
				break;

			default:
				break;
		}
	}
	return u8Ret;
}

/**
 * @brief 给secc读日志停止命令
 * @param GUN_ID枪号:0-1. base CAN号:1-2
 */
static U8 SeccReadLogStop(U8 GUN_ID,CAN_Type *base)
{
	if(base == NULL)
	{
		return (U8)FALSE;
	}

	U8 u8Ret = WAITING;
	U8 ReadLogStopFrame[12] = {0x5A,0x0C,0x52,0x5C,0x52,0x65,0x61,0x64,0x45,0x6E,0x64,0x80};

	while(SECC_ReadLog_Cmd[GUN_ID] != Read_Log_Unknow)
	{
		switch(SECC_ReadLog_Cmd[GUN_ID])
		{
			case Read_Log_Unknow:

				break;
			case Read_Log_Stop:
				u8Ret = SeccPacketAlternatelyProcess(ReadLogStopFrame,GUN_ID,base);//read log stop

				if(TRUE == u8Ret)
				{
					SECC_ReadLog_Cmd[GUN_ID] = Read_Log_Unknow;
				}
				else if(FALSE == u8Ret)
				{
					SECC_ReadLog_Cmd[GUN_ID] = Read_Log_Unknow;
				}
				else
				{}
				break;
			case Read_Log_Stop_Ack:

				break;

			default:
				break;
		}
	}
	return u8Ret;
}

/**
 * @brief SECC升级任务中循环执行的secc的升级函数，获取从以太网接收处释放的二值信号量来
 * 		  触发升级步骤，10秒收不到升级交互就挂起任务
 * @param
 */
static void SeccUpgradeFunction(void)
{
	S32 s32UpdateType = 0;
    BaseType_t err = pdFALSE;

    s32UpdateType = (S32)g_Secc_sam_upgrade_state;

    if((NULL != SECC_BinarySemaphore[0].SeccUpgrade_Binary_Semaphore) && (SECC_UPGRADE == s32UpdateType))
    {
        err = xSemaphoreTake(SECC_BinarySemaphore[0].SeccUpgrade_Binary_Semaphore, COMM_TIME_OUT_VAL);
        if(pdTRUE == err)
        {
        	SeccUpgradeFunc();
        }
		else
		{
			SeccUpdateTimeOutFunc();
			SuspendSeccLogAndUpgradeTaskFunc();
		}
    }
    else
    {
		vTaskDelay(300);
    }
}

/**
 * @brief 日志读取函数步骤，上位机先发读取日志请求，ccu给上位机响应
 * 		  日志长度，然后发送第一包日志，根据上位机响应发送后续日志
 * @param GUN_ID枪号:0-1. base CAN号:1-2
 */
static void SeccReadLogFunc(U8 GUN_ID,CAN_Type *base)
{
	if(base == NULL)
	{
		return ;
	}
	static U16 u16SendMessageIdCount = 0;
	U16 u16DataType = 0;
	U8 u8Ret[GUN_MAX_NUM] = {(U8)FALSE,(U8)FALSE};

	u16DataType = g_SeccUpgrade_CommBuf[GUN_ID].u8Recvbuf[3] | (g_SeccUpgrade_CommBuf[GUN_ID].u8Recvbuf[4]<<8);

	switch((SECC_READLOG_STEP_CMD)u16DataType)
	{
		case SECC_READLOG_REQUEST_CMD://日志请求
			SECC_ReadLog_Cmd[GUN_ID] = Read_Log_Start_Ready;
			u8Ret[GUN_ID] = SeccReadLogStart(GUN_ID,base);

			if((u8Ret[GUN_ID] == TRUE) && (g_Secc_LogAndUpgrade[GUN_ID].LogSize != 0U))
			{
				g_Secc_LogAndUpgrade[GUN_ID].LogPageNum = (U16)(g_Secc_LogAndUpgrade[GUN_ID].LogLength/g_Secc_LogAndUpgrade[GUN_ID].LogSize);
				SECC_READLOG_ACK(TRUE,GUN_ID,g_Secc_LogAndUpgrade[GUN_ID].LogPageNum);
				vTaskDelay(10);//延时后直接发log
			}
			else
			{
				SECC_READLOG_ACK(FALSE,GUN_ID,g_Secc_LogAndUpgrade[GUN_ID].LogPageNum);
				my_printf(USER_ERROR, "Error:SECC_READLOG_REQUEST_CMD %d: %d:\n",GUN_ID,g_Secc_LogAndUpgrade[GUN_ID].LogPageNum);
			}

			//break;
		case SECC_READLOG_ACK_CMD://响应日志
			if(g_Secc_LogAndUpgrade[GUN_ID].LogSize != 0U)
			{
				if((g_Secc_LogAndUpgrade[GUN_ID].LogLength/g_Secc_LogAndUpgrade[GUN_ID].LogSize) > g_Secc_LogAndUpgrade[GUN_ID].ReadPageIndex)
				{
					SECC_ReadLog_Cmd[GUN_ID] = Read_Log_Data_Ready;
					g_Secc_LogAndUpgrade[GUN_ID].ReadPageIndex++;
					u8Ret[GUN_ID] = SeccReadLogData(GUN_ID,base);

					if(u8Ret[GUN_ID] == FALSE)
					{
						my_printf(USER_ERROR, "Error:SECC_READLOG_ACK_CMD again try%d:\n",GUN_ID);
						SECC_ReadLog_Cmd[GUN_ID] = Read_Log_Data_Ready;
						u8Ret[GUN_ID] = SeccReadLogData(GUN_ID,base);
					}

					if(u8Ret[GUN_ID] == TRUE)
					{
						u16SendMessageIdCount++;
						SendMessageIdAndGunId[GUN_ID][1] = (U8)((u16SendMessageIdCount & 0xFF00U) >> 8);
						SendMessageIdAndGunId[GUN_ID][2] = (U8)(u16SendMessageIdCount & 0xFFU);

						if(g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[1] == 0xF7U)
						{

							if((g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[7] == 0U) && (g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[8] == 0U) &&
							   (g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[9] == 0U) && (g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[10] == 0U))
							{
								my_printf(USER_ERROR, "secclog error\n");
							}


							TCP_LogAndUpgradeReply(&g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[7],
							g_Secc_LogAndUpgrade[GUN_ID].ReadPageIndex,0x15,
							u16SendMessageIdCount,GUN_ID,
							(U16)(g_Secc_LogAndUpgrade[GUN_ID].SECC_DataRec[1] - 7U));
						}

					}
					else
					{
						my_printf(USER_ERROR, "Error:SECC_READLOG_ACK_CMD %d:\n",GUN_ID);
					}
				}
				if((g_Secc_LogAndUpgrade[GUN_ID].LogLength/g_Secc_LogAndUpgrade[GUN_ID].LogSize) <= g_Secc_LogAndUpgrade[GUN_ID].ReadPageIndex)//所有页都读完了，给secc发停止
				{
					SECC_PacketStep[GUN_ID] = CCU_HANDSHAKE_REQ;
					SECC_ReadLog_Cmd[GUN_ID] = Read_Log_Stop;
					u8Ret[GUN_ID] = SeccReadLogStop(GUN_ID,base);
				}
			}
			else
			{

			}
			break;

		default:
			break;
	}

	return;
}

static void SeccUpgradeSendCmd(U8 GUN_ID,CAN_Type *base)
{
	switch(g_enumSeccPreUpgradeStep)
	{
		case SECC_UPGRADE_READY_CMD:	//升级准备
			//给SECC发送升级请求
			SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Req_Ready;
			u8SeccSendRet[GUN_ID] = SECC_UpgradeCmdFun[GUN_ID].UpgradeReq(GUN_ID,base);

			if(u8SeccSendRet[GUN_ID] == TRUE)
			{
				vTaskDelay(1000);
				//获取SECC信息
				SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Config_Req_Ready;
				u8SeccSendRet[GUN_ID] = SECC_UpgradeCmdFun[GUN_ID].UpgradeConfig(GUN_ID,base);

				if(u8SeccSendRet[GUN_ID] == FALSE)//如果第一次失败了就重发一次
				{
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Config_Req_Ready;
					u8SeccSendRet[GUN_ID] = SECC_UpgradeCmdFun[GUN_ID].UpgradeConfig(GUN_ID,base);
				}
			}
			if(u8SeccSendRet[GUN_ID] == TRUE)
			{
				my_printf(USER_INFO, "UpgradeConfig success %d:\n",GUN_ID);
				//给SECC发送配置信息
				SECC_Upgrade_Cmd[GUN_ID] = Upgrade_FileInfo_Req_Ready;
				u8SeccSendRet[GUN_ID] = SECC_UpgradeCmdFun[GUN_ID].UpgradeFileInfo(GUN_ID,base);

				if(u8SeccSendRet[GUN_ID] == FALSE)
				{
					//给SECC发送配置信息
					SECC_Upgrade_Cmd[GUN_ID] = Upgrade_FileInfo_Req_Ready;
					u8SeccSendRet[GUN_ID] = SECC_UpgradeCmdFun[GUN_ID].UpgradeFileInfo(GUN_ID,base);
				}
				if(u8SeccSendRet[GUN_ID] == TRUE)
				{
					my_printf(USER_INFO, "UpgradeFileInfo true %d:\n",GUN_ID);
				}
			}

			break;

		case SECC_UPGRADE_BEGIN_CMD:	//起始包
		case SECC_UPGRADE_MIDDLE_CMD:	//中间包
		case SECC_UPGRADE_END_CMD:		//结束包
			//给SECC发送bin文件包
			SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Program_Req_Ready;
			u8SeccSendRet[GUN_ID] = SECC_UpgradeCmdFun[GUN_ID].UpgradeProgram(GUN_ID,base);

			break;

		case SECC_UPGRADE_CHECKSUM_CMD:		//固件文件校验码包
			//向SECC询问校验码
			SECC_Upgrade_Cmd[GUN_ID] = Upgrade_Verify_Req_Ready;
			u8SeccSendRet[GUN_ID] = SECC_UpgradeCmdFun[GUN_ID].UpgradeVerify(GUN_ID,base);

			break;

		default:
			break;
	}

	(void)xEventGroupSetBits(SECC_SendEventHandle,SECC_SEND_EVENT[GUN_ID]);
}
/**
 * @brief 日志读取任务中循环执行的secc的日志读取函数，获取从以太网接收处释放的二值信号量来
 * 		  触发日志读取步骤，10秒收不到交互就挂起任务
 * @param GUN_ID枪号，0-1. base CAN号，1-2
 */
static void SeccLogAndUpgradeFunction(U8 GUN_ID,CAN_Type *base)
{
	UPGRADE_TYPE UpdateType = 0;
	SECC_ReagLog_t SeccReadLog = 0;
    BaseType_t err = pdFALSE;

    SeccReadLog = g_Secc_sam_ReadLog_state[GUN_ID];
    UpdateType = g_Secc_sam_upgrade_state;

    if((NULL != SECC_BinarySemaphore[GUN_ID].SeccReadLog_Binary_Semaphore) && (READLOG == SeccReadLog))
    {
        err = xSemaphoreTake(SECC_BinarySemaphore[GUN_ID].SeccReadLog_Binary_Semaphore, COMM_TIME_OUT_VAL);
        if(pdTRUE == err)
        {
        	(void)SeccReadLogFunc(GUN_ID,base);
        }
		else
		{
			SeccReadLogTimeOutFunc(GUN_ID);
			SuspendSeccLogAndUpgradeFun[GUN_ID]();
		}
    }
    else if((NULL != SECC_BinarySemaphore[GUN_ID].SeccUpgradeSend_Binary_Semaphore) && (SECC_UPGRADE == UpdateType))
    {
    	err = xSemaphoreTake(SECC_BinarySemaphore[GUN_ID].SeccUpgradeSend_Binary_Semaphore, COMM_TIME_OUT_VAL);
    	if(pdTRUE == err)
    	{
    		SeccUpgradeSendCmd(GUN_ID,base);
    	}
    	else
    	{
    		SeccReadLogTimeOutFunc(GUN_ID);
    		SuspendSeccLogAndUpgradeFun[GUN_ID]();
    	}
    }
    else
    {
		vTaskDelay(300);
    }
}

static void SuspendSeccALogAndUpgradeTaskFunc(void)
{
	if((SeccA_LogAndUpgradeTask_Handler != NULL)
        && (eSuspended != eTaskGetState(SeccA_LogAndUpgradeTask_Handler)))
	{
		vTaskSuspend(SeccA_LogAndUpgradeTask_Handler);
	}
}

static void ResumeSeccALogAndUpgradeTaskFunc(void)
{
	if((NULL != SeccA_LogAndUpgradeTask_Handler)
	&& (eSuspended == eTaskGetState(SeccA_LogAndUpgradeTask_Handler)))
	{
		vTaskResume(SeccA_LogAndUpgradeTask_Handler);
	}
}

BOOL blSeccCheckReadLogMsg(U8 GUN_ID)
{
	BOOL blRet = FALSE;

	if((NO_READLOG != g_Secc_sam_ReadLog_state[GUN_ID])
	&& (READLOG != g_Secc_sam_ReadLog_state[GUN_ID]))
	{
		return blRet;
	}

	blRet = TRUE;

	if(FALSE == g_blSeccReadLogFuncEnFg[GUN_ID])
	{
		if ((FALSE == CheckChargingStatus(GUN_A)) && (FALSE == CheckChargingStatus(GUN_B)))
		{
			g_sGun_data[GUN_ID].eGun_special_status = STA_UPLOAD_LOG;//通知充电逻辑部分

			g_blSeccReadLogFuncEnFg[GUN_ID] = TRUE;
			g_Secc_sam_ReadLog_state[GUN_ID] = READLOG;
			ResumeSeccLogAndUpgradeFun[GUN_ID]();

		}
		else
		{
			blRet = FALSE;
		}
	}

	return blRet;
}

static void SuspendSeccBLogAndUpgradeTaskFunc(void)
{
	if((SeccB_LogAndUpgradeTask_Handler != NULL)
        && (eSuspended != eTaskGetState(SeccB_LogAndUpgradeTask_Handler)))
	{
		vTaskSuspend(SeccB_LogAndUpgradeTask_Handler);
	}
}

static void ResumeSeccBLogAndUpgradeTaskFunc(void)
{
	if((SeccB_LogAndUpgradeTask_Handler != NULL)
	&& (eSuspended == eTaskGetState(SeccB_LogAndUpgradeTask_Handler)))
	{
		vTaskResume(SeccB_LogAndUpgradeTask_Handler);
	}
}

void SeccA_LogAndUpgrade_Task(void * pvParameters)
{
	while(1)
	{
		SeccLogAndUpgradeFunction(GUN_A,CAN1);
	}
}

void SeccB_LogAndUpgrade_Task(void * pvParameters)
{
	while(1)
	{
		SeccLogAndUpgradeFunction(GUN_B,CAN2);
	}
}

/**
 * @brief SECC升级任务
 * @param
 */
void SeccUpgrade_Task(void * pvParameters)
{
	SECC_SendEventHandle = xEventGroupCreate();

	while(1)
	{
		if(g_Secc_sam_upgrade_state == SECC_UPGRADE)
		{
//			if(g_sBms_ctrl[GUN_A].sMsg2bms_ctrl[CHM].ucSend_flag == TRUE)
//			{
//				g_sBms_ctrl[GUN_A].sMsg2bms_ctrl[CHM].ucSend_flag = FALSE;
//			}
//			if(g_sBms_ctrl[GUN_B].sMsg2bms_ctrl[CHM].ucSend_flag == TRUE)
//			{
//				g_sBms_ctrl[GUN_B].sMsg2bms_ctrl[CHM].ucSend_flag = FALSE;
//			}

			SeccUpgradeFunction();
		}
		else
		{
			vTaskDelay(1000);
		}
	}
}
