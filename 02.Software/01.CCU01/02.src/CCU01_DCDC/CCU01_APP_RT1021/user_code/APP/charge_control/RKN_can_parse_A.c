/*
 * RKN_can_parse_A.c
 *
 *  Created bTimer_flag: 2024年11月1日
 *      Author: qjwu
 */
#include <board.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "hal_can.h"
#include "fsl_flexcan.h"

#include "AS_charge_comm.h"
#include "AS_charge_parse.h"
#include "tcp_client_IF.h"
#include "RKN_can_comm.h"
#include "emergency_fault_IF.h"
#include "SignalManage.h"
#include "uart_comm.h"

__BSS(SRAM_OC) AS_Charging_Bms_Data_t g_sAS_Bms_data[GUN_MAX_NUM] = {0};
__BSS(SRAM_OC) Rkn_Stop_Data_t g_sRkn_stop_data[GUN_MAX_NUM] = {0};

static BOOL s_bRec_BCP_flag = FALSE;
static BOOL s_bRec_BCS_flag = FALSE;
__BSS(SRAM_OC) static U8 s_ucMultipack_cnt[REC_MUL_MAX] = {0};
__BSS(SRAM_OC) static U8 s_ucMultipack_buf[REC_MUL_MAX][CAN_MAX_MSG_LEN_27930] = {0};

/**
 * @brief 多包报文超时回调函数
 * @param
 * @return
 */
static void RecvMultipackTimeoutCb_A(TimerHandle_t xTimer)
{
    if (NULL == xTimer)
    {
        return;
    }
	if (xTimer == g_sRec_BCP_timer[GUN_A])
    {
        (void)xTimerStop(g_sRec_BCP_timer[GUN_A], 0);
    }
	else if (xTimer == g_sRec_BCS_timer[GUN_A])
    {
        SendGunRecBCSEndPack(GUN_A);
        g_ucSend_BCS_end_cnt[GUN_A]++;
        if (g_ucSend_BCS_end_cnt[GUN_A] > 3U)
        {
            g_ucSend_BCS_end_cnt[GUN_A] = 0;
            (void)xTimerStop(g_sRec_BCS_timer[GUN_A], 0);
        }
        my_printf(USER_INFO, "GUN_A BCS recv TimeOut,send pack end\n");
    }
    else
    {
    	my_printf(USER_ERROR, "%s:%d GUN_A Other protocol recv TimeOut\n", __FILE__, __LINE__);
    }

	return;
}

/**
 * @brief 多包报文定时器控制函数
 * @param ucPack_num：包类型
 * @param bTimer_flag：定时器
 * @return
 */
void RecvMultipackTimerTreat_A(U8 ucPack_num, BOOL bTimer_flag)
{
	if ((U8)REC_MUL_BCP == ucPack_num)
    {
        if (NULL == g_sRec_BCP_timer[GUN_A])
        {
            g_sRec_BCP_timer[GUN_A] = xTimerCreate("BCPtimer_A",
            		REC_MUL_TIMEOUT_TICK,
					pdTRUE,
					"BCP_A",
					&RecvMultipackTimeoutCb_A);

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
    }
    else if ((U8)REC_MUL_BCS == ucPack_num)
    {
        if (NULL == g_sRec_BCS_timer[GUN_A])
        {
            g_sRec_BCS_timer[GUN_A] = xTimerCreate("BCStimer_A",
            		REC_MUL_TIMEOUT_TICK,
					pdTRUE,
					"BCS_A",
					&RecvMultipackTimeoutCb_A);

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
    }
    else
    {
    	my_printf(USER_ERROR, "%s:%d GUN_A Other ucPack_num error = %d\n", __FILE__, __LINE__, ucPack_num);
    }
}

/**
 * @brief BCP解析
 * @param ucBuf：接受到的数据
 * @param ucLen：数据长度
 * @param eGun_id：枪id
 * @return
 */
void AS_BcpPrase(const U8 *ucBuf, const U8 ucLen, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

    (void)memcpy(&g_sAS_Bms_data[eGun_id].sBCP, ucBuf, sizeof(AS_BCP_Data_t));

    g_sGun_data[eGun_id].sTemp.unEV_max_imd_vol = g_sAS_Bms_data[eGun_id].sBCP.unCR_Plc_EvmaxVolt;

	U16 unActual_current = g_sAS_Bms_data[eGun_id].sBCP.unCR_Plc_EvmaxCurrent & 0x7FFFU;
	if (0U == (g_sAS_Bms_data[eGun_id].sBCP.unCR_Plc_EvmaxCurrent & 0x8000U))
	{
		g_sGun_data[eGun_id].sTemp.unEV_max_allow_cur = 4000U - unActual_current;
	}
	else
	{
		g_sGun_data[eGun_id].sTemp.unEV_max_allow_cur = 4000U + unActual_current;
	}

    g_sGun_data[eGun_id].sTemp.unStart_soc = g_sAS_Bms_data[eGun_id].sBCP.unCR_Plc_EvRessSOC;
    my_printf(USER_INFO, "%s receive BCP message:VOL = %d(0.1V), CUR = %d(0.1A) SOC = %d(0.1)\n", (eGun_id == GUN_A)?"GUN_A":"GUNB", g_sGun_data[eGun_id].sTemp.unEV_max_imd_vol,
    		g_sGun_data[eGun_id].sTemp.unEV_max_allow_cur, g_sGun_data[eGun_id].sTemp.unStart_soc);

    g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BCP].unRecv_cnt++;
}

/**
 * @brief BCS解析
 * @param ucBuf：接受到的数据
 * @param ucLen：数据长度
 * @param eGun_id：枪id
 * @return
 */
void AS_BcsPrase(const U8 *ucBuf, const U8 ucLen, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	static U8 s_ucBsc_cnt = 0;
    (void)memcpy(&g_sAS_Bms_data[eGun_id].sBCS, ucBuf, sizeof(AS_BCS_Data_t));

    g_sGun_data[eGun_id].sTemp.ucCurrent_soc = g_sAS_Bms_data[eGun_id].sBCS.ucCR_Plc_EvRessSOC;

    if (INVALID_REMAIEND_TIME != g_sAS_Bms_data[eGun_id].sBCS.unCR_Plc_FullSOCRemainedTime)
    {
    	g_sGun_data[eGun_id].sTemp.unRemain_time = g_sAS_Bms_data[eGun_id].sBCS.unCR_Plc_FullSOCRemainedTime;
    }
    else
    {
    	g_sGun_data[eGun_id].sTemp.unRemain_time = 0;
    }

    g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BCS].unRecv_cnt++;

    s_ucBsc_cnt++;
    if (s_ucBsc_cnt > 40U)
    {
    	my_printf(USER_INFO, "%s receive full BCS message SOC = %d unRemain_time = %d\n", (eGun_id == GUN_A)?"GUN_A":"GUNB",
    			g_sAS_Bms_data[eGun_id].sBCS.ucCR_Plc_EvRessSOC, g_sGun_data[eGun_id].sTemp.unRemain_time);
    	s_ucBsc_cnt = 0;
    }
}

