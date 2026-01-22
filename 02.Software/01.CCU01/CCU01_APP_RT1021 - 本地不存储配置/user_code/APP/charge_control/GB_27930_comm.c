/*
 * GB_27930_comm.c
 *
 *  Created on: 2025年5月5日
 *      Author: qjwu
 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stdio.h"
#include "hal_can.h"
#include "fsl_flexcan.h"
#include "hal_ext_rtc.h"
#include "fsl_gpio.h"
#include "hal_sys_IF.h"
#include "hal_ext_rtc.h"

#include "pos_IF.h"
#include "imd_IF.h"
#include "tcp_client_IF.h"
#include "meter_IF.h"
#include "SignalManage.h"
#include "charge_process_IF.h"
#include "emergency_fault_IF.h"
#include "uart_comm.h"
#include "charge_general.h"
#include "GB_27930_comm.h"
#include "cig_IF.h"

static g_psCharge_Control_t g_sGB_charge_control = {0};
static U32 g_uiCharger_Num = 0x12345678U;

static void Send2BmsCHM(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	static const U8 ucCharger_protocol_ver[3]={0x00,0x01,0x01};  //V1.1
	flexcan_frame_t sFrame = {0};

	sFrame.dataByte0 = ucCharger_protocol_ver[2];
	sFrame.dataByte1 = ucCharger_protocol_ver[1];
	sFrame.dataByte2 = ucCharger_protocol_ver[0];

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_CHM_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = GB_CHM_DATA_LEN;

	if ((ucGun_id == GUN_A))
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

static void Send2BmsCRM(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	static const U8 s_ucCharger_area[3]="123";
	flexcan_frame_t sFrame = {0};

	sFrame.dataByte0 = g_sGB_Charger_data[ucGun_id].ucCrm_status;
	sFrame.dataByte1 = (U8)((g_uiCharger_Num >> 24) & 0xFFU);
	sFrame.dataByte2 = (U8)((g_uiCharger_Num >> 16) & 0xFFU);
	sFrame.dataByte3 = (U8)((g_uiCharger_Num >> 8) & 0xFFU);
	sFrame.dataByte4 = (U8)((g_uiCharger_Num) & 0xFFU);
	sFrame.dataByte5 = s_ucCharger_area[0];
	sFrame.dataByte6 = s_ucCharger_area[1];
	sFrame.dataByte7 = s_ucCharger_area[2];

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_CRM_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = GB_CRM_DATA_LEN;

	if ((ucGun_id == GUN_A))
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}

    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRM].uiSend_cnt++;
    if (1U == g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRM].uiSend_cnt)
    {
        g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRM].uiStart_tick = xTaskGetTickCount();
    }
}

static void Send2BmsCTS(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	flexcan_frame_t sFrame = {0};

	U32 year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;

	SYS_GetDate(&year, &month, &day, &hour, &minute, &second);

	if ((year+1900U) > 2021U)
    {
		sFrame.dataByte0 = DECtoBCD(second);
		sFrame.dataByte1 = DECtoBCD(minute);
		sFrame.dataByte2 = DECtoBCD(hour+8U);
		sFrame.dataByte3 = DECtoBCD(day);
		sFrame.dataByte4 = DECtoBCD(month);
		sFrame.dataByte5 = DECtoBCD((year+1900U)%100U);
		sFrame.dataByte6 = DECtoBCD((year+1900U)/100U);
    }
    else
    {
    	sFrame.dataByte0=0xFFU;
    	sFrame.dataByte1=0xFFU;
    	sFrame.dataByte2=0xFFU;
    	sFrame.dataByte3=0xFFU;
    	sFrame.dataByte4=0xFFU;
    	sFrame.dataByte5=0xFFU;
    	sFrame.dataByte6=0xFFU;
    }

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_CTS_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = GB_CTS_DATA_LEN;

	if ((ucGun_id == GUN_A))
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}

}

static void Send2BmsCML(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	flexcan_frame_t sFrame = {0};
	U32 uiTemp_max_vol = g_sStorage_data.sPublic_data[ucGun_id].unGun_max_vol*10U;
	U32 uiTemp_min_vol = g_sStorage_data.sPublic_data[ucGun_id].unGun_min_vol*10U;
	U32 uiTemp_max_cur = g_sStorage_data.sPublic_data[ucGun_id].unGun_max_cur*10U;
	U32 uiTemp_min_cur = g_sStorage_data.sPublic_data[ucGun_id].unGun_min_cur*10U;

	sFrame.dataByte0 = (U8)(uiTemp_max_vol & 0xFFU);
	sFrame.dataByte1 = (U8)((uiTemp_max_vol >> 8) & 0xFFU);
	sFrame.dataByte2 = (U8)(uiTemp_min_vol & 0xFFU);
	sFrame.dataByte3 = (U8)((uiTemp_min_vol >> 8) & 0xFFU);

	sFrame.dataByte4 = (U8)((4000U - uiTemp_max_cur) & 0xFFU);
	sFrame.dataByte5 = (U8)(((4000U - uiTemp_max_cur) >> 8) & 0xFFU);
	sFrame.dataByte6 = (U8)((4000U - uiTemp_min_cur) & 0xFFU);
	sFrame.dataByte7 = (U8)(((4000U - uiTemp_min_cur) >>8 ) & 0xFFU);

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_CML_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = GB_CML_DATA_LEN;

	if ((ucGun_id == GUN_A))
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

static void Send2BmsCRO(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	flexcan_frame_t sFrame = {0};

	sFrame.dataByte0 = g_sGB_Charger_data[ucGun_id].ucCro_status;

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_CRO_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = GB_CRO_DATA_LEN;

	if ((ucGun_id == GUN_A))
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

static void Send2BmsCCS(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	CHECK_PTR_NULL_NO_RETURN(g_psMeter_data);

	flexcan_frame_t sFrame = {0};
	U32 uiTemp_vol = g_psMeter_data->sPublic_data[ucGun_id].uiDc_vol/10U;
	U32 uiTemp_cur = g_psMeter_data->sPublic_data[ucGun_id].uiDc_cur/10U;
	U16 unTemp_charge_time = g_sGun_data[ucGun_id].sTemp.unAlready_charge_time/60U;

	sFrame.dataByte0 = (U8)(uiTemp_vol & 0xffU);
	sFrame.dataByte1 = (U8)((uiTemp_vol >> 8) & 0xffU);
	sFrame.dataByte2 = (U8)((4000U - uiTemp_cur) & 0xffU);
	sFrame.dataByte3 = (U8)(((4000U - uiTemp_cur) >> 8) & 0xffU);
	sFrame.dataByte4 = (U8)(unTemp_charge_time & 0xffU);
	sFrame.dataByte5 = (U8)((unTemp_charge_time >> 8) & 0xffU);
	sFrame.dataByte6 = (U8)g_sGun_data[ucGun_id].sTemp.bAllow_charge_flag & 0x03U;

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_CCS_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = GB_CCS_DATA_LEN;

	if ((ucGun_id == GUN_A))
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}

	return;
}

static void Send2BmsCST(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	flexcan_frame_t sFrame = {0};
	U8 ucBuf[SEND_BMS_BYTE_NUM] = {0};

	(void)memcpy(ucBuf, &g_sGB_Charger_data[ucGun_id].sCST, sizeof(GB_CST_Data_t));

	sFrame.dataByte0 = ucBuf[0];
	sFrame.dataByte1 = ucBuf[1];
	sFrame.dataByte2 = ucBuf[2];
	sFrame.dataByte3 = ucBuf[3];

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_CST_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = GB_CST_DATA_LEN;

	if ((ucGun_id == GUN_A))
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

static void Send2BmsCSD(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	CHECK_PTR_NULL_NO_RETURN(g_psMeter_data);

	flexcan_frame_t sFrame = {0};
	U16 unTemp_charge_time = g_sGun_data[ucGun_id].sTemp.unAlready_charge_time/60U;

	sFrame.dataByte0 = (U8)(unTemp_charge_time & 0xffU);
	sFrame.dataByte1 = (U8)((unTemp_charge_time >> 8) & 0xffU);
	sFrame.dataByte2 = (U8)(((g_psMeter_data->sPublic_data[ucGun_id].uiDc_vol/10U) >> 8) & 0xffU);
	sFrame.dataByte3 = (U8)(g_sGun_data[ucGun_id].uiCurrent_meter_dn - g_sGun_data[ucGun_id].sTemp.uiStart_meter_dn);
	sFrame.dataByte4 = (U8)((g_uiCharger_Num >> 24) & 0xFFU);
	sFrame.dataByte5 = (U8)((g_uiCharger_Num >> 16) & 0xFFU);
	sFrame.dataByte6 = (U8)((g_uiCharger_Num >> 8) & 0xFFU);
	sFrame.dataByte7 = (U8)((g_uiCharger_Num) & 0xFFU);

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_CSD_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = GB_CSD_DATA_LEN;

	if ((ucGun_id == GUN_A))
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}

	return;
}

static void Send2BmsCEM(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	static CEM_Data_t s_sCEM_Data = {0};
	flexcan_frame_t sFrame = {0};
	U8 ucBuf[SEND_BMS_BYTE_NUM] = {0};

    if (TRUE == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRM].bTimeout_flag)
    {
    	//ucBuf[0] |= 0x01;
    	s_sCEM_Data.sTimeout.brm = (U8)TRUE;
    }
    else if (TRUE == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCP].bTimeout_flag)
    {
    	//ucBuf[1] |= 0x01;
    	s_sCEM_Data.sTimeout.bcp = (U8)TRUE;
    }
    else if ((TRUE == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].bTimeout_flag) || (TRUE == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_AA].bTimeout_flag))
    {
    	//ucBuf[1] |= 0x04;
    	s_sCEM_Data.sTimeout.bro = (U8)TRUE;
    }
    else if (TRUE == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].bTimeout_flag)
    {
    	//ucBuf[2] |= 0x01;
    	s_sCEM_Data.sTimeout.bcs = (U8)TRUE;
    }
    else if (TRUE == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].bTimeout_flag)
    {
    	//ucBuf[2] |= 0x04;
    	s_sCEM_Data.sTimeout.bcl = (U8)TRUE;
    }
    else if (TRUE == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].bTimeout_flag)
    {
    	//ucBuf[2] |= 0x10;
    	s_sCEM_Data.sTimeout.bst = (U8)TRUE;
    }
    else if (TRUE == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].bTimeout_flag)
    {
    	//ucBuf[3] |= 0x01;
    	s_sCEM_Data.sTimeout.bsd = (U8)TRUE;
    }
    else if (TRUE == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSM].bTimeout_flag)
    {
    	s_sCEM_Data.sTimeout.bsm = (U8)TRUE;
    }
    else
    {
    	//
    }

    (void)memcpy(ucBuf, &s_sCEM_Data, sizeof(CEM_Data_t));

	sFrame.dataByte0 = ucBuf[0];
	sFrame.dataByte1 = ucBuf[1];
	sFrame.dataByte2 = ucBuf[2];
	sFrame.dataByte3 = ucBuf[3];

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_CEM_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = GB_CEM_DATA_LEN;

	if ((ucGun_id == GUN_A))
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}

    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CEM].uiSend_cnt++;
}

/**
 * @brief 充电中故障检测
 * @param ucGun_id；枪id
 * @return FALSE：有故障  TRUE：正常
 */
