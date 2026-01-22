/*
 * JWT_can_parse_A.c
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
#include "charge_process_IF.h"
#include "emergency_fault_IF.h"
#include "SignalManage.h"
#include "uart_comm.h"

__BSS(SRAM_OC) JWT_Stop_Data_t g_sJwt_stop_data[2] = {0};

__BSS(SRAM_OC) static U8 s_ucMultipack_cnt[REC_MUL_MAX] = {0};
__BSS(SRAM_OC) static U8 s_ucMultipack_buf[REC_MUL_MAX][CAN_MAX_MSG_LEN_27930] = {0};
static BOOL s_bRec_BCP_flag = FALSE;
static BOOL s_bRec_BCS_flag = FALSE;

/**
 * @brief JWT ZCL故障码解析
 * @param unError_code 故障码
 * @param ucGun_id：枪id
 * @return
 */
static void JWTSeccFaultCodeParse(U16 unError_code, U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	switch (unError_code)
	{
	case CHADEMO_LockErr:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_LockErr != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_LockErr_Bit);
		}
		break;
	case CHADEMO_Start_NoPlugIn_TimeOut:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_Start_NoPlugIn_TimeOut != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_Start_NoPlugIn_TimeOut_Bit);
		}
		break;
	case CHADEMO_WaitComm_TimeOut:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_WaitComm_TimeOut != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_WaitComm_TimeOut_Bit);
		}
		break;
	case CHADEMO_VehicleShiftPosition_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_VehicleShiftPosition_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_VehicleShiftPosition_Error_Bit);
		}
		break;
	case CHADEMO_Comm_VehicleParaUpdated_TimeOut:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_Comm_VehicleParaUpdated_TimeOut != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_Comm_VehicleParaUpdated_TimeOut_Bit);
		}
		break;
	case CHADEMO_Comm_ReqVolOverMaxVol:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_Comm_ReqVolOverMaxVol != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_Comm_ReqVolOverMaxVol_Bit);
		}
		break;
	case CHADEMO_Comm_ReqVolLessAvailableVol:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_Comm_ReqVolLessAvailableVol != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_Comm_ReqVolLessAvailableVol_Bit);
		}
		break;
	case CHADEMO_WaitEvseSelfCheckOK_TimeOut:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_WaitEvseSelfCheckOK_TimeOut != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_WaitEvseSelfCheckOK_TimeOut_Bit);
		}
		break;
	case CHADEMO_CP3_ChargePermission_BeforeError:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_CP3_ChargePermission_BeforeError != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_CP3_ChargePermission_BeforeError_Bit);
		}
		break;
	case CHADEMO_102_5_ChargingBit_BeforeError:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_102_5_ChargingBit_BeforeError != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_102_5_ChargingBit_BeforeError_Bit);
		}
		break;
	case CHADEMO_PreChargeWaitEvContactors_TimeOut:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_PreChargeWaitEvContactors_TimeOut != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_PreChargeWaitEvContactors_TimeOut_Bit);
		}
		break;
	case CHADEMO_PreChargeWaitEvReqCurrent_TimeOut:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_PreChargeWaitEvReqCurrent_TimeOut != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_PreChargeWaitEvReqCurrent_TimeOut_Bit);
		}
		break;
	case CHADEMO_VEHICLE_OVERVOLT_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_VEHICLE_OVERVOLT_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_VEHICLE_OVERVOLT_Error_Bit);
		}
		break;
	case CHADEMO_VEHICLE_UNDERVOLT_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_VEHICLE_UNDERVOLT_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_VEHICLE_UNDERVOLT_Error_Bit);
		}
		break;
	case CHADEMO_VEHICLE_CURRDIFF_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_VEHICLE_CURRDIFF_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_VEHICLE_CURRDIFF_Error_Bit);
		}
		break;
	case CHADEMO_VEHICLE_TEMPERATURE_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_VEHICLE_TEMPERATURE_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_VEHICLE_TEMPERATURE_Error_Bit);
		}
		break;
	case CHADEMO_VEHICLE_VOLTDIFF_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_VEHICLE_VOLTDIFF_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_VEHICLE_VOLTDIFF_Error_Bit);
		}
		break;
	case CHADEMO_VehicleGENERAL_ERROR_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_VehicleGENERAL_ERROR_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_VehicleGENERAL_ERROR_Error_Bit);
		}
		break;
	case CHADEMO_Charing_ChargePermission_Disable:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_Charing_ChargePermission_Disable != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_Charing_ChargePermission_Disable_Bit);
		}
		break;
	case CHADEMO_EvseTimeOut_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_EvseTimeOut_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_EvseTimeOut_Error_Bit);
		}
		break;
	case CHADEMO_EvTimeOut_100_101_102_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CHADEMO_EvTimeOut_100_101_102_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CHADEMO_EvTimeOut_100_101_102_Error_Bit);
		}
		break;
	case CCS2_FastProtect_CPliotDetected:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_FastProtect_CPliotDetected != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_FastProtect_CPliotDetected_Bit);
		}
		break;
	case CCS2_Slac_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_Slac_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_Slac_Error_Bit);
		}
		break;
	case CCS2_Session_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_Session_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_Session_Error_Bit);
		}
		break;
	case CCS2_EvseTimeOut_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_EvseTimeOut_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_EvseTimeOut_Error_Bit);
		}
		break;
	case CCS2_SlacDetected_Check_TimeOut:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_SlacDetected_Check_TimeOut != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_SlacDetected_Check_TimeOut_Bit);
		}
		break;
	case CCS2_Start_NoPlugIn_TimeOut:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_Start_NoPlugIn_TimeOut != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_Start_NoPlugIn_TimeOut_Bit);
		}
		break;
	case CCS2_Wait_NetWorkInitOK_TimeOut:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_Wait_NetWorkInitOK_TimeOut != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_Wait_NetWorkInitOK_TimeOut_Bit);
		}
		break;
	case CCS2_Wait_SDP_TimeOut:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_Wait_SDP_TimeOut != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_Wait_SDP_TimeOut_Bit);
		}
		break;
	case CCS2_Wait_SAP_TimeOut:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_Wait_SAP_TimeOut != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_Wait_SAP_TimeOut_Bit);
		}
		break;
	case CCS2_SLAC_Static_SET_KEY_TimeOut:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_SLAC_Static_SET_KEY_TimeOut != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_SLAC_Static_SET_KEY_TimeOut_Bit);
		}
		break;
	case CCS2_SLAC_Wait_CM_SLAC_PARM_REQ_TimeOut:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_SLAC_Wait_CM_SLAC_PARM_REQ_TimeOut != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_SLAC_Wait_CM_SLAC_PARM_REQ_TimeOut_Bit);
		}
		break;
	case CCS2_SLAC_Wait_ATTEN_CHAR_IND_TimeOut:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_SLAC_Wait_ATTEN_CHAR_IND_TimeOut != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_SLAC_Wait_ATTEN_CHAR_IND_TimeOut_Bit);
		}
		break;
	case CCS2_SLAC_Wait_ATTEN_CHAR_RSP_TimeOut:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_SLAC_Wait_ATTEN_CHAR_RSP_TimeOut != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_SLAC_Wait_ATTEN_CHAR_RSP_TimeOut_Bit);
		}
		break;
	case CCS2_SLAC_Wait_MATCH_OR_VALIDATE_TimeOut:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_SLAC_Wait_MATCH_OR_VALIDATE_TimeOut != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_SLAC_Wait_MATCH_OR_VALIDATE_TimeOut_Bit);
		}
		break;
	case CCS2_SLAC_Wait_LinkDetected_TimeOut:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_SLAC_Wait_LinkDetected_TimeOut != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_SLAC_Wait_LinkDetected_TimeOut_Bit);
		}
		break;
	case CCS2_SLAC_SlacTimeout:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_SLAC_SlacTimeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_SLAC_SlacTimeout_Bit);
		}
		break;
	case CCS2_SLAC_RSP_RETRY_LIMIT:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_SLAC_RSP_RETRY_LIMIT != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_SLAC_RSP_RETRY_LIMIT);
		}
		break;
	case CCS2_SLAC_Match_ErrLIMIT:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_SLAC_Match_ErrLIMIT != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_SLAC_Match_ErrLIMIT_Bit);
		}
		break;
	case CCS2_SLAC_CM_START_ATTEN_CHAR_TIMEOUT:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_SLAC_CM_START_ATTEN_CHAR_TIMEOUT != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_SLAC_CM_START_ATTEN_CHAR_TIMEOUT_Bit);
		}
		break;
	case CCS2_ServiceCategoryFlag_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_ServiceCategoryFlag_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_ServiceCategoryFlag_Error_Bit);
		}
		break;
	case CCS2_SelectedPaymentOption_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_SelectedPaymentOption_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_SelectedPaymentOption_Error_Bit);
		}
		break;
	case CCS2_GenChallenge_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_GenChallenge_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_GenChallenge_Error_Bit);
		}
		break;
	case CCS2_AC_EVChargeParameter_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_AC_EVChargeParameter_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_AC_EVChargeParameter_Error_Bit);
		}
		break;
	case CCS2_PhysicalValue_Multiplier_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_PhysicalValue_Multiplier_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_PhysicalValue_Multiplier_Error_Bit);
		}
		break;
	case CCS2_PhysicalValue_Unit_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_PhysicalValue_Unit_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_PhysicalValue_Unit_Error_Bit);
		}
		break;
	case CCS2_RequestedEnergyTransferType_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_RequestedEnergyTransferType_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_RequestedEnergyTransferType_Error_Bit);
		}
		break;
	case CCS2_EVMaxVolLessThanSeccMinVol_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_EVMaxVolLessThanSeccMinVol_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_EVMaxVolLessThanSeccMinVol_Error_Bit);
		}
		break;
	case CCS2_EVMaxCurLessThanSeccMinCur_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_EVMaxCurLessThanSeccMinCur_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_EVMaxCurLessThanSeccMinCur_Error_Bit);
		}
		break;
	case CCS2_EVReqVolLessThanSeccMinVol_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_EVReqVolLessThanSeccMinVol_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_EVReqVolLessThanSeccMinVol_Error_Bit);
		}
		break;
	case CCS2_EVReqVolMoreThanSeccMaxVol_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_EVReqVolMoreThanSeccMaxVol_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_EVReqVolMoreThanSeccMaxVol_Error_Bit);
		}
		break;
	case CCS2_EVReqVolMoreThanEVMaxVol_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_EVReqVolMoreThanEVMaxVol_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_EVReqVolMoreThanEVMaxVol_Error_Bit);
		}
		break;
	case CCS2_EVReqCurMoreThanEVMaxCur_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_EVReqCurMoreThanEVMaxCur_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_EVReqCurMoreThanEVMaxCur_Error_Bit);
		}
		break;
	case CCS2_SessionUNKNOW_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_SessionUNKNOW_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_SessionUNKNOW_Error_Bit);
		}
		break;
	case CCS2_CP_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_CP_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_CP_Error_Bit);
			my_printf(USER_INFO, "unError_code = %d flag = %lld\n", unError_code, g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag);
		}
		break;
	case CCS2_NO_DC_EVPowerDeliveryParameter_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_NO_DC_EVPowerDeliveryParameter_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_NO_DC_EVPowerDeliveryParameter_Error_Bit);
		}
		break;
	case CCS2_CHARGING_PROFILE_INVALID_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_CHARGING_PROFILE_INVALID_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_CHARGING_PROFILE_INVALID_Error_Bit);
		}
		break;
	case CCS2_EVCloseSocket_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_EVCloseSocket_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_EVCloseSocket_Error_Bit);
		}
		break;
	case CCS2_Sequence_Timeout_SetupTimer_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_Sequence_Timeout_SetupTimer_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_Sequence_Timeout_SetupTimer_Error_Bit);
		}
		break;
	case CCS2_Sequence_Timeout_AppSession_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_Sequence_Timeout_AppSession_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_Sequence_Timeout_AppSession_Error_Bit);
		}
		break;
	case CCS2_CPState_Detection_Timeout_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_CPState_Detection_Timeout_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_CPState_Detection_Timeout_Error_Bit);
		}
		break;
	case CCS2_EV_BatteryTemperature_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_EV_BatteryTemperature_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_EV_BatteryTemperature_Error_Bit);
		}
		break;
	case CCS2_EV_ShiftPosition_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fourth.sItem.CCS2_EV_ShiftPosition_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_EV_ShiftPosition_Error_Bit);
		}
		break;
	case CCS2_EV_ConnectorLock_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fifth.sItem.CCS2_EV_ConnectorLock_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fifth.unWhole_flag, BIT64_STRUCT, CCS2_EV_ConnectorLock_Error_Bit);
		}
		break;
	case CCS2_EV_BatteryMalfunction_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fifth.sItem.CCS2_EV_BatteryMalfunction_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fifth.unWhole_flag, BIT64_STRUCT, CCS2_EV_BatteryMalfunction_Error_Bit);
		}
		break;
	case CCS2_EV_CurrentDifferential_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fifth.sItem.CCS2_EV_CurrentDifferential_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fifth.unWhole_flag, BIT64_STRUCT, CCS2_EV_CurrentDifferential_Error_Bit);
		}
		break;
	case CCS2_EV_VoltageOutOfRange_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fifth.sItem.CCS2_EV_VoltageOutOfRange_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fifth.unWhole_flag, BIT64_STRUCT, CCS2_EV_VoltageOutOfRange_Error_Bit);
		}
		break;
	case CCS2_EV_SystemIncompatable_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fifth.sItem.CCS2_EV_SystemIncompatable_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fifth.unWhole_flag, BIT64_STRUCT, CCS2_EV_SystemIncompatable_Error_Bit);
		}
		break;
	case CCS2_EV_UNKNOWN_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fifth.sItem.CCS2_EV_UNKNOWN_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fifth.unWhole_flag, BIT64_STRUCT, CCS2_EV_UNKNOWN_Error_Bit);
		}
		break;
	case CCS2_SERVICE_IDINVALID_Error:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fifth.sItem.CCS2_SERVICE_IDINVALID_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fifth.unWhole_flag, BIT64_STRUCT, CCS2_SERVICE_IDINVALID_Error_Bit);
		}
		break;
	case ComTimeOut_startcharge:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fifth.sItem.ComTimeOut_startcharge != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fifth.unWhole_flag, BIT64_STRUCT, ComTimeOut_startcharge_Bit);
		}
		break;
	case ComTimeOut_parametersync:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fifth.sItem.ComTimeOut_parametersync != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fifth.unWhole_flag, BIT64_STRUCT, ComTimeOut_parametersync_Bit);
		}
		break;
	case ComTimeOut_selftestcomplete:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fifth.sItem.ComTimeOut_selftestcomplete != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fifth.unWhole_flag, BIT64_STRUCT, ComTimeOut_selftestcomplete_Bit);
		}
		break;
	case ComTimeOut_chargingparameter:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fifth.sItem.ComTimeOut_chargingparameter != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fifth.unWhole_flag, BIT64_STRUCT, ComTimeOut_chargingparameter_Bit);
		}
		break;
	case ComTimeOut_stopcharging:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fifth.sItem.ComTimeOut_stopcharging != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fifth.unWhole_flag, BIT64_STRUCT, ComTimeOut_stopcharging_Bit);
		}
		break;
	case GQ_CAN_BSM_TIMEOUT_ERROR:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fifth.sItem.Gq_Can_Bsm_Timeout_Error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fifth.unWhole_flag, BIT64_STRUCT, GQ_CAN_BSM_TIMEOUT_ERROR_Bit);
		}
		break;
	default:
		if (g_sSecc_fault[ucGun_id].sSecc_fault_fifth.sItem.ComTimeOut_stopcharging != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_fifth.unWhole_flag, BIT64_STRUCT, ComTimeOut_stopcharging_Bit);
		}
		break;
	}
}

