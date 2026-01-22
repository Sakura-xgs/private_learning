/*
 * HW_rfid.c
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#include "hw_rfid.h"
#include "rfid.h"
#include "calculate_crc.h"
#include "hal_uart_IF.h"
#include "SignalManage.h"
#include "tcp_client_IF.h"
#include "uart_comm.h"

void HwRfidParse(const U8 *pucBuf, U16 unInput_len);

static 	HW_ReadStep_st s_HW_stReadStep = Read_Unknow;
__BSS(SRAM_OC) static RFID_data_t g_sHW_RFID = {0};

/**
 * note:将字符串转换为int类型数据
 * @param pcStr:目标字符串
 * @param ucLen：需转换的数据长度
 * @return
 */
static U32 HexStrToInt(const U8 *pcStr, U8 ucLen)
{
	CHECK_PTR_NULL(pcStr);

	if (ucLen > 8U)
	{
		return 0;
	}

    U32 uiResult = 0;

    for (U8 i = 0; i < ucLen; i += 2U) {
        U8 byte_str[3] = {pcStr[i], pcStr[i + 1U], (U8)'\0'};
        U32 uiByte = (U32)strtol(byte_str, NULL, 16);
        uiResult = (uiResult << 8) | uiByte;
    }

    return uiResult;
}

/**
 * @brief 异或校验函数
 * @param name: 校验数据
 * @param size：数据长度
 * @param checksum：校验值
 * @return
 */
static void XorChecksum(const U8 *pucData, U32 ucSize, U8 *pucChecksum)
{
	CHECK_PTR_NULL_NO_RETURN(pucData);
	CHECK_PTR_NULL_NO_RETURN(pucChecksum);

	U32 i;
    *pucChecksum = pucData[0];

    for (i = 1; i < ucSize; i++) {
        *pucChecksum = *pucChecksum ^ pucData[i];
    }
}

/**
 * @brief M1卡数据读取函数
 * @param
 * @return
 */
static void SendFirstDataRequest(void)
{
	//数据查询，合并密码验证和读取指令
	U8 ucBuf[] = {
			HW_HEAD_SIGN,
			HW_M1_READ_CMD,
			0,
			HW_M1_READ_DATA_LEN,
			g_sHW_RFID.ucM1_key_mode_A,
			g_sHW_RFID.ucM1_sector_A,
			g_sHW_RFID.ucM1_block_A,
			g_sHW_RFID.ucPassword_A[0], g_sHW_RFID.ucPassword_A[1], g_sHW_RFID.ucPassword_A[2],
			g_sHW_RFID.ucPassword_A[3], g_sHW_RFID.ucPassword_A[4], g_sHW_RFID.ucPassword_A[5],
			0x00,
			HW_END_SIGN
	};
//	U8 ucBuf[] = {
//			HW_HEAD_SIGN,
//			HW_M1_READ_CMD,
//			0,
//			0x09,
//			0,
//			4,//1扇区0块 一个扇区4块，扇区和块号都从0开始，0块不可用
//			4,
//			0xFF, 0xFF, 0xFF,
//			0xFF, 0xFF, 0xFF,
//			0x00,
//			HW_END_SIGN
//	};
	XorChecksum(&ucBuf[1], sizeof(ucBuf) - 3U, &ucBuf[sizeof(ucBuf)-2U]);
	//发送
	Uart_Dma_Send(RFID_UART, sizeof(ucBuf) , ucBuf);
}

/**
 * @brief M1卡数据读取函数
 * @param
 * @return
 */
static void SendSecondDataRequest(void)
{
	//数据查询，合并密码验证和读取指令
	U8 ucBuf[] = {
			HW_HEAD_SIGN,
			HW_M1_READ_CMD,
			0,
			HW_M1_READ_DATA_LEN,
			g_sHW_RFID.ucM1_key_mode_B,
			g_sHW_RFID.ucM1_sector_B,
			g_sHW_RFID.ucM1_block_B,
			g_sHW_RFID.ucPassword_B[0], g_sHW_RFID.ucPassword_B[1], g_sHW_RFID.ucPassword_B[2],
			g_sHW_RFID.ucPassword_B[3], g_sHW_RFID.ucPassword_B[4], g_sHW_RFID.ucPassword_B[5],
			0x00,
			HW_END_SIGN
	};
//	U8 ucBuf[] = {
//			HW_HEAD_SIGN,
//			HW_M1_READ_CMD,
//			0,
//			0x09,
//			0,
//			8,//2扇区0块 一个扇区4块，扇区和块号都从0开始，0块不可用
//			8,
//			0xFF, 0xFF, 0xFF,
//			0xFF, 0xFF, 0xFF,
//			0x00,
//			HW_END_SIGN
//	};
	XorChecksum(&ucBuf[1], sizeof(ucBuf) - 3U, &ucBuf[sizeof(ucBuf)-2U]);
	//发送
	Uart_Dma_Send(RFID_UART, sizeof(ucBuf) , ucBuf);
}