static BOOL GBCheckSysChargingErr(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return FALSE;
	}

	Alarm_Status_t eError_status = NORMAL_STATUS;

	eError_status = FaultCodeCheck(ucGun_id);

	if (ALARM_STATUS == eError_status)
	{
		my_printf(USER_ERROR, "%s:%d %s fault detected, stop charging\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");
        g_sBms_ctrl[ucGun_id].sStage.sNow_status = STOPCHARGE;
        g_sGB_Charger_data[ucGun_id].sCST.sStop_reason.sItem.ucFault_stop = (U8)TRUE;

        return FALSE;
	}

	if ((g_sGun_data[ucGun_id].unCC1_vol > g_sStorage_data.unCC1_4V_max) || (g_sGun_data[ucGun_id].unCC1_vol < g_sStorage_data.unCC1_4V_min))
	{
		//滤波500ms
		vTaskDelay(500);
		if ((g_sGun_data[ucGun_id].unCC1_vol > g_sStorage_data.unCC1_4V_max) || (g_sGun_data[ucGun_id].unCC1_vol < g_sStorage_data.unCC1_4V_min))
		{
			if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.unCp_fault != TRUE)
			{
				GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_CP_FAULT);
			}
			//填充CST
			g_sGB_Charger_data[ucGun_id].sCST.sStop_reason.sItem.ucFault_stop = (U8)TRUE;
			g_sGB_Charger_data[ucGun_id].sCST.sFault_reason.sItem.unConnector_fault = TRUE;
			g_sBms_ctrl[ucGun_id].sStage.sNow_status = STOPCHARGE;
			my_printf(USER_ERROR, "%s:%d %s GUN check CC1 error = %d\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B", g_sGun_data[ucGun_id].unCC1_vol);

			return FALSE;
		}
	}

	if (0U != g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BEM].unRecv_cnt)
	{
        my_printf(USER_ERROR, "%s:%d %s BEM detected during charging\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");
        g_sBms_ctrl[ucGun_id].sStage.sNow_status = TIMEOUT;

        return FALSE;
	}

	if (0U != g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].unRecv_cnt)
	{
        my_printf(USER_INFO, "%s:%d %s BST detected during charging\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");
        g_sBms_ctrl[ucGun_id].sStage.sNow_status = STOPCHARGE;
        //填充CST
        g_sGB_Charger_data[ucGun_id].sCST.sStop_reason.sItem.ucBms_stop = (U8)TRUE;

        return FALSE;
	}

	//电子锁继电器控制故障
	if (CIG_LOCK_RELAY_OPEN == g_sGun_data[ucGun_id].ucGun_lock_relay_feedback_status)
	{
        g_sBms_ctrl[ucGun_id].sStage.sNow_status = STOPCHARGE;
        //填充CST
        g_sGB_Charger_data[ucGun_id].sCST.sFault_reason.sItem.unOther_fault = TRUE;
		//电子锁驱动失败
		if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.unCig_gun_relay_close != TRUE)
		{
			GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_CIG_GUN_RELAY_CLOSE);
			my_printf(USER_ERROR, "%s:%d %s Gun_lock relay status error\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");
		}
        my_printf(USER_ERROR, "%s:%d %s charging:check gun_lock relay status error\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");

		return FALSE;
	}

	//辅源继电器控制故障
	if (CIG_ASSIST_OPEN == g_sGun_data[ucGun_id].ucBms_assist_power_feedback_status)
	{
        g_sBms_ctrl[ucGun_id].sStage.sNow_status = STOPCHARGE;
        //填充CST
        g_sGB_Charger_data[ucGun_id].sCST.sFault_reason.sItem.unOther_fault = TRUE;
		//辅源驱动失败
		if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.unCig_assist_relay_close != TRUE)
		{
			GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_CIG_ASSIST_RELAY_CLOSE);
			my_printf(USER_ERROR, "%s:%d %s close assist power supply failed\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");
		}
        my_printf(USER_ERROR, "%s:%d %s charging: check assist_power relay status error\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");

		return FALSE;
	}

	if (g_sBms_ctrl[ucGun_id].sStage.sNow_status == STOPCHARGE)
	{
		my_printf(USER_INFO, "%s:%d %s STOPCHARGE detected during charging\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");

		return FALSE;
	}

    return TRUE;
}

/**
 * @brief 充电前准备
 * @param ucGun_id：枪id
 * @return 绝缘是否成功
 */
