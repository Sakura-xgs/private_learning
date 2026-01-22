/*
 * hmi.c
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "cr_section_macros.h"
#include "hal_sys_IF.h"

#include "hmi.h"
#include "XRD_hmi.h"
#include "SignalManage.h"
#include "emergency_fault_IF.h"
#include "tcp_client_IF.h"
#include "calculate_crc.h"
#include "pos_IF.h"
#include "rfid_IF.h"
#include "uart_comm.h"
#include "hal_uart_IF.h"

HMI_data_t *g_psHMI_data = NULL;
static U8 g_unHmi_timeout_cnt = 0;

__BSS(SRAM_OC) HMI_Charge_Control_t g_HMI_charge_control = {0};
__BSS(SRAM_OC) HMI_Set_Num_t g_HMI_set_num = {0};
__BSS(SRAM_OC) HMI_Get_Status_t g_HMI_get_status = {0};

/**
 * @brief hmi兼容性函数，用于选择不同厂家的控制函数
 * @param
 * @return
 */
static void HmiModuleSelect(void)
{
	//获取HMI型号
	S32 uiModule_flag = 0;

	(void)GetSigVal(CCU_SET_SIG_ID_HMI_MODEL, &uiModule_flag);

	if (XRD_HMI_MODULE_FLAG == uiModule_flag)
	{
		g_psHMI_data = GetXRDHmiModel();
		my_printf(USER_INFO, "get hmi config model:XRD\n", __FILE__, __LINE__);
	}
	else
	{
		g_psHMI_data = GetXRDHmiModel();
		my_printf(USER_INFO, "use default display screen model:XRD\n", __FILE__, __LINE__);
	}
}

/**
 * @brief 用户控制HMI启动充电
 * @param
 * @return
 */
void HmiStartCharge(const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	CHECK_PTR_NULL_NO_RETURN(g_psHMI_data);
	CHECK_PTR_NULL_NO_RETURN(g_psPOS_data);
	CHECK_PTR_NULL_NO_RETURN(g_psRFID_data);

	if ((U8)POS_START_MODE == g_HMI_charge_control.ucCharge_type)
	{
		if ((FALSE == CheckChargingStatus(eGun_id))
			&& (g_psPOS_data->POS_Transaction[eGun_id].swipeStartFlag == FALSE))//充电中和充电启动中不要再次启动
		{
			g_psHMI_data->eGun_id = g_HMI_charge_control.eGun_id;
			g_psHMI_data->eHmi_control_type = START_CHARGE;
			g_psHMI_data->ucCharge_control_type = (U8)POS_START_MODE;
			//g_psHMI_data->ucPrepayment_amount = pucBuf[6];

			//告知pos启动充电
			g_psPOS_data->POS_Transaction[eGun_id].bHmi_TransactionStart_flag = TRUE;
			g_psPOS_data->POS_Transaction[eGun_id].swipeStartFlag = TRUE;//置刷卡标志，表示正在刷卡
			g_psPOS_data->POS_Transaction[eGun_id].swipeCount = 0;//清超时时间
			my_printf(USER_INFO, "receive HMI control: POS start charge\n", __FILE__, __LINE__);
		}
		else
		{
			my_printf(USER_ERROR, "Charging,Not allowed to start again\n");
		}
	}
	else if ((U8)RFID_START_MODE == g_HMI_charge_control.ucCharge_type)
	{
		g_psHMI_data->eGun_id = g_HMI_charge_control.eGun_id;
		g_psHMI_data->eHmi_control_type = START_CHARGE;
		g_psHMI_data->ucCharge_control_type = (U8)RFID_START_MODE;
		//使能刷卡板记时
		g_psRFID_data->eRfid_start_timing_flag = TIMER_OPEN_STATUS;
		//使能刷卡板寻卡
		g_psRFID_data->bRfid_search_flag = TRUE;
		my_printf(USER_INFO, "receive HMI control: RFID start charge\n", __FILE__, __LINE__);
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d HMI start charge type error: %d\n", __FILE__, __LINE__, g_HMI_charge_control.ucCharge_type);
	}
}

