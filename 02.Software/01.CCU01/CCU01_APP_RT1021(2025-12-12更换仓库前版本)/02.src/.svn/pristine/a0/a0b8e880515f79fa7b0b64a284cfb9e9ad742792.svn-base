/*
 * GB_27930_parse_B.c
 *
 *  Created on: 2025年5月5日
 *      Author: qjwu
 */
#include <board.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "hal_can.h"
#include "fsl_flexcan.h"

#include "tcp_client_IF.h"
#include "emergency_fault_IF.h"
#include "SignalManage.h"
#include "uart_comm.h"
#include "GB_27930_comm.h"
#include "charge_general.h"

static U8 s_ucRec_msg_flag = REC_MUL_MAX;
__BSS(SRAM_OC) static U8 s_ucMultipack_cnt[REC_MUL_MAX] = {0};
__BSS(SRAM_OC) static U8 s_ucMultipack_buf[REC_MUL_MAX][CAN_MAX_MSG_LEN_27930] = {0};

/**
 * @brief 多包报文超时回调函数
 * @param xTimer:定时器
 * @return
 */
static void GB_RecvMultipackTimeoutCb(TimerHandle_t xTimer)
{
    if (NULL == xTimer)
    {
        return;
    }
	if (xTimer == g_sRec_BCP_timer[GUN_B])
    {
		(void)xTimerStop(g_sRec_BCP_timer[GUN_B], 0);
		my_printf(USER_ERROR, "GUN_B BCP recv TimeOut,send pack end\n");
    }
	if (xTimer == g_sRec_BRM_timer[GUN_B])
    {
		(void)xTimerStop(g_sRec_BRM_timer[GUN_B], 0);
		my_printf(USER_ERROR, "GUN_B BRM recv TimeOut,send pack end\n");
    }
	else if (xTimer == g_sRec_BCS_timer[GUN_B])
    {
        SendGunRecBCSEndPack(GUN_B);
        g_ucSend_BCS_end_cnt[GUN_B]++;
        if (g_ucSend_BCS_end_cnt[GUN_B] > 3U)
        {
            g_ucSend_BCS_end_cnt[GUN_B] = 0;
            (void)xTimerStop(g_sRec_BCS_timer[GUN_B], 0);
            my_printf(USER_ERROR, "GUN_B BCS recv TimeOut,send pack end\n");
        }
    }
    else
    {
    	my_printf(USER_ERROR, "GUN_B Other protocol recv TimeOut\n");
    }

	return;
}

/**
 * @brief 多包报文定时器控制函数
 * @param ucPack_num：包类型
 * @param bTimer_flag：定时器
 * @return
 */