static void HwRfidSearchRequest(void)
{
	U8 ucCnt = 0;
	//寻卡指令
	U8 ucBuf[] = {HW_HEAD_SIGN, HW_SEARCH_CMD, 0, 0, 0, HW_END_SIGN};
	XorChecksum(&ucBuf[1], sizeof(ucBuf) - 3U, &ucBuf[sizeof(ucBuf)-2U]);
	//发送
	Uart_Dma_Send(RFID_UART, sizeof(ucBuf), ucBuf);
	//等待数据响应,防止出现获取到数据期间又发送了寻卡指令
	while(ucCnt < (RFID_SEND_TIMEOUT/RFID_WAIT_TIMEOUT))
	{
		if(TRUE == g_psRFID_data->bRfid_rec_flag)
		{
			break;
		}
		vTaskDelay(RFID_WAIT_TIMEOUT);
		ucCnt++;
	}
	g_psRFID_data->bRfid_rec_flag = FALSE;
}

static void HwRfidHeartRequest(void)
{
	//版本查询,用作心跳功能
	U8 ucBuf[] = {HW_HEAD_SIGN, HW_VERSION_CMD, 0, 0, 0, HW_END_SIGN};
	XorChecksum(&ucBuf[1], sizeof(ucBuf) - 3U, &ucBuf[sizeof(ucBuf)-2U]);
	//发送
	Uart_Dma_Send(RFID_UART, sizeof(ucBuf) , ucBuf);
}

/**
 * @brief 读取rfid卡的idtag
 * @param s_HW_stReadStep: 读取步骤
 * @param pucBuf：响应数据
 * @return
 */
static void RfidReadData(const U8 *pucBuf)
{
	switch(s_HW_stReadStep)
	{
		case Read_Unknow:

			break;
		case ReadFirst:
			//读取成功
			if (HW_SUCCESS_STATUS == pucBuf[RESPONSE_STATUS_INDEX])
			{
				if((g_sHW_RFID.ucM1_sector_B != 0U) && (g_sHW_RFID.ucM1_block_B != 0U))
				{
					(void)memcpy(g_sHW_RFID.ucId_tag, &pucBuf[VALID_DATA_OFFSET], 16);
					SendSecondDataRequest();
					s_HW_stReadStep = ReadSecond;
					my_printf(USER_INFO, "card data is stored in two areas. The first area has been read\n");
				}
				else
				{
					//获取到数据，关闭寻卡
					g_psRFID_data->bRfid_search_flag = FALSE;
					(void)xTimerStop(g_sTimer_rfid, 0);
					(void)memcpy(g_sHW_RFID.ucId_tag, &pucBuf[VALID_DATA_OFFSET], 16);
					g_sHW_RFID.bGet_data_flag = TRUE;
					s_HW_stReadStep = Read_Unknow;
					//寻卡指令流程完成
					g_psRFID_data->bRfid_rec_flag = TRUE;
					my_printf(USER_INFO, "single area has been read completed tick = %d\n",  xTaskGetTickCount());
				}
			}

			break;
		case ReadSecond:
			//读取成功
			if (HW_SUCCESS_STATUS == pucBuf[RESPONSE_STATUS_INDEX])
			{
				//获取到数据，关闭寻卡
				g_psRFID_data->bRfid_search_flag = FALSE;
				(void)xTimerStop(g_sTimer_rfid, 0);
				(void)memcpy(&g_sHW_RFID.ucId_tag[16], &pucBuf[VALID_DATA_OFFSET], 4);
				g_sHW_RFID.bGet_data_flag = TRUE;
				s_HW_stReadStep = Read_Unknow;
				//寻卡指令流程完成
				g_psRFID_data->bRfid_rec_flag = TRUE;
				my_printf(USER_INFO, "The second area has been read tick = %d\n",  xTaskGetTickCount());
			}
		break;
	default:
		my_printf(USER_ERROR, "%s:%d RFID step error\n", __FILE__, __LINE__);
		break;
	}
}