/**
 * @brief 用户控制HMI关闭当前启动充电方式
 * @param
 * @return
 */
void HmiCancelCharge(const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	CHECK_PTR_NULL_NO_RETURN(g_psHMI_data);
	CHECK_PTR_NULL_NO_RETURN(g_psPOS_data);

	if ((U8)POS_START_MODE == g_HMI_charge_control.ucCharge_type)
	{
		g_psHMI_data->eGun_id = g_HMI_charge_control.eGun_id;
		g_psHMI_data->ucCharge_control_type = (U8)UKNOWN_STOP_MODE;
		g_psHMI_data->eHmi_control_type = UKNOWN_STATUS;
		//告知POS用户取消使用POS启动
		//如果上一个指令是开始交易,但是开始交易还未执行,所以就无需执行取消开始交易指令,防止还未执行POS开始交易HMI发了取消
		if(g_psPOS_data->POS_Transaction[eGun_id].bHmi_TransactionStart_flag == TRUE)
		{
			g_psPOS_data->POS_Transaction[eGun_id].bHmi_TransactionStart_flag = FALSE;
		}
		else
		{
			g_psPOS_data->POS_Transaction[eGun_id].bHmi_Abort_flag = TRUE;
		}
		g_psPOS_data->POS_Transaction[eGun_id].swipeStartFlag = FALSE;//清刷卡标志
		my_printf(USER_INFO, "receive HMI cancel POS start charge\n", __FILE__, __LINE__);
	}
	else if ((U8)RFID_START_MODE == g_HMI_charge_control.ucCharge_type)
	{
		//取消刷卡板定时
		g_psRFID_data->eRfid_start_timing_flag = TIMER_CLOSE_STATUS;
		//取消刷卡板寻卡
		g_psRFID_data->bRfid_search_flag = FALSE;
		g_psHMI_data->ucCharge_control_type = (U8)UKNOWN_STOP_MODE;
		g_psHMI_data->eHmi_control_type = UKNOWN_STATUS;
		my_printf(USER_INFO, "receive HMI cancel RFID start charge\n", __FILE__, __LINE__);
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d HMI cancel charge type error: %d\n", __FILE__, __LINE__, g_HMI_charge_control.ucCharge_type);
	}
}

/**
 * @brief 用户控制HMI结束充电
 * @param
 * @return
 */
void HmiStopCharge(const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	CHECK_PTR_NULL_NO_RETURN(g_psHMI_data);

	if ((U8)POS_STOP_MODE == g_HMI_charge_control.ucCharge_type)
	{
		g_psHMI_data->eGun_id = g_HMI_charge_control.eGun_id;
		g_psHMI_data->ucCharge_control_type = (U8)POS_STOP_MODE;
		g_psHMI_data->eHmi_control_type = STOP_CHARGE;
		//告知POS充电完成，发送寻卡指令
		if (NULL != g_psPOS_data)
		{
			g_psPOS_data->POS_Transaction[eGun_id].bHmi_GetCardInfo_flag = TRUE;
		}
		my_printf(USER_INFO, "receive HMI control: POS stop charge\n", __FILE__, __LINE__);
	}
	else if ((U8)RFID_STOP_MODE == g_HMI_charge_control.ucCharge_type)
	{
		g_psHMI_data->eGun_id = g_HMI_charge_control.eGun_id;
		g_psHMI_data->ucCharge_control_type = (U8)RFID_STOP_MODE;
		g_psHMI_data->eHmi_control_type = STOP_CHARGE;
		//使能刷卡板记时
		g_psRFID_data->eRfid_start_timing_flag = TIMER_OPEN_STATUS;
		//使能刷卡板寻卡
		g_psRFID_data->bRfid_search_flag = TRUE;
		my_printf(USER_INFO, "receive HMI control: RFID stop charge\n", __FILE__, __LINE__);
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d HMI control stop charge type error: %d\n", __FILE__, __LINE__, g_HMI_charge_control.ucCharge_type);
	}
}

/**
 * @brief 桩响应HMI状态读取报文填充函数
 * @param
 * @return
 */