static BOOL BmsChargingInit(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return FALSE;
	}

	U8 ucLock_relay_cnt[2] = {0};
	U8 ucAssist_realy_cnt[2] = {0};

	//检测电子锁状态
	if (CIG_LOCK_RELAY_OPEN == g_sGun_data[ucGun_id].ucGun_lock_relay_feedback_status)
	{
		//电子锁闭合控制
		if (GUN_A == ucGun_id)
		{
			CigContrlDataInput(CIG_CLOSE, CLOSE_GUNA_LOCK_REALY);
		}
		else
		{
			CigContrlDataInput(CIG_CLOSE, CLOSE_GUNB_LOCK_REALY);
		}
		my_printf(USER_INFO, "%s:%d %s close Gun_lock relay\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");

		while (1)
		{
			//等待反馈
			if (CIG_LOCK_RELAY_CLOSE == g_sGun_data[ucGun_id].ucGun_lock_relay_feedback_status)
			{
				my_printf(USER_INFO, "%s:%d %s Gun_lock relay control success\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");

				break;
			}
			else
			{
				//超时
				if (ucLock_relay_cnt[ucGun_id] > (CIG_RELAY_TIMEOUT/CIG_RELAY_RES_CYCLE))
				{
					//电子锁驱动失败
					if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.unCig_gun_relay_close != TRUE)
					{
						GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_CIG_GUN_RELAY_CLOSE);
						my_printf(USER_ERROR, "%s:%d %s Gun_lock relay status error\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");
					}

					return FALSE;
				}
				vTaskDelay(CIG_RELAY_RES_CYCLE);
				ucLock_relay_cnt[ucGun_id]++;
			}
		}
	}

	//辅源状态未打开
	if (CIG_ASSIST_OPEN == g_sGun_data[ucGun_id].ucBms_assist_power_feedback_status)
	{
		//闭合辅源地和12V
		if (GUN_A == ucGun_id)
		{
			CigContrlDataInput(CIG_CLOSE, CLOSE_GUNA_12V_ASSIST);
			//闭合辅源地和24V
			//CigContrlDataInput(CIG_CLOSE, CLOSE_GUNA_24V_ASSIST);
		}
		else
		{
			CigContrlDataInput(CIG_CLOSE, CLOSE_GUNB_12V_ASSIST);
			//闭合辅源地和24V
			//CigContrlDataInput(CIG_CLOSE, CLOSE_GUNB_24V_ASSIST);
		}
		my_printf(USER_INFO, "%s:%d %s close assist relay\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");

		while (1)
		{
			//等待反馈
			if (CIG_ASSIST_CLOSE == g_sGun_data[ucGun_id].ucBms_assist_power_feedback_status)
			{
				my_printf(USER_INFO, "%s:%d %s close assist power supply success\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");

				break;
			}
			else
			{
				if (ucAssist_realy_cnt[ucGun_id] > (CIG_RELAY_TIMEOUT/CIG_RELAY_RES_CYCLE))
				{
					//辅源驱动失败
					if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.unCig_assist_relay_close != TRUE)
					{
						GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_CIG_ASSIST_RELAY_CLOSE);
						my_printf(USER_ERROR, "%s:%d %s close assist power supply failed\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");
					}

					return FALSE;
				}
				vTaskDelay(CIG_RELAY_RES_CYCLE);
				ucAssist_realy_cnt[ucGun_id]++;
			}
		}
	}

	if ((g_sGun_data[ucGun_id].unCC1_vol > g_sStorage_data.unCC1_4V_max) || (g_sGun_data[ucGun_id].unCC1_vol < g_sStorage_data.unCC1_4V_min))
	{
		//滤波500ms
		vTaskDelay(500);
		if ((g_sGun_data[ucGun_id].unCC1_vol > g_sStorage_data.unCC1_6V_max) || (g_sGun_data[ucGun_id].unCC1_vol < g_sStorage_data.unCC1_6V_min))
		{
			my_printf(USER_ERROR, "%s:%d %s GUN check CC1 error = %d\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B", g_sGun_data[ucGun_id].unCC1_vol);

			return FALSE;
		}
	}
	return TRUE;
}

/**
 * @brief 绝缘检测
 * @param ucGun_id：枪id
 * @return 绝缘是否成功
 */
static BOOL GB_BmsInsulationCheck(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return FALSE;
	}

	//绝缘监测
    if (TRUE == BmsInsulationCheck(ucGun_id))
    {
    	return TRUE;
    }
    else
    {
    	return FALSE;
    }
}

/**
 * @brief 充电握手交互
 * @param bMode 模式：正常/重连
 * @param ucGun_id:枪id
 * @return 下一步操作：配置/故障/超时
 */
static Com_Stage_Def_t GB_BmsHandshakeTreat(U8 ucMode, U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return STOPCHARGE;
	}

    U32 uiNow_tick = 0;
    U8 handshake_step = HANDSHAKE_BHM;
    BOOL bRet = FALSE;

    //状态变更，发送充电实时数据上传报文
    g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = (U8)HANDSHAKE_BHM;
    TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);

    if(ucMode == (U8)HANDSHAKE_RECONNECT)
    {
    	(void)memset(&g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CHM], 0, sizeof(Msg2bms_Ctrl_t)*(CMAX-1));
        g_sGB_Charger_data[ucGun_id].ucCrm_status = 0x00;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRM].bSend_flag = TRUE;
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRM].uiStart_tick = xTaskGetTickCount();
		g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = (U8)HANDSHAKE_BRM;
		handshake_step = HANDSHAKE_BRM;
    }
    else
    {
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CHM].bSend_flag = TRUE;
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[CHM].uiStart_tick = xTaskGetTickCount();
    }

    while(1)
    {
        uiNow_tick = xTaskGetTickCount();

        if (TRUE != GBCheckSysChargingErr(ucGun_id))
        {
    		return STOPCHARGE;
        }
        switch(handshake_step)
        {
            case HANDSHAKE_BHM:
            	if (ucMode != (U8)HANDSHAKE_RECONNECT)
            	{
					if ((0U == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BHM].unRecv_cnt)
						&& (0U != (uiNow_tick - g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BHM].uiStart_tick))
						&& ((uiNow_tick - g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BHM].uiStart_tick) > GB_BHM_TIMEOUT_TICK))
					{
#ifdef 	ENABLE_GB_2011
						//兼容2011版不响应BHM的车型，仍然进行绝缘检测
						handshake_step = CONFIG_ISO_CHECK;
						//2011版设置默认绝缘电压
						g_sGun_data[ucGun_id].sTemp.unEV_max_imd_vol = DEFAULT_VEHICLE_IMD_VOL;
#else
				        g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = (U8)REACH_TOTAL_SOC_TARGET;
						ChargeFinishProcess(ucGun_id, TRUE);
#endif
						my_printf(USER_INFO, "%s:%d %s HANDSHAKE_BHM timeout start_tick = %d now = %d\n", __FILE__, __LINE__,
								(ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BHM].uiStart_tick, uiNow_tick);
					}
					else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BHM].unRecv_cnt > 0U)
					{
						//绝缘检测
						handshake_step = CONFIG_ISO_CHECK;
					}
					else
					{
						//等待
					}
            	}
                break;
            case CONFIG_ISO_CHECK:
                //状态变更，发送充电实时数据上传报文
                g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = (U8)CONFIG_ISO_CHECK;
                TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);

            	bRet = GB_BmsInsulationCheck(ucGun_id);
                if(FALSE == bRet)
                {
					if (FALSE == CheckEVStop(ucGun_id))
					{
						g_sGB_Charger_data[ucGun_id].sCST.sFault_reason.sItem.unSelf_check_fault = TRUE;
					}
                	my_printf(USER_ERROR, "%s:%d %s IMD check failed\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B");

                    return STOPCHARGE;
                }
                else
                {
                	uiNow_tick = xTaskGetTickCount();
                	g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRM].uiStart_tick = uiNow_tick;
                	g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CHM].bSend_flag = FALSE;
                    g_sGB_Charger_data[ucGun_id].ucCrm_status = 0x00;
                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRM].bSend_flag = TRUE;
                    handshake_step = HANDSHAKE_BRM;
					 //状态变更，发送充电实时数据上传报文
					g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = HANDSHAKE_BRM;
					TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
					my_printf(USER_INFO, "%s stop send CHM, start send CRM 00\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
                }
            	break;
            case HANDSHAKE_BRM:
                if ((0U == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRM].unRecv_cnt)
                    && (0U != (uiNow_tick-g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRM].uiStart_tick))
                    && ((uiNow_tick-g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRM].uiStart_tick) > (GB_BRM_TIMEOUT_TICK+DELAY_200MS)))
                {
                    my_printf(USER_ERROR, "%s:%d %s BRM message timeout start = %d, now_time = %d\n", __FILE__, __LINE__,
                    		(ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRM].uiStart_tick, uiNow_tick);
            		if (g_sGun_warn[ucGun_id].sRecoverable_warn.sItem.BMS_BRM_timeout != TRUE)
            		{
            			GunAlarmSet(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_BMS_BRM_TIMEOUT);
    	    		}
            		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRM].bSend_flag = FALSE;
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRM].bTimeout_flag = TRUE;

                    return TIMEOUT;
                }
                else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRM].unRecv_cnt > 0U)
                {
					my_printf(USER_INFO, "%s stop send CRM 00, start send CRM AA\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
                    g_sGB_Charger_data[ucGun_id].ucCrm_status = 0xAAU;
                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRM].bSend_flag = TRUE;
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCP].uiStart_tick = uiNow_tick;
                    handshake_step = CONFIG_BCP;
					 //状态变更，发送充电实时数据上传报文
					g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = CONFIG_BCP;
					TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
                }
                else
                {
                	//
                }
                break;
            case CONFIG_BCP:
                if ((0U == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCP].unRecv_cnt)
                    && (0U != g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCP].uiStart_tick)
                    && ((uiNow_tick - g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCP].uiStart_tick) > (GB_BCP_TIMEOUT_TICK+DELAY_200MS)))
                {
                    my_printf(USER_ERROR, "%s:%d %s BCP timeout start_tick = %d now = %d\n", __FILE__, __LINE__,
                    		(ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCP].uiStart_tick, uiNow_tick);
            		if (g_sGun_warn[ucGun_id].sRecoverable_warn.sItem.BMS_BCP_timeout != TRUE)
            		{
            			GunAlarmSet(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_BMS_BCP_TIMEOUT);
    	    		}
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCP].bTimeout_flag = TRUE;
                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRM].bSend_flag = FALSE;

                    return TIMEOUT;
                }
                else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCP].unRecv_cnt > 0U)
                {
                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRM].bSend_flag = FALSE;

                    return CONFIGURE;
                }
                else
                {
                	//
                }
                break;
            default:
            	my_printf(USER_ERROR, "%s:%d %s handshake step error step = %d\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B", handshake_step);

            	return STOPCHARGE;
        }
        vTaskDelay(50);
    }
}

