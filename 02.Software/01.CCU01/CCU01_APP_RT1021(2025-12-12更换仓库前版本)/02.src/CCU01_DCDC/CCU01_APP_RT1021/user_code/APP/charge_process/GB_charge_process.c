/*
 * GB_charge_process.c
 *
 *  Created on: 2025年5月5日
 *      Author: qjwu
 */
#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"
#include "string.h"
#include "hal_ext_rtc.h"

#include "AS_charge_process.h"
#include "charge_process.h"
#include "tcp_client_IF.h"
#include "meter_IF.h"
#include "charge_price_IF.h"
#include "emergency_fault_IF.h"
#include "cig_IF.h"
#include "charge_general.h"

#define ENABLE_CIG_RELAY_CONTROL

g_psCharge_Process_t* GB_ChargeProcess(void);
void GBChargeProcessInit(void);

static g_psCharge_Process_t g_sGB_Charge_process = {0};

/**
 * @brief 枪插启动充电态处理函数
 * @param eGun_id：枪id
 * @return
 */
static void GB_StartChargeStatusProcess(Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	static BOOL g_bPrecharge_flag[2] = {FALSE, FALSE};

	if (FAULT_STOP_MODE == g_sGun_data[eGun_id].sTemp.eStop_Charge_type)
	{
		g_eCharge_status[eGun_id] = STA_FAULT;
		my_printf(USER_INFO, "%s:%d %s STA_START_CHARGE -> STA_FAULT\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		return;
	}

	//充电启动中收到车端主动停止
	if (TRUE == CheckEVStop(eGun_id))
	{
		g_eCharge_status[eGun_id] = STA_CHARG_FINISHED;
		my_printf(USER_INFO, "%s:%d %s STA_START_CHARGE -> STA_CHARG_FINISHED\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		return;
	}

	//开始预充
	if (CONFIG_PRECHARGE == g_sGun_data[eGun_id].sTemp.eCharge_step)
	{
		g_bPrecharge_flag[eGun_id] = TRUE;
	}
	//预充完成
	else if (CHARGING_BCL_BCS == g_sGun_data[eGun_id].sTemp.eCharge_step)
	{
		if (TRUE == g_bPrecharge_flag[eGun_id])
		{
			g_eCharge_status[eGun_id] = STA_CHARGING;
			GunStatusChange(eGun_id, STA_CHARGING, FALSE, FALSE);

			g_bPrecharge_flag[eGun_id] = FALSE;
			if (NULL != g_sStart_timing_sem[eGun_id])
			{
				//启动计费
				(void)xSemaphoreGive(g_sStart_timing_sem[eGun_id]);
			}
			my_printf(USER_INFO, "%s:%d %s STA_START_CHARGE -> STA_CHARGING\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		}
	}
	else
	{
		//等待进入预充阶段
	}

	return;
}

/**
 * @brief 国标充电流程处理函数
 * @param
 * @return
 */
static void GB_GUNAChargeProcess(void)
{
	static U8 s_cnt = 0;
	s_cnt ++;
	if (s_cnt > 60U)
	{
		my_printf(USER_INFO, "GUN_A status = %d %d %d CC1 = %d GUN_B status = %d %d %d CC1 = %d\n", g_eCharge_status[GUN_A], g_sGun_data[GUN_A].eGun_common_status,
				g_sGun_data[GUN_A].sTemp.eCharge_step, g_sGun_data[GUN_A].unCC1_vol, g_eCharge_status[GUN_B], g_sGun_data[GUN_B].eGun_common_status,
				g_sGun_data[GUN_B].sTemp.eCharge_step, g_sGun_data[GUN_B].unCC1_vol);
		s_cnt = 0;
	}

	AlarmStatusProcess(GUN_A);

	switch(g_eCharge_status[GUN_A])
	{
	case STA_IDLE:
		IdleStatusProcess(GUN_A);
#ifdef	ENABLE_CIG_RELAY_CONTROL
		//出厂模式使用，否则会自动恢复导致调试/生产测试模式继电器控制不生效
		if (RELEASE_MODE == g_sPile_data.ucPile_config_mode)
		{
			//检测辅源和继电器状态，非断开则下发断开命令
			CigContrlDataInput(CIG_OPEN, OPEN_GUNA_LOCK_RELAY_AND_12V_ASSIST);
		}
#endif
		break;
	case STA_PLUGING:
		PlugingStatusProcess(GUN_A);
		//g_sBms_ctrl[GUN_A].sMsg2bms_ctrl[CHM].bSend_flag = TRUE;
		break;
	case STA_START_CHARGE:
		GB_StartChargeStatusProcess(GUN_A);
		break;
	case STA_CHARGING:
		ChargingStatusProcess(GUN_A);
		break;
	case STA_CHARG_FINISHED:
		//测试屏蔽使用，便于不插枪即可再次启动充电
		if ((U8)UNPLUGGED == GetGunStatus(GUN_A))
		{
			my_printf(USER_DEBUG, "STA_CHARG_FINISHED: GUN_A disconnection detected\n");
			g_eCharge_status[GUN_A] = STA_IDLE;
			GunStatusChange(GUN_A, STA_IDLE, FALSE, FALSE);
		}
#ifdef	ENABLE_CIG_RELAY_CONTROL
		//出厂模式使用，否则会自动恢复导致调试/生产测试模式继电器控制不生效
		if (RELEASE_MODE == g_sPile_data.ucPile_config_mode)
		{
			//检测辅源和继电器状态，非断开则下发断开命令
			CigContrlDataInput(CIG_OPEN, OPEN_GUNA_LOCK_RELAY_AND_12V_ASSIST);
		}
#endif
		break;
	case STA_FAULT:
		//配合PCU端账单生成，充电中不立即置位故障
		if (FALSE == CheckChargingStatus(GUN_A))
		{
			FaultStatusProcess(GUN_A);
		}
#ifdef	ENABLE_CIG_RELAY_CONTROL
		//出厂模式使用，否则会自动恢复导致调试/生产测试模式继电器控制不生效
		if (RELEASE_MODE == g_sPile_data.ucPile_config_mode)
		{
			//检测辅源和继电器状态，非断开则下发断开命令
			CigContrlDataInput(CIG_OPEN, OPEN_GUNA_LOCK_RELAY_AND_12V_ASSIST);
		}
#endif
		break;
	default:
		my_printf(USER_ERROR, "%s:%d Detected GUN_A status error = %d\n", __FILE__, __LINE__, g_eCharge_status[GUN_A]);
		break;
	}
}

/**
 * @brief 国标充电流程处理函数
 * @param
 * @return
 */
static void GB_GUNBChargeProcess(void)
{
	AlarmStatusProcess(GUN_B);

	switch(g_eCharge_status[GUN_B])
	{
	case STA_IDLE:
		IdleStatusProcess(GUN_B);
#ifdef	ENABLE_CIG_RELAY_CONTROL
		//出厂模式使用，否则会自动恢复导致调试/生产测试模式继电器控制不生效
		if (RELEASE_MODE == g_sPile_data.ucPile_config_mode)
		{
			//检测辅源和继电器状态，非断开则下发断开命令
			CigContrlDataInput(CIG_OPEN, OPEN_GUNB_LOCK_RELAY_AND_12V_ASSIST);
		}
#endif
		break;
	case STA_PLUGING:
		PlugingStatusProcess(GUN_B);
		//g_sBms_ctrl[GUN_B].sMsg2bms_ctrl[CHM].bSend_flag = TRUE;
		break;
	case STA_START_CHARGE:
		GB_StartChargeStatusProcess(GUN_B);
		break;
	case STA_CHARGING:
		ChargingStatusProcess(GUN_B);
		break;
	case STA_CHARG_FINISHED:
		//测试可屏蔽，便于不插枪即可再次启动充电
		if ((U8)UNPLUGGED == GetGunStatus(GUN_B))
		{
			my_printf(USER_DEBUG, "STA_CHARG_FINISHED: GUN_B disconnection detected\n");
			g_eCharge_status[GUN_B] = STA_IDLE;
			GunStatusChange(GUN_B, STA_IDLE, FALSE, FALSE);
		}
#ifdef	ENABLE_CIG_RELAY_CONTROL
		//出厂模式使用，否则会自动恢复导致调试/生产测试模式继电器控制不生效
		if (RELEASE_MODE == g_sPile_data.ucPile_config_mode)
		{
			//检测辅源和继电器状态，非断开则下发断开命令
			CigContrlDataInput(CIG_OPEN, OPEN_GUNB_LOCK_RELAY_AND_12V_ASSIST);
		}
#endif
		break;
	case STA_FAULT:
		//配合PCU端账单生成，充电中不立即置位故障
		if (FALSE == CheckChargingStatus(GUN_B))
		{
			FaultStatusProcess(GUN_B);
		}
#ifdef	ENABLE_CIG_RELAY_CONTROL
		//出厂模式使用，否则会自动恢复导致调试/生产测试模式继电器控制不生效
		if (RELEASE_MODE == g_sPile_data.ucPile_config_mode)
		{
			//检测辅源和继电器状态，非断开则下发断开命令
			CigContrlDataInput(CIG_OPEN, OPEN_GUNB_LOCK_RELAY_AND_12V_ASSIST);
		}
#endif
		break;
	default:
		my_printf(USER_ERROR, "%s:%d Detected GUN_B status error = %d\n", __FILE__, __LINE__, g_eCharge_status[GUN_B]);
		break;
	}
}

g_psCharge_Process_t* GB_ChargeProcess(void)
{
	return &g_sGB_Charge_process;
}

void GBChargeProcessInit(void)
{
	g_sGB_Charge_process.GUNAChargeProcess = &GB_GUNAChargeProcess;
	g_sGB_Charge_process.GUNBChargeProcess = &GB_GUNBChargeProcess;
}