static void GB_RecvMultipackTimerTreat_B(U8 ucPack_num, BOOL bTimer_flag)
{
	if ((U8)REC_MUL_BCP == ucPack_num)
    {
        if (NULL == g_sRec_BCP_timer[GUN_B])
        {
            g_sRec_BCP_timer[GUN_B] = xTimerCreate("BCPtimer_B",
            		REC_MUL_TIMEOUT_TICK,
					pdTRUE,
					"BCP_B",
					&GB_RecvMultipackTimeoutCb);

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
    else if ((U8)REC_MUL_BRM == ucPack_num)
    {
        if (NULL == g_sRec_BRM_timer[GUN_B])
        {
            g_sRec_BRM_timer[GUN_B] = xTimerCreate("BRMtimer_B",
            		REC_MUL_TIMEOUT_TICK,
					pdTRUE,
					"BRM_B",
					&GB_RecvMultipackTimeoutCb);

            if (TRUE == bTimer_flag)
            {
                if (NULL != g_sRec_BRM_timer[GUN_B])
                {
                	(void)xTimerStart(g_sRec_BRM_timer[GUN_B], 0);
                }
            }
            else
            {
                if (NULL != g_sRec_BRM_timer[GUN_B])
                {
                	(void)xTimerStop(g_sRec_BRM_timer[GUN_B], 0);
                }
            }
        }
        else
        {
            if (TRUE == bTimer_flag)
            {
                (void)xTimerStart(g_sRec_BRM_timer[GUN_B], 0);
            }
            else
            {
                (void)xTimerStop(g_sRec_BRM_timer[GUN_B], 0);
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
					&GB_RecvMultipackTimeoutCb);

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
    	//my_printf(USER_ERROR, "%s:%d GUN_B Other ucPack_num error = %d\n", __FILE__, __LINE__, ucPack_num);
    }
}

static void GB_HandleEBMultipack_B(U8 mul_type, const char *cName, const U8 *ucBuf)
{
	if ((mul_type < REC_MUL_BRM) && (mul_type >= REC_MUL_MAX))
	{
		return;
	}

    if ((s_ucMultipack_cnt[mul_type] + 1) == ucBuf[0])
    {
        s_ucMultipack_cnt[mul_type]++;

        (void)memcpy(&s_ucMultipack_buf[mul_type][7U * (s_ucMultipack_cnt[mul_type] - 1U)], &ucBuf[1], 7);

        //重置定时器
        GB_RecvMultipackTimerTreat_B(mul_type, TRUE);

        //判断长度是否达标，决定是否结束接收
        if ((s_ucMultipack_cnt[mul_type] * 7U) >= g_ucRec_multipack_len[GUN_B][mul_type])
        {
            //校验包数是否匹配
            if (s_ucMultipack_cnt[mul_type] == g_ucRec_packnum[GUN_B][mul_type])
            {
                //调用对应类型的解析函数
            	switch (mul_type)
            	{
            	case REC_MUL_BRM:
            		GB_BrmPrase(&s_ucMultipack_buf[mul_type][0], g_ucRec_multipack_len[GUN_B][mul_type], GUN_B);
            		break;
            	case REC_MUL_BCP:
            		GB_BcpPrase(&s_ucMultipack_buf[mul_type][0], g_ucRec_multipack_len[GUN_B][mul_type], GUN_B);
            		break;
            	case REC_MUL_BCS:
            		GB_BcsPrase(&s_ucMultipack_buf[mul_type][0], g_ucRec_multipack_len[GUN_B][mul_type], GUN_B);
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
        	case REC_MUL_BRM:
        		SendGunRecBRMEndPack(GUN_B);
        		break;
        	case REC_MUL_BCP:
        		SendGunRecBCPEndPack(GUN_B);
        		break;
        	case REC_MUL_BCS:
        		SendGunRecBCSEndPack(GUN_B);
        		break;
        	case REC_MUL_BSP:
        		SendGunRecBSPEndPack(GUN_B);
        		break;
        	case REC_MUL_BMV:
        		SendGunRecBMVEndPack(GUN_B);
        		break;
        	case REC_MUL_BMT:
        		SendGunRecBMTEndPack(GUN_B);
        		break;
        	default:
        		//
        		break;
        	}

            //重置状态
            s_ucMultipack_cnt[mul_type] = 0;
            GB_RecvMultipackTimerTreat_B(mul_type, FALSE);

            //重置对应类型的标志位
            s_ucRec_msg_flag = REC_MUL_MAX;
        }
    }
    else
    {
        //包编号不连续日志
        my_printf(USER_ERROR, "GUN_B %s pack num discontinuous: expect %d, got %d",
        		cName, s_ucMultipack_cnt[mul_type] + 1, ucBuf[0]);
    }
}

void GBCAN2Parse(const U32 uiCan_id, const U8* pucBuf)
{
    U8 ucBuf[SEND_BMS_BYTE_NUM] = {0};

    (void)memcpy(ucBuf, pucBuf, sizeof(ucBuf));

    switch(uiCan_id)
    {
    case READ_BMS_BHM_PACK:
    	if (sizeof(GB_BHM_Data_t) <= SEND_BMS_BYTE_NUM)
    	{
			(void)memcpy(&g_sGB_Bms_data[GUN_B].sBHM, ucBuf, sizeof(GB_BHM_Data_t));

			g_sGun_data[GUN_B].sTemp.unEV_max_imd_vol = g_sGB_Bms_data[GUN_B].sBHM.unMax_allow_vol;

			g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BHM].unRecv_cnt++;
			//防止接受次数溢出
			if (g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BHM].unRecv_cnt > 65000U)
			{
				g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BHM].unRecv_cnt = 1;
			}
    	}

        break;
    case READ_BMS_EC_PACK:
    	if ((0x10U == ucBuf[0]) && (0U == ucBuf[5]) && (0U == ucBuf[7]))
    	{
    		switch (ucBuf[6])
    		{
    		case 0x02:
                g_ucRec_multipack_len[GUN_B][REC_MUL_BRM] = (U8)(ucBuf[1] + ((U16)ucBuf[2] << 8));
                if (g_ucRec_multipack_len[GUN_B][REC_MUL_BRM] <= CAN_MAX_MSG_LEN_27930)
                {
					g_ucRec_packnum[GUN_B][REC_MUL_BRM] = ucBuf[3];
					(void)memset(&s_ucMultipack_buf[REC_MUL_BRM][0], 0, CAN_MAX_MSG_LEN_27930);
					SendGunRecBRMRequestPack(GUN_B);
					GB_RecvMultipackTimerTreat_B(REC_MUL_BRM, TRUE);
					s_ucRec_msg_flag = REC_MUL_BRM;
					s_ucMultipack_cnt[REC_MUL_BRM] = 0;
					my_printf(USER_INFO, "GUN_B: receive BRM request message, len = %d pack_num = %d\n", g_ucRec_multipack_len[GUN_B][REC_MUL_BRM], g_ucRec_packnum[GUN_B][REC_MUL_BRM]);
                }
    			break;
    		case 0x06:
                g_ucRec_multipack_len[GUN_B][REC_MUL_BCP] = (U8)(ucBuf[1] + ((U16)ucBuf[2] << 8));
                if (g_ucRec_multipack_len[GUN_B][REC_MUL_BCP] <= CAN_MAX_MSG_LEN_27930)
                {
					g_ucRec_packnum[GUN_B][REC_MUL_BCP] = ucBuf[3];
					(void)memset(&s_ucMultipack_buf[REC_MUL_BCP][0], 0, CAN_MAX_MSG_LEN_27930);
					SendGunRecBCPRequestPack(GUN_B);
					GB_RecvMultipackTimerTreat_B(REC_MUL_BCP, TRUE);
					s_ucRec_msg_flag = REC_MUL_BCP;
					s_ucMultipack_cnt[REC_MUL_BCP] = 0;
					my_printf(USER_INFO, "GUN_B: receive BCP request message, len = %d pack_num = %d\n", g_ucRec_multipack_len[GUN_B][REC_MUL_BCP], g_ucRec_packnum[GUN_B][REC_MUL_BCP]);
                }
    			break;
    		case 0x11:
                g_ucRec_multipack_len[GUN_B][REC_MUL_BCS] = (U8)(ucBuf[1] + ((U16)ucBuf[2] << 8));

                if (g_ucRec_multipack_len[GUN_B][REC_MUL_BCS] <= CAN_MAX_MSG_LEN_27930)
                {
					g_ucRec_packnum[GUN_B][REC_MUL_BCS] = ucBuf[3];
					(void)memset(&s_ucMultipack_buf[REC_MUL_BCS][0], 0, CAN_MAX_MSG_LEN_27930);
					SendGunRecBCSRequestPack(GUN_B);
					GB_RecvMultipackTimerTreat_B(REC_MUL_BCS, TRUE);
					s_ucRec_msg_flag = REC_MUL_BCS;
					s_ucMultipack_cnt[REC_MUL_BCS] = 0;
                }
    			break;
    		case 0x15:
    			g_ucRec_multipack_len[GUN_B][REC_MUL_BSP] = (U8)(ucBuf[1] + ((U16)ucBuf[2] << 8));
    			g_ucRec_packnum[GUN_B][REC_MUL_BSP] = ucBuf[3];
    			SendGunRecBSPRequestPack(GUN_B);
    			s_ucRec_msg_flag = REC_MUL_BSP;
    			s_ucMultipack_cnt[REC_MUL_BSP] = 0;
    			break;
    		case 0x16:
    			g_ucRec_multipack_len[GUN_B][REC_MUL_BMV] = (U8)(ucBuf[1] + ((U16)ucBuf[2] << 8));
    			g_ucRec_packnum[GUN_B][REC_MUL_BMV] = ucBuf[3];
    			SendGunRecBMVRequestPack(GUN_B);
    			s_ucRec_msg_flag = REC_MUL_BMV;
    			s_ucMultipack_cnt[REC_MUL_BMV] = 0;
    			break;
    		case 0x17:
    			g_ucRec_multipack_len[GUN_B][REC_MUL_BMT] = (U8)(ucBuf[1] + ((U16)ucBuf[2] << 8));
    			g_ucRec_packnum[GUN_B][REC_MUL_BMT] = ucBuf[3];
    			SendGunRecBMTRequestPack(GUN_B);
    			s_ucRec_msg_flag = REC_MUL_BMT;
    			s_ucMultipack_cnt[REC_MUL_BMT] = 0;
    			break;
    		default:
				U32 uiPgn = 0;
				uiPgn = ((U32)ucBuf[5] << 16) | ((U32)ucBuf[6] << 8) | ucBuf[7];
				SendGunConnectAbort(uiPgn, GUN_B);
    			break;
    		}
    	}

		break;
    case READ_BMS_EB_PACK:
    	switch (s_ucRec_msg_flag)
    	{
    	case REC_MUL_BRM:
    		GB_HandleEBMultipack_B(REC_MUL_BRM, "BRM", ucBuf);
    		break;
    	case REC_MUL_BCP:
    		GB_HandleEBMultipack_B(REC_MUL_BCP, "BCP", ucBuf);
    		break;
    	case REC_MUL_BCS:
    		GB_HandleEBMultipack_B(REC_MUL_BCS, "BCS", ucBuf);
    		break;
    	case REC_MUL_BSP:
    		GB_HandleEBMultipack_B(REC_MUL_BSP, "BSP", ucBuf);
    		break;
    	case REC_MUL_BMV:
    		GB_HandleEBMultipack_B(REC_MUL_BMV, "BMV", ucBuf);
    		break;
    	case REC_MUL_BMT:
    		GB_HandleEBMultipack_B(REC_MUL_BMT, "BMT", ucBuf);
    		break;
    	}

		break;
	case READ_BMS_BRO_PACK:
		GB_BroMessageProcess(ucBuf[0], GUN_B);
		break;
    case READ_BMS_BCL_PACK:
		if (sizeof(GB_BCL_Data_t) <= SEND_BMS_BYTE_NUM)
		{
			(void)memcpy(&g_sGB_Bms_data[GUN_B].sBCL, ucBuf, sizeof(GB_BCL_Data_t));
			GB_BclMessageProcess(g_sGB_Bms_data[GUN_B].sBCL, GUN_B);
		}
		break;
	case READ_BMS_BSM_PACK:
        {
        	if (sizeof(GB_BSM_Data_t) <= SEND_BMS_BYTE_NUM)
        	{
        		if (g_sGB_Version[GUN_B] == GB_2023)
        		{
        			g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BSM].unRecv_cnt++;
        			g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BSM].bEnable_flag = TRUE;
        		}

				(void)memcpy(&g_sGB_Bms_data[GUN_B].sBSM, ucBuf, sizeof(GB_BSM_Data_t));

				GB_BsmMessageProcess(g_sGB_Bms_data[GUN_B].sBSM, GUN_B);
        	}
		}
		break;
	case READ_BMS_BST_PACK:
        {
        	if (sizeof(GB_BST_Data_t) <= SEND_BMS_BYTE_NUM)
        	{
				if (1U == (g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BST].unRecv_cnt%20U))
				{
					//防止充电中BCS多包报文未接收完成突然触发BST
					GB_RecvMultipackTimerTreat_B(REC_MUL_BCS, FALSE);
					my_printf(USER_INFO, "GUN_B: receive BST message\n");
				}

    			if (FALSE == CheckChargingStatus(GUN_B))
    			{
    				return;
    			}

				(void)memcpy(&g_sGB_Bms_data[GUN_B].sBST, ucBuf, sizeof(GB_BST_Data_t));


				g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BST].unRecv_cnt++;
				GB_BstMessageProcess(g_sGB_Bms_data[GUN_B].sBST, GUN_B);
        	}
        }
		break;
	case READ_BMS_BSD_PACK:
    	if (sizeof(GB_BSD_Data_t) <= SEND_BMS_BYTE_NUM)
    	{
			if (1U == (g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BSD].unRecv_cnt%20U))
			{
				my_printf(USER_INFO, "GUN_B: receive BSD message\n");
			}

			if (FALSE == CheckChargingStatus(GUN_B))
			{
				return;
			}
			(void)memcpy(&g_sGB_Bms_data[GUN_B].sBSD, ucBuf, sizeof(GB_BSD_Data_t));
			g_sGun_data[GUN_B].sTemp.ucCurrent_soc = g_sGB_Bms_data[GUN_B].sBSD.ucStop_SOC;
	//        g_sGB_Bms_data[GUN_B].sBSD.ucMax_battery_temp;
	//        g_sGB_Bms_data[GUN_B].sBSD.ucMin_battery_temp;
	//        g_sGB_Bms_data[GUN_B].sBSD.unMax_single_vol;
	//        g_sGB_Bms_data[GUN_B].sBSD.unMin_single_vol;
			g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BSD].unRecv_cnt++;
    	}
		break;
	case READ_BMS_BEM_PACK:
    	if (sizeof(GB_BEM_Data_t) <= SEND_BMS_BYTE_NUM)
    	{
			if (1U == (g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BEM].unRecv_cnt%10U))
			{
				my_printf(USER_INFO, "GUN_B: receive BEM message\n");
			}

			if (FALSE == CheckChargingStatus(GUN_B))
			{
				return;
			}
			(void)memcpy(&g_sGB_Bms_data[GUN_B].sBEM, ucBuf, sizeof(GB_BEM_Data_t));

			g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BEM].unRecv_cnt++;
			//BEM报文超时处理
			GB_BemTimeoutProcess(g_sGB_Bms_data[GUN_B].sBEM, GUN_B);
    	}
		break;
	default:
		//my_printf(USER_ERROR, "%s:%d uiCan_id error = %d\n", __FILE__, __LINE__, uiCan_id);
	    break;
    }

    return ;
}