void PileStatusResponse(void)
{
	CHECK_PTR_NULL_NO_RETURN(g_psHMI_data);
	static __BSS(SRAM_OC) U8 s_ucResponse_buf[PILE_STATUS_RESPONSE_LEN+7U] = {0};
	static __BSS(SRAM_OC) U8 hmi_charge_finished[2] = {0};

	U32 i = 0;;
	S32 iTmp;
	U8 ucTmp_data[PILE_STATUS_RESPONSE_LEN] = {0};

	//sn码
	(void)memcpy(ucTmp_data, ucSN, 16);
	i += 16;
	//宽限时间
	(void)GetSigVal(CCU_SET_SIG_ID_GRACE_PERIOD, &iTmp);
	ucTmp_data[i] = (U8)(((U32)iTmp >> 8) & 0xffU);
	i++;
	ucTmp_data[i] = (U8)((U32)iTmp & 0xffU);
	i++;
	//充电单价
	(void)GetSigVal(CCU_SET_SIG_ID_CHARGE_PRICE_UNIT_PRICE, &iTmp);
	ucTmp_data[i] = (U8)(((U32)iTmp >> 8) & 0xffU);
	i++;
	ucTmp_data[i] = (U8)((U32)iTmp & 0xffU);
	i++;
	//占位单价
	(void)GetSigVal(CCU_SET_SIG_ID_OCCUPATION_FEE_UNIT_PRICE, &iTmp);
	ucTmp_data[i] = (U8)(((U32)iTmp >> 8) & 0xffU);
	i++;
	ucTmp_data[i] = (U8)((U32)iTmp & 0xffU);
	i++;
	(void)memcpy(&ucTmp_data[i], g_sStorage_data.ucCharge_id, sizeof(g_sStorage_data.ucCharge_id));
	//(void)memcpy(ucTmp_data+i, "1234567890", 10);
	i+=32U;
	if (TRUE == g_sStorage_data.bUse_ad_rfid)
	{
		(void)GetSigVal(ALARM_ID_RFID_COMM_LOST_ALARM, &iTmp);
		ucTmp_data[i] = (U8)iTmp;
	}
	else
	{
		//未使能，设置为通信丢失
		ucTmp_data[i] = TRUE;
	}
	i++;
	if (TRUE == g_sStorage_data.bUse_ad_pos)
	{
		(void)GetSigVal(ALARM_ID_POS_COMM_LOST_ALARM, &iTmp);
		ucTmp_data[i] = (U8)iTmp;
	}
	else
	{
		//未使能，设置为通信丢失
		ucTmp_data[i] = TRUE;
	}
	i++;
	ucTmp_data[i] = SetAlarmFlagFunc();
	i++;
	//桩编号
	ucTmp_data[i] = g_sStorage_data.ucPile_num;
	i++;
#ifdef IDLE_FEE
	//占位费开启：0 关闭：1
	ucTmp_data[i] = 0;
	i++;
#else
	//占位费开启：0 关闭：1
	ucTmp_data[i] = 1;
	i++;
#endif
	//重启控制位 正常运行：0 重启：1
	ucTmp_data[i] = g_psHMI_data->bRestore_flag;
	i++;
	//预留
	i+=9U;

	//枪id
	ucTmp_data[i] = g_sStorage_data.sPublic_data[GUN_A].eGun_id;
	i++;
	//枪类型
	ucTmp_data[i] = g_sStorage_data.sPublic_data[GUN_A].ucGun_type;
	i++;
	//枪普通状态
	//同步禁用状态到枪状态为不可用，适配HMI
	if ((g_sGun_data[GUN_A].eGun_common_status != STA_FAULT) && (g_sGun_data[GUN_A].ucGun_disable_status == (U8)GUN_DISABLE_STATUS))
	{
		ucTmp_data[i] = STA_UNAVAILABLE;
		hmi_charge_finished[GUN_A] = FALSE;
	}
	else
	{
		if(g_sGun_data[GUN_A].eGun_common_status == STA_CHARG_FINISHED)//如果枪状态有过充电完成
		{
			hmi_charge_finished[GUN_A] = TRUE;
		}
		else
		{
			if((g_sGun_data[GUN_A].eGun_common_status == STA_UNAVAILABLE) || (g_sGun_data[GUN_A].eGun_common_status == STA_FAULT))
			{
				hmi_charge_finished[GUN_A] = FALSE;
			}
		}

		if((U8)GET_GUN1_FB_STATUS() == FALSE)//还枪了
		{
			hmi_charge_finished[GUN_A] = FALSE;
		}
#ifndef IDLE_FEE
		hmi_charge_finished[GUN_A] = FALSE;
#endif

		if(hmi_charge_finished[GUN_A] == TRUE)
		{
			ucTmp_data[i] = STA_CHARG_FINISHED;
		}
		else
		{
			ucTmp_data[i] = (U8)g_sGun_data[GUN_A].eGun_common_status;
		}
	}
	i++;
	//枪特殊状态
	ucTmp_data[i] = (U8)g_sGun_data[GUN_A].eGun_special_status;
	i++;
	//枪禁用状态
	ucTmp_data[i] = g_sGun_data[GUN_A].ucGun_disable_status;
	i++;

	//如果预充状态为开始预充了，就把给hmi的绝缘状态给有效
	if(g_sGun_data[GUN_A].sTemp.ucPrecharge_status == 1)
	{
		//绝缘监测状态
		ucTmp_data[i] = 1;
	}
	else
	{
		//绝缘监测状态
		ucTmp_data[i] = g_sGun_data[GUN_A].sTemp.ucImd_status;
	}
	i++;

	//如果枪状态为充电中了，就把给hmi的预充状态给预充成功
	if(g_sGun_data[GUN_A].eGun_common_status == STA_CHARGING)
	{
		//预充状态
		ucTmp_data[i] = 2;
	}
	else
	{
		//预充状态
		ucTmp_data[i] = g_sGun_data[GUN_A].sTemp.ucPrecharge_status;
	}
	i++;

	//枪最大功率
	ucTmp_data[i] = (U8)((g_sGun_data[GUN_A].unAdmin_limit_power >> 8) & 0xffU);
	i++;
	ucTmp_data[i] = (U8)(g_sGun_data[GUN_A].unAdmin_limit_power & 0xffU);
	i++;
	//鉴权状态
	ucTmp_data[i] = g_psHMI_data->ucAuthorize_flag[GUN_A];
	i++;
	//起始电表读数
	Int2Bigendian(&ucTmp_data[i], g_sGun_data[GUN_A].sTemp.uiStart_meter_dn);
	i += 4U;
	//当前电表读数
	Int2Bigendian(&ucTmp_data[i], g_sGun_data[GUN_A].uiCurrent_meter_dn);
	i += 4U;
	//输出功率
	Int2Bigendian(&ucTmp_data[i], g_sGun_data[GUN_A].uiOutput_power);
	i += 4U;
	//已充电时间
	ucTmp_data[i] = (U8)((g_sGun_data[GUN_A].sTemp.unAlready_charge_time >> 8) & 0xffU);
	i++;
	ucTmp_data[i] = (U8)(g_sGun_data[GUN_A].sTemp.unAlready_charge_time & 0xffU);
	i++;
	//剩余充电时间
	ucTmp_data[i] = (U8)((g_sGun_data[GUN_A].sTemp.unRemain_time >> 8) & 0xffU);
	i++;
	ucTmp_data[i] = (U8)(g_sGun_data[GUN_A].sTemp.unRemain_time & 0xffU);
	i++;
	//SOC
	if (STA_START_CHARGE == g_sGun_data[GUN_A].eGun_common_status)
	{
		ucTmp_data[i] = 0U;
	}
	else
	{
		ucTmp_data[i] = g_sGun_data[GUN_A].sTemp.ucCurrent_soc;
	}
	i++;
	//占位金额
	(void)GetSigVal(CCU_SAM_SIG_ID_OCCUPATION_FEE_A, &iTmp);
	ucTmp_data[i] = (U8)(((U32)iTmp >> 8) & 0xffU);
	i++;
	ucTmp_data[i] = (U8)((U32)iTmp & 0xffU);
	i++;
	//充电金额
	(void)GetSigVal(CCU_SAM_SIG_ID_CHARGE_PRICE_A, &iTmp);
	ucTmp_data[i] = (U8)(((U32)iTmp >> 8) & 0xffU);
	i++;
	ucTmp_data[i] = (U8)((U32)iTmp & 0xffU);
	i++;
#ifdef IDLE_FEE
	//回枪状态
	ucTmp_data[i] = (U8)GET_GUN1_FB_STATUS();
#else
	ucTmp_data[i] = 0;
#endif
	i++;
	//预留
	i+=12U;

	//枪id
	ucTmp_data[i] = g_sStorage_data.sPublic_data[GUN_B].eGun_id;
	i++;
	//枪类型
	ucTmp_data[i] = g_sStorage_data.sPublic_data[GUN_B].ucGun_type;
	i++;
	//枪普通状态
	//同步禁用状态到枪状态为不可用，适配HMI
	if ((g_sGun_data[GUN_B].eGun_common_status != STA_FAULT) && (g_sGun_data[GUN_B].ucGun_disable_status == (U8)GUN_DISABLE_STATUS))
	{
		ucTmp_data[i] = STA_UNAVAILABLE;
		hmi_charge_finished[GUN_B] = FALSE;
	}
	else
	{
		if(g_sGun_data[GUN_B].eGun_common_status == STA_CHARG_FINISHED)//如果枪状态有过充电完成
		{
			hmi_charge_finished[GUN_B] = TRUE;
		}
		else
		{
			if((g_sGun_data[GUN_B].eGun_common_status == STA_UNAVAILABLE) || (g_sGun_data[GUN_B].eGun_common_status == STA_FAULT))
			{
				hmi_charge_finished[GUN_B] = FALSE;
			}
		}

		if((U8)GET_GUN2_FB_STATUS() == FALSE)//还枪了
		{
			hmi_charge_finished[GUN_B] = FALSE;
		}
#ifndef IDLE_FEE
		hmi_charge_finished[GUN_B] = FALSE;
#endif

		if(hmi_charge_finished[GUN_B] == TRUE)
		{
			ucTmp_data[i] = STA_CHARG_FINISHED;
		}
		else
		{
			ucTmp_data[i] = (U8)g_sGun_data[GUN_B].eGun_common_status;
		}
	}
	i++;
	//枪特殊状态
	ucTmp_data[i] = (U8)g_sGun_data[GUN_B].eGun_special_status;
	i++;
	//枪禁用状态
	ucTmp_data[i] = g_sGun_data[GUN_B].ucGun_disable_status;
	i++;

	//如果预充状态等于开始预充了，就把给hmi的绝缘状态有效
	if(g_sGun_data[GUN_B].sTemp.ucPrecharge_status == 1)
	{
		//绝缘监测状态
		ucTmp_data[i] = 1;
	}
	else
	{
		//绝缘监测状态
		ucTmp_data[i] = g_sGun_data[GUN_B].sTemp.ucImd_status;
	}
	i++;

	//如果枪状态是充电中了，就把预充状态给成预充成功
	if(g_sGun_data[GUN_B].eGun_common_status == STA_CHARGING)
	{
		//预充状态
		ucTmp_data[i] = 2;
	}
	else
	{
		//预充状态
		ucTmp_data[i] = g_sGun_data[GUN_B].sTemp.ucPrecharge_status;
	}
	i++;
	//枪最大功率
	ucTmp_data[i] = (U8)((g_sGun_data[GUN_B].unAdmin_limit_power >> 8) & 0xffU);
	i++;
	ucTmp_data[i] = (U8)(g_sGun_data[GUN_B].unAdmin_limit_power & 0xffU);
	i++;
	//鉴权状态
	ucTmp_data[i] = g_psHMI_data->ucAuthorize_flag[GUN_B];
	i++;
	//起始电表读数
	Int2Bigendian(&ucTmp_data[i], g_sGun_data[GUN_B].sTemp.uiStart_meter_dn);
	i += 4U;
	//当前电表读数
	Int2Bigendian(&ucTmp_data[i], g_sGun_data[GUN_B].uiCurrent_meter_dn);
	i += 4U;
	//输出功率
	Int2Bigendian(&ucTmp_data[i], g_sGun_data[GUN_B].uiOutput_power);
	i += 4U;
	//已充电时间
	ucTmp_data[i] = (U8)((g_sGun_data[GUN_B].sTemp.unAlready_charge_time >> 8) & 0xffU);
	i++;
	ucTmp_data[i] = (U8)(g_sGun_data[GUN_B].sTemp.unAlready_charge_time & 0xffU);
	i++;
	//剩余充电时间
	ucTmp_data[i] = (U8)((g_sGun_data[GUN_B].sTemp.unRemain_time >> 8) & 0xffU);
	i++;
	ucTmp_data[i] = (U8)(g_sGun_data[GUN_B].sTemp.unRemain_time & 0xffU);
	i++;
	//SOC
	if (STA_START_CHARGE == g_sGun_data[GUN_B].eGun_common_status)
	{
		ucTmp_data[i] = 0U;
	}
	else
	{
		ucTmp_data[i] = g_sGun_data[GUN_B].sTemp.ucCurrent_soc;
	}
	i++;
	//占位金额
	(void)GetSigVal(CCU_SAM_SIG_ID_OCCUPATION_FEE_B, &iTmp);
	ucTmp_data[i] = (U8)(((U32)iTmp >> 8) & 0xffU);
	i++;
	ucTmp_data[i] = (U8)((U32)iTmp & 0xffU);
	i++;
	//充电金额
	(void)GetSigVal(CCU_SAM_SIG_ID_CHARGE_PRICE_B, &iTmp);
	ucTmp_data[i] = (U8)(((U32)iTmp >> 8) & 0xffU);
	i++;
	ucTmp_data[i] = (U8)((U32)iTmp & 0xffU);
	i++;
#ifdef IDLE_FEE
	//回枪状态
	ucTmp_data[i] = (U8)GET_GUN2_FB_STATUS();
#else
	ucTmp_data[i] = 0;
#endif
	i++;
	//预留
	i+=12U;

	if (i == PILE_STATUS_RESPONSE_LEN)
	{
		s_ucResponse_buf[0] = HEAD_NUM;
		s_ucResponse_buf[1] = PILE_STATUS;
		s_ucResponse_buf[2] = (U8)((U16)PILE_STATUS_RESPONSE_LEN >> 8U);
		s_ucResponse_buf[3] = PILE_STATUS_RESPONSE_LEN;
		//数据域
		(void)memcpy(&s_ucResponse_buf[4], ucTmp_data, PILE_STATUS_RESPONSE_LEN);
		//包尾
		s_ucResponse_buf[PILE_STATUS_RESPONSE_LEN+4U] = TAIL_NUM;
		//校验位
		U16 unCrcTemp = CalCrc16(s_ucResponse_buf, PILE_STATUS_RESPONSE_LEN+5U);
		s_ucResponse_buf[PILE_STATUS_RESPONSE_LEN+5U] = (U8)(unCrcTemp >> 8U);
		s_ucResponse_buf[PILE_STATUS_RESPONSE_LEN+6U] = (U8)unCrcTemp;

		Uart_Dma_Send(HMI_UART, PILE_STATUS_RESPONSE_LEN+7U , s_ucResponse_buf);

#ifdef HMI_DEBUG
		uPRINTF("HMI response = ");
		for (U32 i = 0; i < PILE_STATUS_RESPONSE_LEN+7; i++)
		{
			uPRINTF("%02x ", s_ucResponse_buf[i]);
		}
		uPRINTF("\n");
#endif
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d response HMI status data len error = %d\n", __FILE__, __LINE__, PILE_STATUS_RESPONSE_LEN);
	}
}

