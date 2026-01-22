/*
 * GB_27930_parse_A.c
 *
 *  Created on: 2025年5月5日
 *      Author: qjwu
 */

#include <board.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "hal_can.h"
#include "fsl_flexcan.h"

#include "GB_27930_comm.h"
#include "tcp_client_IF.h"
#include "emergency_fault_IF.h"
#include "SignalManage.h"
#include "uart_comm.h"
#include "charge_general.h"

GB_Version_t g_sGB_Version[2] = {0};
__BSS(SRAM_OC) GB_Charging_Charger_Data_t g_sGB_Charger_data[2] = {0};
__BSS(SRAM_OC) GB_Charging_Bms_Data_t g_sGB_Bms_data[2] = {0};
TimerHandle_t g_sRec_BRM_timer[2] = {NULL, NULL};

__BSS(SRAM_OC) static U8 s_ucMultipack_cnt[REC_MUL_MAX] = {0};
__BSS(SRAM_OC) static U8 s_ucMultipack_buf[REC_MUL_MAX][CAN_MAX_MSG_LEN_27930] = {0};
static U8 s_ucRec_msg_flag = REC_MUL_MAX;

/**
 * @brief 多包报文超时回调函数
 * @param
 * @return
 */
static void GB_RecvMultipackTimeoutCb_A(TimerHandle_t xTimer)
{
	if (NULL == xTimer)
	{
		return;
	}
	if (g_sRec_BRM_timer[GUN_A] == xTimer)
	{
		(void)xTimerStop(g_sRec_BRM_timer[GUN_A], 0);
		my_printf(USER_ERROR, "GUN_A: BRM recv TimeOut,send pack end\n");
	}
	else if (g_sRec_BCP_timer[GUN_A] == xTimer)
	{
		(void)xTimerStop(g_sRec_BCP_timer[GUN_A], 0);
		my_printf(USER_ERROR, "GUN_A: BCP recv TimeOut,send pack end\n");
	}
	else if (g_sRec_BCS_timer[GUN_A] == xTimer)
	{
		SendGunRecBCSEndPack(GUN_A);
		g_ucSend_BCS_end_cnt[GUN_A]++;
		if (g_ucSend_BCS_end_cnt[GUN_A] > 3U)
		{
			g_ucSend_BCS_end_cnt[GUN_A] = 0;
			(void)xTimerStop(g_sRec_BCS_timer[GUN_A], 0);
		}
		my_printf(USER_ERROR, "GUN_A: BCS recv TimeOut,send pack end\n");
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d GUN_A: Other protocol recv TimeOut\n", __FILE__, __LINE__);
	}

	return;
}

/**
 * @brief 多包报文定时器控制函数
 * @param ucPack_num：包类型
 * @param bTimer_flag：定时器
 * @return
 */
static void GB_RecvMultipackTimerTreat_A(U8 ucPack_num, BOOL bTimer_flag)
{
	switch (ucPack_num)
	{
	case REC_MUL_BRM:
        if (NULL == g_sRec_BRM_timer[GUN_A])
        {
        	g_sRec_BRM_timer[GUN_A] = xTimerCreate("BRMtimer_A",
            		REC_MUL_TIMEOUT_TICK,
					pdTRUE,
					"BRM_A",
					&GB_RecvMultipackTimeoutCb_A);

            if (TRUE == bTimer_flag)
            {
                if (NULL != g_sRec_BRM_timer[GUN_A])
                {
                	(void)xTimerStart(g_sRec_BRM_timer[GUN_A], 0);
                }
            }
            else
            {
                if (NULL != g_sRec_BRM_timer[GUN_A])
                {
                	(void)xTimerStop(g_sRec_BRM_timer[GUN_A], 0);
                }
            }
        }
        else
        {
            if (TRUE == bTimer_flag)
            {
                (void)xTimerStart(g_sRec_BRM_timer[GUN_A], 0);
            }
            else
            {
                (void)xTimerStop(g_sRec_BRM_timer[GUN_A], 0);
            }
        }
		break;
	case REC_MUL_BCP:
        if (NULL == g_sRec_BCP_timer[GUN_A])
        {
            g_sRec_BCP_timer[GUN_A] = xTimerCreate("BCPtimer_A",
            		REC_MUL_TIMEOUT_TICK,
					pdTRUE,
					"BCP_A",
					&GB_RecvMultipackTimeoutCb_A);

            if (TRUE == bTimer_flag)
            {
                if (NULL != g_sRec_BCP_timer[GUN_A])
                {
                	(void)xTimerStart(g_sRec_BCP_timer[GUN_A], 0);
                }
            }
            else
            {
                if (NULL != g_sRec_BCP_timer[GUN_A])
                {
                	(void)xTimerStop(g_sRec_BCP_timer[GUN_A], 0);
                }
            }
        }
        else
        {
            if (TRUE == bTimer_flag)
            {
                (void)xTimerStart(g_sRec_BCP_timer[GUN_A], 0);
            }
            else
            {
                (void)xTimerStop(g_sRec_BCP_timer[GUN_A], 0);
            }
        }
		break;
	case REC_MUL_BCS:
        if (NULL == g_sRec_BCS_timer[GUN_A])
        {
            g_sRec_BCS_timer[GUN_A] = xTimerCreate("BCStimer_A",
            		REC_MUL_TIMEOUT_TICK,
					pdTRUE,
					"BCS_A",
					&GB_RecvMultipackTimeoutCb_A);

            if (TRUE == bTimer_flag)
            {
                if (NULL != g_sRec_BCS_timer[GUN_A])
                {
                	(void)xTimerStart(g_sRec_BCS_timer[GUN_A], 0);
                }
                g_ucSend_BCS_end_cnt[GUN_A] = 0;
            }
            else
            {
                if (NULL != g_sRec_BCS_timer[GUN_A])
                {
                	(void)xTimerStop(g_sRec_BCS_timer[GUN_A], 0);
                }
            }
        }
        else
        {
            if (TRUE == bTimer_flag)
            {
                (void)xTimerStart(g_sRec_BCS_timer[GUN_A], 0);
                g_ucSend_BCS_end_cnt[GUN_A] = 0;
            }
            else
            {
                (void)xTimerStop(g_sRec_BCS_timer[GUN_A], 0);
            }
        }
		break;
	default:
		//my_printf(USER_ERROR, "%s:%d GUN_A Other ucPack_num error = %d\n", __FILE__, __LINE__, ucPack_num);
		break;
	}
}