/**
 * @brief SECC ZCL故障码解析
 * @param unError_code 故障码
 * @param ucGun_id：枪id
 * @return
 */
void ZCLErrorCodeParseFunc(const U16 unError_code, const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	U16 unTemp = (U16)((unError_code >> 8U) & 0xFFU) | ((unError_code << 8U) & 0xFF00U);

#ifndef TEST_MODE
	//此时SECC在等待车端结束流程,此时故障原因为0，无效
	if (0U == unTemp)
	{
		//过滤掉0，否则会出现没有充电停止原因
		return;
	}
#endif

	switch (unTemp)
	{
	case CCUStop_Normal:
		g_sAS_Charger_data[ucGun_id].sCST.CF_Secc_BmsSuspended = (U8)TRUE;
		break;
	case Vehicle_RemainTimeis_0_Normal:
		g_sAS_Charger_data[ucGun_id].sCST.CF_Secc_BmsSuspended = (U8)TRUE;
		g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = EV_DISCON_MODE;
		break;
	case Vehicle_CharingDisabled_Normal:
		g_sAS_Charger_data[ucGun_id].sCST.CF_Secc_BmsSuspended = (U8)TRUE;
		g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = EV_DISCON_MODE;
		break;
	case Vehicle_ReqChangeto_0_Normal:
		g_sAS_Charger_data[ucGun_id].sCST.CF_Secc_BmsSuspended = (U8)TRUE;
		g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = EV_DISCON_MODE;
		break;
	case Vehicle_BeforeCharge_Normal:
		g_sAS_Charger_data[ucGun_id].sCST.CF_Secc_BmsSuspended = (U8)TRUE;
		g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = EV_DISCON_MODE;
		break;
	case CCS2_Trip_EVComplete_StopReq:
		g_sAS_Charger_data[ucGun_id].sCST.CF_Secc_BmsSuspended = (U8)TRUE;
		g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = EV_DISCON_MODE;
		break;
	case CCS2_Trip_SOCFull_StopReq:
		g_sAS_Charger_data[ucGun_id].sCST.CF_Secc_BmsSuspended = (U8)TRUE;
		g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = REACH_TOTAL_SOC_TARGET;
		break;
	default:
		JWTSeccFaultCodeParse(unTemp, ucGun_id);
		g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = (U8)FAULT_STOP_MODE;
		break;
	}
	//防止CCU充电中重启后接受到SECC的错误信息，导致下一次充电错误, 只有充电中才切换停止状态
	if (TRUE == CheckChargingStatus(ucGun_id))
	{
		g_sBms_ctrl[ucGun_id].sStage.sNow_status = STOPCHARGE;
	}

	return;
}

