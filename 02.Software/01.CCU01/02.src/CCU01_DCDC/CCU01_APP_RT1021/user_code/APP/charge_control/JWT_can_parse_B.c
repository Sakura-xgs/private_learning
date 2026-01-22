/*
 * JWT_can_parse_B.c
 *
 *  Created on: 2025年4月14日
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
#include "JWT_can_comm.h"
#include "emergency_fault_IF.h"
#include "SignalManage.h"
#include "uart_comm.h"

static BOOL s_bRec_BCP_flag = FALSE;
static BOOL s_bRec_BCS_flag = FALSE;
//__BSS(SRAM_OC) static U8 s_ucMultipack_cnt[REC_MUL_MAX] = {0};
__BSS(SRAM_OC) static U8 s_ucMultipack_buf[REC_MUL_MAX][CAN_MAX_MSG_LEN_27930] = {0};

void JWTCAN2Parse(const U32 uiCan_id, const U8* pucBuf)
{
	static CP_STATE_e eCp_status = ERROR_STATUS;
    U8 ucBuf[SEND_BMS_BYTE_NUM] = {0};

    (void)memcpy(ucBuf, pucBuf, sizeof(ucBuf));

    switch (uiCan_id)
    {
    case READ_BMS_BHM_PACK:
    	if (sizeof(AS_BHM_Data_t) <= SEND_BMS_BYTE_NUM)
    	{
			(void)memcpy(&g_sAS_Bms_data[GUN_B].sBHM, ucBuf, sizeof(AS_BHM_Data_t));
			if ((U8)CP_DUTY_ON == g_sAS_Bms_data[GUN_B].sBHM.CR_Plc_CpStatus)
			{
				g_sGun_data[GUN_B].eConnect_status = PLUGGED_IN;
			}
			else
			{
				g_sGun_data[GUN_B].eConnect_status = (CP_STATE_e)g_sAS_Bms_data[GUN_B].sBHM.CR_Plc_CpStatus;
			}
			//版本解析
			SECCVersionParse(g_sAS_Bms_data[GUN_B].sBHM, GUN_B);

			if (eCp_status != g_sGun_data[GUN_B].eConnect_status)
			{
				my_printf(USER_INFO, "GUN_B receive BHM current_status = %d last_status = %d\n", g_sGun_data[GUN_B].eConnect_status, eCp_status);
				eCp_status = g_sGun_data[GUN_B].eConnect_status;
			}

			g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BHM].unRecv_cnt++;
			//防止接受次数溢出
			if (g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BHM].unRecv_cnt > 65000U)
			{
				g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BHM].unRecv_cnt = 1;
			}
			//g_sBms_ctrl[GUN_A].sMsg2bms_ctrl[CHM].ucSend_flag = (U8)TRUE;
    	}
        break;
    case READ_BMS_BRM_PACK:
    	if (sizeof(AS_BRM_Data_t) <= SEND_BMS_BYTE_NUM)
    	{
			(void)memcpy(&g_sAS_Bms_data[GUN_B].sBRM, (AS_BRM_Data_t*)ucBuf, sizeof(AS_BRM_Data_t));

			SECCMacParse(g_sAS_Bms_data[GUN_B].sBRM, GUN_B);

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
				SendGunRecBCPRequestPack(GUN_B);
				RecvMultipackTimerTreat_B(REC_MUL_BCP, TRUE);
				s_bRec_BCP_flag = TRUE;
				s_bRec_BCS_flag = FALSE;
				my_printf(USER_INFO, "GUN_B receive BCP request message, len = %d pack_num = %d\n", g_ucRec_multipack_len[GUN_B][REC_MUL_BCP], g_ucRec_packnum[GUN_B][REC_MUL_BCP]);
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
            uiPgn = (((U32)ucBuf[5] << 16) | ((U32)ucBuf[6] << 8) | ucBuf[7]);
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
        	if (sizeof(Stop_Data_t) <= SEND_BMS_BYTE_NUM)
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

				(void)memcpy(&g_sJwt_stop_data[GUN_B].sBST, ucBuf, sizeof(Stop_Data_t));

				ZCLErrorCodeParseFunc(g_sJwt_stop_data[GUN_B].sBST.unZclErrorCode, GUN_B);

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
    	if (sizeof(Stop_Data_t) <= SEND_BMS_BYTE_NUM)
    	{
			if (1U == (g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BEM].unRecv_cnt%40U))
			{
				my_printf(USER_INFO, "GUN_B receive BEM message\n");
			}

			if (FALSE == CheckChargingStatus(GUN_B))
			{
				return;
			}

			(void)memcpy(&g_sJwt_stop_data[GUN_B].sBEM, ucBuf, sizeof(Stop_Data_t));

			ZCLErrorCodeParseFunc(g_sJwt_stop_data[GUN_B].sBEM.unZclErrorCode, GUN_B);

			g_sBms_ctrl[GUN_B].sRecv_bms_timeout[BEM].unRecv_cnt++;
    	}
		break;
	default:
		my_printf(USER_ERROR, "%s:%d uiCan_id error = %d\n", __FILE__, __LINE__, uiCan_id);
	    break;
    }
    return ;
}