/**
 * @brief SECC BEM MessageSequence解析
 * @param ucError_code 故障码
 * @param eGun_id：枪id
 * @return
 */
static void MessageSequenceParseFunc(U8 ucError_code, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	if ((ucError_code <= (U8)SEQUENCE_RESTART_PORCESS) && (ucError_code >= (U8)INITIALIZATION_PROCESS))
	{
		if ((U8)SEQUENCE_RESTART_PORCESS == ucError_code)
		{
			if (((BOOL)(g_sSecc_fault[eGun_id].sSecc_fault_third.uiWhole_flag >> (32U-1U)) && 0x01U) != FALSE)
			{
				GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_third.uiWhole_flag, BIT32_STRUCT, (U32)(32-1));
			}
		}
		else
		{
			U8 ucTemp = ucError_code-1U;

			if (((BOOL)(g_sSecc_fault[eGun_id].sSecc_fault_third.uiWhole_flag >> ucTemp) && 0x01U) != FALSE)
			{
				GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_third.uiWhole_flag, BIT32_STRUCT, ucTemp);
			}
		}
	}
	g_sGun_data[eGun_id].sTemp.eStop_Charge_type = FAULT_STOP_MODE;
}

/**
 * @brief SECC BEM ErrorCodeByCP解析
 * @param ucError_code 故障码
 * @param eGun_id：枪id
 * @return
 */
static void ErrorCodeByCP_ParseFunc(U8 ucError_code, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	switch (ucError_code)
	{
	case GQ_ERR_CP_STATE_ZERO:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.err_cp_state_zero != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, ERR_CP_STATE_ZERO);
		}
		break;
	case GQ_ERR_PP_UNPLUG:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.err_pp_unplug != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, ERR_PP_UNPLUG);
		}
		break;
	case GQ_ERR_CP_STATE_A:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.err_cp_state_a != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, ERR_CP_STATE_A);
		}
		break;
	case GQ_ERR_CP_STATE_B:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.err_cp_state_b != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, ERR_CP_STATE_B);
		}
		break;
	case GQ_ERR_CP_STATE_C:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.err_cp_state_c != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, ERR_CP_STATE_C);
		}
		break;
	default:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.err_cp_state_c != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, ERR_CP_STATE_C);
		}
		my_printf(USER_ERROR, "%s:%d %s into ErrorCodeByCP_ParseFunc default\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUNB");
		break;
	}

	g_sGun_data[eGun_id].sTemp.eStop_Charge_type = FAULT_STOP_MODE;
}

/**
 * @brief SECC BEM ErrorCodeII解析
 * @param ucError_code 故障码
 * @param eGun_id：枪id
 * @return
 */