/**
 * @brief 解析BRO报文函数
 * @param sBSM_data 数据
 * @param ucGun_id：枪id
 * @return
 */
void AS_BroMessageProcess(const U8 ucBro_status, const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	static U32 uiNow_tick = 0;

	if (0xaaU == ucBro_status)
	{
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_AA].unRecv_cnt++;

		if (1U == (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_AA].unRecv_cnt))
		{
			uiNow_tick = xTaskGetTickCount();
			g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].uiStart_tick = uiNow_tick;
			g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_AA].uiStart_tick = uiNow_tick;
			my_printf(USER_INFO, "%s: receive BRO AA\n", (ucGun_id == GUN_A?"GUN_A":"GUN_B"));
		}
	}
	else
	{
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].unRecv_cnt++;

		if (1U == (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].unRecv_cnt))
		{
			uiNow_tick = xTaskGetTickCount();
			g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].uiStart_tick = uiNow_tick;
			my_printf(USER_INFO, "%s: receive BRO 00\n", (ucGun_id == GUN_A?"GUN_A":"GUN_B"));
		}
	}
}

/**
 * @brief 解析BCL报文函数
 * @param sBCL_data 数据
 * @param ucGun_id：枪id
 * @return
 */
void AS_BclMessageProcess(const AS_BCL_Data_t sBCL_data, const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	static U8 s_ucBcl_print_cnt[2] = {0};
	static BOOL s_Send_flag = FALSE;

	g_sGun_data[ucGun_id].sTemp.unEV_max_limit_vol = sBCL_data.unCR_Plc_EvMaxVoltLimit;
	g_sGun_data[ucGun_id].sTemp.ucPrecharge_status = sBCL_data.ucCR_Plc_PreCharge_Stauts;

	//记录之前的需求电流
	U16 unTemp_cur = g_sGun_data[ucGun_id].sTemp.unEV_target_cur;

	U16 unActual_current = sBCL_data.unCR_Plc_EvTargetCurrent & 0x7FFFU;
	if (0U == (sBCL_data.unCR_Plc_EvTargetCurrent & 0x8000U))
	{
	    g_sGun_data[ucGun_id].sTemp.unEV_target_cur = 4000U - unActual_current;
	}
	else
	{
	    g_sGun_data[ucGun_id].sTemp.unEV_target_cur = 4000U + unActual_current;
	}

	//触发式上传需求电流，增加响应速度
	if (TRUE == my_unsigned_abs(g_sGun_data[ucGun_id].sTemp.unEV_target_cur, unTemp_cur, TARGET_CUR_DIFF))
	{
		my_printf(USER_INFO, "%s receive BCL target CUR change: %d-->%dA\n", (ucGun_id == GUN_A?"GUN_A":"GUN_B"),
				unTemp_cur/10U, g_sGun_data[ucGun_id].sTemp.unEV_target_cur/10U);

		s_Send_flag = TRUE;
	}

	U16 unTemp_vol = g_sGun_data[ucGun_id].sTemp.unEV_target_vol;
	//需求电压小于最大允许电压
	if (sBCL_data.unCR_Plc_EvTargetVolt < (g_sStorage_data.sPublic_data[ucGun_id].unGun_max_vol*10U))
	{
		g_sGun_data[ucGun_id].sTemp.unEV_target_vol = sBCL_data.unCR_Plc_EvTargetVolt;
	}
	else
	{
		g_sGun_data[ucGun_id].sTemp.unEV_target_vol = (g_sStorage_data.sPublic_data[ucGun_id].unGun_max_vol*10U);
	}

	//触发式上传需求电压，增加响应速度
	if (TRUE == my_unsigned_abs(sBCL_data.unCR_Plc_EvTargetVolt, unTemp_vol, TARGET_VOL_DIFF))
	{
		my_printf(USER_INFO, "%s receive BCL target VOL change: %d-->%dV limit VOL = %dV\n", (ucGun_id == GUN_A?"GUN_A":"GUN_B"),
				unTemp_vol/10U, sBCL_data.unCR_Plc_EvTargetVolt/10U, g_sGun_data[ucGun_id].sTemp.unEV_max_limit_vol/10U);

		s_Send_flag = TRUE;
	}

	//检测到设定压差或流差，立即上传需求
	if (s_Send_flag == TRUE)
	{
		TcpSendControl(&g_sCMD_msg_control.sCharging_control.bCharging_timeup_flag);
		s_Send_flag = FALSE;
	}

	if (1U == (s_ucBcl_print_cnt[ucGun_id] / 200U))
	{
		s_ucBcl_print_cnt[ucGun_id] = 0;
		my_printf(USER_INFO, "%s receive BCL: charge_type = %s target VOL = %dV cur = %dA output VOL = %dV cur = %dA\n", (ucGun_id == GUN_A?"GUN_A":"GUN_B"),
				(sBCL_data.ucCR_Plc_ChargingMode==1U)?"CV":"CC", g_sGun_data[ucGun_id].sTemp.unEV_target_vol/10U,
				g_sGun_data[ucGun_id].sTemp.unEV_target_cur/10U, g_sGun_data[ucGun_id].uiOutput_vol/100U, g_sGun_data[ucGun_id].uiOutput_cur/100U);
	}
	g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].unRecv_cnt++;
	s_ucBcl_print_cnt[ucGun_id]++;
}

