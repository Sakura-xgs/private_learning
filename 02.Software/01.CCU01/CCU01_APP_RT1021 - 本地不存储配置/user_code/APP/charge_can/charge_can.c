/*
 * charge_can.c
 *
 *  Created on: 2024年8月26日
 *      Author: Bono
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "fsl_flexcan.h"
#include "timers.h"

#include "hal_sys_IF.h"
#include "hal_can_IF.h"
#include "hal_can.h"
#include "charge_can.h"
#include "SignalManage.h"
#include "PublicDefine.h"
#include "tcp_client_IF.h"
#include "emergency_fault_IF.h"

#include "boot.h"
#include "secc_logandupgrade.h"
#include "rgb_pwm_IF.h"
#include "led_IF.h"
#include "charge_parse_IF.h"
#include "uart_comm.h"
#include "fan_pwm_IF.h"
#include "charge_comm_IF.h"
#include "datasample.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/
static U8 s_ucSecc_timeout_cnt[2] = {0};
static U32 s_uiProduction_mode_id = 0x10010080U;
static U32 s_uiProduction_uns_id = 0x10018000U;

/*******************************************************************************
 * Code
 ******************************************************************************/
static void ProductionDataSendToEth(const U8 *pucBuf,U16 len)
{
	if(len < 80U)
	{
		U16 i = 0;
		U8 Sendbuf[100] = {0};

		Sendbuf[i] = 0xAAU;
		i++;
		Sendbuf[i] = 0x55U;
		i++;
		Sendbuf[i] = g_sStorage_data.ucPile_num;//桩编号
		i++;
		Sendbuf[i] = 0x00;
		i++;
		Sendbuf[i] = 0x36U;
		i++;
		Sendbuf[i] = 0;//message id
		i++;
		Sendbuf[i] = 0;//message id
		i++;
		Sendbuf[i] = (U8)((len & 0xFF00U) >> 8);
		i++;
		Sendbuf[i] = (U8)(len & 0xFFU);
		i++;
		(void)memcpy(&Sendbuf[i],&pucBuf[0],len);
		i = i +len;

		if(sockfd != -1)
		{
			if(i < 100U)
			{
				//(void)send(sockfd,Sendbuf,i,0);
				(void)TcpSend(Sendbuf, i);
			}
		}
	}
}

static void ModeControlProcess(const U8* pucBuf, const U8 ucGun_id, const U8 ComType)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	flexcan_frame_t sFrame = {0};

	//开启生产测试模式
	if (0xAAU == pucBuf[1])
	{
		g_sPile_data.ucPile_config_mode = PRODUCTION_MODE;

		if(ComType == CANTYPE)
		{
			sFrame.dataByte0 = pucBuf[0];
			sFrame.dataByte1 = pucBuf[1];
			sFrame.type = (U8)kFLEXCAN_FrameTypeData;
			sFrame.id = s_uiProduction_uns_id;
			sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
			sFrame.length = 2;

			if (ucGun_id == GUN_A)
			{
				(void)ChgACanSendFrame(&sFrame);
			}
			else
			{
				(void)ChgBCanSendFrame(&sFrame);
			}
		}
		else if(ComType == ETHTYPE)
		{
			ProductionDataSendToEth(&pucBuf[0],2);
		}
		else
		{

		}

		//生产测试模式挂起LED任务
	    if (NULL != LEDA_TASK_Handler)
	    {
	    	vTaskSuspend(LEDA_TASK_Handler);
	    }
	    if (NULL != LEDB_TASK_Handler)
	    {
	    	vTaskSuspend(LEDB_TASK_Handler);
	    }
	}
	else if (0x55U == pucBuf[1])	//关闭生产测试模式
	{
		if (g_sPile_data.ucPile_config_mode == PRODUCTION_MODE)
		{
			if(ComType == CANTYPE)
			{
				sFrame.dataByte0 = pucBuf[0];
				sFrame.dataByte1 = pucBuf[1];
				sFrame.type = (U8)kFLEXCAN_FrameTypeData;
				sFrame.id = s_uiProduction_uns_id;
				sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
				sFrame.length = 2;

				if (ucGun_id == GUN_A)
				{
					(void)ChgACanSendFrame(&sFrame);
					vTaskDelay(100);
				}
				else
				{
					(void)ChgBCanSendFrame(&sFrame);
					vTaskDelay(100);
				}
			}
			else if(ComType == ETHTYPE)
			{
				ProductionDataSendToEth(&pucBuf[0],2);
			}
			else
			{

			}

			//重启CCU
			SystemResetFunc();
		}
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d pucBuf[1] error:%d\n", __FILE__, __LINE__, pucBuf[1]);
	}
}