static void HmiDataProcess(void)
{
	static TickType_t g_iNow_tick_hmi = 0;
	__BSS(SRAM_OC) static U8 s_ucHMI_data[UART_COMM_BUF_LEN] = {0};
	U16 u16RecByteNum = 0;

	u16RecByteNum = RecUartData(HMI_UART, s_ucHMI_data, UART_COMM_BUF_LEN);

	if(UART_COMM_BUF_LEN < u16RecByteNum)
	{
		return;
	}

	if(u16RecByteNum == 0U)
	{
		ReconfigUart6();
		Uart_Dma_Init(HMI_UART);
		Uart_Init(HMI_UART);
		my_printf(USER_ERROR, "%s:%d reconfig hmi uart\n", __FILE__, __LINE__);
		Uart_Init(HMI_UART);
		return ;
	}

	CHECK_PTR_NULL_NO_RETURN(g_psHMI_data);
    //解析
	g_psHMI_data->HmiDataParse(s_ucHMI_data, u16RecByteNum);

	(void)memcpy(g_sPile_data.ucHmi_version, g_psHMI_data->ucHmi_version, sizeof(g_sPile_data.ucHmi_version));

	g_iNow_tick_hmi = xTaskGetTickCount();
	//复位超时计数
	g_unHmi_timeout_cnt = 0;

	//自检通过
	if ((U8)FALSE == g_sSelf_check.sItem.hmi_comm_flag)
	{
		g_sSelf_check.sItem.hmi_comm_flag = (U8)TRUE;
	}
}