/**
 * @brief 解析BSM报文函数
 * @param ucGun_id：枪id
 * @return
 */
void AS_BsmMessageProcess(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	if ((U8)FALSE == g_sAS_Bms_data[ucGun_id].sBSM.CF_Plc_EvReady)
	{
		if (TRUE == g_sGun_data[ucGun_id].sTemp.bAllow_charge_flag)
		{
			g_sAS_Charger_data[ucGun_id].stop_start_tick = xTaskGetTickCount();
			g_sGun_data[ucGun_id].sTemp.bAllow_charge_flag = FALSE;
		}
		else
		{
			//充电暂停时间不超过10分钟
			if ((xTaskGetTickCount() - g_sAS_Charger_data[ucGun_id].stop_start_tick) > (10U*60U*1000UL))
			{
				g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].unRecv_cnt = 0;
				g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].uiStart_tick = xTaskGetTickCount();
				g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].uiStart_tick = xTaskGetTickCount();
				g_sBms_ctrl[ucGun_id].sStage.sNow_status = STOPCHARGE;
			}
		}
		my_printf(USER_INFO, "%s Charging is not allowed\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
	}
	else
	{
		if (FALSE == g_sGun_data[ucGun_id].sTemp.bAllow_charge_flag)
		{
			g_sGun_data[ucGun_id].sTemp.bAllow_charge_flag = TRUE;
		}
	}
}