/**
 * @brief BRM解析
 * @param ucBuf：接受到的数据
 * @param ucLen：数据长度
 * @param eGun_id：枪id
 * @return
 */
void GB_BrmPrase(const U8 *ucBuf, const U8 ucLen, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

    (void)memcpy(&g_sGB_Bms_data[eGun_id].sBRM, ucBuf, ucLen);

    //2015版
    if (g_sGB_Bms_data[eGun_id].sBRM.ucBms_version[0] == 0x01U)
    {
    	g_sGB_Version[eGun_id] = GB_2015;
    	(void)memcpy(g_sGun_data[eGun_id].sTemp.ucBms_version, "1.1", 3);
    }
    //2023版
    else if (g_sGB_Bms_data[eGun_id].sBRM.ucBms_version[0] == 0x02U)
    {
    	g_sGB_Version[eGun_id] = GB_2023;
    	(void)memcpy(g_sGun_data[eGun_id].sTemp.ucBms_version, "1.2", 3);
    }
    else
    {
    	//未知的充电协议
    	my_printf(USER_ERROR, "%s:%d %s unknown 27930 charge protocol\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
    }

    g_sGun_data[eGun_id].sTemp.ucBattery_type = g_sGB_Bms_data[eGun_id].sBRM.ucBattery_type;
    //车端传递VIN才解析
    if (ucLen > 25U)
    {
    	(void)memcpy(g_sGun_data[eGun_id].sTemp.ucVin, g_sGB_Bms_data[eGun_id].sBRM.VIN, sizeof(g_sGB_Bms_data[eGun_id].sBRM.VIN));
    }
    else
    {
    	(void)memset(g_sGun_data[eGun_id].sTemp.ucVin, 0, sizeof(g_sGB_Bms_data[eGun_id].sBRM.VIN));
    }

    g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BRM].unRecv_cnt++;
}

/**
 * @brief BCP解析
 * @param ucBuf：接受到的数据
 * @param ucLen：数据长度
 * @param eGun_id：枪id
 * @return
 */
void GB_BcpPrase(const U8 *ucBuf, const U8 ucLen, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

    (void)memcpy(&g_sGB_Bms_data[eGun_id].sBCP, ucBuf, sizeof(GB_BCP_Data_t));

    g_sGun_data[eGun_id].sTemp.unSingle_bat_max_vol = g_sGB_Bms_data[eGun_id].sBCP.unSingle_battery_max_allow_vol;
    g_sGun_data[eGun_id].sTemp.unEV_max_allow_cur = 4000U-g_sGB_Bms_data[eGun_id].sBCP.unMax_allow_cur;
    g_sGun_data[eGun_id].sTemp.unBat_total_energy = g_sGB_Bms_data[eGun_id].sBCP.unBattery_total_energy;
    g_sGun_data[eGun_id].sTemp.unEV_max_limit_vol = g_sGB_Bms_data[eGun_id].sBCP.unMax_allow_total_vol;
    g_sGun_data[eGun_id].sTemp.ucBms_max_allow_temp = g_sGB_Bms_data[eGun_id].sBCP.ucBattery_max_allow_temp;
    g_sGun_data[eGun_id].sTemp.unStart_soc = g_sGB_Bms_data[eGun_id].sBCP.unBattery_SOC;
    g_sGun_data[eGun_id].sTemp.unVehicle_check_vol = g_sGB_Bms_data[eGun_id].sBCP.unVehicle_check_vol;

    g_sGun_data[eGun_id].sTemp.unEV_target_vol = g_sGB_Bms_data[eGun_id].sBCP.unVehicle_check_vol;
    my_printf(USER_INFO, "%s: receive full BCP message:limit VOL = %d(0.1)V, target VOL = %d(0.1)V, cur = %d(0.1)A SOC = %d(0.1) temperature = %d(-50)\n",
    		eGun_id == GUN_A?"GUN_A":"GUN_B", g_sGun_data[eGun_id].sTemp.unEV_max_imd_vol, g_sGun_data[eGun_id].sTemp.unVehicle_check_vol,
			g_sGun_data[eGun_id].sTemp.unEV_max_allow_cur, g_sGun_data[eGun_id].sTemp.unStart_soc, g_sGB_Bms_data[eGun_id].sBCP.ucBattery_max_allow_temp);

    g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BCP].unRecv_cnt++;
}

/**
 * @brief BCS解析
 * @param ucBuf：接受到的数据
 * @param ucLen：数据长度
 * @param eGun_id：枪id
 * @return
 */