static void HmiCommCheck(void)
{
	S32 iTmp_flag = 0;

	if (g_unHmi_timeout_cnt < 255U)
	{
		g_unHmi_timeout_cnt++;
	}

	//置位显示屏通讯故障
	if (g_unHmi_timeout_cnt > (DEVICE_TIMEOUT_MS/WORKING_STATUS_TASK_PERIOD_MS))
	{
		(void)GetSigVal(ALARM_ID_HMI_COMM_LOST_ALARM, &iTmp_flag);
		if (iTmp_flag != COMM_LOST)
		{
			PileAlarmSet(ALARM_ID_HMI_COMM_LOST_ALARM);
			my_printf(USER_ERROR, "%s:%d trigger screen comm lost warning\n", __FILE__, __LINE__);
		}
	}
	else
	{
		(void)GetSigVal(ALARM_ID_HMI_COMM_LOST_ALARM, &iTmp_flag);
		if (iTmp_flag == COMM_LOST)
		{
			//恢复
			PileAlarmReset(ALARM_ID_HMI_COMM_LOST_ALARM);
			my_printf(USER_INFO, "%s:%d trigger screen comm restore\n", __FILE__, __LINE__);
		}
	}
}

static void Hmi_COMM_CHECK_Task(void *pvParameter)
{
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
	volatile UBaseType_t uxHighWaterMark;
#endif
	while (1)
	{
		HmiCommCheck();

		vTaskDelay(WORKING_STATUS_TASK_PERIOD_MS);
		#ifdef FREE_STACK_SPACE_CHECK_ENABLE
		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
		#endif
	}
}