static void ErrorCodeIIParseFunc(U8 ucError_code, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	switch (ucError_code)
	{
	case GQ_SIG_ERROR_CP_PP_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_sig_error_cp_pp != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_SIG_ERROR_CP_PP);
		}
	break;
	case GQ_INIT_ERROR_UNKNOWN_STATE_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_init_error_unknown_state != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_INIT_UNKNOWN_STATE);
		}
		break;
	case GQ_WAIT_ERROR_UNKNOWN_STATE_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_wait_error_unknown_state != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_WAIT_UNKNOWN_STATE);
		}
		break;
	case GQ_SLAC_ERROR_UNKNOWN_STATE_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_unknown_state != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_UNKNOWN_STATE);
		}
		break;
	case GQ_SDP_ERROR_UNKNOWN_STATE_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_sdp_error_unknown_state != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SDP_UNKNOWN_STATE);
		}
		break;
	case SDP_BIND_ERROR_UNKNOWN_STATE_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_sdp_bind_error_unknown_state != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SDP_BIND_UNKNOWN_STATE);
		}
		break;
	case GQ_V2G_ACCEPT_UNKNOWN_STATE_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_v2g_accept_unknown_state != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_V2G_ACCEPT_UNKNOWN_STATE);
		}
		break;
	case GQ_V2G_HANDSHAKE_UNKNOWN_STATE_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_v2g_handshake_unknown_state != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_V2G_HANDSHAKE_UNKNOWN_STATE);
		}
		break;
	case GQ_V2G_ERROR_UNKNOWN_STATE_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_v2g_error_unknown_state != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_V2G_ERROR_UNKNOWN_STATE);
		}
		break;
	case GQ_INIT_ERROR_IFADDR_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_init_error_ifaddr != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_INIT_IFADDR);
		}
		break;
	case GQ_INIT_ERROR_OPENCHANNEL_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_init_error_openchannel != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_INIT_OPENCHANNEL);
		}
		break;
	case GQ_INIT_ERROR_KEY_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_init_error_key != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_INIT_KEY);
		}
		break;
	case GQ_SLAC_ERROR_INIT_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_slac_error_init != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_SLAC_ERROR_INIT);
		}
		break;
	case GQ_SLAC_ERROR_PARAM_TIMEOUT_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_param_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_PARAM_TIMEOUT);
		}
		break;
	case GQ_SLAC_ERROR_PARAM_SOCKET_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_param_socket != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_PARAM_SOCKET);
		}
		break;
	case SLAC_ERROR_MNBC_SOUND_TIMEOUT_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_mnbc_sound_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_MNBC_SOUND_TIMEOUT);
		}
		break;
	case SLAC_ERROR_ATTEN_CHAR_TIMEOUT_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_atten_char_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_ATTEN_CHAR_TIMEOUT);
		}
		break;
	case SLAC_ERROR_ATTEN_CHAR_SOCKET_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_atten_char_socket != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_ATTEN_CHAR_SOCKET);
		}
		break;
	case SLAC_ERROR_VALIDATE_1_TIMEOUT_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_validate_1_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_VALIDATE_1_TIMEOUT);
		}
		break;
	case SLAC_ERROR_VALIDATE_1_SOCKET_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_validate_1_socket != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_VALIDATE_1_SOCKET);
		}
		break;
	case SLAC_ERROR_BCB_TOGGLE_TIMEOUT_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_bcb_toggle_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_BCB_TOGGLE_TIMEOUT);
		}
		break;
	case GQ_SLAC_ERROR_MATCH_TIMEOUT_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_match_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_MATCH_TIMEOUT);
		}
		break;
	case GQ_SLAC_ERROR_MATCH_SOCKET_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_match_socket != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_MATCH_SOCKET);
		}
		break;
	case GQ_SLAC_ERROR_LINK_DETECT_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_link_detect != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_LINK_DETECT);
		}
		break;
	case GQ_SDP_ERROR_INIT_SOCKET_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_sdp_error_init_socket != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SDP_INIT_SOCKET);
		}
		break;
	case GQ_SDP_ERROR_TIMEOUT_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_sdp_error_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SDP_TIMEOUT);
		}
		break;
	case GQ_DIN_ERROR_TIMEOUT_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_TIMEOUT);
		}
		break;
	case GQ_DIN_ERROR_V2GTP_HEADER_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_v2gtp_header != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_V2GTP_HEADER);
		}
		break;
	case GQ_DIN_ERROR_V2GTP_HEADER_LEN_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_v2gtp_header_len != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_V2GTP_HEADER_LEN);
		}
		break;
	case GQ_DIN_ERROR_DECODE_EXI_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_decode_exi != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_DECODE_EXI);
		}
		break;
	case GQ_DIN_ERROR_CREATE_RESPONSE_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_create_response != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_CREATE_RESPONSE);
		}
		break;
	case GQ_DIN_ERROR_ENCODE_EXI_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_encode_exi != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_ENCODE_EXI);
		}
		break;
	case DIN_ERROR_V2GTP_HEADER_WRITE_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_v2gtp_header_write != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_V2GTP_HEADER_WRITE);
		}
		break;
	case GQ_DIN_ERROR_SOCKET_SEND_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_socket_send != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_SOCKET_SEND);
		}
		break;
	case GQ_DIN_ERROR_NO_PROTOCOL_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_no_protocol != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_NO_PROTOCOL);
		}
		break;
	case GQ_CHM_TIMEOUT_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_chm_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_CHM_TIMEOUT);
		}
		break;
	case GQ_DIN_FAILED_RESPONSE_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_failed_response != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_DIN_FAILED_RESPONSE);
		}
		break;
	case GQ_DIN_SEQUENCE_ERROR_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_sequence_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_DIN_SEQUENCE_ERROR);
		}
		break;
	case GQ_DIN_SIGNATURE_ERROR_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_signature_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_DIN_SIGNATURE_ERROR);
		}
		break;
	case GQ_DIN_UNKNOWN_SESSION_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_unknown_session != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_DIN_UNKNOWN_SESSION);
		}
		break;
	case GQ_DIN_SUDDEN_STOP_BY_SECC_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_sudden_stop_by_secc != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_DIN_SUDDEN_STOP_BY_SECC);
		}
		break;
	case GQ_DIN_STOP_BY_SECC_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_stop_by_secc != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_DIN_STOP_BY_SECC);
		}
		break;
	case GQ_DIN_RECEIVED_CST_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_received_cst != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_DIN_RECEIVED_CST);
			my_printf(USER_ERROR, "%s:%d %s receive CEM:GQ_DIN_RECEIVED_CST\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUNB");
		}
		break;
	case GQ_DIN_STOP_BY_EV_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_stop_by_ev != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_DIN_STOP_BY_EV);
			my_printf(USER_ERROR, "%s:%d %s receive CEM:GQ_DIN_STOP_BY_EV\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUNB");
		}
		break;
	case GQ_DIN_RECEIVED_CEM_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_received_cem != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_DIN_RECEIVED_CEM);
			my_printf(USER_ERROR, "%s:%d %s receive CEM:GQ_DIN_RECEIVED_CEM\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUNB");
		}
		break;
	case GQ_DIN_NO_REQUEST_FROM_EV_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_no_request_from_ev != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_DIN_NO_REQUEST_FROM_EV);
		}
		break;
	case GQ_DIN_EV_DO_NOT_STOP_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_ev_do_not_stop != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_DIN_EV_DO_NOT_STOP);
		}
		break;
	case GQ_V2G_COMM_PORT_MAY_LOST_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_v2g_comm_port_may_lost != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_V2G_COMM_PORT_MAY_LOST);
		}
		break;
	case GQ_CP_STATE_B_2SECONDS_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_cp_state_b_2seconds != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_CP_STATE_B_2SECONDS);
		}
		break;
	case GQ_CP_STATE_C_2SECONDS_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_cp_state_c_2seconds != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_CP_STATE_C_2SECONDS);
		}
		break;
	case GQ_DETECT_CP_ZERO_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_detect_cp_zero != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_DETECT_CP_ZERO);
		}
		break;
	case GQ_DETECT_PP_UNPLUG_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_detect_pp_unplug != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_DETECT_PP_UNPLUG);
		}
		break;
	case GQ_DETECT_CP_STATE_A_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_detect_cp_state_a != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_DETECT_CP_STATE_A);
		}
		break;
	case GQ_DETECT_CP_STATE_B_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_detect_cp_state_b != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_DETECT_CP_STATE_B);
		}
		break;
	case GQ_DETECT_CP_STATE_C_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_detect_cp_state_c != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_DETECT_CP_STATE_C);
		}
		break;
	case GQ_DETECT_CP_STATE_EF_II:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_detect_cp_state_ef != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_DETECT_CP_STATE_EF);
		}
		break;
	default:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_detect_cp_state_ef != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_DETECT_CP_STATE_EF);
		}
		my_printf(USER_ERROR, "%s:%d default %s into ErrorCodeIIParseFunc\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUNB");
		break;
	}

	g_sGun_data[eGun_id].sTemp.eStop_Charge_type = FAULT_STOP_MODE;
}

/**
 * @brief SECC BEM EVErrorCodeByEV解析
 * @param ucError_code 故障码
 * @param eGun_id：枪id
 * @return
 */
static void EVErrorCodeByEV_ParseFunc(U8 ucError_code, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	switch (ucError_code)
	{
	case EV_RESSTEMPERATUREINHIBIT:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.ev_resstemperatureinhibit != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_EV_RESSTEMPERATUREINHIBIT);
		}
	break;
	case EV_FAILED_EVSHIFTPOSITION:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.ev_failed_evshiftposition != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_EV_EVSHIFTPOSITION);
		}
		break;
	case EV_CHARGERCONNECTORLOCKFAULT:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.ev_chargerconnectorlockfault != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_EV_CHARGERCONNECTORLOCKFAULT);
		}
		break;
	case EV_FAILED_EVRESSMALFUNCTION:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.ev_failed_evressmalfunction != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_EV_EVRESSMALFUNCTION);
		}
		break;
	case EV_CHARGINGCURRENTDIFFERENTIAL:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.ev_chargingcurrentdifferential != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_EV_CHARGINGCURRENTDIFFERENTIAL);
		}
		break;
	case EV_CHARGINGVOLTAGEOUTOFRANGE:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.ev_chargingvoltageoutofrange != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_EV_CHARGINGVOLTAGEOUTOFRANGE);
		}
		break;
	case EV_CHARGINGSYSTEMINCOMPATIBILITY:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.ev_chargingsystemincompatibility != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_EV_CHARGINGSYSTEMINCOMPATIBILITY);
		}
		break;
	case EV_NODATA:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.ev_nodata != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_EV_NODATA);
		}
		break;
	default:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.ev_nodata != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_EV_NODATA);
		}
		my_printf(USER_ERROR, "%s:%d %s into EVErrorCodeByEV_ParseFunc fault\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUNB");
		break;
	}

	g_sGun_data[eGun_id].sTemp.eStop_Charge_type = FAULT_STOP_MODE;
}

