/*
 * RKN_can_parse_B.c
 *
 *  Created bTimer_flag: 2024年11月1日
 *      Author: qjwu
 */

#include <board.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "hal_can.h"
#include "fsl_flexcan.h"

#include "AS_charge_comm.h"
#include "AS_charge_parse.h"
#include "tcp_client_IF.h"
#include "RKN_can_comm.h"
#include "charge_process_IF.h"
#include "emergency_fault_IF.h"
#include "SignalManage.h"
#include "uart_comm.h"

static BOOL s_bRec_BCP_flag = FALSE;
static BOOL s_bRec_BCS_flag = FALSE;
__BSS(SRAM_OC) static U8 s_ucMultipack_cnt[REC_MUL_MAX] = {0};
__BSS(SRAM_OC) static U8 s_ucMultipack_buf[REC_MUL_MAX][CAN_MAX_MSG_LEN_27930] = {0};

/**
 * @brief 多包报文超时回调函数
 * @param xTimer:定时器
 * @return
 */
static void RecvMultipackTimeoutCb_B(TimerHandle_t xTimer)
{
    if (NULL == xTimer)
    {
        return;
    }
	if (xTimer == g_sRec_BCP_timer[GUN_B])
    {
		(void)xTimerStop(g_sRec_BCP_timer[GUN_B], 0);
    }
	else if (xTimer == g_sRec_BCS_timer[GUN_B])
    {
        SendGunRecBCSEndPack(GUN_B);
        g_ucSend_BCS_end_cnt[GUN_B]++;
        if (g_ucSend_BCS_end_cnt[GUN_B] > 3U)
        {
            g_ucSend_BCS_end_cnt[GUN_B] = 0;
            (void)xTimerStop(g_sRec_BCS_timer[GUN_B], 0);
        }
        my_printf(USER_INFO, "GUN_B BCS recv TimeOut,send pack end\n");
    }
    else
    {
    	my_printf(USER_INFO, "GUN_B Other protocol recv TimeOut\n");
    }

	return;
}

/**
 * @brief 多包报文定时器控制函数
 * @param ucPack_num：包类型
 * @param bTimer_flag：定时器
 * @return
 */
void RecvMultipackTimerTreat_B(U8 ucPack_num, BOOL bTimer_flag)
{
	if ((U8)REC_MUL_BCP == ucPack_num)
    {
        if (NULL == g_sRec_BCP_timer[GUN_B])
        {
            g_sRec_BCP_timer[GUN_B] = xTimerCreate("BCPtimer_B",
            		REC_MUL_TIMEOUT_TICK,
					pdTRUE,
					"BCP_B",
					&RecvMultipackTimeoutCb_B);

            if (TRUE == bTimer_flag)
            {
                if (NULL != g_sRec_BCP_timer[GUN_B])
                {
                	(void)xTimerStart(g_sRec_BCP_timer[GUN_B], 0);
                }
            }
            else
            {
                if (NULL != g_sRec_BCP_timer[GUN_B])
                {
                	(void)xTimerStop(g_sRec_BCP_timer[GUN_B], 0);
                }
            }
        }
        else
        {
            if (TRUE == bTimer_flag)
            {
                (void)xTimerStart(g_sRec_BCP_timer[GUN_B], 0);
            }
            else
            {
                (void)xTimerStop(g_sRec_BCP_timer[GUN_B], 0);
            }
        }
    }
    else if ((U8)REC_MUL_BCS == ucPack_num)
    {
        if (NULL == g_sRec_BCS_timer[GUN_B])
        {
            g_sRec_BCS_timer[GUN_B] = xTimerCreate("BCStimer_B",
            		REC_MUL_TIMEOUT_TICK,
					pdTRUE,
					"BCS_B",
					&RecvMultipackTimeoutCb_B);

            if (TRUE == bTimer_flag)
            {
                if (NULL != g_sRec_BCS_timer[GUN_B])
                {
                	(void)xTimerStart(g_sRec_BCS_timer[GUN_B], 0);
                }
                g_ucSend_BCS_end_cnt[GUN_B] = 0;
            }
            else
            {
                if (NULL != g_sRec_BCS_timer[GUN_B])
                {
                	(void)xTimerStop(g_sRec_BCS_timer[GUN_B], 0);
                }
            }
        }
        else
        {
            if (TRUE == bTimer_flag)
            {
                (void)xTimerStart(g_sRec_BCS_timer[GUN_B], 0);
                g_ucSend_BCS_end_cnt[GUN_B] = 0;
            }
            else
            {
                (void)xTimerStop(g_sRec_BCS_timer[GUN_B], 0);
            }
        }
    }
    else
    {
    	my_printf(USER_ERROR, "%s:%d GUN_B Other ucPack_num error = %d\n", __FILE__, __LINE__, ucPack_num);
    }
}