static void Hmi_Parse_Task(void *pvParameter)
{
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
	volatile UBaseType_t uxHighWaterMark;
#endif
	BaseType_t err = pdFALSE;

	while(1)
	{
		if(NULL != Uart_recBinarySemaphore(HMI_UART))
		{
			err = xSemaphoreTake(Uart_recBinarySemaphore(HMI_UART), portMAX_DELAY);
			if(pdPASS == err)
			{
				HmiDataProcess();
			}
			else
			{
				vTaskDelay(300);
			}
		}
		else
		{
			vTaskDelay(300);
		}
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
#endif
	}
}

void Hmi_Init_Task(void * pvParameters)
{
	//外设初始化
	XRDHmiInit();
	HmiModuleSelect();
    vTaskDelay(500);

	taskENTER_CRITICAL();

	if (TRUE == g_sStorage_data.bUse_ad_screen)
	{
		//add other hmi_init task here
		(void)xTaskCreate(&Hmi_Parse_Task, "HMI_PARSE", 1000U/4U, NULL, GENERAL_TASK_PRIO, NULL);
		(void)xTaskCreate(&Hmi_COMM_CHECK_Task, "HMI_COMM_CHECK", 500U/4U, NULL, GENERAL_TASK_PRIO, NULL);
	}

	vTaskDelete(NULL);

	taskEXIT_CRITICAL();
}