/**
 * @brief SECC BEM ErrorCode解析
 * @param ucError_code：CR_Plc_ErrCode
 * @param eGun_id：枪id
 * @return
 */
static void ErrorCodeParseFunc(U8 ucError_code, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	switch (ucError_code)
	{
	case GQ_SIG_ERROR_CP:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_sig_error_cp != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SIG_ERROR_CP);
		}
		break;
	case GQ_RECEIVED_ALL_STOP:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_received_all_stop != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_RECEIVED_ALL_STOP);
		}
		break;
	case GQ_J1772_ERROR_UNKNOWN_STATE:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_j1772_error_unknown_state != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_J1772_UNKNOWN_STATE);
		}
		break;
	case GQ_CHADEMO_ERROR_UNKNOWN_STATE:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_chademo_error_unknown_state != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_CHADEMO_UNKNOWN_STATE);
		}
		break;
	case GQ_CHADEMO_RX_ERROR_UNKNOWN_STATE:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_chademo_rx_error_unknown_state != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_CHADEMO_RX_UNKNOWN_STATE);
		}
		break;
	case GQ_CHADEMO_TX_ERROR_UNKNOWN_STATE:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_chademo_tx_error_unknown_state != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_CHADEMO_TX_UNKNOWN_STATE);
		}
		break;
	case GQ_INIT_ERROR_UNKNOWN_STATE:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_init_error_unknown_state != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_INIT_UNKNOWN_STATE);
		}
		break;
	case GQ_WAIT_ERROR_UNKNOWN_STATE:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_wait_error_unknown_state != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_WAIT_UNKNOWN_STATE);
		}
		break;
	case GQ_SLAC_ERROR_UNKNOWN_STATE:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_unknown_state != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_UNKNOWN_STATE);
		}
		break;
	case GQ_SDP_ERROR_UNKNOWN_STATE:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_sdp_error_unknown_state != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SDP_UNKNOWN_STATE);
		}
		break;
	case GQ_SDP_BIND_ERROR_UNKNOWN_STATE:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_sdp_bind_error_unknown_state != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SDP_BIND_UNKNOWN_STATE);
		}
		break;
	case GQ_V2G_ACCEPT_UNKNOWN_STATE:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_v2g_accept_unknown_state != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_V2G_ACCEPT_UNKNOWN_STATE);
		}
		break;
	case GQ_V2G_HANDSHAKE_UNKNOWN_STATE:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_v2g_handshake_unknown_state != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_V2G_HANDSHAKE_UNKNOWN_STATE);
		}
		break;
	case GQ_INIT_ERROR_GENERAL:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_init_error_general != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_INIT_GENERAL);
		}
		break;
	case GQ_INIT_ERROR_IFADDR:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_init_error_ifaddr != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_INIT_IFADDR);
		}
		break;
	case GQ_INIT_ERROR_THREAD:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_init_error_thread != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_INIT_THREAD);
		}
		break;
	case GQ_INIT_ERROR_OPENCHANNEL:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_init_error_openchannel != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_INIT_OPENCHANNEL);
		}
		break;
	case GQ_INIT_ERROR_KEY:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_init_error_key != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_INIT_KEY);
		}
		break;
	case GQ_SLAC_ERROR_GENERAL:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_general != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_GENERAL);
		}
		break;
	case GQ_SLAC_ERROR_TIMER_INIT:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_timer_init != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_TIMER_INIT);
		}
		break;
	case GQ_SLAC_ERROR_TIMER_TIMEOUT:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_timer_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_TIMER_TIMEOUT);
		}
		break;
	case GQ_SLAC_ERROR_TIMER_MISC:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_timer_misc != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_TIMER_MISC);
		}
		break;
	case GQ_SLAC_ERROR_PARAM_TIMEOUT:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_param_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_PARAM_TIMEOUT);
		}
		break;
	case GQ_SLAC_ERROR_PARAM_SOCKET:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_param_socket != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_PARAM_SOCKET);
		}
		break;
	case GQ_SLAC_ERROR_START_ATTEN_CHAR_TIMEOUT:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_starttten_char_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_STARTTTEN_CHAR_TIMEOUT);
		}
		break;
	case GQ_SLAC_ERROR_MNBC_SOUND_TIMEOUT:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_mnbc_sound_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_MNBC_SOUND_TIMEOUT);
		}
		break;
	case GQ_SLAC_ERROR_ATTEN_CHAR_TIMEOUT:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_atten_char_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_ATTEN_CHAR_TIMEOUT);
		}
		break;
	case GQ_SLAC_ERROR_ATTEN_CHAR_SOCKET:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_atten_char_socket != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_ATTEN_CHAR_SOCKET);
		}
		break;
	case GQ_SLAC_ERROR_VALIDATE_1_TIMEOUT:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_validate_1_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_VALIDATE_1_TIMEOUT);
		}
		break;
	case GQ_SLAC_ERROR_VALIDATE_1_SOCKET:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_validate_1_socket != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_VALIDATE_1_SOCKET);
		}
		break;
	case GQ_SLAC_ERROR_VALIDATE_2_TIMEOUT:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_validate_2_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_VALIDATE_2_TIMEOUT);
		}
		break;
	case GQ_SLAC_ERROR_VALIDATE_2_SOCKET:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_validate_2_socket != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_VALIDATE_2_SOCKET);
		}
		break;
	case GQ_SLAC_ERROR_BCB_TOGGLE_TIMEOUT:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_bcb_toggle_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_BCB_TOGGLE_TIMEOUT);
		}
		break;
	case GQ_SLAC_ERROR_MATCH_TIMEOUT:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_match_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_MATCH_TIMEOUT);
		}
		break;
	case GQ_SLAC_ERROR_MATCH_SOCKET:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_match_socket != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_MATCH_SOCKET);
		}
		break;
	case GQ_SLAC_ERROR_READ_SOCKET:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_read_socket != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_READ_SOCKET);
		}
		break;