static void RelayDoControlProcess(const U8* pucBuf, const U8 ucGun_id, const U8 ComType)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	if (g_sPile_data.ucPile_config_mode == PRODUCTION_MODE)
	{
		flexcan_frame_t sFrame = {0};

		switch (pucBuf[1])
		{
		case 1:
			if (0xAAU == pucBuf[2])//闭合
			{
				RelayControl_A(POSITIVE_RELAY, CLOSE_CONTROL);
			}
			else if (0x55U == pucBuf[2])//断开
			{
				RelayControl_A(POSITIVE_RELAY, OPEN_CONTROL);
			}
			else
			{
				//不响应
			}
			break;
		case 2:
			if (0xAAU == pucBuf[2])//闭合
			{
				RelayControl_A(NEGATIVE_RELAY, CLOSE_CONTROL);
			}
			else if (0x55U == pucBuf[2])//断开
			{
				RelayControl_A(NEGATIVE_RELAY, OPEN_CONTROL);
			}
			else
			{
				//不响应
			}
			break;
		case 3:
			if (0xAAU == pucBuf[2])//闭合
			{
				RelayControl_B(POSITIVE_RELAY, CLOSE_CONTROL);
			}
			else if (0x55U == pucBuf[2])//断开
			{
				RelayControl_B(POSITIVE_RELAY, OPEN_CONTROL);
			}
			else
			{
				//不响应
			}
			break;
		case 4:
			if (0xAAU == pucBuf[2])//闭合
			{
				RelayControl_B(NEGATIVE_RELAY, CLOSE_CONTROL);
			}
			else if (0x55U == pucBuf[2])//断开
			{
				RelayControl_B(NEGATIVE_RELAY, OPEN_CONTROL);
			}
			else
			{
				//不响应
			}
			break;
		case 5:
			if (0xAAU == pucBuf[2])//闭合
			{
				RelayControl_A(PRECHARGE_RELAY, CLOSE_CONTROL);
			}
			else if (0x55U == pucBuf[2])//断开
			{
				RelayControl_A(PRECHARGE_RELAY, OPEN_CONTROL);
			}
			else
			{
				//不响应
			}
			break;
		case 6:
			if (0xAAU == pucBuf[2])//闭合
			{
				RelayControl_B(PRECHARGE_RELAY, CLOSE_CONTROL);
			}
			else if (0x55U == pucBuf[2])//断开
			{
				RelayControl_B(PRECHARGE_RELAY, OPEN_CONTROL);
			}
			else
			{
				//不响应
			}
			break;
		default:
			my_printf(USER_ERROR, "%s:%d pucBuf[1] error:%d\n", __FILE__, __LINE__, pucBuf[1]);
			break;
		}

		if(ComType == CANTYPE)
		{
			sFrame.dataByte0 = pucBuf[0];
			sFrame.dataByte1 = pucBuf[1];
			sFrame.dataByte2 = pucBuf[2];
			sFrame.type = (U8)kFLEXCAN_FrameTypeData;
			sFrame.id = s_uiProduction_uns_id;
			sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
			sFrame.length = 3;

			if (ucGun_id == GUN_A)
			{
				(void)ChgACanSendFrame(&sFrame);
			}
			else
			{
				(void)ChgBCanSendFrame(&sFrame);
			}
		}
		else if(ComType == ETHTYPE)
		{
			ProductionDataSendToEth(&pucBuf[0],3);
		}
		else
		{

		}
	}
}

static U32 GetFu1FbStatus(void)
{
	return GET_FU1_FB_STATUS();
}

static U32 GetFu2FbStatus(void)
{
	return GET_FU2_FB_STATUS();
}

static U32 GetPositiveRelayAFbStatus(void)
{
	return GET_POSITIVE_RELAY_A_FB_STATUS();
}

static U32 GetNegativeRelayAFbStatus(void)
{
	return GET_NEGATIVE_RELAY_A_FB_STATUS();
}

static U32 GetPositiveRelayBFbStatus(void)
{
	return GET_POSITIVE_RELAY_B_FB_STATUS();
}

static U32 GetNegativeRelayBFbStatus(void)
{
	return GET_NEGATIVE_RELAY_B_FB_STATUS();
}

static U32 GetGun1FbStatus(void)
{
	return GET_GUN1_FB_STATUS();
}

static U32 GetGun2FbStatus(void)
{
	return GET_GUN2_FB_STATUS();
}

static U32 GetDiScramStatus(void)
{
	return GET_DI_SCRAM_STATUS();
}

static U32 GetDiSpdStatus(void)
{
	return GET_DI_SPD_STATUS();
}

static U32 GetDiHitStatus(void)
{
	return GET_DI_HIT_STATUS();
}

static U32 GetDiWaterStatus(void)
{
	return GET_DI_WATER_STATUS();
}

static U32 GetDiDoorStatus(void)
{
	return GET_DI_DOOR_STATUS();
}

static U32 GetDiMcbStatus(void)
{
	return GET_DI_MCB_STATUS();
}

static U32 GetWdiPfoStatus(void)
{
	return GET_WDI_PFO_STATUS();
}

static void RelayDiCheckProcess(const U8* pucBuf, const U8 ucGun_id, const U8 ComType)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	static PinReadStatus CheckPinReadStatus[15] =
	{
		&GetDiScramStatus,
		&GetDiSpdStatus,
		&GetFu1FbStatus,
		&GetFu2FbStatus,
		&GetPositiveRelayAFbStatus,
		&GetNegativeRelayAFbStatus,
		&GetPositiveRelayBFbStatus,
		&GetNegativeRelayBFbStatus,
		&GetDiHitStatus,
		&GetDiWaterStatus,
		&GetDiDoorStatus,
		&GetGun1FbStatus,
		&GetGun2FbStatus,
		&GetDiMcbStatus,
		&GetWdiPfoStatus
	};

	U32 Status = 0;

	for(U8 i = 0;i < 15U;i++)
	{
		Status = Status | (CheckPinReadStatus[i]() << i);
	}

	if(ComType == CANTYPE)
	{
		flexcan_frame_t sFrame = {0};

		sFrame.dataByte0 = (U8)(Status & 0xFFU);
		sFrame.dataByte1 = (U8)((Status & 0xFF00U) >> 8U);

		sFrame.id = s_uiProduction_uns_id;
		sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
		sFrame.length = 2U;

		if (ucGun_id == GUN_A)
		{
			(void)ChgACanSendFrame(&sFrame);
		}
		else
		{
			(void)ChgBCanSendFrame(&sFrame);
		}
	}
	else if(ComType == ETHTYPE)
	{
		U8 EthBuf[3] = {0};

		EthBuf[0] = pucBuf[0];
		EthBuf[1] = (U8)(Status & 0xFFU);
		EthBuf[2] = (U8)((Status & 0xFF00U) >> 8U);

		ProductionDataSendToEth(&EthBuf[0],3);
	}
	else
	{

	}
}