/**
 * @brief 检测电压是否正常
 * @param
 * @return 正常/错误
 */
static BOOL CheckBmsBatteryVol(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return FALSE;
	}

	if ((g_sGun_data[ucGun_id].sTemp.unVehicle_check_vol + 50U) > (g_sStorage_data.sPublic_data[ucGun_id].unGun_max_vol*10U))
	{
		my_printf(USER_ERROR, "Vehicle VOL %d(0.1V) > Gun_max_vol %d(0.1V)\n", g_sGun_data[ucGun_id].sTemp.unVehicle_check_vol + 50U, g_sStorage_data.sPublic_data[ucGun_id].unGun_max_vol*10U);
		return FALSE;
	}

	U32 uiTemp_vol = ((g_sGun_data[ucGun_id].sTemp.unVehicle_check_vol + 50U) * GB_UNDER_VOL_LIMIT / 100U);

	if (FALSE == my_unsigned_abs(g_sGun_data[ucGun_id].uiOutput_vol/10U, g_sGun_data[ucGun_id].sTemp.unVehicle_check_vol + 50U, uiTemp_vol))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
 * @brief: 国标预充完成继电器控制
 * @param: ucGun_id；枪id
 * @return
 */
void GB_PrechargeFinishRelayControl(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	if (GUN_A == ucGun_id)
	{
		//闭合负极继电器
		RelayControl_A(NEGATIVE_RELAY, CLOSE_CONTROL);
		vTaskDelay(10);
		//闭合主回路正极继电器
		RelayControl_A(POSITIVE_RELAY, CLOSE_CONTROL);
	}
	else if (GUN_B == ucGun_id)
	{
		//闭合负极继电器
		RelayControl_B(NEGATIVE_RELAY, CLOSE_CONTROL);
		vTaskDelay(10);
		//闭合主回路正极继电器
		RelayControl_B(POSITIVE_RELAY, CLOSE_CONTROL);
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d ucGun_id error = %d\n", __func__, __LINE__, ucGun_id);
	}
}

/**
 * @brief 预充
 * @param ucGun_id 枪id
 * @return
 */
static Com_Stage_Def_t GB_BmsPrechargeTreat(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return STOPCHARGE;
	}

	g_sGun_data[ucGun_id].sTemp.ucPrecharge_status = TRUE;
	U32 uiPrecharge_start_tick = xTaskGetTickCount();
	my_printf(USER_INFO, "%s start precharge tick = %d VOL = %dV\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B", uiPrecharge_start_tick,
			g_sGun_data[ucGun_id].sTemp.unVehicle_check_vol/10U);

	//清空BRO 00接受次数
	g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].unRecv_cnt = 0;
	g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_AA].uiStart_tick = xTaskGetTickCount();


	//预充超时60s,可自己计时或者由车端判定CRO_AA超时
	while (1)
	{
		U32 uiNow_tick = xTaskGetTickCount();

        if (TRUE != GBCheckSysChargingErr(ucGun_id))
        {
        	return STOPCHARGE;
        }

        //车端BRO状态从AA->00
        if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].unRecv_cnt > 2U)
        {
    		if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.unBms_other_fault != (U8)TRUE)
    		{
    			GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_OTHER_FAULT);
    		}

    		g_sGB_Charger_data[ucGun_id].sCST.sFault_reason.sItem.unOther_fault = TRUE;
    		my_printf(USER_ERROR, "%s:%d %s BMS BRO status AA->00\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B");

    		return STOPCHARGE;
        }

        if ((uiNow_tick - g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_AA].uiStart_tick) > GB_BRO_00_TIMEOUT_TICK)
        {
            my_printf(USER_ERROR, "%s:%d %s BRO AA message timeout 5s\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
    		if (g_sGun_warn[ucGun_id].sRecoverable_warn.sItem.BMS_BRO_timeout != TRUE)
    		{
    			GunAlarmSet(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_BMS_BRO_TIMEOUT);
    		}
            g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CTS].bSend_flag = FALSE;
            g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CML].bSend_flag = FALSE;
            g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_AA].bTimeout_flag = TRUE;

            return TIMEOUT;
        }

    	//预充未超时
    	if ((uiNow_tick - uiPrecharge_start_tick) < GB_PRECHARGE_TIMEOUT)
    	{
			if ((uiNow_tick - uiPrecharge_start_tick) > 6000)
			{
				U16 unTemp_vol = g_sGun_data[ucGun_id].sTemp.unVehicle_check_vol - GB_STANDARD_PRECHARGE_LIMIT_VOL;
				if (g_psIMD_data->sPublic_data[ucGun_id].unBus_vol >= unTemp_vol)
				{
					GB_PrechargeFinishRelayControl(ucGun_id);
					g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = CHARGING_BCL_BCS;
					//状态变更，发送充电实时数据上传报文
					TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
					my_printf(USER_INFO, "%s precharge complete:meter_vol = %dV IMD_VOL = %dV target_vol = %dV tick = %d\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B",
							g_sGun_data[ucGun_id].uiOutput_vol/100U, g_psIMD_data->sPublic_data[ucGun_id].unBus_vol/10U, g_sGun_data[ucGun_id].sTemp.unVehicle_check_vol/10U, xTaskGetTickCount());

					return TRUE;
				}
    		}
    	}
    	else
    	{
    		g_sGB_Charger_data[ucGun_id].sCST.sFault_reason.sItem.unPrecharge_fault = TRUE;
    		PrechargeFailProcess(ucGun_id);

    		return STOPCHARGE;
    	}
		vTaskDelay(50);
	}
}