void GB_BcsPrase(const U8 *ucBuf, const U8 ucLen, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	static U8 s_ucBsc_cnt = 0;
    (void)memcpy(&g_sGB_Bms_data[eGun_id].sBCS, ucBuf, ucLen);

    g_sGun_data[eGun_id].sTemp.unVehicle_check_vol = g_sGB_Bms_data[eGun_id].sBCS.unEV_detect_vol;
    g_sGun_data[eGun_id].sTemp.unVehicle_check_cur = 4000U-g_sGB_Bms_data[eGun_id].sBCS.unEV_detect_cur;
    //g_sGB_Bms_data[eGun_id].sBCS.unMax_single_vol_and_group;
    g_sGun_data[eGun_id].sTemp.ucCurrent_soc = g_sGB_Bms_data[eGun_id].sBCS.ucCurrent_SOC;
    g_sGun_data[eGun_id].sTemp.unRemain_time = g_sGB_Bms_data[eGun_id].sBCS.unRemained_time;

    g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BCS].unRecv_cnt++;

    s_ucBsc_cnt++;
    if (s_ucBsc_cnt > 40U)
    {
    	my_printf(USER_INFO, "%s: receive BCS message SOC = %d VOL = %d(0.1V) cur = %d(0.1A)\n", (eGun_id == GUN_A?"GUN_A":"GUN_B"),
    			g_sGB_Bms_data[eGun_id].sBCS.ucCurrent_SOC, g_sGun_data[eGun_id].sTemp.unVehicle_check_vol
				,g_sGun_data[eGun_id].sTemp.unVehicle_check_cur);
    	s_ucBsc_cnt = 0;
    }
}

/**
 * @brief 解析BRO报文函数
 * @param sBSM_data 数据
 * @param eGun_id：枪id
 * @return
 */
void GB_BroMessageProcess(const U8 ucBro_status, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	static U32 uiNow_tick = 0;

	if (0xaaU == ucBro_status)
	{
		g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BRO_AA].unRecv_cnt++;

		uiNow_tick = xTaskGetTickCount();
		g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BRO_00].uiStart_tick = uiNow_tick;
		g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BRO_AA].uiStart_tick = uiNow_tick;
		if (1U == (g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BRO_AA].unRecv_cnt))
		{
			my_printf(USER_INFO, "%s: receive BRO AA\n", (eGun_id == GUN_A?"GUN_A":"GUN_B"));
		}
	}
	else
	{
		g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BRO_00].unRecv_cnt++;

		if (1U == (g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BRO_00].unRecv_cnt))
		{
			uiNow_tick = xTaskGetTickCount();
			g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BRO_00].uiStart_tick = uiNow_tick;
			my_printf(USER_INFO, "%s: receive BRO 00\n", (eGun_id == GUN_A?"GUN_A":"GUN_B"));
		}
	}
}

/**
 * @brief 解析BCL报文函数
 * @param sBSM_data 数据
 * @param eGun_id：枪id
 * @return
 */
void GB_BclMessageProcess(const GB_BCL_Data_t sBCL_data, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	static U8 s_ucBcl_print_cnt[2] = {0};
	static BOOL s_Send_flag = FALSE;

	U16 unTemp_cur = g_sGun_data[eGun_id].sTemp.unEV_target_cur;
	g_sGun_data[eGun_id].sTemp.unEV_target_cur	= 4000U-sBCL_data.unTarget_cur;
	//触发式上传需求电流，增加响应速度
	if (TRUE == my_unsigned_abs(g_sGun_data[eGun_id].sTemp.unEV_target_cur, unTemp_cur, TARGET_CUR_DIFF))
	{
		my_printf(USER_INFO, "%s receive BCL target CUR change: %d-->%d(0.1A)\n", (eGun_id == GUN_A?"GUN_A":"GUN_B"),
				unTemp_cur, g_sGun_data[eGun_id].sTemp.unEV_target_cur);

		s_Send_flag = TRUE;
	}

	U16 unTemp_vol = g_sGun_data[eGun_id].sTemp.unEV_target_vol;
	if (sBCL_data.unTarget_vol < (g_sStorage_data.sPublic_data[eGun_id].unGun_max_vol * 10U))
	{
		g_sGun_data[eGun_id].sTemp.unEV_target_vol = sBCL_data.unTarget_vol;
	}
	else
	{
		g_sGun_data[eGun_id].sTemp.unEV_target_vol = (g_sStorage_data.sPublic_data[eGun_id].unGun_max_vol * 10U);
	}

	// 基于最终生效的值判断是否触发上传
	if (TRUE == my_unsigned_abs(g_sGun_data[eGun_id].sTemp.unEV_target_vol, unTemp_vol, TARGET_VOL_DIFF))
	{
	    my_printf(USER_INFO, "%s receive BCL target VOL change: %d-->%d(0.1V) limit VOL = %d(0.1V)\n",
	              (eGun_id == GUN_A?"GUN_A":"GUN_B"),
				  unTemp_vol,
	              g_sGun_data[eGun_id].sTemp.unEV_target_vol,
	              g_sGun_data[eGun_id].sTemp.unEV_max_limit_vol);
	    s_Send_flag = TRUE;
	}

	//检测到设定压差或流差，立即上传需求
	if (s_Send_flag == TRUE)
	{
		TcpSendControl(&g_sMsg_control.sCharging_control.bCharging_timeup_flag);
		s_Send_flag = FALSE;
	}

	if (s_ucBcl_print_cnt[eGun_id] > 200U)
	{
		my_printf(USER_INFO, "%s receive BCL: charge_type = %s VOL = %d(0.1V) cur = %d(0.1A)\n", (eGun_id == GUN_A?"GUN_A":"GUN_B"),
				(sBCL_data.ucCharge_mode==1U)?"CV":"CC", g_sGun_data[eGun_id].sTemp.unEV_target_vol, g_sGun_data[eGun_id].sTemp.unEV_target_cur);
		s_ucBcl_print_cnt[eGun_id] = 1;
	}
	g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BCL].unRecv_cnt++;
	s_ucBcl_print_cnt[eGun_id]++;
}

