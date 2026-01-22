/*
 * AS_charge_process.c
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
#include "hmi_IF.h"

static g_psCharge_Process_t g_sAS_Charge_process = {0};

/**
 * @brief 枪插启动充电态处理函数
 * @param eGun_id：枪id
 * @return
 */
static void StartChargeStatusProcess(const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	static BOOL s_bPrecharge_flag[2] = {FALSE, FALSE};

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
	if (TRUE == AS_GetGunPrechargeStatus(eGun_id))
	{
		s_bPrecharge_flag[eGun_id] = TRUE;
	}
	//预充完成
	else if (FALSE == AS_GetGunPrechargeStatus(eGun_id))
	{
		if (TRUE == s_bPrecharge_flag[eGun_id])
		{
			my_printf(USER_INFO, "%s:%d %s STA_START_CHARGE -> STA_CHARGING\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");

			g_eCharge_status[eGun_id] = STA_CHARGING;
			GunStatusChange(eGun_id, STA_CHARGING, FALSE, FALSE);

			s_bPrecharge_flag[eGun_id] = FALSE;
			if (NULL != g_sStart_timing_sem[eGun_id])
			{
				//启动计费
				(void)xSemaphoreGive(g_sStart_timing_sem[eGun_id]);
			}
		}
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d %s precharge status error\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
	}

	return;
}

/**
 * @brief 欧美标充电流程处理函数
 * @param
 * @return
 */
static void AS_GUNAChargeProcess(void)
{
	AlarmStatusProcess(GUN_A);

	switch(g_eCharge_status[GUN_A])
	{
	case STA_IDLE:
		IdleStatusProcess(GUN_A);
		break;
	case STA_PLUGING:
		PlugingStatusProcess(GUN_A);
		break;
	case STA_START_CHARGE:
		StartChargeStatusProcess(GUN_A);
		break;
	case STA_CHARGING:
		ChargingStatusProcess(GUN_A);
		break;
	case STA_CHARG_FINISHED:
		//测试屏蔽使用，便于不插枪即可再次启动充电
		if ((U8)UNPLUGGED == GetGunStatus(GUN_A))
		{
			my_printf(USER_INFO, "STA_CHARG_FINISHED: GUN_A disconnection detected\n");
			g_eCharge_status[GUN_A] = STA_IDLE;
			GunStatusChange(GUN_A, STA_IDLE, FALSE, FALSE);
		}
		break;
	case STA_FAULT:
		//配合PCU端账单生成，充电中不立即置位故障
		if (FALSE == CheckChargingStatus(GUN_A))
		{
			FaultStatusProcess(GUN_A);
		}
		break;
	default:
		my_printf(USER_ERROR, "%s:%d Detected GUN_A status error = %d\n", __FILE__, __LINE__, g_eCharge_status[GUN_A]);
		break;
	}
}

/**
 * @brief 欧美标充电流程处理函数
 * @param
 * @return
 */
static void AS_GUNBChargeProcess(void)
{
	static U8 s_cnt = 0;
	s_cnt ++;
	if (s_cnt > 60U)
	{
		my_printf(USER_INFO, "GUN_A status = %d %d %d GUN_B status = %d %d %d\n", g_eCharge_status[GUN_A], g_sGun_data[GUN_A].eGun_common_status,
				g_sGun_data[GUN_A].sTemp.eCharge_step, g_eCharge_status[GUN_B], g_sGun_data[GUN_B].eGun_common_status,
				g_sGun_data[GUN_B].sTemp.eCharge_step);
		s_cnt = 0;
	}

	AlarmStatusProcess(GUN_B);

	switch(g_eCharge_status[GUN_B])
	{
	case STA_IDLE:
		IdleStatusProcess(GUN_B);
		break;
	case STA_PLUGING:
		PlugingStatusProcess(GUN_B);
		break;
	case STA_START_CHARGE:
		StartChargeStatusProcess(GUN_B);
		break;
	case STA_CHARGING:
		ChargingStatusProcess(GUN_B);
		break;
	case STA_CHARG_FINISHED:
		//检测到拔枪
		if ((U8)UNPLUGGED == GetGunStatus(GUN_B))
		{
			my_printf(USER_INFO, "STA_CHARG_FINISHED: GUN_B disconnection detected\n");
			g_eCharge_status[GUN_B] = STA_IDLE;
			GunStatusChange(GUN_B, STA_IDLE, FALSE, FALSE);
		}
		break;
	case STA_FAULT:
		//配合PCU端账单生成，充电中不立即置位故障
		if (FALSE == CheckChargingStatus(GUN_B))
		{
			FaultStatusProcess(GUN_B);
		}
		break;
	default:
		my_printf(USER_ERROR, "%s:%d Detected GUN_B status error = %d\n", __FILE__, __LINE__, g_eCharge_status[GUN_B]);
		break;
	}
}

g_psCharge_Process_t* AS_ChargeProcess(void)
{
	return &g_sAS_Charge_process;
}

void ASChargeProcessInit(void)
{
	g_sAS_Charge_process.GUNAChargeProcess = &AS_GUNAChargeProcess;
	g_sAS_Charge_process.GUNBChargeProcess = &AS_GUNBChargeProcess;
}