static void TempCheckProcess(const U8* pucBuf, const U8 ucGun_id, const U8 ComType)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	if (g_sPile_data.ucPile_config_mode == PRODUCTION_MODE)
	{
		if(ComType == CANTYPE)
		{
			flexcan_frame_t sFrame = {0};
			S32 iTemp = 0;

			sFrame.dataByte0 = pucBuf[0];
			iTemp = (S32)g_sGun_data[GUN_A].nDC_positive_temp+(S32)60;
			sFrame.dataByte1 = (U8)iTemp;
			iTemp = (S32)g_sGun_data[GUN_A].nDC_negative_temp+(S32)60;
			sFrame.dataByte2 = (U8)iTemp;
			iTemp = (S32)g_sGun_data[GUN_B].nDC_positive_temp+(S32)60;
			sFrame.dataByte3 = (U8)iTemp;
			iTemp = (S32)g_sGun_data[GUN_B].nDC_negative_temp+(S32)60;
			sFrame.dataByte4 = (U8)iTemp;
			sFrame.dataByte5 = g_cTemperatureHMI[0];
			sFrame.dataByte6 = g_cTemperatureHMI[1];

			sFrame.type = (U8)kFLEXCAN_FrameTypeData;
			sFrame.id = s_uiProduction_uns_id;
			sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
			sFrame.length = 5;

			if (ucGun_id == GUN_A)
			{
				(void)ChgACanSendFrame(&sFrame);
			}
			else
			{
				(void)ChgBCanSendFrame(&sFrame);
			}
		}
		else if(ComType == ETHTYPE)
		{
			U8 EthBuf[5] = {0};
			S32 iTemp = 0;

			EthBuf[0] = pucBuf[0];
			EthBuf[1] = (U8)(g_sGun_data[GUN_A].nDC_positive_temp+(S32)60);
			EthBuf[2] = (U8)(g_sGun_data[GUN_A].nDC_negative_temp+(S32)60);
			EthBuf[3] = (U8)(g_sGun_data[GUN_B].nDC_positive_temp+(S32)60);
			EthBuf[4] = (U8)(g_sGun_data[GUN_B].nDC_negative_temp+(S32)60);
			EthBuf[5] = g_cTemperatureHMI[0];
			EthBuf[6] = g_cTemperatureHMI[1];

			ProductionDataSendToEth(&EthBuf[0],7);
		}
		else
		{
			//
		}
	}
}

static void PwmCheckProcess(const U8* pucBuf, const U8 ucGun_id, const U8 ComType)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	if (g_sPile_data.ucPile_config_mode == PRODUCTION_MODE)
	{
		switch (pucBuf[1])
		{
		case 0x01:
			Rgb_Pwm_LED_B[0].period = 1000;
			Rgb_Pwm_LED_B[0].dutycycle = pucBuf[2];
			g_sGun_data[GUN_B].ucLed_dutycycle[0] = Rgb_Pwm_LED_B[0].dutycycle;
			break;
		case 0x02:
			Rgb_Pwm_LED_B[1].period = 1000;
			Rgb_Pwm_LED_B[1].dutycycle = pucBuf[2];
			g_sGun_data[GUN_B].ucLed_dutycycle[1] = Rgb_Pwm_LED_B[1].dutycycle;
			break;
		case 0x03:
			Rgb_Pwm_LED_B[2].period = 1000;
			Rgb_Pwm_LED_B[2].dutycycle = pucBuf[2];
			g_sGun_data[GUN_B].ucLed_dutycycle[2] = Rgb_Pwm_LED_B[2].dutycycle;
			break;
		case 0x04:
			Rgb_Pwm_LED_A[0].period = 1000;
			Rgb_Pwm_LED_A[0].dutycycle = pucBuf[2];
			g_sGun_data[GUN_A].ucLed_dutycycle[0] = Rgb_Pwm_LED_A[0].dutycycle;
			break;
		case 0x05:
			Rgb_Pwm_LED_A[1].period = 1000;
			Rgb_Pwm_LED_A[1].dutycycle = pucBuf[2];
			g_sGun_data[GUN_A].ucLed_dutycycle[1] = Rgb_Pwm_LED_A[1].dutycycle;
			break;
		case 0x06:
			Rgb_Pwm_LED_A[2].period = 1000;
			Rgb_Pwm_LED_A[2].dutycycle = pucBuf[2];
			g_sGun_data[GUN_A].ucLed_dutycycle[2] = Rgb_Pwm_LED_A[2].dutycycle;
			break;
		case 0x07:
			Fan_Pwm.dutycycle = pucBuf[2];
			g_sPile_data.ucFan_pwm = Fan_Pwm.dutycycle;
			break;
		default:
			my_printf(USER_ERROR, "%s:%d pucBuf[1] error:%d\n", __FILE__, __LINE__, pucBuf[1]);
			break;
		}

		if (g_sLed_change_sem != NULL)
		{
			(void)xSemaphoreGive(g_sLed_change_sem);
		}

		if(ComType == CANTYPE)
		{
			flexcan_frame_t sFrame = {0};

			sFrame.dataByte0 = pucBuf[0];
			sFrame.dataByte1 = pucBuf[1];
			sFrame.dataByte2 = pucBuf[2];
			sFrame.type = (U8)kFLEXCAN_FrameTypeData;
			sFrame.id = s_uiProduction_uns_id;
			sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
			sFrame.length = 3;

			if (ucGun_id == GUN_A)
			{
				(void)ChgACanSendFrame(&sFrame);
			}
			else
			{
				(void)ChgBCanSendFrame(&sFrame);
			}
		}
		else if(ComType == ETHTYPE)
		{
			ProductionDataSendToEth(&pucBuf[0],3);
		}
		else
		{

		}
	}
}