/**
 * @brief 解析BSM报文函数
 * @param sBSM_data 数据
 * @param eGun_id：枪id
 * @return
 */
void GB_BsmMessageProcess(const GB_BSM_Data_t sBSM_data, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	U8 err_cnt = 0;

	if ((U8)FALSE != sBSM_data.ucSingle_battery_vol_state)
	{
		if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.unBms_single_vol_Abnormal != TRUE)
		{
			GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_SINGLE_VOL_ABNORMAL);
	    	my_printf(USER_ERROR, "%s: receive BSM single vol Abnormal\n", (eGun_id == GUN_A)?"GUN_A":"GUN_B");
		}
		err_cnt++;
	}
	if ((U8)FALSE != sBSM_data.ucBattery_soc_state)
	{
		if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.unBms_SOC_Abnormal != TRUE)
		{
			GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_SOC_ABNORMAL);
			my_printf(USER_ERROR, "%s: receive BSM SOC Abnormal\n", (eGun_id == GUN_A)?"GUN_A":"GUN_B");
		}
		err_cnt++;
	}
	//不可信状态不处理
	if ((U8)TRUE == sBSM_data.ucBattery_over_cur_state)
	{
		if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.unBms_over_cur != TRUE)
		{
			GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_OVER_CUR);
			my_printf(USER_ERROR, "%s: receive BSM current Abnormal\n", (eGun_id == GUN_A)?"GUN_A":"GUN_B");
		}
		err_cnt++;
	}
	if ((U8)TRUE == sBSM_data.ucBattery_over_temp_state)
	{
		if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.unBms_bat_over_temp != TRUE)
		{
			GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_BAT_OVER_TEMP);
			my_printf(USER_ERROR, "%s: receive BSM temperature Abnormal\n", (eGun_id == GUN_A)?"GUN_A":"GUN_B");
		}
		err_cnt++;
	}
	if ((U8)TRUE == sBSM_data.ucBattery_insula_state)
	{
		if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.unBms_insulation_error != TRUE)
		{
			GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_INSULATION_ERROR);
			my_printf(USER_ERROR, "%s: receive BSM insulation Abnormal\n", (eGun_id == GUN_A)?"GUN_A":"GUN_B");
		}
		err_cnt++;
	}
	if ((U8)TRUE == sBSM_data.ucBattery_connect_state)
	{
		if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.unBms_charge_conn_error != TRUE)
		{
			GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_CHARGE_CONN_ERROR);
			my_printf(USER_ERROR, "%s: receive BSM connector Abnormal\n", (eGun_id == GUN_A)?"GUN_A":"GUN_B");
		}
		err_cnt++;
	}

	g_sGun_data[eGun_id].sTemp.ucSingle_bat_max_temp = sBSM_data.ucMax_battery_temp;
	g_sGun_data[eGun_id].sTemp.ucSingle_bat_min_temp = sBSM_data.ucMin_battery_temp;

	if (0U != err_cnt)
	{
		g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BST].unRecv_cnt = 0;
		g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BST].uiStart_tick = xTaskGetTickCount();
		g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BSD].uiStart_tick = xTaskGetTickCount();
		g_sGun_data[eGun_id].sTemp.eStop_Charge_type = FAULT_STOP_MODE;
		g_sBms_ctrl[eGun_id].sStage.eNow_status = STOPCHARGE;
	}
	else
	{
		if ((U8)FALSE == sBSM_data.ucCharge_enable_state)
		{
			if (TRUE == g_sGun_data[eGun_id].sTemp.bAllow_charge_flag)
			{
				g_sGB_Charger_data[eGun_id].stop_start_tick = xTaskGetTickCount();
				g_sGun_data[eGun_id].sTemp.bAllow_charge_flag = FALSE;
				my_printf(USER_INFO, "%s: Charging is not allowed\n", (eGun_id==GUN_A)?"GUN_A":"GUN_B");
			}
			else
			{
				//充电暂停时间不超过10分钟
				if ((xTaskGetTickCount() - g_sGB_Charger_data[eGun_id].stop_start_tick) > (10U*60U*996U))
				{
					g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BST].unRecv_cnt = 0;
					g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BST].uiStart_tick = xTaskGetTickCount();
					g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BSD].uiStart_tick = xTaskGetTickCount();
					g_sBms_ctrl[eGun_id].sStage.eNow_status = STOPCHARGE;
					my_printf(USER_ERROR, "%s: pause Charging is timeout\n", (eGun_id==GUN_A)?"GUN_A":"GUN_B");
				}
			}
		}
		else
		{
			if (FALSE == g_sGun_data[eGun_id].sTemp.bAllow_charge_flag)
			{
				g_sGun_data[eGun_id].sTemp.bAllow_charge_flag = TRUE;
				my_printf(USER_INFO, "%s: Charging is allowed\n", (eGun_id==GUN_A)?"GUN_A":"GUN_B");
			}
		}
	}
}

/**
 * @brief 解析BST报文函数
 * @param sBEM_data 数据
 * @param eGun_id：枪id
 * @return
 */
