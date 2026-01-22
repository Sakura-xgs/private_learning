/*
 * RKN_can_comm.c
 *
 *  Created on: 2024年11月1日
 *      Author: qjwu
 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stdio.h"
#include "fsl_gpio.h"
#include "hal_sys_IF.h"
#include "hal_ext_rtc.h"

#include "pos_IF.h"
#include "imd_IF.h"
#include "AS_charge_comm.h"
#include "AS_charge_parse.h"
#include "RKN_can_comm.h"
#include "tcp_client_IF.h"
#include "meter_IF.h"
#include "SignalManage.h"
#include "charge_process_IF.h"
#include "emergency_fault_IF.h"
#include "uart_comm.h"

static g_psCharge_Control_t g_sRKN_Secc = {0};
__BSS(SRAM_OC) AS_Charging_Charger_Data_t g_sAS_Charger_data[2] = {0};

static void RKNChargeControl(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	Com_Stage_Def_t eResult = INITDATA;
	(void)memset(&g_sBms_ctrl[ucGun_id], 0, sizeof(g_sBms_ctrl[ucGun_id]));

	if ((NULL != g_psMeter_data) && (NULL != g_psIMD_data))
	{
		U16 unImd_vol = g_psIMD_data->sPublic_data[ucGun_id].unBus_vol;
		U16 unMeter_vol = g_sGun_data[ucGun_id].uiOutput_vol / 10U;
		//判定充电启动前电压是否安全
		if ((unImd_vol > START_CHARGE_VOL_LIMIT) || (unMeter_vol > START_CHARGE_VOL_LIMIT))
		{
			vTaskDelay(1000);
			if ((unImd_vol > START_CHARGE_VOL_LIMIT) || (unMeter_vol > START_CHARGE_VOL_LIMIT))
			{
				g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = (U8)FAULT_STOP_MODE;
				if (g_sGun_fault[ucGun_id].sGeneral_fault.sItem.start_vol_more_than_safe_vol != TRUE)
				{
					GunAlarmSet(&g_sGun_fault[ucGun_id].sGeneral_fault.uiWhole_flag, BIT32_STRUCT, FAULT_START_VOL_MORE_THAN_SAFE_VOL);
				}
				my_printf(USER_ERROR, "%s:%d %s detected start meter VOL = %d or IMD VOL = %d > 10V\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B",
						unMeter_vol, unImd_vol);

				ChargeFinishProcess(ucGun_id, TRUE);

				return;
			}
		}
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d %s g_psMeter_data = NULL\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
		vTaskDelay(2000);
		return ;
	}

	CheckAnotherGunStatus(ucGun_id);

	while (1)
	{
		switch (g_sBms_ctrl[ucGun_id].sStage.sNow_status)
		{
			case INITDATA:
				if (TRUE == BmsStageSwitchCheck(&g_sBms_ctrl[ucGun_id].sStage))
				{
					//只用作初始化，不做处理
				}
				g_sBms_ctrl[ucGun_id].sStage.sNow_status = HANDSHAKE;
				break;
			case HANDSHAKE:
				if (TRUE == BmsStageSwitchCheck(&g_sBms_ctrl[ucGun_id].sStage))
				{
					g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CHM].bSend_flag = TRUE;
					g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BHM].uiStart_tick = xTaskGetTickCount();
				}
				else
				{
					eResult = AS_BmsHandshakeTreat(ucGun_id);

					if (CONFIGURE == eResult)
					{
						g_sGun_data[ucGun_id].sTemp.bAllow_charge_flag = TRUE;
						g_sBms_ctrl[ucGun_id].sStage.sNow_status = CONFIGURE;
					}
					else
					{
						//握手失败，恢复状态
						g_ucCF_Secc_StartPlc[ucGun_id] = PLC_WAIT_TO_START;
						if (TIMEOUT == eResult)
						{
							g_sBms_ctrl[ucGun_id].sStage.sNow_status = TIMEOUT;
						}
						else
						{
							g_sBms_ctrl[ucGun_id].sStage.sNow_status = STOPCHARGE;
						}
						if (FALSE == CheckEVStop(ucGun_id))
						{
							g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = (U8)FAULT_STOP_MODE;
						}
						my_printf(USER_ERROR, "%s:%d GUN_B charge HANDSHAKE failed\n", __FILE__, __LINE__);
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
					eResult = AS_BmsConfigTreat(ucGun_id);

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
							g_sBms_ctrl[ucGun_id].sStage.sNow_status = STOPCHARGE;
						}
						g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = (U8)FAULT_STOP_MODE;
						my_printf(USER_ERROR, "%s:%d %s charge CONFIGURE failed\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
					}
				}
				break;
			case CHARGING:
				if (TRUE == BmsStageSwitchCheck(&g_sBms_ctrl[ucGun_id].sStage))
				{
					g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CCS].bSend_flag = TRUE;

					if (TRUE != AS_BmsPrechargeTreat(ucGun_id))
					{
						g_sBms_ctrl[ucGun_id].sStage.sNow_status = STOPCHARGE;
						g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = (U8)FAULT_STOP_MODE;
					}
				}
				else
				{
					//充电启动完成
					g_bStart_charge_success_flag[ucGun_id] = TRUE;

					eResult = AS_BmsChargingTreat(ucGun_id);

					if ((Com_Stage_Def_t)TRUE != eResult)
					{
						if (TIMEOUT == eResult)
						{
							g_sBms_ctrl[ucGun_id].sStage.sNow_status = TIMEOUT;
						}
						else
						{
							g_sBms_ctrl[ucGun_id].sStage.sNow_status = STOPCHARGE;
						}
						g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = (U8)FAULT_STOP_MODE;
						my_printf(USER_ERROR, "%s:%d %s charge stop\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
					}
				}
				break;
			case TIMEOUT:
				if (TRUE == BmsStageSwitchCheck(&g_sBms_ctrl[ucGun_id].sStage))
				{
					g_sGun_data[ucGun_id].sTemp.unEV_target_cur = 0U;
					g_sGun_data[ucGun_id].sTemp.unEV_target_vol = 0U;
					g_psIMD_data->sPublic_data[ucGun_id].ucCheck_flag = (U8)IMD_DISABLE;
	            	//状态变更，发送充电实时数据上传报文
					g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = END_OF_CHARGE_CEM;
	            	TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);

					if (GUN_A == ucGun_id)
					{
						StopRelayControl_A();
					}
					else
					{
						StopRelayControl_B();
					}

					g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = (U8)FAULT_STOP_MODE;
					//接受到车端BEM
					if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BEM].unRecv_cnt > 0U)
					{
						g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CST].bSend_flag = TRUE;
						g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].uiStart_tick = xTaskGetTickCount();
						g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].uiStart_tick = xTaskGetTickCount();
						my_printf(USER_ERROR, "%s:%d %s receive EV BEM fault stop charge\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
						vTaskDelay(500);//确保BMS收到CST
					}
					else//桩端主动发送CEM
					{
						//清除BST接受次数
						g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].unRecv_cnt = 0;

						for (U8 i = 1; i < (U8)BMAX; i++)
						{
							if (0U != g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[i].bTimeout_flag)
							{
								my_printf(USER_ERROR, "%s:%d %s TIMEOUT: recv_bms_timeout flag = %d\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B", i);
								//清除之前的发送报文标志
								(void)memset(&g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[0], 0, sizeof(Msg2bms_Ctrl_t)*(U8)CMAX);
								g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CEM].bSend_flag = TRUE;

								g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].uiStart_tick = xTaskGetTickCount();
								g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].uiStart_tick = xTaskGetTickCount();
							}
						}
						my_printf(USER_INFO, "%s send CEM fault stop charge\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
					}
				}
				else
				{
					if (TRUE == AS_BmsTimeoutTreat(ucGun_id))
					{
						my_printf(USER_INFO, "%s TIMEOUT process complete\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
						ChargeFinishProcess(ucGun_id, FALSE);

						return;
					}
				}
				break;
			case STOPCHARGE:
				if (TRUE == BmsStageSwitchCheck(&g_sBms_ctrl[ucGun_id].sStage))
				{
					g_sGun_data[ucGun_id].sTemp.unEV_target_cur = 0U;
					g_sGun_data[ucGun_id].sTemp.unEV_target_vol = 0U;
					g_psIMD_data->sPublic_data[ucGun_id].ucCheck_flag = (U8)IMD_DISABLE;
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

					//接受到车端BST停止
					if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].unRecv_cnt > 0U)
					{
						my_printf(USER_INFO, "%s receive BMS BST\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
					}
					else//桩端主动CST停止
					{
						my_printf(USER_INFO, "%s CCU send stop charge\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
					}
					g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].uiStart_tick = xTaskGetTickCount();
					g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].uiStart_tick = g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].uiStart_tick;
					g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CCS].bSend_flag = FALSE;
					g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CST].bSend_flag = TRUE;

					vTaskDelay(500);//确保BMS收到CST
				}
				else
				{
					if (TRUE == AS_BmsStopChargeTreat(ucGun_id))
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
				my_printf(USER_ERROR, "%s:%d %s charge status error:sNow_status=%d\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B",
						g_sBms_ctrl[ucGun_id].sStage.sNow_status);
				break;
		}
		vTaskDelay(15);
	}
}

g_psCharge_Control_t* GetRknSeccModel(void)
{
	return &g_sRKN_Secc;
}

void RKNSeccInit(void)
{
	g_sRKN_Secc.ChargeControl = &RKNChargeControl;
	g_sRKN_Secc.CAN1DataParse = &RKNCAN1Parse;
	g_sRKN_Secc.CAN2DataParse = &RKNCAN2Parse;
	g_sRKN_Secc.CANMessageSend = &SeccMessageSend;
}
