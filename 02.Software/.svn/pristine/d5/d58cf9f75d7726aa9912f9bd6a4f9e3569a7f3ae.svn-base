/*
 * charge_process.c
 *
 *  Created on: 2024年11月1日
 *      Author: qjwu
 */
#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"
#include "string.h"
#include "hal_ext_rtc.h"

#include "AS_charge_process.h"
#include "GB_charge_process.h"
#include "charge_process.h"
#include "tcp_client_IF.h"
#include "SignalManage.h"
#include "emergency_fault_IF.h"
#include "uart_comm.h"
#include "hmi_IF.h"

static SemaphoreHandle_t g_sGun_status_change_mutex = NULL;
static g_psCharge_Process_t *g_psCharge_process = NULL;
static g_psCharge_Process_t *g_psCharge_process_A = NULL;
static g_psCharge_Process_t *g_psCharge_process_B = NULL;

Charge_Sta_e g_eCharge_status[GUN_MAX_NUM] = {STA_IDLE, STA_IDLE};
BOOL g_bStart_charge_flag[GUN_MAX_NUM] = {FALSE, FALSE};

/**
 * @brief 兼容性函数，用于选择不同标准的控制函数
 * @param
 * @return
 */
static void ChargeProcessSelect(void)
{
    if (g_iGun_module[0] == g_iGun_module[1])
    {
		if (g_iGun_module[0] == GB)
		{
			g_psCharge_process = GB_ChargeProcess();
			my_printf(USER_INFO, "use GB charge process\n");
		}
		else
		{
			g_psCharge_process = AS_ChargeProcess();
			my_printf(USER_INFO, "use AS charge process\n");
		}
    }
    else
    {
		if (g_iGun_module[0] == GB)
		{
			g_psCharge_process_A = GB_ChargeProcess();
			my_printf(USER_INFO, "GUNA use GB charge process\n");
		}
		else
		{
			g_psCharge_process_A = AS_ChargeProcess();
			my_printf(USER_INFO, "GUNA use AS charge process\n");
		}

		if (g_iGun_module[1] == GB)
		{
			g_psCharge_process_B = GB_ChargeProcess();
			my_printf(USER_INFO, "GUNB use GB charge process\n");
		}
		else
		{
			g_psCharge_process_B = AS_ChargeProcess();
			my_printf(USER_INFO, "GUNB use AS charge process\n");
		}
    }
}

/**
 * @brief 检查枪号下标是否有效
 * @param eGun_id：枪id
 * @return
 */
BOOL GunIdValidCheck(const Gun_Id_e eGun_id)
{
	if (eGun_id >= GUN_MAX_NUM)
	{
		my_printf(USER_ERROR, "%s:%d eGun_id = %d > 1\n", __FILE__, __LINE__);
		return FALSE;
	}

	return TRUE;
}

/**
 * @brief 检查车端结束
 * @param eGun_id：枪id
 * @return
 */