/**
 * @brief 充电配置交互
 * @param ucGun_id:枪id
 * @return下一步操作：预充/故障/超时
 */
static Com_Stage_Def_t GB_BmsConfigTreat(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return STOPCHARGE;
	}

    U32 uiNow_tick = 0;
    U8 ucConfig_step = CONFIG_BRO;
    U8 ucBcl_bcs_recv_flag = 0;

	//状态变更，发送充电实时数据上传报文
	g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = CONFIG_BRO;
	TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);

    while (1)
    {
        uiNow_tick = xTaskGetTickCount();

        if (TRUE != GBCheckSysChargingErr(ucGun_id))
        {
            return STOPCHARGE;
        }

        switch(ucConfig_step)
        {
            case CONFIG_BRO:
                if ((0U == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].unRecv_cnt)
                    && (0U != g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].uiStart_tick)
                    && ((uiNow_tick-g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].uiStart_tick) > GB_BRO_00_TIMEOUT_TICK))
                {
                    my_printf(USER_ERROR, "%s:%d %s BRO 00 message timeout 5s start_tick = %d now = %d\n", __FILE__, __LINE__,
                    		(ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].uiStart_tick, uiNow_tick);
            		if (g_sGun_warn[ucGun_id].sRecoverable_warn.sItem.BMS_BRO_timeout != TRUE)
            		{
            			GunAlarmSet(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_BMS_BRO_TIMEOUT);
    	    		}
                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CTS].bSend_flag = FALSE;
                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CML].bSend_flag = FALSE;
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].bTimeout_flag = TRUE;

                    return TIMEOUT;
                }
                else if (0U != g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].unRecv_cnt)
                {
                   //等待BRO_AA
                }
                else
                {
                	//等待BRO_00
                }
                if ((0U == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_AA].unRecv_cnt) &&
                  ((uiNow_tick-g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_AA].uiStart_tick) > GB_BRO_AA_TIMEOUT_TICK))
                {
                    my_printf(USER_ERROR, "%s:%d %s BRO AA message timeout 60s\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
            		if (g_sGun_warn[ucGun_id].sRecoverable_warn.sItem.BMS_BRO_timeout != TRUE)
            		{
            			GunAlarmSet(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_BMS_BRO_TIMEOUT);
    	    		}
                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CTS].bSend_flag = FALSE;
                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CML].bSend_flag = FALSE;
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_AA].bTimeout_flag = TRUE;

                    return TIMEOUT;
                }
                else
                {
                    if ((g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_AA].unRecv_cnt) > 0U)
                    {
                    	static U8 ucCnt = 0;
						g_sGB_Charger_data[ucGun_id].ucCro_status = 0;
						g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRO].bSend_flag = TRUE;

                    	if (TRUE == CheckBmsBatteryVol(ucGun_id))
                    	{
                    		ucCnt = 0;
							g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CTS].bSend_flag = FALSE;
							g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CML].bSend_flag = FALSE;
							ucConfig_step = CONFIG_PRECHARGE;
							//状态变更，发送充电实时数据上传报文
							g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = CONFIG_PRECHARGE;
							TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
							my_printf(USER_INFO, "%s start send CRO 00\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
                    	}
                    	else
                    	{
                    		ucCnt++;
                    	}

                		//滤波2S
                		if (ucCnt >= (2000/15U))
                		{
                			ucCnt = 0;
                			g_sGB_Charger_data[ucGun_id].sCST.sFault_reason.sItem.unPrecharge_fault = TRUE;
							g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CTS].bSend_flag = FALSE;
							g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CML].bSend_flag = FALSE;
							g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRO].bSend_flag = FALSE;

		            		if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.unBms_vol_neq_msg_vol != TRUE)
		            		{
		            			GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_VOL_NEQ_MSG_VOL);
		            		}
							my_printf(USER_ERROR, "%s:%d %s BMS VOL unusual, meter VOL = %d BMS response VOL = %d\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B",
									g_sGun_data[ucGun_id].uiOutput_vol/10U, g_sGun_data[ucGun_id].sTemp.unVehicle_check_vol+5U);

							return STOPCHARGE;
                		}
                    }
                }
                break;
            case CONFIG_PRECHARGE:
            	//预充
            	Com_Stage_Def_t sTemp = GB_BmsPrechargeTreat(ucGun_id);
                if (STOPCHARGE == sTemp)
                {
                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRO].bSend_flag = FALSE;

					return STOPCHARGE;
                }
                else if (TIMEOUT == sTemp)
                {
                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRO].bSend_flag = FALSE;

					return TIMEOUT;
                }
                else
                {
					g_sGB_Charger_data[ucGun_id].ucCro_status = 0xAAU;
					g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRO].bSend_flag = TRUE;
					//状态变更，发送充电实时数据上传报文
					g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = (U8)CHARGING_BCL_BCS;
					TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
					my_printf(USER_INFO, "%s precharge complete, start send CRO AA\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
					g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].uiStart_tick = xTaskGetTickCount();
					g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].uiStart_tick = xTaskGetTickCount();
					ucConfig_step = (U8)CHARGING_BCL_BCS;
                }
                break;
            case CHARGING_BCL_BCS:
                if ((0U == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].unRecv_cnt)
                    && (0U != g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].uiStart_tick)
                    && ((uiNow_tick-g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].uiStart_tick) > GB_BCL_TIMEOUT_TICK))
                {
                    my_printf(USER_ERROR, "%s:%d %s BCL message timeout 1s start_tick = %d now = %d\n", __FILE__, __LINE__,
                    		(ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].uiStart_tick, uiNow_tick);
            		if (g_sGun_warn[ucGun_id].sRecoverable_warn.sItem.BMS_BCL_timeout != TRUE)
            		{
            			GunAlarmSet(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_BMS_BCL_TIMEOUT);
    	    		}
                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRO].bSend_flag = FALSE;
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].bTimeout_flag = TRUE;

                    return TIMEOUT;
                }
                else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].unRecv_cnt > 0U)
                {
                	ucBcl_bcs_recv_flag |= 0x01U;
                }
                else
                {
                	//
                }
                if ((0U == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].unRecv_cnt)
                    && (0U != g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].uiStart_tick)
                    && ((uiNow_tick-g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].uiStart_tick) > (GB_BCS_TIMEOUT_TICK+DELAY_200MS)))
                {
                    my_printf(USER_ERROR, "%s:%d %s BCS message timeout 5s start_tick = %d now = %d\n", __FILE__, __LINE__,
                    		(ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].uiStart_tick, uiNow_tick);
            		if (g_sGun_warn[ucGun_id].sRecoverable_warn.sItem.BMS_BCS_timeout != TRUE)
            		{
            			GunAlarmSet(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_BMS_BCS_TIMEOUT);
    	    		}
                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRO].bSend_flag = FALSE;
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].bTimeout_flag = TRUE;

                    return TIMEOUT;
                }
                else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].unRecv_cnt > 0U)
                {
                	ucBcl_bcs_recv_flag |= 0x02U;
                }
                else
                {
                	//
                }
                if (0x03U == ucBcl_bcs_recv_flag)
                {
                	g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRO].bSend_flag = FALSE;
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].uiStart_tick = uiNow_tick;  //更新时间
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].uiStart_tick = uiNow_tick;
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].unRecv_cnt = 0;
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].unRecv_cnt = 0;

                    return CHARGING;
                }
                break;
            default:
            	my_printf(USER_ERROR, "%s:%d %s configure type error :ucConfig_step = %d\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B", ucConfig_step);

            	return STOPCHARGE;
        }
        vTaskDelay(50);
    }
}