/**
 * @brief 解析SECC版本号
 * @param sBHM_data 数据
 * @param ucGun_id：枪id
 * @return
 */
void SECCVersionParse(const AS_BHM_Data_t sBHM_data, const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	if (FALSE == g_sAS_Charger_data[ucGun_id].bVersion_flag)
	{
		U8 ucTemp[16] = {0};
		(void)snprintf(ucTemp, 16, "%02x%02x%02x%02x", sBHM_data.ucSecc_type, sBHM_data.ucVersion_year,
				sBHM_data.ucVersion_month, sBHM_data.ucVersion_day);
		(void)memcpy(g_sGun_data[ucGun_id].ucSecc_version, ucTemp, strlen(ucTemp));
		g_sAS_Charger_data[ucGun_id].bVersion_flag = TRUE;
	}
}

/**
 * @brief 解析SECC MAC
 * @param sBRM_data 数据
 * @param ucGun_id：枪id
 * @return
 */
void SECCMacParse(const AS_BRM_Data_t sBRM_data, const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return ;
	}

	U8 ucTemp[17] = {0};
	(void)snprintf(ucTemp, 18, "%02x:%02x:%02x:%02x:%02x:%02x", sBRM_data.CR_Plc_EvCCID[0], sBRM_data.CR_Plc_EvCCID[1], sBRM_data.CR_Plc_EvCCID[2],
			sBRM_data.CR_Plc_EvCCID[3], sBRM_data.CR_Plc_EvCCID[4], sBRM_data.CR_Plc_EvCCID[5]);

	(void)memcpy(g_sGun_data[ucGun_id].sTemp.ucVin, ucTemp, strlen(ucTemp));
}