static void SoftwareCheckProcess(const U8 ucGun_id, const U8 ComType)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	if (g_sPile_data.ucPile_config_mode == PRODUCTION_MODE)
	{
		if(ComType == CANTYPE)
		{
			flexcan_frame_t sFrame = {0};
			U8 ucBuf[8] = {0};

			(void)memcpy(ucBuf,g_sPile_data.ucSoftware, 8);

			sFrame.dataByte0 = ucBuf[0];
			sFrame.dataByte1 = ucBuf[1];
			sFrame.dataByte2 = ucBuf[2];
			sFrame.dataByte3 = ucBuf[3];
			sFrame.dataByte4 = ucBuf[4];
			sFrame.dataByte5 = ucBuf[5];
			sFrame.dataByte6 = ucBuf[6];
			sFrame.dataByte7 = ucBuf[7];

			sFrame.type = (U8)kFLEXCAN_FrameTypeData;
			sFrame.id = s_uiProduction_uns_id;
			sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
			sFrame.length = 8;

			if (ucGun_id == GUN_A)
			{
				(void)ChgACanSendFrame(&sFrame);
			}
			else
			{
				(void)ChgBCanSendFrame(&sFrame);
			}

			vTaskDelay(100);
			(void)memcpy(ucBuf, &g_sPile_data.ucSoftware[8], 8);

			sFrame.dataByte0 = ucBuf[0];
			sFrame.dataByte1 = ucBuf[1];
			sFrame.dataByte2 = ucBuf[2];
			sFrame.dataByte3 = ucBuf[3];
			sFrame.dataByte4 = ucBuf[4];
			sFrame.dataByte5 = ucBuf[5];
			sFrame.dataByte6 = ucBuf[6];
			sFrame.dataByte7 = ucBuf[7];

			if (ucGun_id == GUN_A)
			{
				(void)ChgACanSendFrame(&sFrame);
			}
			else
			{
				(void)ChgBCanSendFrame(&sFrame);
			}
		}
		else if(ComType == ETHTYPE)
		{
			U8 EthBuf[17] = {0};

			EthBuf[0] = 0x07;
			(void)memcpy(&EthBuf[1],&g_sPile_data.ucSoftware,16);

			ProductionDataSendToEth(&EthBuf[0],17);
		}
		else
		{
			//
		}
	}
}

static void EepromCheckProcess(const U8* pucBuf, const U8 ucGun_id, const U8 ComType)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	if (g_sPile_data.ucPile_config_mode == PRODUCTION_MODE)
	{
		if(ComType == CANTYPE)
		{
			flexcan_frame_t sFrame = {0};

			sFrame.dataByte0 = pucBuf[0];
			if (g_blExtEepromErrFg)//失败
			{
				sFrame.dataByte1 = 0x55;
			}
			else//成功
			{
				sFrame.dataByte1 = 0xAA;
			}
			sFrame.type = (U8)kFLEXCAN_FrameTypeData;
			sFrame.id = s_uiProduction_uns_id;
			sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
			sFrame.length = 2;

			if (ucGun_id == GUN_A)
			{
				(void)ChgACanSendFrame(&sFrame);
			}
			else
			{
				(void)ChgBCanSendFrame(&sFrame);
			}
		}
		else if(ComType == ETHTYPE)
		{
			U8 EthBuf[2] = {0};

			EthBuf[0] = pucBuf[0];
			if (g_blExtEepromErrFg)//失败
			{
				EthBuf[1] = 0x55;
			}
			else//成功
			{
				EthBuf[1] = 0xAA;
			}

			ProductionDataSendToEth(&EthBuf[0],2);
		}
		else
		{

		}
	}
}

static void WatchdogCheckProcess(const U8* pucBuf, const U8 ucGun_id, const U8 ComType)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	if (g_sPile_data.ucPile_config_mode == PRODUCTION_MODE)
	{
		if(ComType == CANTYPE)
		{
			flexcan_frame_t sFrame = {0};

			sFrame.dataByte0 = pucBuf[0];

			sFrame.dataByte1 = 0xAA;

			sFrame.type = (U8)kFLEXCAN_FrameTypeData;
			sFrame.id = s_uiProduction_uns_id;
			sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
			sFrame.length = 2;

			if (ucGun_id == GUN_A)
			{
				(void)ChgACanSendFrame(&sFrame);
			}
			else
			{
				(void)ChgBCanSendFrame(&sFrame);
			}
			vTaskDelay(500);
			while(1)//一直处于循环中，测试看门狗能否重启
			{

			}
		}
		else if(ComType == ETHTYPE)
		{
			U8 EthBuf[2] = {0};

			EthBuf[0] = pucBuf[0];
			EthBuf[1] = 0xAA;

			ProductionDataSendToEth(&EthBuf[0],2);
			vTaskDelay(500);
			while(1)//一直处于循环中，测试看门狗能否重启
			{

			}
			//WatchdogFeedingStop = 1;
		}
		else
		{

		}
	}
}

static void DeviceAddressCheck(const U8* pucBuf, const U8 ucGun_id, const U8 ComType)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	if (g_sPile_data.ucPile_config_mode == PRODUCTION_MODE)
	{
		if(ComType == CANTYPE)
		{
			flexcan_frame_t sFrame = {0};

			sFrame.dataByte0 = pucBuf[0];
			//电表A通讯超时
			if (0U != g_sGun_fault[GUN_A].sGeneral_fault.sItem.meter_comm_lost)
			{
				sFrame.dataByte1 = 0;
			}
			else
			{
				sFrame.dataByte1 = 1;
			}
			//电表B通讯超时
			if (0U != g_sGun_fault[GUN_B].sGeneral_fault.sItem.meter_comm_lost)
			{
				sFrame.dataByte2 = 0;
			}
			else
			{
				sFrame.dataByte2 = 1;
			}

			//绝缘监测板A通讯超时
			if (0U != g_sGun_fault[GUN_A].sGeneral_fault.sItem.imd_comm_lost)
			{
				sFrame.dataByte3 = 0;
			}
			else
			{
				sFrame.dataByte3 = 1;
			}
			//绝缘监测板B通讯超时
			if (0U != g_sGun_fault[GUN_B].sGeneral_fault.sItem.imd_comm_lost)
			{
				sFrame.dataByte4 = 0;
			}
			else
			{
				sFrame.dataByte4 = 1;
			}

			sFrame.type = (U8)kFLEXCAN_FrameTypeData;
			sFrame.id = s_uiProduction_uns_id;
			sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
			sFrame.length = 5;

			if (ucGun_id == GUN_A)
			{
				(void)ChgACanSendFrame(&sFrame);
			}
			else
			{
				(void)ChgBCanSendFrame(&sFrame);
			}
		}
		else if(ComType == ETHTYPE)
		{
			U8 EthBuf[5] = {0};

			EthBuf[0] = pucBuf[0];
			if (1U == g_sGun_fault[GUN_A].sGeneral_fault.sItem.meter_comm_lost)
			{
				EthBuf[1] = 0;
			}
			else
			{
				EthBuf[1] = 1;
			}

			ProductionDataSendToEth(&EthBuf[0], 5);
		}
		else
		{

		}
	}
}