/**
 * @brief 充电中交互
 * @param ucGun_id 枪id
 * @return 超时/故障/继续充电
 */
static Com_Stage_Def_t GB_BmsChargingTreat(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return STOPCHARGE;
	}

	U32 uiNow_tick = xTaskGetTickCount();

	if (TRUE != GBCheckSysChargingErr(ucGun_id))
	{
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CCS].bSend_flag = FALSE;

		return STOPCHARGE;
	}

	//2023版需要支持BSM超时且BSM不走重连
	if (TRUE == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSM].bEnable_flag)
	{
		if ((0U == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSM].unRecv_cnt) &&
				((uiNow_tick - g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSM].uiStart_tick) > GB_BSM_TIMEOUT_TICK))
		{
			my_printf(USER_ERROR, "%s:%d %s: BSM timeout 5s start_tick = %d now = %d\n", __FILE__, __LINE__,
					(ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSM].uiStart_tick, uiNow_tick);
    		if (g_sGun_warn[ucGun_id].sRecoverable_warn.sItem.BMS_BSM_timeout != TRUE)
    		{
    			GunAlarmSet(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_BMS_BSM_TIMEOUT);
			}
			g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSM].bTimeout_flag = TRUE;
			g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CCS].bSend_flag = FALSE;

			return STOPCHARGE;
		}
		else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSM].unRecv_cnt > 0U)
		{
			g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSM].uiStart_tick = uiNow_tick;
			g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSM].unRecv_cnt = 0;
		}
		else
		{
			//
		}
	}

	if ((0U == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].unRecv_cnt) &&
		((uiNow_tick - g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].uiStart_tick) > GB_BCL_TIMEOUT_TICK))
	{
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].bTimeout_flag = TRUE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CCS].bSend_flag = FALSE;
		if (g_sGun_warn[ucGun_id].sRecoverable_warn.sItem.BMS_BCL_timeout != TRUE)
		{
			GunAlarmSet(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_BMS_BCL_TIMEOUT);
		}
		my_printf(USER_ERROR, "%s:%d %s: BCL timeout 1S start_tick = %d now = %d\n", __FILE__, __LINE__,
				(ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].uiStart_tick, uiNow_tick);

		return TIMEOUT;
	}
	else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].unRecv_cnt > 0U)
	{
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].uiStart_tick = uiNow_tick;
		//重置CNT，防止越界
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].unRecv_cnt = 0;

		if (g_sGB_Version[GUN_B] == GB_2023)
		{
			//需求电压小于充电桩最小输出电压
			if (g_sGun_data[ucGun_id].sTemp.unEV_target_vol < (g_sStorage_data.sPublic_data[ucGun_id].unGun_min_vol*10U))
			{
				my_printf(USER_ERROR, "%s:%d %s receive BCL target VOL < gun minimum output VOL\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B");

				return STOPCHARGE;
			}
		}

		//记录充电时间
		g_sGun_data[ucGun_id].sTemp.unAlready_charge_time = (U16)((uiNow_tick - g_sGun_data[ucGun_id].sTemp.uiStart_time)/1000U);

		if (0U == g_sGun_data[ucGun_id].sTemp.unEV_target_vol)
		{
			if (0U == g_uiErr_vol_tick[ucGun_id])
			{
				g_uiErr_vol_tick[ucGun_id] = uiNow_tick;
			}
			if ((uiNow_tick - g_uiErr_vol_tick[ucGun_id]) > FAULT_OVER_VOL_TIMEOUT)
			{
				if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.unBms_vol_err != TRUE)
				{
					GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_VOL_ERR);
					my_printf(USER_ERROR, "%s check target_vol = 0\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
				}
				g_uiErr_vol_tick[ucGun_id] = 0U;

				return STOPCHARGE;
			}
		}
		else
		{
			g_uiErr_vol_tick[ucGun_id] = 0U;

			if (TRUE == BmsChargingAlarmCheck(uiNow_tick, ucGun_id))
			{
				return STOPCHARGE;
			}
		}
	}
	else
	{
		//
	}

	//兼容部分车型不发BCS，取消BCS超时判定
	if ((0U == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].unRecv_cnt) &&
			((uiNow_tick - g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].uiStart_tick) > GB_BCS_TIMEOUT_TICK))
	{
		if (g_sGun_warn[ucGun_id].sRecoverable_warn.sItem.BMS_BCS_timeout != TRUE)
		{
			GunAlarmSet(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_BMS_BCS_TIMEOUT);
		}
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].bTimeout_flag = TRUE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CCS].bSend_flag = FALSE;
		my_printf(USER_ERROR, "%s:%d %s: BCS timeout 5s start_tick = %d now = %d\n", __FILE__, __LINE__,
				(ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].uiStart_tick, uiNow_tick);

		return TIMEOUT;
	}
	else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].unRecv_cnt > 0U)
	{
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].uiStart_tick = uiNow_tick;
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].unRecv_cnt = 0;
	}
	else
	{
		//
	}

	return (Com_Stage_Def_t)TRUE;
}

/**
 * @brief 充电结束
 * @param ucGun_id: 枪id
 * @return TRUE:结束充电处理完成  FALSE:结束充电处理中
 */
static BOOL GB_BmsStopChargeTreat(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return TRUE;
	}

	U32 uiNow_tick = xTaskGetTickCount();

	//BST超时
	if ((2U > g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].unRecv_cnt) &&
			((uiNow_tick - g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].uiStart_tick) > GB_BST_TIMEOUT_TICK))
	{
		//走故障逻辑
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].bTimeout_flag = TRUE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CEM].bSend_flag = TRUE;
        //状态变更，发送充电实时数据上传报文
		g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = END_OF_CHARGE_CEM;
		TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
   		if (g_sGun_warn[ucGun_id].sRecoverable_warn.sItem.BMS_BST_timeout != TRUE)
    	{
    		GunAlarmSet(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_BMS_BST_TIMEOUT);
		}
		//g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = FAULT_STOP_MODE;

		my_printf(USER_ERROR, "%s:%d STOPCHARGE: %s BST timeout 5s CNT = %d\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B",
				g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].unRecv_cnt);
		vTaskDelay(1000);

		return TRUE;
	}
	else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].unRecv_cnt >= 2U)
	{
    	//状态变更，发送充电实时数据上传报文
		if (END_OF_CHARGE_CSD != g_sGun_data[ucGun_id].sTemp.ucCharge_current_step)
		{
			g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = END_OF_CHARGE_CSD;
			TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
		}
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CEM].bSend_flag = FALSE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CST].bSend_flag = FALSE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CSD].bSend_flag = TRUE;

		if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].uiStart_tick ==
				g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].uiStart_tick)
		{
			g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].uiStart_tick = uiNow_tick;
			my_printf(USER_INFO, "STOPCHARGE: %s receive EV BST CNT = %d BSD CNT = %d start send CSD\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B",
					g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].unRecv_cnt, g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].unRecv_cnt);
		}
	}
	else
	{
		//
	}

	//BSD超时
	if ((2U > g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].unRecv_cnt) &&
			((uiNow_tick - g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].uiStart_tick) > GB_BSD_TIMEOUT_TICK))
	{
		//走故障逻辑
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].bTimeout_flag = TRUE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CEM].bSend_flag = TRUE;
        //状态变更，发送充电实时数据上传报文
		g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = END_OF_CHARGE_CEM;
		TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
   		if (g_sGun_warn[ucGun_id].sRecoverable_warn.sItem.BMS_BSD_timeout != TRUE)
    	{
    		GunAlarmSet(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_BMS_BSD_TIMEOUT);
		}
		//g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = FAULT_STOP_MODE;

		my_printf(USER_ERROR, "%s:%d STOPCHARGE: %s BSD timeout 10s CNT = %d\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B",
				g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].unRecv_cnt);
		vTaskDelay(1000);

		return TRUE;
	}
	else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].unRecv_cnt >= 2U)
	{
    	//状态变更，发送充电实时数据上传报文
		if (END_OF_CHARGE_CSD != g_sGun_data[ucGun_id].sTemp.ucCharge_current_step)
		{
			g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = END_OF_CHARGE_CSD;
			TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
		}
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CEM].bSend_flag = FALSE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CST].bSend_flag = FALSE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CSD].bSend_flag = TRUE;

		my_printf(USER_INFO, "STOPCHARGE: %s receive BSD CNT = %d send CSD\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].unRecv_cnt);
		vTaskDelay(1000);

		return TRUE;
	}
	else
	{
		//
	}

	return FALSE;
}