//	case GQ_SLAC_ERROR_SET_KEY:
//		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_set_key != (U8)TRUE)
//		{
//			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_SET_KEY);
//		}
//		break;
	case GQ_SLAC_ERROR_LINK_DETECT:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_slac_error_link_detect != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SLAC_LINK_DETECT);
		}
		break;
	case GQ_SDP_ERROR_INIT_SOCKET:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_sdp_error_init_socket != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SDP_INIT_SOCKET);
		}
		break;
	case GQ_SDP_ERROR_GENERAL:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_sdp_error_general != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SDP_GENERAL);
		}
		break;
	case GQ_SDP_ERROR_INIT_SOCKOPT1:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_sdp_error_init_sockopt1 != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SDP_INIT_SOCKOPT1);
		}
		break;
	case GQ_SDP_ERROR_INIT_SOCKOPT2:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_sdp_error_init_sockopt2 != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SDP_INIT_SOCKOPT2);
		}
		break;
	case GQ_SDP_ERROR_INIT_BIND:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_sdp_error_init_bind != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SDP_INIT_BIND);
		}
		break;
	case GQ_SDP_ERROR_THREAD_SOCKET1:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_sdp_error_thread_socket1 != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SDP_THREAD_SOCKET1);
		}
		break;
	case GQ_SDP_ERROR_THREAD_SOCKET2:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_sdp_error_thread_socket2 != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SDP_THREAD_SOCKET2);
		}
		break;
	case GQ_SDP_ERROR_TIMEOUT:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_sdp_error_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_SDP_TIMEOUT);
		}
		break;
	case GQ_DIN_ERROR_GENERAL:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_general != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_GENERAL);
		}
		break;
	case GQ_DIN_ERROR_INIT_SOCKET:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_init_socket != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_INIT_SOCKET);
		}
		break;
	case GQ_DIN_ERROR_INIT_SOCKOPT_I:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_error_init_sockopt != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_DIN_ERROR_INIT_SOCKOPT);
		}
		break;
	case GQ_DIN_ERROR_INIT_BIND:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_init_bind != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_INIT_BIND);
		}
		break;
	case GQ_DIN_ERROR_INIT_LISTEN:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_init_listen != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_INIT_LISTEN);
		}
		break;
	case GQ_DIN_ERROR_INIT_SELECT:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_init_select != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_INIT_SELECT);
		}
		break;
	case GQ_DIN_ERROR_INIT_ACCEPT:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_init_accept != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_INIT_ACCEPT);
		}
		break;
	case GQ_DIN_ERROR_TIMEOUT:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_TIMEOUT);
		}
		break;
	case GQ_DIN_ERROR_V2GTP_HEADER:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_v2gtp_header != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_V2GTP_HEADER);
		}
		break;
	case GQ_DIN_ERROR_V2GTP_HEADER_LEN:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_v2gtp_header_len != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_V2GTP_HEADER_LEN);
		}
		break;
	case GQ_DIN_ERROR_DECODE_EXI:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_decode_exi != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_DECODE_EXI);
		}
		break;
	case GQ_DIN_ERROR_CREATE_RESPONSE:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_create_response != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_CREATE_RESPONSE);
		}
		break;
	case GQ_DIN_ERROR_ENCODE_EXI:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_encode_exi != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_ENCODE_EXI);
		}
		break;
	case GQ_DIN_ERROR_V2GTP_HEADER_WRITE:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_v2gtp_header_write != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_V2GTP_HEADER_WRITE);
		}
		break;
	case GQ_DIN_ERROR_SOCKET_EXCEPTION:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_socket_exception != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_SOCKET_EXCEPTION);
		}
		break;
	case GQ_DIN_ERROR_SOCKET_SEND:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_socket_send != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_SOCKET_SEND);
		}
		break;
	case GQ_DIN_ERROR_NO_PROTOCOL:
		if (g_sSecc_fault[eGun_id].sSecc_fault_first.sItem.gq_din_error_no_protocol != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_DIN_NO_PROTOCOL);
		}
		break;
	case GQ_DIN_SHUTDOWN_BY_EVSE:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_shutdown_by_evse != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, GQ_ERROR_CHADEMO_RX_UNKNOWN_STATE);
		}
		break;
	case GQ_DIN_FAILED_RESPONSE:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_failed_response != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_DIN_FAILED_RESPONSE);
		}
		break;
	case GQ_DIN_SEQUENCE_ERROR:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_sequence_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_DIN_SEQUENCE_ERROR);
		}
		break;
	case GQ_DIN_SIGNATURE_ERROR:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_signature_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_DIN_SIGNATURE_ERROR);
		}
		break;
	case GQ_DIN_UNKNOWN_SESSION:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_unknown_session != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_DIN_UNKNOWN_SESSION);
		}
		break;
	case GQ_DIN_NO_REQUEST_FROM_EV:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_no_request_from_ev != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_DIN_NO_REQUEST_FROM_EV);
		}
		break;
//	case GQ_V2G_COMM_PORT_MAY_LOST:
//		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_v2g_comm_port_may_lost != (U8)TRUE)
//		{
//			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_V2G_COMM_PORT_MAY_LOST);
//		}
//		break;
	case GQ_CP_STATE_B_2SECONDS_I:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_cp_state_b_2seconds != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_CP_STATE_B_2SECONDS);
		}
		break;
	case GQ_CP_STATE_C_2SECONDS_I:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_cp_state_c_2seconds != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_CP_STATE_C_2SECONDS);
		}
		break;
	case GQ_DETECT_CP_ZERO_I:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_detect_cp_zero != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_DETECT_CP_ZERO);
		}
		break;
	case GQ_DETECT_PP_UNPLUG_I:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_detect_pp_unplug != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_DETECT_PP_UNPLUG);
		}
		break;
	case GQ_DETECT_CP_STATE_A_I:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_detect_cp_state_a != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_DETECT_CP_STATE_A);
		}
		break;
	case GQ_DETECT_CP_STATE_B_I:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_detect_cp_state_b != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_DETECT_CP_STATE_B);
		}
		break;
	case GQ_DETECT_CP_STATE_C_I:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_detect_cp_state_c != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_DETECT_CP_STATE_C);
		}
		break;
	case GQ_DETECT_CP_STATE_EF_I:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_detect_cp_state_ef != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_DETECT_CP_STATE_EF);
		}
		break;
	case GQ_CAN_LOSS_DETECT_ERROR:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_can_loss_detect_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_LOSS_DETECT_ERROR);
		}
		break;
	case GQ_CAN_CRMAA_TIMEOUT_ERROR:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_can_crmaa_timeout_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_CRMAA_TIMEOUT_ERROR);
		}
		break;
	case GQ_CAN_CRM00_TIMEOUT_ERROR:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_can_crm00_timeout_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_CRM00_TIMEOUT_ERROR);
		}
		break;
	case GQ_CAN_CTSCML_TIMEOUT_ERROR:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_can_ctscml_timeout_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_CTSCML_TIMEOUT_ERROR);
		}
		break;
	case GQ_CAN_CROAA_TIMEOUT_ERROR:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_can_croaa_timeout_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_CROAA_TIMEOUT_ERROR);
		}
		break;
	case GQ_CAN_CST_TIMEOUT_ERROR:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_can_cst_timeout_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_CST_TIMEOUT_ERROR);
		}
		break;
	case GQ_CAN_CCS_TIMEOUT_ERROR:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_can_ccs_timeout_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_CCS_TIMEOUT_ERROR);
		}
		break;
	case GQ_DIN_STOP_BY_EV_I:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_stop_by_ev != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, GQ_DIN_STOP_BY_EV);
		}
		g_sAS_Charger_data[eGun_id].sCST.CF_Secc_BmsSuspended = (U8)TRUE;
		g_sGun_data[eGun_id].sTemp.eStop_Charge_type = EV_DISCON_MODE;
		break;
	case GQ_ISO_PAUSE_BY_EV:
		g_sAS_Charger_data[eGun_id].sCST.CF_Secc_BmsSuspended = (U8)TRUE;
		g_sGun_data[eGun_id].sTemp.eStop_Charge_type = EV_DISCON_MODE;
		break;
	case GQ_DIN_EMERGENCY_STOP_BY_SECC:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_emergency_stop_by_secc != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_DIN_EMERGENCY_STOP_BY_SECC);
		}
		break;
	case GQ_DIN_STOP_BY_CHARGER:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_stop_by_charger != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_DIN_STOP_BY_CHARGER);
		}
		break;
	default:
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_din_stop_by_charger != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_first.u64Whole_flag, BIT64_STRUCT, SItem_GQ_DIN_STOP_BY_CHARGER);
		}
		my_printf(USER_ERROR, "%s:%d %s into ErrorCodeParseFunc fault\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUNB");
		break;
	}

	g_sGun_data[eGun_id].sTemp.eStop_Charge_type = FAULT_STOP_MODE;
}