static void DeviceLedControl(const U8* pucBuf, const U8 ucGun_id, const U8 ComType)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	if (g_sPile_data.ucPile_config_mode == PRODUCTION_MODE)
	{
		if(ComType == CANTYPE)
		{
			USER_LED_TOGGLE();

			flexcan_frame_t sFrame = {0};

			sFrame.dataByte0 = pucBuf[0];
			sFrame.dataByte1 = pucBuf[1];

			sFrame.type = (U8)kFLEXCAN_FrameTypeData;
			sFrame.id = s_uiProduction_uns_id;
			sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
			sFrame.length = 2;

			if (ucGun_id == GUN_A)
			{
				(void)ChgACanSendFrame(&sFrame);
			}
			else
			{
				(void)ChgBCanSendFrame(&sFrame);
			}
		}
		else if(ComType == ETHTYPE)
		{

		}
		else
		{

		}
	}
}

static void DeviceSnWrite(const U8* pucBuf, const U8 ucGun_id, const U8 ComType)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	if (g_sPile_data.ucPile_config_mode == PRODUCTION_MODE)
	{
		if (ComType == CANTYPE)
		{
			if (NULL == pucBuf)
			{
				return;
			}

			U8 ucPacket_num = pucBuf[1];

			 if ((ucPacket_num >= 1U) && (ucPacket_num <= 5U))
			 {
				// 计算偏移：每包6字节
				U8 ucOffset = (ucPacket_num - 1U) * 6U;
	            // 实际数据从 pucBuf[2] 开始
				U8 data_len = (ucPacket_num == 5U) ? 2U : 6U;  // 第5包只取2字节
				// 执行拷贝
				(void)memcpy(&ucSN[ucOffset], &pucBuf[2], data_len);

				if (ucPacket_num == 5U)
				{
	                // 所有包已接收完毕，开始写入
	                S32 s32TempVal = 0;

	                // === 第1组：SN[0]~SN[3] ===
	                U8 i = 0;
	                s32TempVal = ((S32)ucSN[i++] << 24) |
	                             ((S32)ucSN[i++] << 16) |
	                             ((S32)ucSN[i++] <<  8) |
	                             ((S32)ucSN[i++] <<  0);
	                (void)SetSigVal(CCU_SET_SIG_ID_SN_NUMBER_1_2_3_4, s32TempVal);

	                // === 第2组：SN[4]~SN[7] ===
	                s32TempVal = ((S32)ucSN[i++] << 24) |
	                             ((S32)ucSN[i++] << 16) |
	                             ((S32)ucSN[i++] <<  8) |
	                             ((S32)ucSN[i++] <<  0);
	                (void)SetSigVal(CCU_SET_SIG_ID_SN_NUMBER_5_6_7_8, s32TempVal);

	                // === 第3组：SN[8]~SN[11] ===
	                s32TempVal = ((S32)ucSN[i++] << 24) |
	                             ((S32)ucSN[i++] << 16) |
	                             ((S32)ucSN[i++] <<  8) |
	                             ((S32)ucSN[i++] <<  0);
	                (void)SetSigVal(CCU_SET_SIG_ID_SN_NUMBER_9_10_11_12, s32TempVal);

	                // === 第4组：SN[12]~SN[15] ===
	                s32TempVal = ((S32)ucSN[i++] << 24) |
	                             ((S32)ucSN[i++] << 16) |
	                             ((S32)ucSN[i++] <<  8) |
	                             ((S32)ucSN[i++] <<  0);
	                (void)SetSigVal(CCU_SET_SIG_ID_SN_NUMBER_13_14_15_16, s32TempVal);

	                // === 第5组：SN[16]~SN[19] ===
	                s32TempVal = ((S32)ucSN[i++] << 24) |
	                             ((S32)ucSN[i++] << 16) |
	                             ((S32)ucSN[i++] <<  8) |
	                             ((S32)ucSN[i++] <<  0);
	                (void)SetSigVal(CCU_SET_SIG_ID_SN_NUMBER_17_18_19_20, s32TempVal);

	                // === 第6组：SN[20]~SN[23] ===
	                s32TempVal = ((S32)ucSN[i++] << 24) |
	                             ((S32)ucSN[i++] << 16) |
	                             ((S32)ucSN[i++] <<  8) |
	                             ((S32)ucSN[i++] <<  0);
	                (void)SetSigVal(CCU_SET_SIG_ID_SN_NUMBER_21_22_23_24, s32TempVal);

	                // === 第7组：SN[24], SN[25], 0xFF, 0xFF ===
	                s32TempVal = ((S32)ucSN[i++] << 24) |
	                             ((S32)ucSN[i++] << 16) |
	                             (0xFFU          <<  8) |
	                             (0xFFU          <<  0);
	                (void)SetSigVal(CCU_SET_SIG_ID_SN_NUMBER_25_26_27_28, s32TempVal);
				}
			 }

			flexcan_frame_t sFrame = {0};
			sFrame.type = (U8)kFLEXCAN_FrameTypeData;
			sFrame.id = s_uiProduction_uns_id;
			sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
			sFrame.length = 2;

			sFrame.dataByte0 = pucBuf[0];
			sFrame.dataByte1 = ((ucPacket_num >= 1U) && (ucPacket_num <= 5U)) ? 0xAAU : 0x55U;  // 成功=0xAA，失败=0x55

			if (ucGun_id == GUN_A)
			{
				(void)ChgACanSendFrame(&sFrame);
			}
			else
			{
				(void)ChgBCanSendFrame(&sFrame);
			}
		}
		else if(ComType == ETHTYPE)
		{

		}
		else
		{

		}
	}
}