/**
 * @brief 充电过程中报文超时
 * @param ucGun_id: 枪id
 * @return TRUE：超时处理完成  FALSE:超时处理中
 */
static BOOL GB_BmsTimeoutTreat(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return TRUE;
	}

	//判断重连次数
    if(g_sBms_ctrl[ucGun_id].ucReconnect_cnt < 3U)
    {
		while (1)
		{
			//等待泄压
			if (((g_psMeter_data->sPublic_data[ucGun_id].uiDc_vol/10U) < SAFE_VOL_LIMIT)
					|| (g_psIMD_data->sPublic_data[ucGun_id].unBus_vol < SAFE_VOL_LIMIT))
			{
				if(g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CEM].uiSend_cnt >= (2000U/250U))
				{
					g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CEM].bSend_flag = FALSE;

					(void)memset(&g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BHM], 0, sizeof(Recv_Bms_Timeout_t)*(U8)BMAX);
					g_sBms_ctrl[ucGun_id].ucReconnect_cnt++;

					return TRUE;
				}
				vTaskDelay(50);
			}
			else
			{
				vTaskDelay(50);
			}
		}
    }

	return FALSE;
}

static void GBChargeControl(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	Com_Stage_Def_t eResult = INITDATA;
	(void)memset(&g_sBms_ctrl[ucGun_id], 0, sizeof(g_sBms_ctrl[ucGun_id]));

	if ((NULL != g_psMeter_data) && (NULL != g_psIMD_data))
	{
		//判定充电启动前电压是否安全
		U16 unImd_vol = g_psIMD_data->sPublic_data[ucGun_id].unBus_vol;
		U16 unMeter_vol = g_sGun_data[ucGun_id].uiOutput_vol / 10U;

		if ((unImd_vol > START_CHARGE_VOL_LIMIT) || (unMeter_vol > START_CHARGE_VOL_LIMIT))
		{
			//过滤
			vTaskDelay(1000);
			if ((unImd_vol > START_CHARGE_VOL_LIMIT) || (unMeter_vol > START_CHARGE_VOL_LIMIT))
			{
				g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = (U8)FAULT_STOP_MODE;
				if (g_sGun_fault[ucGun_id].sGeneral_fault.sItem.start_vol_more_than_safe_vol != (U8)TRUE)
				{
					GunAlarmSet(&g_sGun_fault[ucGun_id].sGeneral_fault.uiWhole_flag, BIT32_STRUCT, (U32)FAULT_START_VOL_MORE_THAN_SAFE_VOL);
				}
				my_printf(USER_ERROR, "%s:%d %s detected start meter VOL = %d(0.1V) or IMD VOL = %d(0.1V) > 10V\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B",
						unMeter_vol, unImd_vol);

				ChargeFinishProcess(ucGun_id, TRUE);

				return;
			}
		}
		if (BmsChargingInit(ucGun_id) == FALSE)
		{
	        g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = (U8)FAULT_STOP_MODE;

			ChargeFinishProcess(ucGun_id, TRUE);

			return;
		}
		my_printf(USER_INFO, "%s:%d %s BmsChargingInit success\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d %s g_psMeter_data = NULL\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
		vTaskDelay(2000);

		return ;
	}

	while (1)
	{
		switch (g_sBms_ctrl[ucGun_id].sStage.sNow_status)
		{
			case INITDATA:
				if (TRUE == BmsStageSwitchCheck(&g_sBms_ctrl[ucGun_id].sStage))
				{
					//只用作状态初始化，不处理
				}
				g_sBms_ctrl[ucGun_id].sStage.sNow_status = HANDSHAKE;
				break;
			case HANDSHAKE:
				if (TRUE == BmsStageSwitchCheck(&g_sBms_ctrl[ucGun_id].sStage))
				{
					g_sGun_data[ucGun_id].sTemp.bAllow_charge_flag = TRUE;
				}
				else
				{
					//非重连
					if(0U == g_sBms_ctrl[ucGun_id].ucReconnect_cnt)
					{
						eResult = GB_BmsHandshakeTreat(HANDSHAKE_NORMAL, ucGun_id);
					}
					else//重连
					{
						eResult = GB_BmsHandshakeTreat(HANDSHAKE_RECONNECT, ucGun_id);
					}

					if (CONFIGURE == eResult)
					{
						g_sBms_ctrl[ucGun_id].sStage.sNow_status = CONFIGURE;
					}
					else
					{
						if (TIMEOUT == eResult)
						{
							g_sBms_ctrl[ucGun_id].sStage.sNow_status = TIMEOUT;
						}
						else
						{
							g_sBms_ctrl[ucGun_id].sStage.sNow_status = STOPCHARGE;
							if (FALSE == CheckEVStop(ucGun_id))
							{
								//填充CST
								g_sGB_Charger_data[ucGun_id].sCST.sStop_reason.sItem.ucFault_stop = TRUE;
								g_sGB_Charger_data[ucGun_id].sCST.sFault_reason.sItem.unOther_fault = TRUE;
								g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = (U8)FAULT_STOP_MODE;
							}
							else
							{
								//填充CST
								g_sGB_Charger_data[ucGun_id].sCST.sStop_reason.sItem.ucBms_stop = TRUE;
							}
						}
						my_printf(USER_ERROR, "%s:%d %s charge HANDSHAKE failed\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
					}
				}
				break;
			case CONFIGURE:
				if (TRUE == BmsStageSwitchCheck(&g_sBms_ctrl[ucGun_id].sStage))
				{
					g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CTS].bSend_flag = TRUE;
					g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CML].bSend_flag = TRUE;
					g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].uiStart_tick = xTaskGetTickCount();
					g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_AA].uiStart_tick = xTaskGetTickCount();
				}
				else
				{
					eResult = GB_BmsConfigTreat(ucGun_id);

					if (CHARGING == eResult)
					{
						g_sBms_ctrl[ucGun_id].sStage.sNow_status = CHARGING;
					}
					else
					{
						if (TIMEOUT == eResult)
						{
							g_sBms_ctrl[ucGun_id].sStage.sNow_status = TIMEOUT;
						}
						else
						{
							//填充CST
							g_sGB_Charger_data[ucGun_id].sCST.sStop_reason.sItem.ucFault_stop = TRUE;
							g_sGB_Charger_data[ucGun_id].sCST.sFault_reason.sItem.unOther_fault = TRUE;
							g_sBms_ctrl[ucGun_id].sStage.sNow_status = STOPCHARGE;
							g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = (U8)FAULT_STOP_MODE;
						}
						my_printf(USER_ERROR, "%s:%d %s charge CONFIGURE failed\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
					}
				}
				break;
			case CHARGING:
				if (TRUE == BmsStageSwitchCheck(&g_sBms_ctrl[ucGun_id].sStage))
				{
					g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CCS].bSend_flag = TRUE;
				}
				else
				{
					g_bStart_charge_success_flag[ucGun_id] = TRUE;

					eResult = GB_BmsChargingTreat(ucGun_id);

					if ((Com_Stage_Def_t)TRUE != eResult)
					{
						if (TIMEOUT == eResult)
						{
							g_sBms_ctrl[ucGun_id].sStage.sNow_status = TIMEOUT;
							my_printf(USER_ERROR, "%s:%d %s CHARGING-> TIMEOUT\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
						}
						else//故障
						{
							//填充CST
							g_sGB_Charger_data[ucGun_id].sCST.sStop_reason.sItem.ucFault_stop = TRUE;
							g_sBms_ctrl[ucGun_id].sStage.sNow_status = STOPCHARGE;
							g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = (U8)FAULT_STOP_MODE;
							my_printf(USER_ERROR, "%s:%d %s CHARGING-> STOPCHARGE\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
						}
					}
				}
				break;
			case TIMEOUT:
				if (TRUE == BmsStageSwitchCheck(&g_sBms_ctrl[ucGun_id].sStage))
				{
	            	//状态变更，发送充电实时数据上传报文
					g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = END_OF_CHARGE_CEM;
					TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);

					if (GUN_A == ucGun_id)
					{
						//电流小于5A且主回路继电器反馈为闭合
						if (((g_sGun_data[GUN_A].uiOutput_cur/10U) < SAFE_CUR_LIMIT)
								&& ((0U == GET_POSITIVE_RELAY_A_FB_STATUS()) || (0U == GET_NEGATIVE_RELAY_A_FB_STATUS())))
						{
							//断开高压继电器
							RelayControl_A(POSITIVE_RELAY, OPEN_CONTROL);
							vTaskDelay(10);
							RelayControl_A(NEGATIVE_RELAY, OPEN_CONTROL);
						    vTaskDelay(10);
						    RelayControl_A(PRECHARGE_RELAY, OPEN_CONTROL);
						}
					}
					else
					{
						//电流小于5A且主回路继电器反馈为闭合
						if (((g_sGun_data[GUN_B].uiOutput_cur/10U) < SAFE_CUR_LIMIT)
								&& ((0U == GET_POSITIVE_RELAY_B_FB_STATUS()) || (0U == GET_NEGATIVE_RELAY_B_FB_STATUS())))
						{
							//断开高压继电器
							RelayControl_B(POSITIVE_RELAY, OPEN_CONTROL);
							vTaskDelay(10);
							RelayControl_B(NEGATIVE_RELAY, OPEN_CONTROL);
						    vTaskDelay(10);
						    RelayControl_B(PRECHARGE_RELAY, OPEN_CONTROL);
						}
					}

					//接受到车端BEM
					if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BEM].unRecv_cnt > 0U)
					{
						my_printf(USER_ERROR, "%s:%d %s receive EV BEM fault stop charge\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
					}
					else//桩端主动发送CEM
					{
						//清除BST接受次数
						g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].unRecv_cnt = 0;

						for (U32 i = 1; i < (U8)BMAX; i++)
						{
							if (0U != g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[i].bTimeout_flag)
							{
								my_printf(USER_ERROR, "%s:%d %s TIMEOUT: recv_bms_timeout flag = %d\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B", i);
								//清除之前的发送报文标志
								(void)memset(&g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[0], 0, sizeof(Msg2bms_Ctrl_t)*(U8)CMAX);
								g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CST].bSend_flag = FALSE;
								g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CEM].bSend_flag = TRUE;
							}
						}
						my_printf(USER_INFO, "%s send CEM fault stop charge\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
					}
				}
				else
				{
					g_sGun_data[ucGun_id].sTemp.unEV_target_cur = 0U;
					g_sGun_data[ucGun_id].sTemp.unEV_target_vol = 0U;

					if (TRUE == GB_BmsTimeoutTreat(ucGun_id))
					{
						my_printf(USER_INFO, "%s receive CEM reconnect_cnt = %d\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].ucReconnect_cnt);
						g_sBms_ctrl[ucGun_id].sStage.sNow_status = HANDSHAKE;
					}
					else
					{
						my_printf(USER_ERROR, "%s timeout reconnect_cnt > 3, stop charge\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
						//填充CST
						g_sGB_Charger_data[ucGun_id].sCST.sFault_reason.sItem.unOther_fault = TRUE;
				   		if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.unTimeout_reconnect_reach_limit != TRUE)
				    	{
				    		GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_TIMEOUT_RECONNECT_REACH_LIMIT);
						}
						g_sBms_ctrl[ucGun_id].sStage.sNow_status = STOPCHARGE;
		            	g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = (U8)FAULT_STOP_MODE;
					}
				}
				break;
			case STOPCHARGE:
				if (TRUE == BmsStageSwitchCheck(&g_sBms_ctrl[ucGun_id].sStage))
				{
	            	//状态变更，发送充电实时数据上传报文
					g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = END_OF_CHARGE_CST;
	            	TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);

					if (GUN_A == ucGun_id)
					{
						StopRelayControl_A();
					}
					else
					{
						StopRelayControl_B();
					}

					g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CEM].bSend_flag = FALSE;

					//接受到车端BST停止
					if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].unRecv_cnt > 0U)
					{
				        //填充CST
				        g_sGB_Charger_data[ucGun_id].sCST.sStop_reason.sItem.ucBms_stop = (U8)TRUE;
						my_printf(USER_INFO, "%s receive BMS BST\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
					}
					else//桩端主动CST停止
					{
						g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].unRecv_cnt = 0;
						g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].unRecv_cnt = 0;
						my_printf(USER_INFO, "%s CCU send stop charge\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
					}
					g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].uiStart_tick = xTaskGetTickCount();
					g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].uiStart_tick = g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].uiStart_tick;
					g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CCS].bSend_flag = FALSE;
					g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CST].bSend_flag = TRUE;

					vTaskDelay(500);
				}
				else
				{
					g_sGun_data[ucGun_id].sTemp.unEV_target_cur = 0U;
					g_sGun_data[ucGun_id].sTemp.unEV_target_vol = 0U;

					if (TRUE == GB_BmsStopChargeTreat(ucGun_id))
					{
						my_printf(USER_INFO, "%s STOPCHARGE process complete\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
						ChargeFinishProcess(ucGun_id, FALSE);

						return;
					}
				}
				break;
			default:
				g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = (U8)FAULT_STOP_MODE;
				ChargeFinishProcess(ucGun_id, FALSE);
				my_printf(USER_ERROR, "%s:%d %s charge status error:sNow_status=%d\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sStage.sNow_status);

				break;
		}
		vTaskDelay(15);
	}
}