void JWTCAN1Parse(const U32 uiCan_id, const U8* pucBuf)
{
	static U8 s_ucCp_status = 0xFF;
    U8 ucBuf[SEND_BMS_BYTE_NUM] = {0};

    (void)memcpy(ucBuf, pucBuf, sizeof(ucBuf));

    switch (uiCan_id)
    {
    case READ_BMS_BHM_PACK:
    	if (sizeof(AS_BHM_Data_t) <= SEND_BMS_BYTE_NUM)
    	{
			(void)memcpy(&g_sAS_Bms_data[GUN_A].sBHM, ucBuf, sizeof(AS_BHM_Data_t));
			if ((U8)CP_DUTY_ON == g_sAS_Bms_data[GUN_A].sBHM.CR_Plc_CpStatus)
			{
				g_sGun_data[GUN_A].ucConnect_status = (U8)PLUGGED_IN;
			}
			else
			{
				g_sGun_data[GUN_A].ucConnect_status = g_sAS_Bms_data[GUN_A].sBHM.CR_Plc_CpStatus;
			}
			//版本解析
			SECCVersionParse(g_sAS_Bms_data[GUN_A].sBHM, GUN_A);

			if (s_ucCp_status != g_sGun_data[GUN_A].ucConnect_status)
			{
				my_printf(USER_INFO, "GUN_A receive BHM current_status = %d last_status = %d\n", g_sGun_data[GUN_A].ucConnect_status, s_ucCp_status);
				s_ucCp_status = g_sGun_data[GUN_A].ucConnect_status;
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
			(void)memcpy(&g_sAS_Bms_data[GUN_A].sBRM, (AS_BRM_Data_t*)ucBuf, sizeof(AS_BRM_Data_t));

			SECCMacParse(g_sAS_Bms_data[GUN_A].sBRM, GUN_A);

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
				SendGunRecBCPRequestPack(GUN_A);
				RecvMultipackTimerTreat_A(REC_MUL_BCP, TRUE);
				s_bRec_BCP_flag = TRUE;
				s_bRec_BCS_flag = FALSE;
				my_printf(USER_INFO, "GUN_A receive BCP request message, len = %d pack_num = %d\n", g_ucRec_multipack_len[GUN_A][REC_MUL_BCP], g_ucRec_packnum[GUN_A][REC_MUL_BCP]);
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
            uiPgn = (((U32)ucBuf[5] << 16) | ((U32)ucBuf[6] << 8) | ucBuf[7]);
            SendGunConnectAbort(uiPgn, GUN_A);
        }
		break;
    case  READ_BMS_EB_PACK:
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
        	if (sizeof(Stop_Data_t) <= SEND_BMS_BYTE_NUM)
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

				(void)memcpy(&g_sJwt_stop_data[GUN_A].sBST, ucBuf, sizeof(Stop_Data_t));

				ZCLErrorCodeParseFunc(g_sJwt_stop_data[GUN_A].sBST.unZclErrorCode, GUN_A);

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
    	if (sizeof(Stop_Data_t) <= SEND_BMS_BYTE_NUM)
    	{
			if (1U == (g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BEM].unRecv_cnt%40U))
			{
				my_printf(USER_INFO, "GUN_A receive BEM message\n");
			}

			if (FALSE == CheckChargingStatus(GUN_A))
			{
				return;
			}
			(void)memcpy(&g_sJwt_stop_data[GUN_A].sBEM, ucBuf, sizeof(Stop_Data_t));

			ZCLErrorCodeParseFunc(g_sJwt_stop_data[GUN_A].sBEM.unZclErrorCode, GUN_A);

			g_sBms_ctrl[GUN_A].sRecv_bms_timeout[BEM].unRecv_cnt++;
    	}
		break;
	default:
		my_printf(USER_ERROR, "%s:%d uiCan_id error = %d\n", __FILE__, __LINE__, uiCan_id);
	    break;
    }
    return ;
}