void HwRfidParse(const U8 *pucBuf, U16 unInput_len)
{
	//校验包格式长度
	CHECK_PTR_NULL_NO_RETURN(pucBuf);
	CHECK_MSG_LEN_NO_RETURN(unInput_len, 7U);

	//解析
	if (HW_HEAD_SIGN == pucBuf[SIGN_INDEX])
	{
		if ((HW_SUCCESS_STATUS == pucBuf[RESPONSE_STATUS_INDEX]) || (HW_FAIL_STATUS == pucBuf[RESPONSE_STATUS_INDEX]))
		{
			U8 ucCheck_data = 0;
			U16 unData_len = (((U16)pucBuf[DATA_LEN_INDEX_H] << 8U) | pucBuf[DATA_LEN_INDEX_L]);
			//校验数据长度
			CHECK_MSG_LEN_NO_RETURN(unData_len+7U, unInput_len);

			XorChecksum(&pucBuf[1], (U32)unData_len + 4U, &ucCheck_data);

			if (ucCheck_data == pucBuf[unData_len+VALID_DATA_OFFSET])
			{
				switch(pucBuf[CMD_INDEX])
				{
				case HW_VERSION_CMD:
					//版本号
					(void)memcpy(g_sHW_RFID.ucVersion, &pucBuf[VALID_DATA_OFFSET], sizeof(g_sHW_RFID.ucVersion) - 2U);//厂家修改版本号长度为14
					my_printf(USER_DEBUG, "RFID version read success\n");
					break;
				case HW_SEARCH_CMD:
					//寻卡成功
					if (HW_SUCCESS_STATUS == pucBuf[RESPONSE_STATUS_INDEX])
					{
						my_printf(USER_INFO, "Card search success, send data query command\n");
						SendFirstDataRequest();
						s_HW_stReadStep = ReadFirst;
					}
					else
					{
						//寻卡指令流程完成
						g_psRFID_data->bRfid_rec_flag = TRUE;
						my_printf(USER_DEBUG, "Card search failed\n");
					}
					break;
				case HW_M1_READ_CMD:
					RfidReadData(pucBuf);
					break;
				default:
					my_printf(USER_ERROR, "%s:%d pucBuf[CMD_INDEX] error:%d\n", __FILE__, __LINE__, pucBuf[CMD_INDEX]);
					break;
				}
			}
			else
			{
				my_printf(USER_ERROR, "%s:%d parse data CRC error\n", __FILE__, __LINE__);
			}
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d parse data status error\n", __FILE__, __LINE__);
		}
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d receive data head error = %d\n", __FILE__, __LINE__, pucBuf[SIGN_INDEX]);
	}
}

/**
 * @brief M1卡配置数据获取
 * @param
 * @return
 */
static void GetRfidConfig(void)
{
	S32 iTmp = 0;

	g_sHW_RFID.ucM1_key_mode_A = g_sStorage_data.ucM1_key_mode_first;
	g_sHW_RFID.ucM1_sector_A = g_sStorage_data.ucM1_sector_first;
	g_sHW_RFID.ucM1_block_A = g_sStorage_data.ucM1_block_first;
	iTmp = (S32)HexStrToInt(g_sStorage_data.ucM1_password_first, 4);
	(void)memcpy(g_sHW_RFID.ucPassword_A, (U8*)&iTmp, 2);
	iTmp = (S32)HexStrToInt(&g_sStorage_data.ucM1_password_first[4], 4);
	(void)memcpy(&g_sHW_RFID.ucPassword_A[2], (U8*)&iTmp, 2);
	iTmp = (S32)HexStrToInt(&g_sStorage_data.ucM1_password_first[8], 4);
	(void)memcpy(&g_sHW_RFID.ucPassword_A[4], (U8*)&iTmp, 2);

	//厂家回复块号和扇区需要相同
	if (g_sHW_RFID.ucM1_sector_A != g_sHW_RFID.ucM1_block_A)
	{
		my_printf(USER_ERROR, "%s:%d RFID config sector_A != block_A\n", __FILE__, __LINE__);
	}

	(void)GetSigVal(CCU_SET_SIG_ID_M1_SECTOR_SECOND, &iTmp);
	if (0 != iTmp)
	{
		g_sHW_RFID.ucM1_key_mode_B = g_sStorage_data.ucM1_key_mode_second;
		g_sHW_RFID.ucM1_sector_B = g_sStorage_data.ucM1_sector_second;
		g_sHW_RFID.ucM1_block_B = g_sStorage_data.ucM1_block_second;

		iTmp = (S32)HexStrToInt(g_sStorage_data.ucM1_password_second, 4);
		(void)memcpy(g_sHW_RFID.ucPassword_B, (U8*)&iTmp, 2);
		iTmp = (S32)HexStrToInt(&g_sStorage_data.ucM1_password_second[4], 4);
		(void)memcpy(&g_sHW_RFID.ucPassword_B[2], (U8*)&iTmp, 2);
		iTmp = (S32)HexStrToInt(&g_sStorage_data.ucM1_password_second[8], 4);
		(void)memcpy(&g_sHW_RFID.ucPassword_B[4], (U8*)&iTmp, 2);
		//厂家回复块号和扇区需要相同
		if (g_sHW_RFID.ucM1_sector_B != g_sHW_RFID.ucM1_block_B)
		{
			my_printf(USER_ERROR, "%s:%d RFID config sector_B != block_B\n", __FILE__, __LINE__);
		}
		//两段数据块号不能相同
		if (g_sHW_RFID.ucM1_sector_A == g_sHW_RFID.ucM1_sector_B)
		{
			my_printf(USER_ERROR, "%s:%d RFID config sector_A == sector_B\n", __FILE__, __LINE__);
		}
	}
}

RFID_data_t* GetHwRfidModel(void)
{
	GetRfidConfig();

	return &g_sHW_RFID;
}

void HwRfidInit(void)
{
	g_sHW_RFID.RfidDataParse = &HwRfidParse;
	g_sHW_RFID.SendRfidSearchRequest = &HwRfidSearchRequest;
	g_sHW_RFID.SendRfidHeartRequest = &HwRfidHeartRequest;
}