/**
 * @brief SECC报文发送控制函数
 * @param ucGun_id:枪id
 * @return
*/
static void GBMessageSend(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	static const Msg2bms_Info_t s_sGB_Msg2bms_info[CMAX] =
	{
	    {SEND_BMS_CHM_PACK,250,&Send2BmsCHM},
	    {SEND_BMS_CRM_PACK,250,&Send2BmsCRM},
	    {SEND_BMS_CTS_PACK,500,&Send2BmsCTS},
	    {SEND_BMS_CML_PACK,250,&Send2BmsCML},
	    {SEND_BMS_CRO_PACK,250,&Send2BmsCRO},
	    {SEND_BMS_CCS_PACK,50,&Send2BmsCCS},
	    {SEND_BMS_CST_PACK,10,&Send2BmsCST},
	    {SEND_BMS_CSD_PACK,250,&Send2BmsCSD},
	    {SEND_BMS_CEM_PACK,250,&Send2BmsCEM},
	};

	U32 uiNow_tick = xTaskGetTickCount();

	for (U8 i = 0; i < (sizeof(g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl)/sizeof(g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[0])); i++)
	{
			if ((0 != g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[i].bSend_flag) && ((uiNow_tick-g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[i].uiLast_send_tick)>=s_sGB_Msg2bms_info[i].unPeriod_time))
			{
				s_sGB_Msg2bms_info[i].pSend_fun(ucGun_id);
				g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[i].uiLast_send_tick = uiNow_tick;
//            		WDI_FEED_DOG();
			}
	}
}

g_psCharge_Control_t* GetGBChargeModel(void)
{
	return &g_sGB_charge_control;
}

void GB2015Init(void)
{
	g_sGB_charge_control.ChargeControl = &GBChargeControl;
	g_sGB_charge_control.CAN1DataParse = &GBCAN1Parse;
	g_sGB_charge_control.CAN2DataParse = &GBCAN2Parse;
	g_sGB_charge_control.CANMessageSend = &GBMessageSend;
}