/**
 * @brief 解析BST报文函数
 * @param sBEM_data 数据
 * @param eGun_id：枪id
 * @return
 */
void RKNBstMessageProcess(const RKN_BST_Data_t sBST_data, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

    if ((U8)TRUE == sBST_data.CF_Plc_ChgFinished)
    {
        if (((U8)FALSE == sBST_data.CF_Plc_ChargerSuspend) && ((U8)TRUE == sBST_data.CF_Plc_FullChgComplete))
        {
    		g_sAS_Charger_data[eGun_id].sCST.CF_Secc_BmsSuspended = (U8)TRUE;
    		g_sGun_data[eGun_id].sTemp.eStop_Charge_type = REACH_TOTAL_SOC_TARGET;
            if (1U == (g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BST].unRecv_cnt%10U))
            {
            	my_printf(USER_INFO, "BST: %s EV charge complete stop\n", (eGun_id==GUN_A)?"GUN_A":"GUNB");
            }
        }
        else if (((U8)FALSE == sBST_data.CF_Plc_ChargerSuspend) && ((U8)FALSE == sBST_data.CF_Plc_FullChgComplete))
        {
    		g_sAS_Charger_data[eGun_id].sCST.CF_Secc_BmsSuspended = (U8)TRUE;
    		g_sGun_data[eGun_id].sTemp.eStop_Charge_type = EV_DISCON_MODE;
            if (1U == (g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BST].unRecv_cnt%10U))
             {
        		my_printf(USER_INFO, "BST: %s EV reasons stop\n", (eGun_id==GUN_A)?"GUN_A":"GUNB");
             }
        }
        else if (((U8)TRUE == sBST_data.CF_Plc_ChargerSuspend) && ((U8)FALSE == sBST_data.CF_Plc_FullChgComplete))
        {
    		g_sAS_Charger_data[eGun_id].sCST.CF_Secc_BmsSuspended = (U8)TRUE;
            if (1U == (g_sBms_ctrl[eGun_id].sRecv_bms_timeout[BST].unRecv_cnt%10U))
             {
            	my_printf(USER_INFO, "BST: %s EVCC reasons stop\r\n", (eGun_id==GUN_A)?"GUN_A":"GUNB");
             }
        }
        else
        {
        	g_sGun_data[eGun_id].sTemp.eStop_Charge_type = FAULT_STOP_MODE;
        	my_printf(USER_INFO, "BST: %s unknown reasons stop\n", (eGun_id==GUN_A)?"GUN_A":"GUNB");
        }
    }
}

/**
 * @brief 解析BEM报文超时函数
 * @param sBEM_data 数据
 * @param eGun_id：枪id
 * @return
 */