void GB_BstMessageProcess(const GB_BST_Data_t sBST_data, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	if ((U8)INVALID_STATUS == sBST_data.sStop_reason.sItem.ucAchieve_target_soc)
	{
		g_sGun_data[eGun_id].sTemp.eStop_Charge_type = REACH_TOTAL_SOC_TARGET;
        //填充CST
        g_sGB_Charger_data[eGun_id].sCST.sStop_reason.sItem.ucReach_set_target = (U8)TRUE;
	}
	else if ((U8)INVALID_STATUS == sBST_data.sStop_reason.sItem.ucAchieve_target_vol)
	{
		g_sGun_data[eGun_id].sTemp.eStop_Charge_type = REACH_TOTAL_VOL_TARGET;
        //填充CST
        g_sGB_Charger_data[eGun_id].sCST.sStop_reason.sItem.ucReach_set_target = (U8)TRUE;
	}
	else if ((U8)INVALID_STATUS == sBST_data.sStop_reason.sItem.ucTarget_single_vol)
	{
		g_sGun_data[eGun_id].sTemp.eStop_Charge_type = REACH_SINGLE_VOL_TARGET;
        //填充CST
        g_sGB_Charger_data[eGun_id].sCST.sStop_reason.sItem.ucReach_set_target = (U8)TRUE;
	}
	else if ((U8)INVALID_STATUS == sBST_data.sStop_reason.sItem.ucReceive_bst)
	{
		//桩端主动停止
	}
	else if ((U8)VALID_STATUS == sBST_data.sStop_reason.sItem.ucReceive_bst)
	{
		//车端回复正常
		g_sGun_data[eGun_id].sTemp.eStop_Charge_type = REACH_TOTAL_SOC_TARGET;
        //填充CST
        g_sGB_Charger_data[eGun_id].sCST.sStop_reason.sItem.ucReach_set_target = (U8)TRUE;
	}
	else
	{
        //填充CST
        g_sGB_Charger_data[eGun_id].sCST.sStop_reason.sItem.ucFault_stop = (U8)TRUE;
		if ((U8)INVALID_STATUS == sBST_data.sFault_reason.sItem.unInsulation_error)
		{
			if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.unBms_insulation_error != TRUE)
			{
				GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_INSULATION_ERROR);
			}
		}
		else if ((U8)INVALID_STATUS == sBST_data.sFault_reason.sItem.unConnector_over_temp)
		{
			if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.unBms_conn_over_temp != TRUE)
			{
				GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_CONN_OVER_TEMP);
			}
		}
		else if ((U8)INVALID_STATUS == sBST_data.sFault_reason.sItem.unBms_cell_over_temp)
		{
			if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.unBms_cell_over_temp != TRUE)
			{
				GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_CELL_OVER_TEMP);
			}
		}
		else if((U8)INVALID_STATUS == sBST_data.sFault_reason.sItem.unCharge_conn_error)
		{
			if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.unBms_charge_conn_error != TRUE)
			{
				GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_CHARGE_CONN_ERROR);
			}
		}
		else if ((U8)INVALID_STATUS == sBST_data.sFault_reason.sItem.unBat_over_temp)
		{
			if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.unBms_bat_over_temp != TRUE)
			{
				GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_BAT_OVER_TEMP);
			}
		}
		else if((U8)INVALID_STATUS == sBST_data.sFault_reason.sItem.unRelay_error)
		{
			if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.unBms_relay_error != TRUE)
			{
				GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_RELAY_ERROR);
			}
		}
		else if ((U8)INVALID_STATUS == sBST_data.sErr_reason.sItem.ucOver_cur)
		{
			if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.unBms_over_cur != TRUE)
			{
				GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_OVER_CUR);
			}
		}
		else if ((U8)INVALID_STATUS == sBST_data.sErr_reason.sItem.ucVol_err)
		{
			if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.unBms_vol_err != TRUE)
			{
				GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_VOL_ERR);
			}
		}
		else if ((U8)INVALID_STATUS == sBST_data.sErr_reason.sItem.ucParam_dismatch)
		{
			if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.unBms_other_fault != TRUE)
			{
				GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_OTHER_FAULT);
			}
		}
		else if ((U8)INVALID_STATUS == sBST_data.sFault_reason.sItem.unCheck_p2_error)
		{
			if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.unBms_check_p2_err != TRUE)
			{
				GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_CHECK_P2_ERROR);
			}
		}
		else if ((U8)INVALID_STATUS == sBST_data.sFault_reason.sItem.other_fault)
		{
			if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.unBms_other_fault != TRUE)
			{
				GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_OTHER_FAULT);
			}
		}
		else
		{
			//未检测到有效停止原因
			my_printf(USER_INFO, "%s:%d %s BST check invalid stop reason\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUNB");
		}
		g_sGun_data[eGun_id].sTemp.eStop_Charge_type = FAULT_STOP_MODE;
	}

	//防止CCU充电中重启后接受到SECC的错误信息，导致下一次充电错误, 只有充电中才切换停止状态
	if (TRUE == CheckChargingStatus(eGun_id))
	{
		g_sBms_ctrl[eGun_id].sStage.eNow_status = STOPCHARGE;
	}
}

/**
 * @brief 解析BEM报文超时函数
 * @param sBEM_data 数据
 * @param eGun_id：枪id
 * @return
 */