/**
 * @brief 多包报文定时器控制函数
 * @param mul_type：多包报文类型
 * @param cName：报文名称定时器
 * @param ucBuf：原始数据
 * @return
 */
void HandleEBMultipack_B(const U8 mul_type, const char *cName, const U8 *ucBuf)
{
	if ((mul_type != REC_MUL_BCP) && (mul_type != REC_MUL_BCS))
	{
		return;
	}

    if ((s_ucMultipack_cnt[mul_type] + 1) == ucBuf[0])
    {
        s_ucMultipack_cnt[mul_type]++;

        (void)memcpy(&s_ucMultipack_buf[mul_type][7U * (s_ucMultipack_cnt[mul_type] - 1U)], &ucBuf[1], 7);

        RecvMultipackTimerTreat_B(mul_type, TRUE);

        if ((s_ucMultipack_cnt[mul_type] * 7U) >= g_ucRec_multipack_len[GUN_B][mul_type])
        {
            if (s_ucMultipack_cnt[mul_type] == g_ucRec_packnum[GUN_B][mul_type])
            {
            	switch (mul_type)
            	{
            	case REC_MUL_BCP:
            		AS_BcpPrase(&s_ucMultipack_buf[mul_type][0], g_ucRec_multipack_len[GUN_B][mul_type], GUN_B);
            		break;
            	case REC_MUL_BCS:
            		AS_BcsPrase(&s_ucMultipack_buf[mul_type][0], g_ucRec_multipack_len[GUN_B][mul_type], GUN_B);
            		break;
            	default:
            		//
            		break;
            	}
            }
            else
            {
                my_printf(USER_ERROR, "GUN_B %s pack num mismatch: expect %d, got %d",
                		cName, g_ucRec_packnum[GUN_B][mul_type], s_ucMultipack_cnt[mul_type]);
            }

        	switch (mul_type)
        	{
        	case REC_MUL_BCP:
        		SendGunRecBCPEndPack(GUN_B);
        		break;
        	case REC_MUL_BCS:
        		SendGunRecBCSEndPack(GUN_B);
        		break;
        	default:
        		//
        		break;
        	}

            //重置状态
            s_ucMultipack_cnt[mul_type] = 0;
            RecvMultipackTimerTreat_B(mul_type, FALSE);

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
        my_printf(USER_ERROR, "GUN_B %s pack num discontinuous: expect %d, got %d",
        		cName, s_ucMultipack_cnt[mul_type] + 1, ucBuf[0]);
    }
}

void RKNCAN2Parse(const U32 uiCan_id, const U8* pucBuf)
{
	static U8 s_ucCp_status = 0xFF;

    U8 ucBuf[SEND_BMS_BYTE_NUM] = {0};

    (void)memcpy(ucBuf, pucBuf, sizeof(ucBuf));

    switch(uiCan_id)
    {
    case READ_BMS_BHM_PACK:
    	if (sizeof(AS_BHM_Data_t) <= SEND_BMS_BYTE_NUM)
    	{
			(void)memcpy(&g_sAS_Bms_data[GUN_B].sBHM, ucBuf, sizeof(AS_BHM_Data_t));
			if ((U8)CP_DUTY_ON == g_sAS_Bms_data[GUN_B].sBHM.CR_Plc_CpStatus)
			{
				g_sGun_data[GUN_B].ucConnect_status = (U8)PLUGGED_IN;
			}
			else
			{
				g_sGun_data[GUN_B].ucConnect_status = g_sAS_Bms_data[GUN_B].sBHM.CR_Plc_CpStatus;
			}
			//版本解析
			SECCVersionParse(g_sAS_Bms_data[GUN_B].sBHM, GUN_B);

			if (s_ucCp_status != g_sGun_data[GUN_B].ucConnect_status)
			{
				my_printf(USER_INFO, "GUN_B receive BHM current_status = %d last_status = %d\n", g_sGun_data[GUN_B].ucConnect_status, s_ucCp_status);
				s_ucCp_status = g_sGun_data[GUN_B].ucConnect_status;
			}

			g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BHM].unRecv_cnt++;
			//防止接受次数溢出
			if (g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BHM].unRecv_cnt > 65000U)
			{
				g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BHM].unRecv_cnt = 1;
			}
			//g_sBms_ctrl[GUN_B].sMsg2bms_ctrl[CHM].ucSend_flag = (U8)TRUE;
    	}
        break;
    case READ_BMS_BRM_PACK:
    	if (sizeof(AS_BRM_Data_t) <= SEND_BMS_BYTE_NUM)
    	{
			(void)memcpy(&g_sAS_Bms_data[GUN_B].sBRM, ucBuf, sizeof(AS_BRM_Data_t));
			(void)memcpy(g_sGun_data[GUN_B].sTemp.ucEVCCID, g_sAS_Bms_data[GUN_B].sBRM.CR_Plc_EvCCID, 6);

			g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BRM].unRecv_cnt++;
    	}
        break;
    case READ_BMS_EC_PACK:
    	if ((0x10U == ucBuf[0]) && (0U == ucBuf[5]) && (0x06U == ucBuf[6]) && (0U == ucBuf[7]))
        {
            g_ucRec_multipack_len[GUN_B][REC_MUL_BCP] = (U8)(ucBuf[1] + ((U16)ucBuf[2] << 8));
            if (g_ucRec_multipack_len[GUN_B][REC_MUL_BCP] <= CAN_MAX_MSG_LEN_27930)
            {
				g_ucRec_packnum[GUN_B][REC_MUL_BCP] = ucBuf[3];
				(void)memset(&s_ucMultipack_buf[REC_MUL_BCP][0], 0, CAN_MAX_MSG_LEN_27930);
				my_printf(USER_INFO, "GUN_B receive multipack BCP request len = %d pack_num = %d\n", g_ucRec_multipack_len[GUN_B][REC_MUL_BCP], g_ucRec_packnum[GUN_B][REC_MUL_BCP]);
				SendGunRecBCPRequestPack(GUN_B);
				RecvMultipackTimerTreat_B(REC_MUL_BCP, TRUE);
				s_bRec_BCP_flag = TRUE;
				s_bRec_BCS_flag = FALSE;
            }
		}
		else if ((0x10U == ucBuf[0]) && (0U == ucBuf[5]) && (0x11U == ucBuf[6]) && (0U == ucBuf[7]))
		{
            g_ucRec_multipack_len[GUN_B][REC_MUL_BCS] = (U8)(ucBuf[1] + ((U16)ucBuf[2] << 8));
            if (g_ucRec_multipack_len[GUN_B][REC_MUL_BCS] <= CAN_MAX_MSG_LEN_27930)
            {
				g_ucRec_packnum[GUN_B][REC_MUL_BCS] = ucBuf[3];
				(void)memset(&s_ucMultipack_buf[REC_MUL_BCS][0], 0, CAN_MAX_MSG_LEN_27930);
				SendGunRecBCSRequestPack(GUN_B);
				RecvMultipackTimerTreat_B(REC_MUL_BCS, TRUE);
				s_bRec_BCP_flag = FALSE;
				s_bRec_BCS_flag = TRUE;
            }
		}
		else if (0x10U == ucBuf[0])
        {
            U32 uiPgn = 0;
            uiPgn = ((U32)ucBuf[5] << 16) | ((U32)ucBuf[6] << 8) | ucBuf[7];
            SendGunConnectAbort(uiPgn, GUN_B);
        }
		break;
    case  READ_BMS_EB_PACK:
    	if (TRUE == s_bRec_BCP_flag)
		{
    		HandleEBMultipack_B(REC_MUL_BCP, "BCP", ucBuf);
		}
    	else
    	{
    		HandleEBMultipack_B(REC_MUL_BCS, "BCS", ucBuf);
    	}
		break;
	case READ_BMS_BRO_PACK:
		AS_BroMessageProcess(ucBuf[0], GUN_B);
		break;
    case READ_BMS_BCL_PACK:
		if (sizeof(AS_BCL_Data_t) <= SEND_BMS_BYTE_NUM)
		{
			(void)memcpy(&g_sAS_Bms_data[GUN_B].sBCL, ucBuf, sizeof(AS_BCL_Data_t));
			AS_BclMessageProcess(g_sAS_Bms_data[GUN_B].sBCL, GUN_B);
		}
		break;
	case READ_BMS_BSM_PACK:
		if (sizeof(AS_BSM_Data_t) <= SEND_BMS_BYTE_NUM)
		{
			(void)memcpy(&g_sAS_Bms_data[GUN_B].sBSM, ucBuf, sizeof(AS_BSM_Data_t));
			AS_BsmMessageProcess(GUN_B);
		}
		break;
	case READ_BMS_BST_PACK:
        {
        	if (sizeof(RKN_BST_Data_t) <= SEND_BMS_BYTE_NUM)
        	{
				if (1U == (g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BST].unRecv_cnt%40U))
				{
					//防止充电中BCS多包报文未接收完成突然触发BST
					RecvMultipackTimerTreat_B(REC_MUL_BCS, FALSE);
					my_printf(USER_INFO, "GUN_B receive BST message\n");
				}

    			if (FALSE == CheckChargingStatus(GUN_B))
    			{
    				return;
    			}

				(void)memcpy(&g_sRkn_stop_data[GUN_B].sBST, ucBuf, sizeof(RKN_BST_Data_t));

				RKNBstMessageProcess(g_sRkn_stop_data[GUN_B].sBST, GUN_B);

				g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BST].unRecv_cnt++;
        	}
        }
		break;
	case READ_BMS_BSD_PACK:
    	if (sizeof(AS_BSD_Data_t) <= SEND_BMS_BYTE_NUM)
    	{
			if (1U == (g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BSD].unRecv_cnt%20U))
			{
				my_printf(USER_INFO, "GUN_B receive BSD message\n");
			}

			if (FALSE == CheckChargingStatus(GUN_B))
			{
				return;
			}
			(void)memcpy(&g_sAS_Bms_data[GUN_B].sBSD, ucBuf, sizeof(AS_BSD_Data_t));
			g_sGun_data[GUN_B].sTemp.ucCurrent_soc = g_sAS_Bms_data[GUN_B].sBSD.ucCR_Plc_SuspendSoc;

			g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BSD].unRecv_cnt++;
    	}
		break;
	case READ_BMS_BEM_PACK:
    	if (sizeof(RKN_BEM_Data_t) <= SEND_BMS_BYTE_NUM)
    	{
			if (1U == (g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BEM].unRecv_cnt%40U))
			{
				my_printf(USER_INFO, "GUN_B receive BEM message\n");
			}

			if (FALSE == CheckChargingStatus(GUN_B))
			{
				return;
			}
			(void)memcpy(&g_sRkn_stop_data[GUN_B].sBEM, ucBuf, sizeof(RKN_BEM_Data_t));

			if ((U8)TRUE == g_sRkn_stop_data[GUN_B].sBEM.sTimeout.uiTimeout_id)
			{
				//BEM超时报文处理
				RKNBemTimeoutProcess(g_sRkn_stop_data[GUN_B].sBEM, GUN_B);
			}
			else
			{
				//BEM故障报文处理
				RKNBemErrorProcess(g_sRkn_stop_data[GUN_B].sBEM, GUN_B);
			}

			g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BEM].unRecv_cnt++;
    	}
		break;
	default:
		my_printf(USER_ERROR, "%s:%d uiCan_id error = %d\n", __FILE__, __LINE__, uiCan_id);
	    break;
    }
    return ;
}