static void DeviceSnRead(const U8* pucBuf, const U8 ucGun_id, const U8 ComType)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	if (g_sPile_data.ucPile_config_mode == PRODUCTION_MODE)
	{
		if(ComType == CANTYPE)
		{
			flexcan_frame_t sFrame = {0};

			sFrame.type = (U8)kFLEXCAN_FrameTypeData;
			sFrame.id = s_uiProduction_uns_id;
			sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;

			sFrame.dataByte0 = ucSN[0];
			sFrame.dataByte1 = ucSN[1];
			sFrame.dataByte2 = ucSN[2];
			sFrame.dataByte3 = ucSN[3];
			sFrame.dataByte4 = ucSN[4];
			sFrame.dataByte5 = ucSN[5];
			sFrame.dataByte6 = ucSN[6];
			sFrame.dataByte7 = ucSN[7];

	        sFrame.length = 8;

			if (ucGun_id == GUN_A)
			{
				(void)ChgACanSendFrame(&sFrame);
			}
			else
			{
				(void)ChgBCanSendFrame(&sFrame);
			}

			vTaskDelay(50);

			sFrame.dataByte0 = ucSN[8];
			sFrame.dataByte1 = ucSN[9];
			sFrame.dataByte2 = ucSN[10];
			sFrame.dataByte3 = ucSN[11];
			sFrame.dataByte4 = ucSN[12];
			sFrame.dataByte5 = ucSN[13];
			sFrame.dataByte6 = ucSN[14];
			sFrame.dataByte7 = ucSN[15];

			if (ucGun_id == GUN_A)
			{
				(void)ChgACanSendFrame(&sFrame);
			}
			else
			{
				(void)ChgBCanSendFrame(&sFrame);
			}
			vTaskDelay(50);

			sFrame.dataByte0 = ucSN[16];
			sFrame.dataByte1 = ucSN[17];
			sFrame.dataByte2 = ucSN[18];
			sFrame.dataByte3 = ucSN[19];
			sFrame.dataByte4 = ucSN[20];
			sFrame.dataByte5 = ucSN[21];
			sFrame.dataByte6 = ucSN[22];
			sFrame.dataByte7 = ucSN[23];

			if (ucGun_id == GUN_A)
			{
				(void)ChgACanSendFrame(&sFrame);
			}
			else
			{
				(void)ChgBCanSendFrame(&sFrame);
			}
			vTaskDelay(50);

			sFrame.dataByte0 = ucSN[24];
			sFrame.dataByte1 = ucSN[25];

			sFrame.length = 2;

			if (ucGun_id == GUN_A)
			{
				(void)ChgACanSendFrame(&sFrame);
			}
			else
			{
				(void)ChgBCanSendFrame(&sFrame);
			}
		}
		else if(ComType == ETHTYPE)
		{

		}
		else
		{

		}
	}
}

/**
 * @brief: 生产测试函数
 * @param: uiCan_id：can id
 * @param: pucBuf:数据包
 * @return
 */
void ProductionProcess(const U8* pucBuf, const U8 ucGun_id, const U8 ComType)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	switch (pucBuf[0])
	{
	case PRODUCTION_MODE_CONTROL:
		ModeControlProcess(pucBuf, ucGun_id, ComType);
		break;
	case RELAY_DO_CONTROL:
		RelayDoControlProcess(pucBuf, ucGun_id, ComType);
		break;
	case RELAY_DI_CHECK:
		RelayDiCheckProcess(pucBuf, ucGun_id, ComType);
		break;
	case TEMP_CHECK:
		TempCheckProcess(pucBuf, ucGun_id, ComType);
		break;
	case UART_COMM_CHECK:
		break;
	case PWM_CHECK:
		PwmCheckProcess(pucBuf, ucGun_id, ComType);
		break;
	case SOFTWARE_CHECK:
		SoftwareCheckProcess(ucGun_id, ComType);
		break;
	case EEPROM_CHECK:
		EepromCheckProcess(pucBuf, ucGun_id, ComType);
		break;
	case WATCHDOG_CHECK:
		WatchdogCheckProcess(pucBuf, ucGun_id, ComType);
		break;
	case DEVICE_ADDRESS_CHECK:
		DeviceAddressCheck(pucBuf, ucGun_id, ComType);
		break;
	case DEVICE_SN_WRITE:
		DeviceSnWrite(pucBuf, ucGun_id, ComType);
		break;
	case DEVICE_SN_READ:
		DeviceSnRead(pucBuf, ucGun_id, ComType);
		break;
	case DEVICE_LED_CONTROL:
		DeviceLedControl(pucBuf, ucGun_id, ComType);
		break;
	default:
		my_printf(USER_ERROR, "%s:%d cmd error:%d\n", __FILE__, __LINE__, pucBuf[0]);
		break;
	}
}

void CAN1_ChargeDataParse(U32 uiCan_id, const U8* pucBuf)
{
	if (g_sStorage_data.sPublic_data[GUN_A].ucGun_type == g_sStorage_data.sPublic_data[GUN_B].ucGun_type)
	{
		//报文交互
		if (NULL != g_psCharge_control)
		{
			if (NULL != g_psCharge_control->CAN1DataParse)
			{
				g_psCharge_control->CAN1DataParse(uiCan_id, pucBuf);
			}
			else
			{
				my_printf(USER_ERROR, "%s:%d g_psCharge_control->CAN1DataParse = NULL\n", __FILE__, __LINE__);
			}
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d g_psCharge_control = NULL\n", __FILE__, __LINE__);
		}
	}
	else
	{
		//报文交互
		if (NULL != g_psCharge_control_A)
		{
			if (NULL != g_psCharge_control_A->CAN1DataParse)
			{
				g_psCharge_control_A->CAN1DataParse(uiCan_id, pucBuf);
			}
			else
			{
				my_printf(USER_ERROR, "%s:%d g_psCharge_control_A->CAN1DataParse = NULL\n", __FILE__, __LINE__);
			}
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d g_psCharge_control_A = NULL\n", __FILE__, __LINE__);
		}
	}
}