void GB_BemTimeoutProcess(const GB_BEM_Data_t sBEM_data, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	if ((U8)TRUE == sBEM_data.sTimeout.crm_00)
	{
		if (g_sGun_warn[eGun_id].sRecoverable_warn.sItem.CCU_CRM_00_timeout != TRUE)
		{
			GunAlarmSet(&g_sGun_warn[eGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_CCU_CRM00_TIMEOUT);
			my_printf(USER_INFO, "GUNB: BEM error: CRM 00 is timeout!!\n");
		}
	}
	if ((U8)TRUE == sBEM_data.sTimeout.crm_AA)
	{
		if (g_sGun_warn[eGun_id].sRecoverable_warn.sItem.CCU_CRM_AA_timeout != TRUE)
		{
			GunAlarmSet(&g_sGun_warn[eGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_CCU_CRMAA_TIMEOUT);
			my_printf(USER_INFO, "GUNB: BEM error: CRM AA is timeout!!\n");
		}
	}
	if ((U8)TRUE == sBEM_data.sTimeout.cml_cts)
	{
		if (g_sGun_warn[eGun_id].sRecoverable_warn.sItem.CCU_CML_CTS_AA_timeout != TRUE)
		{
			GunAlarmSet(&g_sGun_warn[eGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_CCU_CML_CTS_TIMEOUT);
			my_printf(USER_INFO, "GUNB: BEM error: CML/CTS is timeout!!\n");
		}
	}
	if ((U8)TRUE == sBEM_data.sTimeout.cro)
	{
		if (g_sGun_warn[eGun_id].sRecoverable_warn.sItem.CCU_CR0_timeout != TRUE)
		{
			GunAlarmSet(&g_sGun_warn[eGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_CCU_CRO_TIMEOUT);
			my_printf(USER_INFO, "GUNB: BEM error: CRO is timeout!!\n");
		}
	}
	if ((U8)TRUE == sBEM_data.sTimeout.ccs)
	{
		if (g_sGun_warn[eGun_id].sRecoverable_warn.sItem.CCU_CCS_timeout != TRUE)
		{
			GunAlarmSet(&g_sGun_warn[eGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_CCU_CCS_TIMEOUT);
			my_printf(USER_INFO, "GUNB: BEM error: CCS is timeout!!\n");
		}
	}
	if ((U8)TRUE == sBEM_data.sTimeout.cst)
	{
		if (g_sGun_warn[eGun_id].sRecoverable_warn.sItem.CCU_CST_timeout != TRUE)
		{
			GunAlarmSet(&g_sGun_warn[eGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_CCU_CST_TIMEOUT);
			my_printf(USER_INFO, "GUNB: BEM error: CST is timeout!!\n");
		}
	}
	if ((U8)TRUE == sBEM_data.sTimeout.csd)
	{
		if (g_sGun_warn[eGun_id].sRecoverable_warn.sItem.CCU_CSD_timeout != TRUE)
		{
			GunAlarmSet(&g_sGun_warn[eGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_CCU_CSD_TIMEOUT);
			my_printf(USER_INFO, "GUNB: BEM error: CSD is timeout!!\n");
		}
	}
	//防止CCU充电中重启后接受到SECC的错误信息，导致下一次充电错误
	if (TRUE == CheckChargingStatus(eGun_id))
	{
		if (g_sBms_ctrl[eGun_id].sStage.eNow_status != STOPCHARGE)
		{
			g_sBms_ctrl[eGun_id].sStage.eNow_status = TIMEOUT;
		}
	}
}

static void GB_HandleEBMultipack_A(U8 mul_type, const char *cName, const U8 *ucBuf)
{
	if ((mul_type < REC_MUL_BRM) && (mul_type >= REC_MUL_MAX))
	{
		return;
	}

    if ((s_ucMultipack_cnt[mul_type] + 1) == ucBuf[0])
    {
        s_ucMultipack_cnt[mul_type]++;

        (void)memcpy(&s_ucMultipack_buf[mul_type][7U * (s_ucMultipack_cnt[mul_type] - 1U)], &ucBuf[1], 7);

        GB_RecvMultipackTimerTreat_A(mul_type, TRUE);

        //判断长度是否达标，决定是否结束接收
        if ((s_ucMultipack_cnt[mul_type] * 7U) >= g_ucRec_multipack_len[GUN_A][mul_type])
        {
            //校验包数是否匹配
            if (s_ucMultipack_cnt[mul_type] == g_ucRec_packnum[GUN_A][mul_type])
            {
                //调用对应类型的解析函数
            	switch (mul_type)
            	{
            	case REC_MUL_BRM:
            		GB_BrmPrase(&s_ucMultipack_buf[mul_type][0], g_ucRec_multipack_len[GUN_A][mul_type], GUN_A);
            		break;
            	case REC_MUL_BCP:
            		GB_BcpPrase(&s_ucMultipack_buf[mul_type][0], g_ucRec_multipack_len[GUN_A][mul_type], GUN_A);
            		break;
            	case REC_MUL_BCS:
            		GB_BcsPrase(&s_ucMultipack_buf[mul_type][0], g_ucRec_multipack_len[GUN_A][mul_type], GUN_A);
            		break;
            	default:
            		//
            		break;
            	}
            }
            else
            {
                my_printf(USER_ERROR, "GUN_A %s pack num mismatch: expect %d, got %d",
                		cName, g_ucRec_packnum[GUN_A][mul_type], s_ucMultipack_cnt[mul_type]);
            }

        	switch (mul_type)
        	{
        	case REC_MUL_BRM:
        		SendGunRecBRMEndPack(GUN_A);
        		break;
        	case REC_MUL_BCP:
        		SendGunRecBCPEndPack(GUN_A);
        		break;
        	case REC_MUL_BCS:
        		SendGunRecBCSEndPack(GUN_A);
        		break;
           	case REC_MUL_BSP:
				SendGunRecBSPEndPack(GUN_A);
				break;
			case REC_MUL_BMV:
				SendGunRecBMVEndPack(GUN_A);
				break;
			case REC_MUL_BMT:
				SendGunRecBMTEndPack(GUN_A);
				break;
        	default:
        		//
        		break;
        	}

            //重置状态
            s_ucMultipack_cnt[mul_type] = 0;
            GB_RecvMultipackTimerTreat_A(mul_type, FALSE);

            //重置对应类型的标志位
            s_ucRec_msg_flag = REC_MUL_MAX;
        }
    }
    else
    {
        //包编号不连续日志
        my_printf(USER_ERROR, "GUN_A %s pack num discontinuous: expect %d, got %d",
        		cName, s_ucMultipack_cnt[mul_type] + 1, ucBuf[0]);
    }
}

void GBCAN1Parse(const U32 uiCan_id, const U8* pucBuf)
{
    U8 ucBuf[SEND_BMS_BYTE_NUM] = {0};

    (void)memcpy(ucBuf, pucBuf, sizeof(ucBuf));

    switch(uiCan_id)
    {
    case READ_BMS_BHM_PACK:
    	if (sizeof(GB_BHM_Data_t) <= SEND_BMS_BYTE_NUM)
    	{
			(void)memcpy(&g_sGB_Bms_data[GUN_A].sBHM, ucBuf, sizeof(GB_BHM_Data_t));

			g_sGun_data[GUN_A].sTemp.unEV_max_imd_vol = g_sGB_Bms_data[GUN_A].sBHM.unMax_allow_vol;

			g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BHM].unRecv_cnt++;
			//防止接受次数溢出
			if (g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BHM].unRecv_cnt > 65000U)
			{
				g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BHM].unRecv_cnt = 1;
			}
    	}

        break;
    case READ_BMS_EC_PACK:
    	if ((0x10U == ucBuf[0]) && (0U == ucBuf[5]) && (0U == ucBuf[7]))
    	{
    		switch (ucBuf[6])
    		{
    		case 0x02:
                g_ucRec_multipack_len[GUN_A][REC_MUL_BRM] = (U8)(ucBuf[1] + ((U16)ucBuf[2] << 8));
                if (g_ucRec_multipack_len[GUN_A][REC_MUL_BRM] <= CAN_MAX_MSG_LEN_27930)
                {
					g_ucRec_packnum[GUN_A][REC_MUL_BRM] = ucBuf[3];
					(void)memset(&s_ucMultipack_buf[REC_MUL_BRM][0], 0, CAN_MAX_MSG_LEN_27930);
					SendGunRecBRMRequestPack(GUN_A);
					GB_RecvMultipackTimerTreat_A(REC_MUL_BRM, TRUE);
					s_ucRec_msg_flag = REC_MUL_BRM;
					s_ucMultipack_cnt[REC_MUL_BRM] = 0;
					my_printf(USER_INFO, "GUN_A: receive BRM request message, len = %d pack_num = %d\n", g_ucRec_multipack_len[GUN_A][REC_MUL_BRM], g_ucRec_packnum[GUN_A][REC_MUL_BRM]);
                }
    			break;
    		case 0x06:
                g_ucRec_multipack_len[GUN_A][REC_MUL_BCP] = (U8)(ucBuf[1] + ((U16)ucBuf[2] << 8));
                if (g_ucRec_multipack_len[GUN_A][REC_MUL_BCP] <= CAN_MAX_MSG_LEN_27930)
                {
					g_ucRec_packnum[GUN_A][REC_MUL_BCP] = ucBuf[3];
					(void)memset(&s_ucMultipack_buf[REC_MUL_BCP][0], 0, CAN_MAX_MSG_LEN_27930);
					SendGunRecBCPRequestPack(GUN_A);
					GB_RecvMultipackTimerTreat_A(REC_MUL_BCP, TRUE);
					s_ucRec_msg_flag = REC_MUL_BCP;
					s_ucMultipack_cnt[REC_MUL_BCP] = 0;
					my_printf(USER_INFO, "GUN_A: receive BCP request message, len = %d pack_num = %d\n", g_ucRec_multipack_len[GUN_A][REC_MUL_BCP], g_ucRec_packnum[GUN_A][REC_MUL_BCP]);
                }
    			break;
    		case 0x11:
                g_ucRec_multipack_len[GUN_A][REC_MUL_BCS] = (U8)(ucBuf[1] + ((U16)ucBuf[2] << 8));
                if (g_ucRec_multipack_len[GUN_A][REC_MUL_BCS] <= CAN_MAX_MSG_LEN_27930)
                {
					g_ucRec_packnum[GUN_A][REC_MUL_BCS] = ucBuf[3];
					(void)memset(&s_ucMultipack_buf[REC_MUL_BCS][0], 0, CAN_MAX_MSG_LEN_27930);
					SendGunRecBCSRequestPack(GUN_A);
					GB_RecvMultipackTimerTreat_A(REC_MUL_BCS, TRUE);
					s_ucRec_msg_flag = REC_MUL_BCS;
					s_ucMultipack_cnt[REC_MUL_BCS] = 0;
                }
    			break;
    		case 0x15:
    			g_ucRec_multipack_len[GUN_A][REC_MUL_BSP] = (U8)(ucBuf[1] + ((U16)ucBuf[2] << 8));
    			g_ucRec_packnum[GUN_A][REC_MUL_BSP] = ucBuf[3];
    			SendGunRecBSPRequestPack(GUN_A);
    			s_ucRec_msg_flag = REC_MUL_BSP;
    			s_ucMultipack_cnt[REC_MUL_BSP] = 0;
    			break;
    		case 0x16:
    			g_ucRec_multipack_len[GUN_A][REC_MUL_BMV] = (U8)(ucBuf[1] + ((U16)ucBuf[2] << 8));
    			g_ucRec_packnum[GUN_A][REC_MUL_BMV] = ucBuf[3];
    			SendGunRecBMVRequestPack(GUN_A);
    			s_ucRec_msg_flag = REC_MUL_BMV;
    			s_ucMultipack_cnt[REC_MUL_BMV] = 0;
    			break;
    		case 0x17:
    			g_ucRec_multipack_len[GUN_A][REC_MUL_BMT] = (U8)(ucBuf[1] + ((U16)ucBuf[2] << 8));
    			g_ucRec_packnum[GUN_A][REC_MUL_BMT] = ucBuf[3];
    			SendGunRecBMTRequestPack(GUN_A);
    			s_ucRec_msg_flag = REC_MUL_BMT;
    			s_ucMultipack_cnt[REC_MUL_BMT] = 0;
    			break;
    		default:
                U32 uiPgn = 0;
                uiPgn = ((U32)ucBuf[5] << 16) | ((U32)ucBuf[6] << 8) | ucBuf[7];
                SendGunConnectAbort(uiPgn, GUN_A);
    			break;
    		}
    	}

		break;
    case READ_BMS_EB_PACK:
    	switch (s_ucRec_msg_flag)
    	{
    	case REC_MUL_BRM:
    		GB_HandleEBMultipack_A(REC_MUL_BRM, "BRM", ucBuf);
    		break;
    	case REC_MUL_BCP:
    		GB_HandleEBMultipack_A(REC_MUL_BCP, "BCP", ucBuf);
    		break;
    	case REC_MUL_BCS:
    		GB_HandleEBMultipack_A(REC_MUL_BCS, "BCS", ucBuf);
    		break;
    	case REC_MUL_BSP:
    		GB_HandleEBMultipack_A(REC_MUL_BSP, "BSP", ucBuf);
    		break;
    	case REC_MUL_BMV:
    		GB_HandleEBMultipack_A(REC_MUL_BMV, "BMV", ucBuf);
    		break;
    	case REC_MUL_BMT:
    		GB_HandleEBMultipack_A(REC_MUL_BMT, "BMT", ucBuf);
    		break;
    	}

		break;
	case READ_BMS_BRO_PACK:
		GB_BroMessageProcess(ucBuf[0], GUN_A);

		break;
    case READ_BMS_BCL_PACK:
		if (sizeof(GB_BCL_Data_t) <= SEND_BMS_BYTE_NUM)
		{
			(void)memcpy(&g_sGB_Bms_data[GUN_A].sBCL, ucBuf, sizeof(GB_BCL_Data_t));
			GB_BclMessageProcess(g_sGB_Bms_data[GUN_A].sBCL, GUN_A);
		}
		break;
	case READ_BMS_BSM_PACK:
        {
        	if (sizeof(GB_BSM_Data_t) <= SEND_BMS_BYTE_NUM)
        	{
        		if (g_sGB_Version[GUN_A] == GB_2023)
        		{
        			g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BSM].unRecv_cnt++;
        			g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BSM].bEnable_flag = TRUE;
        		}

				(void)memcpy(&g_sGB_Bms_data[GUN_A].sBSM, ucBuf, sizeof(GB_BSM_Data_t));

				GB_BsmMessageProcess(g_sGB_Bms_data[GUN_A].sBSM, GUN_A);
        	}
		}
		break;
	case READ_BMS_BST_PACK:
		{
        	if (sizeof(GB_BST_Data_t) <= SEND_BMS_BYTE_NUM)
        	{
				if (1U == (g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BST].unRecv_cnt%20U))
				{
					my_printf(USER_INFO, "GUN_A: receive BST message\n");
					//防止充电中BCS多包报文未接收完成突然触发BST
					GB_RecvMultipackTimerTreat_A(REC_MUL_BCS, FALSE);
				}

    			if (FALSE == CheckChargingStatus(GUN_A))
    			{
    				return;
    			}

				(void)memcpy(&g_sGB_Bms_data[GUN_A].sBST, ucBuf, sizeof(GB_BST_Data_t));

				g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BST].unRecv_cnt++;
				GB_BstMessageProcess(g_sGB_Bms_data[GUN_A].sBST, GUN_A);
        	}
        }
		break;
	case READ_BMS_BSD_PACK:
    	if (sizeof(GB_BSD_Data_t) <= SEND_BMS_BYTE_NUM)
    	{
			if (1U == (g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BSD].unRecv_cnt%20U))
			{
				my_printf(USER_INFO, "GUN_A: receive BSD message\n");
			}

			if (FALSE == CheckChargingStatus(GUN_A))
			{
				return;
			}

			(void)memcpy(&g_sGB_Bms_data[GUN_A].sBSD, ucBuf, sizeof(GB_BSD_Data_t));
			g_sGun_data[GUN_A].sTemp.ucCurrent_soc = g_sGB_Bms_data[GUN_A].sBSD.ucStop_SOC;
	//        g_sGB_Bms_data[GUN_A].sBSD.ucMax_battery_temp;
	//        g_sGB_Bms_data[GUN_A].sBSD.ucMin_battery_temp;
	//        g_sGB_Bms_data[GUN_A].sBSD.unMax_single_vol;
	//        g_sGB_Bms_data[GUN_A].sBSD.unMin_single_vol;

			g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BSD].unRecv_cnt++;
    	}
		break;
	case READ_BMS_BEM_PACK:
    	if (sizeof(GB_BEM_Data_t) <= SEND_BMS_BYTE_NUM)
    	{
			if (1U == (g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BEM].unRecv_cnt%10U))
			{
				my_printf(USER_INFO, "GUN_A: receive BEM message\n");
			}

			if (FALSE == CheckChargingStatus(GUN_A))
			{
				return;
			}

			(void)memcpy(&g_sGB_Bms_data[GUN_A].sBEM, ucBuf, sizeof(GB_BEM_Data_t));

			g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BEM].unRecv_cnt++;
			//BEM报文超时处理
			GB_BemTimeoutProcess(g_sGB_Bms_data[GUN_A].sBEM, GUN_A);
    	}
		break;
	default:
		//my_printf(USER_ERROR, "%s:%d uiCan_id error = %d\n", __FILE__, __LINE__, uiCan_id);
	    break;
    }
    return ;
}