BOOL CheckEVStop(const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return FALSE;
	}

	switch (g_sGun_data[eGun_id].sTemp.eStop_Charge_type)
	{
	case EV_DISCON_MODE:
		break;
	case REACH_TOTAL_SOC_TARGET:
		break;
	case REACH_TOTAL_VOL_TARGET:
		break;
	case REACH_SINGLE_VOL_TARGET:
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

/**
 * @brief 枪空闲状态处理函数
 * @param eGun_id：枪id
 * @return
 */
void IdleStatusProcess(const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	static BOOL s_bClear_flag[2] = {0};

	//清除充电数据
	if (FALSE == s_bClear_flag[eGun_id])
	{
		ClearChargingData(eGun_id);
		s_bClear_flag[eGun_id] = TRUE;
		my_printf(USER_INFO, "%s:%d STA_IDLE: %s unplug clear data\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
	}

	//检测到插枪
	if ((U8)PLUGGED_IN == GetGunStatus(eGun_id))
	{
		s_bClear_flag[eGun_id] = FALSE;
		my_printf(USER_INFO, "%s:%d STA_IDLE: %s connection detected\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		g_eCharge_status[eGun_id] = STA_PLUGING;
		GunStatusChange(eGun_id, STA_PLUGING, FALSE, FALSE);
	}
	else
	{
		if (STA_IDLE != g_sGun_data[eGun_id].eGun_common_status)
		{
			my_printf(USER_INFO, "%s:%d %s set STA_IDLE\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
			GunStatusChange(eGun_id, STA_IDLE,	FALSE, FALSE);
		}
	}
}

/**
 * @brief 即插即充启动充电函数
 * @param eGun_id：枪id
 * @return
 */
static void PlugStartCharge(const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	g_sGun_data[eGun_id].sTemp.eStart_Charge_type = PNC_START_MODE;
	g_sMsg_control.sGun_charge_control.ucStart_id = eGun_id+1U;
	g_sGun_data[eGun_id].sTemp.ucAuth_type = START_CHARGE_AUTH;
	TcpSendControl(&g_sMsg_control.sGun_charge_control.bSend_auth_flag);
}

/**
 * @brief 枪插枪状态处理函数
 * @param eGun_id：枪id
 * @return
 */
void PlugingStatusProcess(const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	//检测到拔枪
	if ((U8)UNPLUGGED == GetGunStatus(eGun_id))
	{
		my_printf(USER_INFO, "%s:%d STA_PLUGING: %s disconnection detected\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		g_eCharge_status[eGun_id] = STA_IDLE;
		GunStatusChange(eGun_id, STA_IDLE, FALSE, FALSE);
	}
	else
	{
#ifdef  ENABLE_PNC
		//即插即充使用
		PlugStartCharge(eGun_id);
		g_eCharge_status[eGun_id] = STA_START_CHARGE;
		g_bStart_charge_flag[eGun_id] = FALSE;
		my_printf(USER_INFO, "%s:%d STA_PLUGING: %s start charge\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
#else
		//接受到充电启动标志
		if (g_bStart_charge_flag[eGun_id])
		{
			g_eCharge_status[eGun_id] = STA_START_CHARGE;
			g_bStart_charge_flag[eGun_id] = FALSE;
			my_printf(USER_INFO, "%s:%d STA_PLUGING: %s start charge\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		}
#endif
	}

	return;
}

/**
 * @brief 枪充电中状态处理函数
 * @param eGun_id：枪id
 * @return
 */
void ChargingStatusProcess(const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	if (FAULT_STOP_MODE == g_sGun_data[eGun_id].sTemp.eStop_Charge_type)
	{
		g_eCharge_status[eGun_id] = STA_FAULT;
		my_printf(USER_INFO, "%s:%d STA_CHARGING:check %s error\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
	}
	else
	{
		//检测到结束id
		if (g_sGun_data[eGun_id].eGun_common_status == STA_CHARG_FINISHED)
		{
			g_eCharge_status[eGun_id] = STA_CHARG_FINISHED;
			my_printf(USER_INFO, "%s:%d STA_CHARGING:%s receive finish charge\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		}
	}
}

/**
 * @brief 枪故障状态处理函数
 * @param eGun_id：枪id
 * @return
 */
void FaultStatusProcess(const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	static BOOL s_bRestore_flag[2] = {FALSE, FALSE};

	if (g_sGun_data[eGun_id].eGun_common_status != STA_FAULT)
	{
		my_printf(USER_INFO, "%s:%d %s set STA_IDLE\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		GunStatusChange(eGun_id, STA_FAULT, FALSE, FALSE);
	}

	//检测到拔枪
	if ((U8)UNPLUGGED == GetGunStatus(eGun_id))
	{
		//防止可恢复故障和不可恢复故障同时存在，频繁调用
		if (FALSE == s_bRestore_flag[eGun_id])
		{
			//拔枪置位可恢复故障
			GunReturnResetAlarm(eGun_id);
			s_bRestore_flag[eGun_id] = TRUE;
		}
		//拔枪可恢复故障
		if (TRUE == RecoverableFaultCodeCheck(eGun_id))
		{
			my_printf(USER_INFO, "%s:%d %s disconnection detected STA_FAULT->STA_IDLE\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
			g_eCharge_status[eGun_id] = STA_IDLE;
			if (STA_IDLE != g_sGun_data[eGun_id].eGun_common_status)
			{
				GunStatusChange(eGun_id, STA_IDLE,	FALSE, FALSE);
			}
			s_bRestore_flag[eGun_id] = FALSE;
		}
	}
}

/**
 * @brief 更新枪状态
 * @param eGun_id：枪id
 * @param ucGun_status：需要变更的状态
 * @param bUnavailable_restore_flag:不可用状态控制
 * @param bBook_restore_flag:预约状态控制
 * @return
 */
void GunStatusChange(const Gun_Id_e eGun_id, const Charge_Sta_e eGun_status, const BOOL bUnavailable_restore_flag, const BOOL bBook_restore_flag)
{
	if (NULL == g_sGun_status_change_mutex)
	{
		return;
	}

	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return;
	}

	static const char* s_pcGun_status_name[STA_MAX]=
	{
		"STA_UNKNOW",
		"STA_IDLE",			/*空闲*/
		"STA_PLUGING",		/*插枪*/
		"STA_START_CHARGE",	/*充电启动中*/
		"STA_CHARGING",		/*充电中*/
		"STA_CHARG_FINISHED",	/*充电完成*/
		"STA_BOOK",			/*预约中*/
		"STA_UNAVAILABLE",	/*枪不可用*/
		"STA_FAULT"			/*故障*/
	};
	U8 ucCurrent_index, ucTarget_index = 0;

	if (pdTRUE == xSemaphoreTake(g_sGun_status_change_mutex, portMAX_DELAY))
	{
		//状态相同
		if (eGun_status == g_sGun_data[eGun_id].eGun_common_status)
		{
			(void)xSemaphoreGive(g_sGun_status_change_mutex);
			return;
		}

		//故障状态码是0xFF
		if (STA_FAULT == g_sGun_data[eGun_id].eGun_common_status)
		{
			ucCurrent_index = STA_MAX-1;
		}
		else
		{
			if ((U8)g_sGun_data[eGun_id].eGun_common_status < STA_MAX)
			{
				ucCurrent_index = (U8)g_sGun_data[eGun_id].eGun_common_status;
			}
			else
			{
				ucCurrent_index = STA_MAX-1;
			}
		}

		switch (eGun_status)
		{
		case STA_FAULT:
			if (g_sGun_data[eGun_id].eGun_common_status != STA_FAULT)
			{
				g_sGun_data[eGun_id].eGun_common_status = STA_FAULT;
				my_printf(USER_INFO, "%s:%d %s status %s -> STA_FAULT\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B", s_pcGun_status_name[ucCurrent_index]);
			}
			break;
		case STA_UNAVAILABLE:
			if (g_sGun_data[eGun_id].eGun_common_status != STA_FAULT)
			{
				g_sGun_data[eGun_id].eGun_common_status = STA_UNAVAILABLE;
				my_printf(USER_INFO, "%s:%d %s status %s -> STA_UNAVAILABLE\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B", s_pcGun_status_name[ucCurrent_index]);
			}
			else
			{
				//判定桩本身是否有故障
				if (PileFaultCodeCheck(eGun_id))
				{
					my_printf(USER_INFO, "%s:%d %s current status = STA_FAULT, not update gun status STA_UNAVAILABLE\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
				}
				else//PCU端状态故障->不可用
				{
					g_sGun_data[eGun_id].eGun_common_status = STA_UNAVAILABLE;
					my_printf(USER_INFO, "%s:%d %s status %s -> STA_UNAVAILABLE\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B", s_pcGun_status_name[ucCurrent_index]);
				}
			}
			break;
		case STA_BOOK:
			//空闲状态可预约
			if (g_sGun_data[eGun_id].eGun_common_status == STA_IDLE)
			{
				g_sGun_data[eGun_id].eGun_common_status = STA_BOOK;
				my_printf(USER_INFO, "%s:%d %s status %s -> STA_BOOK\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B", s_pcGun_status_name[ucCurrent_index]);
			}
			break;
		default:
			//PCU控制状态恢复
			if (TRUE == bUnavailable_restore_flag)
			{
				if (g_sGun_data[eGun_id].eGun_common_status == STA_UNAVAILABLE)
				{
					g_sGun_data[eGun_id].eGun_common_status = STA_IDLE;
					g_eCharge_status[eGun_id] = STA_IDLE;
					my_printf(USER_INFO, "%s:%d %s status STA_UNAVAILABLE -> STA_IDLE\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
				}
			}
			else if (TRUE == bBook_restore_flag)
			{
				if (g_sGun_data[eGun_id].eGun_common_status == STA_BOOK)
				{
					g_sGun_data[eGun_id].eGun_common_status = STA_IDLE;
					g_eCharge_status[eGun_id] = STA_IDLE;
					my_printf(USER_INFO, "%s:%d %s status STA_BOOK -> STA_IDLE\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
				}
			}
			else
			{
				if ((g_sGun_data[eGun_id].eGun_common_status != STA_UNAVAILABLE)
					&& (g_sGun_data[eGun_id].eGun_common_status != STA_BOOK))
				{
					//故障状态码是0xFF
					if (STA_FAULT == eGun_status)
					{
						ucTarget_index = STA_MAX-1;
					}
					else
					{
						if ((U8)eGun_status < STA_MAX)
						{
							ucTarget_index = (U8)eGun_status;
						}
						else
						{
							ucTarget_index = STA_MAX-1;
						}
					}
					if (STA_IDLE == eGun_status)
					{
						//充电状态不直接置位空闲
						if (FALSE == CheckChargingStatus(eGun_id))
						{
							g_sGun_data[eGun_id].eGun_common_status = eGun_status;
							my_printf(USER_INFO, "%s:%d %s change status %s -> %s\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B", s_pcGun_status_name[ucCurrent_index],
									s_pcGun_status_name[ucTarget_index]);
						}
					}
					else
					{
						g_sGun_data[eGun_id].eGun_common_status = eGun_status;
						my_printf(USER_INFO, "%s:%d %s change status %s -> %s\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B", s_pcGun_status_name[ucCurrent_index],
								s_pcGun_status_name[ucTarget_index]);
					}
				}
			}
			break;
		}

		//状态变更，触发心跳
		TcpSendControl(&g_sMsg_control.sHeart_control.bHeart_timeup_flag);
		(void)xSemaphoreGive(g_sGun_status_change_mutex);
	}
}

/**
 * @brief 枪故障状态处理函数
 * @param eGun_id：枪id
 * @return
 */
void AlarmStatusProcess(const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	static Alarm_Status_e s_ucError_status[2] = {0};

	//故障检测
	s_ucError_status[eGun_id] = FaultCodeCheck(eGun_id);

	switch (s_ucError_status[eGun_id])
	{
	case NORMAL_STATUS:
		//恢复静态故障
		if (STA_FAULT == g_eCharge_status[eGun_id])
		{
			//防止充电中检测到故障进入停止流程后故障恢复，此时继电器还未断开，仍在充电中，暂不复位故障
			if (FALSE == CheckChargingStatus(eGun_id))
			{
				//只恢复到空闲，防止恢复到插枪但上一次充电数据未清除导致启动失败
				g_eCharge_status[eGun_id] = STA_IDLE;
				if (STA_IDLE != g_sGun_data[eGun_id].eGun_common_status)
				{
					my_printf(USER_INFO, "%s:%d %s alarm restore STA_FAULT->STA_IDLE %d\n", __FILE__, __LINE__,
							(eGun_id==GUN_A)?"GUN_A":"GUN_B", g_sGun_data[eGun_id].eGun_common_status);
					GunStatusChange(eGun_id, STA_IDLE,	FALSE, FALSE);
				}
			}
		}
		break;
	case WARNING_STATUS:
		//拔枪
		if ((U8)UNPLUGGED == GetGunStatus(eGun_id))
		{
			//存在拔枪恢复告警
			if (TRUE == RecoverableWarnCodeCheck(eGun_id))
			{
				//拔枪置位可恢复故障
				GunReturnResetAlarm(eGun_id);
			}
		}
		//故障恢复，切换状态
		if (STA_FAULT == g_eCharge_status[eGun_id])
		{
			//只恢复到空闲，防止恢复到插枪但上一次充电数据未清除导致启动失败
			if (FALSE == CheckChargingStatus(eGun_id))
			{
				g_eCharge_status[eGun_id] = STA_IDLE;
				if (STA_IDLE != g_sGun_data[eGun_id].eGun_common_status)
				{
					my_printf(USER_INFO, "%s:%d %s restore STA_FAULT->STA_IDLE\n", __FILE__, __LINE__,
							(eGun_id==GUN_A)?"GUN_A":"GUN_B");
					GunStatusChange(eGun_id, STA_IDLE,	FALSE, FALSE);
				}
			}
		}
		break;
	case ALARM_STATUS:
		g_eCharge_status[eGun_id] = STA_FAULT;
		if (STA_FAULT != g_sGun_data[eGun_id].eGun_common_status)
		{
			//配合PCU端账单生成，充电中不立即置位故障
			if (FALSE == CheckChargingStatus(eGun_id))
			{
				my_printf(USER_INFO, "%s:%d %s set STA_FAULT\n", __FILE__, __LINE__, ((eGun_id==GUN_A))?"GUN_A":"GUN_B");
				GunStatusChange(eGun_id, STA_FAULT, FALSE, FALSE);
			}
		}
		break;
	default:
		my_printf(USER_ERROR, "%s:%d switch error\n", __FILE__, __LINE__);
		break;
	}

	return;
}

static void ChargeProcessExec(g_psCharge_Process_t *psProcess, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
        vTaskDelay(2000);
		return ;
	}

    if (NULL == psProcess)
    {
        my_printf(USER_ERROR, "%s:%d GUN g_psCharge_process is NULL\n", __FILE__, __LINE__);
        vTaskDelay(2000);
    }
    else
	{
		if (GUN_A == eGun_id)
		{
			if (NULL == psProcess->GUNAChargeProcess)
			{
				my_printf(USER_ERROR, "%s:%d GUNA ChargeProcess function is NULL\n", __FILE__, __LINE__);
				 vTaskDelay(2000);
			}
			else
			{
				psProcess->GUNAChargeProcess();
			}
		}
		else
		{
			if (NULL == psProcess->GUNBChargeProcess)
			{
				my_printf(USER_ERROR, "%s:%d GUNB ChargeProcess function is NULL\n", __FILE__, __LINE__);
				 vTaskDelay(2000);
			}
			else
			{
				psProcess->GUNBChargeProcess();
			}
		}
	}
}

static void Charge_A_Process_Task(void *parameter)
{
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
    volatile UBaseType_t uxHighWaterMark;
#endif

    while (1)
    {
        if (g_iGun_module[0] == g_iGun_module[1])
        {
        	ChargeProcessExec(g_psCharge_process, GUN_A);
        }
        else
        {
        	ChargeProcessExec(g_psCharge_process_A, GUN_A);
        }

        vTaskDelay(100);

#ifdef FREE_STACK_SPACE_CHECK_ENABLE
        uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
#endif
    }
}

static void Charge_B_Process_Task(void *parameter)
{
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
    volatile UBaseType_t uxHighWaterMark;
#endif

    while (1)
    {
        if (g_iGun_module[0] == g_iGun_module[1])
        {
        	ChargeProcessExec(g_psCharge_process, GUN_B);
        }
        else
        {
        	ChargeProcessExec(g_psCharge_process_B, GUN_B);
        }

        vTaskDelay(100);

#ifdef FREE_STACK_SPACE_CHECK_ENABLE
        uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
#endif
    }
}

void Charge_Process_Init_Task(void * pvParameters)
{
    vTaskDelay(200);
    //初始化
	ASChargeProcessInit();
	GBChargeProcessInit();
	//选择
	ChargeProcessSelect();

    g_sGun_status_change_mutex = xSemaphoreCreateMutex();
    if (NULL == g_sGun_status_change_mutex)
    {
    	my_printf(USER_ERROR, "%s:%d Charge_Process_Init_Task error\n", __FILE__, __LINE__);
    	vTaskSuspend(NULL);
    }

	//初始化枪状态为空闲
	g_sGun_data[0].eGun_common_status = STA_IDLE;
	g_sGun_data[1].eGun_common_status = STA_IDLE;

	taskENTER_CRITICAL();

	//add other iso_init task here
	(void)xTaskCreate(&Charge_A_Process_Task, "CHARGE_A_PROCESS", 700U/4U, NULL, GENERAL_TASK_PRIO, NULL);
	(void)xTaskCreate(&Charge_B_Process_Task, "CHARGE_B_PROCESS", 700U/4U, NULL, GENERAL_TASK_PRIO, NULL);

	vTaskDelete(NULL);

	taskEXIT_CRITICAL();
}