void CAN2_ChargeDataParse(U32 uiCan_id, const U8* pucBuf)
{
	if (g_sStorage_data.sPublic_data[GUN_A].ucGun_type == g_sStorage_data.sPublic_data[GUN_B].ucGun_type)
	{
		//报文交互
		if (NULL != g_psCharge_control)
		{
			if (NULL != g_psCharge_control->CAN2DataParse)
			{
				g_psCharge_control->CAN2DataParse(uiCan_id, pucBuf);
			}
			else
			{
				my_printf(USER_ERROR, "%s:%d g_psCharge_control->CAN2DataParse = NULL\n", __FILE__, __LINE__);
			}
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d g_psCharge_control = NULL\n", __FILE__, __LINE__);
		}
	}
	else
	{
		//报文交互
		if (NULL != g_psCharge_control_B)
		{
			if (NULL != g_psCharge_control_B->CAN2DataParse)
			{
				g_psCharge_control_B->CAN2DataParse(uiCan_id, pucBuf);
			}
			else
			{
				my_printf(USER_ERROR, "%s:%d g_psCharge_control_B->CAN2DataParse = NULL\n", __FILE__, __LINE__);
			}
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d g_psCharge_control_B = NULL\n", __FILE__, __LINE__);
		}
	}
}

static void Charge_A_Can_Parse(const flexcan_frame_t *Frame)
{
	//收到secc A数据，复位超时计数
	s_ucSecc_timeout_cnt[GUN_A] = 0;
	//充电交互
	if((SECC_ReadLog_Cmd[GUN_A] == Read_Log_Unknow) && (g_Secc_sam_upgrade_state == NO_UPGRADE))
	{
		U8 ucTmp_buf[8] = {0};

		ucTmp_buf[0] = Frame->dataByte0;
		ucTmp_buf[1] = Frame->dataByte1;
		ucTmp_buf[2] = Frame->dataByte2;
		ucTmp_buf[3] = Frame->dataByte3;
		ucTmp_buf[4] = Frame->dataByte4;
		ucTmp_buf[5] = Frame->dataByte5;
		ucTmp_buf[6] = Frame->dataByte6;
		ucTmp_buf[7] = Frame->dataByte7;

		if (Frame->id == s_uiProduction_mode_id)
		{
			my_printf(USER_INFO, "GUN_A receive production test\n");
			ProductionProcess(ucTmp_buf, GUN_A,CANTYPE);
		}
		else
		{
			CAN1_ChargeDataParse(Frame->id, ucTmp_buf);
		}
	}
	else//日志/升级交互
	{
		if((Frame->id == SECC_TO_CCU_Cmd_ID) || (Frame->id == SECC_TO_CCU_Pack_ID))
		{
			g_Secc_LogAndUpgrade[GUN_A].rec[0]=Frame->dataByte0;
			g_Secc_LogAndUpgrade[GUN_A].rec[1]=Frame->dataByte1;
			g_Secc_LogAndUpgrade[GUN_A].rec[2]=Frame->dataByte2;
			g_Secc_LogAndUpgrade[GUN_A].rec[3]=Frame->dataByte3;
			g_Secc_LogAndUpgrade[GUN_A].rec[4]=Frame->dataByte4;
			g_Secc_LogAndUpgrade[GUN_A].rec[5]=Frame->dataByte5;
			g_Secc_LogAndUpgrade[GUN_A].rec[6]=Frame->dataByte6;
			g_Secc_LogAndUpgrade[GUN_A].rec[7]=Frame->dataByte7;

            if (pdTRUE == CANCache_Write(&CanBufferGun[GUN_A], g_Secc_LogAndUpgrade[GUN_A].rec, 0, 8))
            {
    			g_Secc_LogAndUpgrade[GUN_A].CanRecFlag = 1;

    			if(NULL != SECC_BinarySemaphore[GUN_A].ReqBinarySemaphore)
    			{
    				(void)xSemaphoreGive(SECC_BinarySemaphore[GUN_A].ReqBinarySemaphore);
    			}
            }
            else
            {
            	//缓存都满了，处理错误
            }
		}
	}
}

static void Charge_B_Can_Parse(const flexcan_frame_t *Frame)
{
	//收到secc B数据，复位超时计数
	s_ucSecc_timeout_cnt[GUN_B] = 0U;
	//充电交互
	if((SECC_ReadLog_Cmd[GUN_B] == Read_Log_Unknow) && (g_Secc_sam_upgrade_state == NO_UPGRADE))
	{
		U8 ucTmp_buf[8] = {0};

		ucTmp_buf[0] = Frame->dataByte0;
		ucTmp_buf[1] = Frame->dataByte1;
		ucTmp_buf[2] = Frame->dataByte2;
		ucTmp_buf[3] = Frame->dataByte3;
		ucTmp_buf[4] = Frame->dataByte4;
		ucTmp_buf[5] = Frame->dataByte5;
		ucTmp_buf[6] = Frame->dataByte6;
		ucTmp_buf[7] = Frame->dataByte7;

		if (Frame->id == s_uiProduction_mode_id)
		{
			my_printf(USER_INFO, "GUN_B receive production test\n");
			ProductionProcess(ucTmp_buf, GUN_B,CANTYPE);
		}
		else
		{
			CAN2_ChargeDataParse(Frame->id, ucTmp_buf);
		}
	}
	else//日志/升级交互
	{
		if((Frame->id == SECC_TO_CCU_Cmd_ID) || (Frame->id == SECC_TO_CCU_Pack_ID))
		{
			g_Secc_LogAndUpgrade[GUN_B].rec[0]=Frame->dataByte0;
			g_Secc_LogAndUpgrade[GUN_B].rec[1]=Frame->dataByte1;
			g_Secc_LogAndUpgrade[GUN_B].rec[2]=Frame->dataByte2;
			g_Secc_LogAndUpgrade[GUN_B].rec[3]=Frame->dataByte3;
			g_Secc_LogAndUpgrade[GUN_B].rec[4]=Frame->dataByte4;
			g_Secc_LogAndUpgrade[GUN_B].rec[5]=Frame->dataByte5;
			g_Secc_LogAndUpgrade[GUN_B].rec[6]=Frame->dataByte6;
			g_Secc_LogAndUpgrade[GUN_B].rec[7]=Frame->dataByte7;

            if (pdTRUE == CANCache_Write(&CanBufferGun[GUN_B], g_Secc_LogAndUpgrade[GUN_B].rec, 0, 8))
            {
    			g_Secc_LogAndUpgrade[GUN_B].CanRecFlag = 1;

    			if(NULL != SECC_BinarySemaphore[GUN_B].ReqBinarySemaphore)
    			{
    				(void)xSemaphoreGive(SECC_BinarySemaphore[GUN_B].ReqBinarySemaphore);
    			}
            }
            else
            {
            	//缓存都满了，处理错误
            }
		}
	}
}