void RKNBemTimeoutProcess(const RKN_BEM_Data_t sBEM_data, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	if ((U8)TRUE == sBEM_data.sTimeout.sItem.crm_00)
	{
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_can_crm00_timeout_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_CRM00_TIMEOUT_ERROR);
			my_printf(USER_INFO, "%s BEM error: CRM 00 is timeout\n", (eGun_id==GUN_A)?"GUN_A":"GUNB");
		}
	}
	if ((U8)TRUE == sBEM_data.sTimeout.sItem.crm_AA)
	{
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_can_crmaa_timeout_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_CRMAA_TIMEOUT_ERROR);
			my_printf(USER_INFO, "%s BEM error: CRM AA is timeout\n", (eGun_id==GUN_A)?"GUN_A":"GUNB");
		}
	}
	if ((U8)TRUE == sBEM_data.sTimeout.sItem.cml_cts)
	{
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_can_ctscml_timeout_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_CTSCML_TIMEOUT_ERROR);
			my_printf(USER_INFO, "%s BEM error: CML/CTS is timeout!!\r\n", (eGun_id==GUN_A)?"GUN_A":"GUNB");
		}
	}
	if ((U8)TRUE == sBEM_data.sTimeout.sItem.cro)
	{
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_can_croaa_timeout_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_CROAA_TIMEOUT_ERROR);
			my_printf(USER_INFO, "%s BEM error: CRO is timeout\n", (eGun_id==GUN_A)?"GUN_A":"GUNB");
		}
	}
	if ((U8)TRUE == sBEM_data.sTimeout.sItem.ccs)
	{
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_can_ccs_timeout_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_CCS_TIMEOUT_ERROR);
			my_printf(USER_INFO, "%s BEM error: CCS is timeout\n", (eGun_id==GUN_A)?"GUN_A":"GUNB");
		}
	}
	if ((U8)TRUE == sBEM_data.sTimeout.sItem.cst)
	{
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_can_cst_timeout_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_CST_TIMEOUT_ERROR);
			my_printf(USER_INFO, "%s BEM error: CST is timeout\n", (eGun_id==GUN_A)?"GUN_A":"GUNB");
		}
	}
	if ((U8)TRUE == sBEM_data.sTimeout.sItem.csd)
	{
		if (g_sSecc_fault[eGun_id].sSecc_fault_second.sItem.gq_can_csd_timeout_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[eGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_CSD_TIMEOUT_ERROR);
			my_printf(USER_INFO, "%s BEM error: CSD is timeout\n", (eGun_id==GUN_A)?"GUN_A":"GUNB");
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

/**
 * @brief 解析BEM报文错误函数
 * @param sBEM_data 数据
 * @param eGun_id：枪id
 * @return
 */
void RKNBemErrorProcess(const RKN_BEM_Data_t sBEM_data, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	if (0U != sBEM_data.sError.ucMessage_Sequence)
	{
		MessageSequenceParseFunc(g_sRkn_stop_data[eGun_id].sBEM.sError.ucMessage_Sequence, eGun_id);
		my_printf(USER_ERROR, "%s:%d %s BEM: into MessageSequenceParseFunc, error code = %d\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUNB", g_sRkn_stop_data[eGun_id].sBEM.sError.ucMessage_Sequence);
	}
	if (0U != sBEM_data.sError.ucErrorCodeII)
	{
		ErrorCodeIIParseFunc(g_sRkn_stop_data[eGun_id].sBEM.sError.ucErrorCodeII, eGun_id);
		my_printf(USER_ERROR, "%s:%d %s BEM: into ErrorCodeIIParseFunc, error code = %d\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUNB", g_sRkn_stop_data[eGun_id].sBEM.sError.ucErrorCodeII);
	}
	if (0U != sBEM_data.sError.ErrorCode_By_CP)
	{
		ErrorCodeByCP_ParseFunc(g_sRkn_stop_data[eGun_id].sBEM.sError.ErrorCode_By_CP, eGun_id);
		my_printf(USER_ERROR, "%s:%d %s BEM: into ErrorCodeByCP_ParseFunc, error code = %d\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUNB", g_sRkn_stop_data[eGun_id].sBEM.sError.ErrorCode_By_CP);
	}
	if (0U != sBEM_data.sError.EVErrorCode_By_EV)
	{
		EVErrorCodeByEV_ParseFunc(g_sRkn_stop_data[eGun_id].sBEM.sError.EVErrorCode_By_EV, eGun_id);
		my_printf(USER_ERROR, "%s:%d %s BEM: into EVErrorCodeByEV_ParseFunc, error code = %d\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUNB", g_sRkn_stop_data[eGun_id].sBEM.sError.EVErrorCode_By_EV);
	}
	if (0U != sBEM_data.sError.ucErrorCode)
	{
		ErrorCodeParseFunc(g_sRkn_stop_data[eGun_id].sBEM.sError.ucErrorCode, eGun_id);
		my_printf(USER_ERROR, "%s:%d %s BEM: into ErrorCodeParseFunc, error code = %d\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUNB", g_sRkn_stop_data[eGun_id].sBEM.sError.ucErrorCode);
		g_sAS_Charger_data[eGun_id].sCST.CF_Secc_ArtificialSuspend = (U8)TRUE;
	}
	//防止CCU充电中重启后接受到SECC的错误信息，导致下一次充电错误
	if (TRUE == CheckChargingStatus(eGun_id))
	{
		g_sBms_ctrl[eGun_id].sStage.eNow_status = STOPCHARGE;
	}
}

/**
 * @brief 多包报文定时器控制函数
 * @param mul_type：多包报文类型
 * @param cName：报文名称
 * @param ucBuf：原始数据
 * @return
 */
void HandleEBMultipack_A(const U8 mul_type, const char *cName, const U8 *ucBuf)
{
	if ((mul_type != REC_MUL_BCP) && (mul_type != REC_MUL_BCS))
	{
		return;
	}

    if ((s_ucMultipack_cnt[mul_type] + 1) == ucBuf[0])
    {
        s_ucMultipack_cnt[mul_type]++;

        (void)memcpy(&s_ucMultipack_buf[mul_type][7U * (s_ucMultipack_cnt[mul_type] - 1U)], &ucBuf[1], 7);

        RecvMultipackTimerTreat_A(mul_type, TRUE);

        if ((s_ucMultipack_cnt[mul_type] * 7U) >= g_ucRec_multipack_len[GUN_A][mul_type])
        {
            if (s_ucMultipack_cnt[mul_type] == g_ucRec_packnum[GUN_A][mul_type])
            {
            	switch (mul_type)
            	{
            	case REC_MUL_BCP:
            		AS_BcpPrase(&s_ucMultipack_buf[mul_type][0], g_ucRec_multipack_len[GUN_A][mul_type], GUN_A);
            		break;
            	case REC_MUL_BCS:
            		AS_BcsPrase(&s_ucMultipack_buf[mul_type][0], g_ucRec_multipack_len[GUN_A][mul_type], GUN_A);
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
        	case REC_MUL_BCP:
        		SendGunRecBCPEndPack(GUN_A);
        		break;
        	case REC_MUL_BCS:
        		SendGunRecBCSEndPack(GUN_A);
        		break;
        	default:
        		//
        		break;
        	}

            //重置状态
            s_ucMultipack_cnt[mul_type] = 0;
            RecvMultipackTimerTreat_A(mul_type, FALSE);

        	switch (mul_type)
        	{
        	case REC_MUL_BCP:
        		s_bRec_BCP_flag = FALSE;
        		break;
        	case REC_MUL_BCS:
        		s_bRec_BCS_flag = FALSE;
        		break;
        	default:
        		//
        		break;
        	}
        }
    }
    else
    {
        //包编号不连续日志
        my_printf(USER_ERROR, "GUN_A %s pack num discontinuous: expect %d, got %d",
        		cName, s_ucMultipack_cnt[mul_type] + 1, ucBuf[0]);
    }
}

void RKNCAN1Parse(const U32 uiCan_id, const U8* pucBuf)
{
	static CP_STATE_e eCp_status = ERROR_STATUS;
    U8 ucBuf[SEND_BMS_BYTE_NUM] = {0};

    (void)memcpy(ucBuf, pucBuf, sizeof(ucBuf));

    switch(uiCan_id)
    {
    case READ_BMS_BHM_PACK:
    	if (sizeof(AS_BHM_Data_t) <= SEND_BMS_BYTE_NUM)
    	{
			(void)memcpy(&g_sAS_Bms_data[GUN_A].sBHM, ucBuf, sizeof(AS_BHM_Data_t));
			if ((U8)CP_DUTY_ON == g_sAS_Bms_data[GUN_A].sBHM.CR_Plc_CpStatus)
			{
				g_sGun_data[GUN_A].eConnect_status = PLUGGED_IN;
			}
			else
			{
				g_sGun_data[GUN_A].eConnect_status = (CP_STATE_e)g_sAS_Bms_data[GUN_A].sBHM.CR_Plc_CpStatus;
			}
			//版本解析
			SECCVersionParse(g_sAS_Bms_data[GUN_A].sBHM, GUN_A);

			if (eCp_status != g_sGun_data[GUN_A].eConnect_status)
			{
				my_printf(USER_INFO, "GUN_A receive BHM current_status = %d last_status = %d\n", g_sGun_data[GUN_A].eConnect_status, eCp_status);
				eCp_status = g_sGun_data[GUN_A].eConnect_status;
			}

			g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BHM].unRecv_cnt++;
			//防止接受次数溢出
			if (g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BHM].unRecv_cnt > 65000U)
			{
				g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BHM].unRecv_cnt = 1;
			}
			//g_sBms_ctrl[GUN_A].sMsg2bms_ctrl[CHM].ucSend_flag = (U8)TRUE;
    	}
        break;
    case READ_BMS_BRM_PACK:
    	if (sizeof(AS_BRM_Data_t) <= SEND_BMS_BYTE_NUM)
    	{
			(void)memcpy(&g_sAS_Bms_data[GUN_A].sBRM, ucBuf, sizeof(AS_BRM_Data_t));
			(void)memcpy(g_sGun_data[GUN_A].sTemp.ucEVCCID, g_sAS_Bms_data[GUN_A].sBRM.CR_Plc_EvCCID, 6);

			g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BRM].unRecv_cnt++;
    	}
        break;
    case READ_BMS_EC_PACK:
    	if ((0x10U == ucBuf[0]) && (0U == ucBuf[5]) && (0x06U == ucBuf[6]) && (0U == ucBuf[7]))
        {
            g_ucRec_multipack_len[GUN_A][REC_MUL_BCP] = (U8)(ucBuf[1] + ((U16)ucBuf[2] << 8));
            if (g_ucRec_multipack_len[GUN_A][REC_MUL_BCP] <= CAN_MAX_MSG_LEN_27930)
            {
				g_ucRec_packnum[GUN_A][REC_MUL_BCP] = ucBuf[3];
				(void)memset(&s_ucMultipack_buf[REC_MUL_BCP][0], 0, CAN_MAX_MSG_LEN_27930);
				my_printf(USER_INFO, "GUN_A receive BCP request message, len = %d pack_num = %d\n", g_ucRec_multipack_len[GUN_A][REC_MUL_BCP], g_ucRec_packnum[GUN_A][REC_MUL_BCP]);
				SendGunRecBCPRequestPack(GUN_A);
				RecvMultipackTimerTreat_A(REC_MUL_BCP, TRUE);
				s_bRec_BCP_flag = TRUE;
				s_bRec_BCS_flag = FALSE;
            }
		}
		else if ((0x10U == ucBuf[0]) && (0U == ucBuf[5]) && (0x11U == ucBuf[6]) && (0U == ucBuf[7]))
		{
            g_ucRec_multipack_len[GUN_A][REC_MUL_BCS] = (U8)(ucBuf[1] + ((U16)ucBuf[2] << 8));
            if (g_ucRec_multipack_len[GUN_A][REC_MUL_BCS] <= CAN_MAX_MSG_LEN_27930)
            {
				g_ucRec_packnum[GUN_A][REC_MUL_BCS] = ucBuf[3];
				(void)memset(&s_ucMultipack_buf[REC_MUL_BCS][0], 0, CAN_MAX_MSG_LEN_27930);
				SendGunRecBCSRequestPack(GUN_A);
				RecvMultipackTimerTreat_A(REC_MUL_BCS, TRUE);
				s_bRec_BCP_flag = FALSE;
				s_bRec_BCS_flag = TRUE;
            }
		}
		else if (0x10U == ucBuf[0])
        {
            U32 uiPgn = 0;
            uiPgn = ((U32)ucBuf[5] << 16) | ((U32)ucBuf[6] << 8) | ucBuf[7];
            SendGunConnectAbort(uiPgn, GUN_A);
        }
		break;
    case READ_BMS_EB_PACK:
    	if (TRUE == s_bRec_BCP_flag)
		{
    		HandleEBMultipack_A(REC_MUL_BCP, "BCP", ucBuf);
		}
    	else
    	{
    		HandleEBMultipack_A(REC_MUL_BCS, "BCS", ucBuf);
    	}
		break;
	case READ_BMS_BRO_PACK:
		AS_BroMessageProcess(ucBuf[0], GUN_A);
		break;
    case READ_BMS_BCL_PACK:
		if (sizeof(AS_BCL_Data_t) <= SEND_BMS_BYTE_NUM)
		{
			(void)memcpy(&g_sAS_Bms_data[GUN_A].sBCL, ucBuf, sizeof(AS_BCL_Data_t));
			AS_BclMessageProcess(g_sAS_Bms_data[GUN_A].sBCL, GUN_A);
		}
		break;
	case READ_BMS_BSM_PACK:
		if (sizeof(AS_BSM_Data_t) <= SEND_BMS_BYTE_NUM)
		{
			(void)memcpy(&g_sAS_Bms_data[GUN_A].sBSM, ucBuf, sizeof(AS_BSM_Data_t));
			AS_BsmMessageProcess(GUN_A);
		}
		break;
	case READ_BMS_BST_PACK:
        {
        	if (sizeof(RKN_BST_Data_t) <= SEND_BMS_BYTE_NUM)
        	{
				if (1U == (g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BST].unRecv_cnt%40U))
				{
					//防止充电中BCS多包报文未接收完成突然触发BST
					RecvMultipackTimerTreat_A(REC_MUL_BCS, FALSE);
					my_printf(USER_INFO, "GUN_A receive BST message\n");
				}

    			if (FALSE == CheckChargingStatus(GUN_A))
    			{
    				return;
    			}

				(void)memcpy(&g_sRkn_stop_data[GUN_A].sBST, ucBuf, sizeof(RKN_BST_Data_t));

				RKNBstMessageProcess(g_sRkn_stop_data[GUN_A].sBST, GUN_A);

				g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BST].unRecv_cnt++;
        	}
        }
		break;
	case READ_BMS_BSD_PACK:
    	if (sizeof(AS_BSD_Data_t) <= SEND_BMS_BYTE_NUM)
    	{
			if (1U == (g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BSD].unRecv_cnt%20U))
			{
				my_printf(USER_INFO, "GUN_A receive BSD message\n");
			}

			if (FALSE == CheckChargingStatus(GUN_A))
			{
				return;
			}
			(void)memcpy(&g_sAS_Bms_data[GUN_A].sBSD, ucBuf, sizeof(AS_BSD_Data_t));
			g_sGun_data[GUN_A].sTemp.ucCurrent_soc = g_sAS_Bms_data[GUN_A].sBSD.ucCR_Plc_SuspendSoc;
			g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BSD].unRecv_cnt++;
    	}
		break;
	case READ_BMS_BEM_PACK:
    	if (sizeof(RKN_BEM_Data_t) <= SEND_BMS_BYTE_NUM)
    	{
			if (1U == (g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BEM].unRecv_cnt%40U))
			{
				my_printf(USER_INFO, "GUN_A receive BEM message\n");
			}

			if (FALSE == CheckChargingStatus(GUN_A))
			{
				return;
			}
			(void)memcpy(&g_sRkn_stop_data[GUN_A].sBEM, ucBuf, sizeof(RKN_BEM_Data_t));

			if ((U8)TRUE == g_sRkn_stop_data[GUN_A].sBEM.sTimeout.uiTimeout_id)
			{
				//BEM报文超时处理
				RKNBemTimeoutProcess(g_sRkn_stop_data[GUN_A].sBEM, GUN_A);
			}
			else
			{
				//BEM故障报文处理
				RKNBemErrorProcess(g_sRkn_stop_data[GUN_A].sBEM, GUN_A);
			}

			g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BEM].unRecv_cnt++;
    	}
		break;
	default:
		my_printf(USER_ERROR, "%s:%d uiCan_id error = %d\n", __FILE__, __LINE__, uiCan_id);
	    break;
    }
    return ;
}