static void ChargeCanCommCheck(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	//防止溢出
	if (s_ucSecc_timeout_cnt[ucGun_id] < 255U)
	{
		s_ucSecc_timeout_cnt[ucGun_id]++;
	}
	//置位通讯超时
	if (s_ucSecc_timeout_cnt[ucGun_id] > (U8)(DEVICE_TIMEOUT_MS/WORKING_STATUS_TASK_PERIOD_MS))
	{
		if (g_sGun_fault[ucGun_id].sGeneral_fault.sItem.secc_comm_lost != TRUE)
		{
			GunAlarmSet(&g_sGun_fault[ucGun_id].sGeneral_fault.uiWhole_flag, BIT32_STRUCT, (U32)FAULT_SECC_COMM_LOST);
			my_printf(USER_ERROR, "%s:%d %s SECC com disconncet\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");
		}
	}
	else//恢复
	{
		if (g_sGun_fault[ucGun_id].sGeneral_fault.sItem.secc_comm_lost != FALSE)
		{
			GunAlarmReset(&g_sGun_fault[ucGun_id].sGeneral_fault.uiWhole_flag, BIT32_STRUCT, (U32)FAULT_SECC_COMM_LOST);
			my_printf(USER_ERROR, "%s:%d %s SECC com reconncet\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");
		}
	}
}

//static void Charge_A_Can_Send_Task(void * pvParameters)
//{
//#ifdef FREE_STACK_SPACE_CHECK_ENABLE
//	volatile UBaseType_t uxHighWaterMark;
//#endif
//	(void)pvParameters;
//
//    for( ;; )
//    {
//    	flexcan_frame_t Frame = {0};
//    	Frame.id     = FLEXCAN_ID_STD(0x123);
//    	Frame.format = (uint8_t)kFLEXCAN_FrameFormatStandard;
//    	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
//    	Frame.length = (uint8_t)8;
////    	ChgACanSendFrame(&Frame);	//test
//
//		vTaskDelay(1000);
//#ifdef FREE_STACK_SPACE_CHECK_ENABLE
//		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
//#endif
//    }
//}

static void Charge_A_Can_Parse_Task(void * pvParameters)
{
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
	volatile UBaseType_t uxHighWaterMark;
#endif
	BaseType_t err = pdFALSE;
	flexcan_frame_t Frame = {0};

	(void)pvParameters;

    for( ;; )
    {
		if (NULL != g_Can1_RecData_xQueue)
		{
			err = xQueueReceive(g_Can1_RecData_xQueue, (void *)&Frame, (TickType_t)portMAX_DELAY);

            if (pdTRUE == err)
            {
            	Charge_A_Can_Parse(&Frame);
            }
		}
		else
		{
			vTaskDelay(300);
		}

#ifdef FREE_STACK_SPACE_CHECK_ENABLE
		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
#endif
    }
}

static void Charge_B_Can_Parse_Task(void * pvParameters)
{
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
	volatile UBaseType_t uxHighWaterMark;
#endif
	BaseType_t err = pdFALSE;
	flexcan_frame_t Frame = {0};

	(void)pvParameters;

    for( ;; )
    {
		if (NULL != g_Can2_RecData_xQueue)
		{
			err = xQueueReceive(g_Can2_RecData_xQueue, (void *)&Frame, (TickType_t)portMAX_DELAY);

            if (pdTRUE == err)
            {
            	Charge_B_Can_Parse(&Frame);
            }
		}
		else
		{
			vTaskDelay(300);
		}

#ifdef FREE_STACK_SPACE_CHECK_ENABLE
		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
#endif
    }
}

static void Charge_Can_COMM_CHECK_Task(void * pvParameters)
{
	while (1)
	{
		if (g_sStorage_data.sPublic_data[GUN_A].ucGun_type != GB)
		{
			ChargeCanCommCheck(GUN_A);
		}
		if (g_sStorage_data.sPublic_data[GUN_B].ucGun_type != GB)
		{
			ChargeCanCommCheck(GUN_B);
		}

		vTaskDelay(WORKING_STATUS_TASK_PERIOD_MS);
	}
}

void Charge_Can_Comm_Init_task(void * pvParameters)
{
	//根据桩编号适配生产测试can id
	s_uiProduction_mode_id = s_uiProduction_mode_id + (U32)(g_sStorage_data.ucPile_num*256U);
	s_uiProduction_uns_id += g_sStorage_data.ucPile_num;

	CheckServerConfig();

	taskENTER_CRITICAL();

	//非国标枪启用SECCC通讯检测
	if ((g_sStorage_data.sPublic_data[GUN_A].ucGun_type != GB) || (g_sStorage_data.sPublic_data[GUN_B].ucGun_type != GB))
	{
		(void)xTaskCreate(&Charge_Can_COMM_CHECK_Task,    "CHG_CAN_COMM_CHECK",		500U/4U,   NULL,   (UBaseType_t)IMPORTENT_TASK_TASK_PRIO,		NULL);
	}
	//xTaskCreate(Charge_A_Can_Send_Task,		"CHG_A_CAN_SEND",			configMINIMAL_STACK_SIZE,   NULL,   CHG_CAN_COMM_TASK_PRIO,		NULL);
	(void)xTaskCreate(&Charge_A_Can_Parse_Task,    "CHG_A_CAN_PARSE",			800U/4U,   NULL,   (UBaseType_t)IMPORTENT_TASK_TASK_PRIO,		NULL);
	(void)xTaskCreate(&Charge_B_Can_Parse_Task,    "CHG_B_CAN_PARSE",			800U/4U,   NULL,   (UBaseType_t)IMPORTENT_TASK_TASK_PRIO,		NULL);

	vTaskDelete(NULL);

	taskEXIT_CRITICAL();
}
