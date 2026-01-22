/*
 * charge_general.c
 *
 *  Created on: 2025年4月14日
 *      Author: qjwu
 */
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "hal_sys_IF.h"
#include "hal_can.h"
#include "fsl_flexcan.h"
#include "hal_ext_rtc.h"

#include "pos_IF.h"
#include "imd_IF.h"
#include "AS_charge_comm.h"
#include "AS_charge_parse.h"
#include "JWT_can_comm.h"
#include "RKN_can_comm.h"
#include "GB_27930_comm.h"
#include "tcp_client_IF.h"
#include "meter_IF.h"
#include "SignalManage.h"
#include "charge_process_IF.h"
#include "emergency_fault_IF.h"
#include "charge_general.h"
#include "cig_IF.h"

g_psCharge_Control_t *g_psCharge_control = NULL;
g_psCharge_Control_t *g_psCharge_control_A = NULL;
g_psCharge_Control_t *g_psCharge_control_B = NULL;
BOOL g_bStart_charge_success_flag[2] = {0};
U8 g_ucCF_Secc_StartPlc[2] = {PLC_WAIT_TO_START, PLC_WAIT_TO_START};
U8 g_ucSend_BCS_end_cnt[2] = {0};
__BSS(SRAM_OC) U8 g_ucRec_multipack_len[2][REC_MUL_MAX] = {0};
__BSS(SRAM_OC) U8 g_ucRec_packnum[2][REC_MUL_MAX] = {0};
TimerHandle_t g_sRec_BCS_timer[2] = {NULL, NULL};
TimerHandle_t g_sRec_BCP_timer[2] = {NULL, NULL};
__BSS(SRAM_OC) Bms_Ctrl_t g_sBms_ctrl[2] = {0};
__BSS(SRAM_OC) U32 g_uiErr_vol_tick[2] = {0};

static SemaphoreHandle_t g_sStart_charge_sem[2] = {NULL, NULL};
__BSS(SRAM_OC) static U32 s_uiOver_vol_timeout[2] = {0};
__BSS(SRAM_OC) static U32 s_uiUnder_vol_timeout[2] = {0};
__BSS(SRAM_OC) static U32 s_uiOver_cur_timeout[2] = {0};

/**
 * @brief 根据枪型+CCU信号，获取对应的厂家实例
 * @param gunType：枪型
 * @param ccuSigId：对应的CCU信号ID
 * @param gunName：日志中显示的枪名
 * @return
 */
static g_psCharge_Control_t* GetSeccModelByGun(S32 gunType, U32 ccuSigId, const char* gunName)
{
    S32 iSecc_flag = 0;
    g_psCharge_Control_t* pModel = NULL;

    if (gunType == GB)
    {
        pModel = GetGBChargeModel();
        my_printf(USER_INFO, "%s use charge model: GB/T\n", gunName);
    }
    else if (gunType == CCS1 || gunType == CCS2 || gunType == NACS)
    {
        (void)GetSigVal(ccuSigId, &iSecc_flag);
        switch (iSecc_flag)
        {
            case JWT_SECC_MODULE_FLAG:
                pModel = GetJwtSeccModel();
                my_printf(USER_INFO, "%s use SECC charge model: JWT\n", gunName);
                break;
            case RKN_SECC_MODULE_FLAG:
                pModel = GetRknSeccModel();
                my_printf(USER_INFO, "%s use SECC charge model: RKN\n", gunName);
                break;
            default:
                pModel = GetJwtSeccModel();
                my_printf(USER_INFO, "%s use default charge model: JWT\n", gunName);
                break;
        }
    }
    else
    {
        pModel = GetJwtSeccModel();
        my_printf(USER_INFO, "%s unknown gun type, use default model: JWT\n", gunName);
    }

    return pModel;
}

/**
 * @brief 兼容性函数，用于选择不同厂家的控制函数
 * @param
 * @return
 */
static void SeccModuleSelect(void)
{
    if (g_sStorage_data.sPublic_data[GUN_A].ucGun_type == g_sStorage_data.sPublic_data[GUN_B].ucGun_type)
    {
        g_psCharge_control = GetSeccModelByGun(g_sStorage_data.sPublic_data[GUN_A].ucGun_type, CCU_SET_SIG_ID_SECC_MODEL_A, "UNIFIED");
    }
    else
    {
        g_psCharge_control_A = GetSeccModelByGun(g_sStorage_data.sPublic_data[GUN_A].ucGun_type, CCU_SET_SIG_ID_SECC_MODEL_A, "GUN_A");
        g_psCharge_control_B = GetSeccModelByGun(g_sStorage_data.sPublic_data[GUN_B].ucGun_type, CCU_SET_SIG_ID_SECC_MODEL_B, "GUN_B");
    }
}

static void Send2SeccCHM(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	static const U8 s_ucCR_Secc_EvseTransferType[2] = {DC_EXTRNDED, DC_EXTRNDED};
	flexcan_frame_t sFrame = {0};

	sFrame.dataByte0 = AS_VERSION_MINOR;
	sFrame.dataByte1 = AS_VERSION_MINORL;
	sFrame.dataByte2 = AS_VERSION_MINORH;

	sFrame.dataByte3 = (U8)(((g_ucCF_Secc_StartPlc[ucGun_id]<<4)&0xF0U) | s_ucCR_Secc_EvseTransferType[ucGun_id]);

	sFrame.dataByte4 = (U8)(MAXPOWERLIMIT & 0xffU);
	sFrame.dataByte5 = (U8)(((U16)MAXPOWERLIMIT >> 8) & 0xffU);

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_CHM_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = AS_CHM_DATA_LEN;

	if ((ucGun_id == GUN_A))
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

static void Send2SeccCRM(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	flexcan_frame_t sFrame = {0};

	sFrame.dataByte0 = g_sAS_Charger_data[ucGun_id].ucCrm_status;

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_CRM_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = AS_CRM_DATA_LEN;

	if ((ucGun_id == GUN_A))
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}

    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRM].uiSend_cnt++;
    if (1U == g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRM].uiSend_cnt)
    {
        g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRM].uiStart_tick = xTaskGetTickCount();
    }
}

/**
 * @brief 用于将一个两位的十进制数转换为压缩BCD格式
 * @param ucData：原数据
 * @return 转换数据
 */
U8 DECtoBCD(const U32 uiData)
{
    // 检查输入是否合法
    if (uiData > 99U) {
        // 输入超过两位十进制数
        return 0;
    }

    // 获取十位和个位
    U8 ucTens = (U8)(uiData / 10U);
    U8 ucUnits = (U8)(uiData % 10U);

    // 将十位和个位转换为BCD
    U8 bcd = (ucTens << 4U) | ucUnits;

    return bcd;
}

static void Send2SeccCTS(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	flexcan_frame_t sFrame = {0};

	U32 year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;

	SYS_GetDate(&year, &month, &day, &hour, &minute, &second);

	if ((year+1900U) > 2021U)
    {
		sFrame.dataByte0 = DECtoBCD(second);
		sFrame.dataByte1 = DECtoBCD(minute);
		sFrame.dataByte2 = DECtoBCD(hour+8U);
		sFrame.dataByte3 = DECtoBCD(day);
		sFrame.dataByte4 = DECtoBCD(month);
		sFrame.dataByte5 = DECtoBCD((year+1900U)%100U);
		sFrame.dataByte6 = DECtoBCD((year+1900U)/100U);
    }
    else
    {
    	sFrame.dataByte0=0xFF;
    	sFrame.dataByte1=0xFF;
    	sFrame.dataByte2=0xFF;
    	sFrame.dataByte3=0xFF;
    	sFrame.dataByte4=0xFF;
    	sFrame.dataByte5=0xFF;
    	sFrame.dataByte6=0xFF;
    }

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_CTS_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = AS_CTS_DATA_LEN;

	if ((ucGun_id == GUN_A))
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}

}

static void Send2SeccCML(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	flexcan_frame_t sFrame = {0};

	U32 uiTemp_max_vol = g_sStorage_data.sPublic_data[ucGun_id].unGun_max_vol*10U;
	U32 uiTemp_min_vol = g_sStorage_data.sPublic_data[ucGun_id].unGun_min_vol*10U;
	U32 uiTemp_max_cur = g_sStorage_data.sPublic_data[ucGun_id].unGun_max_cur*10U;
	U32 uiTemp_min_cur = g_sStorage_data.sPublic_data[ucGun_id].unGun_min_cur*10U;

	sFrame.dataByte0 = (U8)(uiTemp_max_vol & 0xFFU);
	sFrame.dataByte1 = (U8)((uiTemp_max_vol >> 8) & 0xFFU);
	sFrame.dataByte2 = (U8)(uiTemp_min_vol & 0xFFU);
	sFrame.dataByte3 = (U8)((uiTemp_min_vol >> 8)&0xFFU);

	sFrame.dataByte4 = (U8)((4000U - uiTemp_max_cur) & 0xFFU);
	sFrame.dataByte5 = (U8)(((4000U - uiTemp_max_cur) >> 8) & 0xFFU);
	sFrame.dataByte6 = (U8)((4000U - uiTemp_min_cur) & 0xFFU);
	sFrame.dataByte7 = (U8)(((4000U - uiTemp_min_cur) >> 8) & 0xFFU);

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_CML_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = AS_CML_DATA_LEN;

	if ((ucGun_id == GUN_A))
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

static void Send2SeccCRO(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	flexcan_frame_t sFrame = {0};

	sFrame.dataByte0 = g_sAS_Charger_data[ucGun_id].ucCro_status;
	//SECC没有检测中这个状态
	if ((U8)IMD_CHECKING == g_sGun_data[ucGun_id].sTemp.ucImd_status)
	{
		sFrame.dataByte1 = IMD_CHECK_INVALID;
	}
	else
	{
		sFrame.dataByte1 = g_sGun_data[ucGun_id].sTemp.ucImd_status;
	}

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_CRO_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = AS_CRO_DATA_LEN;

	if ((ucGun_id == GUN_A))
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

static void Send2SeccCCS(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	CHECK_PTR_NULL_NO_RETURN(g_psMeter_data);

	static const U8 s_ucCF_Secc_EvseSuspended[2] = {PERMISSIBLE_CHARGE, PERMISSIBLE_CHARGE};
	static const U8 s_ucCR_Secc_EvseMaxCurr_IsUse[2] = {NO_USE_MAX_CURRENT, NO_USE_MAX_CURRENT};
	flexcan_frame_t sFrame = {0};

	U32 uiTemp_vol = g_psMeter_data->sPublic_data[ucGun_id].uiDc_vol/10U;
	U32 uiTemp_cur = g_psMeter_data->sPublic_data[ucGun_id].uiDc_cur/10U;
	U32 uiTemp_power = (g_sStorage_data.sPublic_data[ucGun_id].unGun_max_vol*g_sStorage_data.sPublic_data[ucGun_id].unGun_max_cur)/1000U;

	sFrame.dataByte0 = (U8)(uiTemp_vol & 0xFFU);
	sFrame.dataByte1 = (U8)((uiTemp_vol >> 8) & 0xFFU);
	sFrame.dataByte2 = (U8)((4000U - uiTemp_cur) & 0xFFU);
	sFrame.dataByte3 = (U8)(((4000U - uiTemp_cur) >> 8) & 0xFFU);
	sFrame.dataByte4 = (U8)(uiTemp_power & 0xFFU);
	sFrame.dataByte5 = (U8)((uiTemp_power >> 8) & 0xFFU);

	U8 ucImd_temp_status = 0;
	//SECC没有检测中这个状态
	if ((U8)IMD_CHECKING == g_sGun_data[ucGun_id].sTemp.ucImd_status)
	{
		ucImd_temp_status = IMD_CHECK_INVALID;
	}
	else
	{
		ucImd_temp_status = g_sGun_data[ucGun_id].sTemp.ucImd_status;
	}

	sFrame.dataByte6 = (U8)((s_ucCF_Secc_EvseSuspended[ucGun_id] & 0x03U) | ((ucImd_temp_status << 2) & 0x1cU) |
			((s_ucCR_Secc_EvseMaxCurr_IsUse[ucGun_id] << 5) & 0x20U) | (U8)((g_sStorage_data.sPublic_data[ucGun_id].unGun_max_cur & 0x3U) << 6));
	sFrame.dataByte7 = (U8)((g_sStorage_data.sPublic_data[ucGun_id].unGun_max_cur >> 2) & 0xffU);

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_CCS_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = AS_CCS_DATA_LEN;

	if ((ucGun_id == GUN_A))
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}

	return;
}

static void Send2SeccCST(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	flexcan_frame_t sFrame = {0};
	U8 ucBuf[SEND_BMS_BYTE_NUM] = {0};

    (void)memcpy(ucBuf, &g_sAS_Charger_data[ucGun_id].sCST,sizeof(CST_Data_t));

	sFrame.dataByte0 = ucBuf[0];
	sFrame.dataByte1 = ucBuf[1];
	sFrame.dataByte2 = ucBuf[2];
	sFrame.dataByte3 = ucBuf[3];

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_CST_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = AS_CST_DATA_LEN;

	if ((ucGun_id == GUN_A))
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}

}

static void Send2SeccCSD(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	CHECK_PTR_NULL_NO_RETURN(g_psMeter_data);

	flexcan_frame_t sFrame = {0};
	U32 uiTemp_vol = g_psMeter_data->sPublic_data[ucGun_id].uiDc_vol/10U;

	sFrame.dataByte2 = (U8)(uiTemp_vol & 0xffU);
	sFrame.dataByte3 = (U8)((uiTemp_vol >> 8U) & 0xffU);

	//SECC没有检测中这个状态
	if ((U8)IMD_CHECKING == g_sGun_data[ucGun_id].sTemp.ucImd_status)
	{
		sFrame.dataByte4 = IMD_CHECK_INVALID;
	}
	else
	{
		sFrame.dataByte4 = g_sGun_data[ucGun_id].sTemp.ucImd_status;
	}

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_CSD_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = AS_CSD_DATA_LEN;

	if ((ucGun_id == GUN_A))
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}

	return;
}

static void Send2SeccCEM(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	static CEM_Data_t s_sCEM_Data = {0};
	flexcan_frame_t sFrame = {0};
	U8 ucBuf[SEND_BMS_BYTE_NUM] = {0};

    if (TRUE == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRM].bTimeout_flag)
    {
    	s_sCEM_Data.sTimeout.brm = (U8)TRUE;
    }
    else if (TRUE == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCP].bTimeout_flag)
    {
    	s_sCEM_Data.sTimeout.bcp = (U8)TRUE;
    }
    else if ((TRUE == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].bTimeout_flag) || (TRUE == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_AA].bTimeout_flag))
    {
    	s_sCEM_Data.sTimeout.bro = (U8)TRUE;
    }
    else if (TRUE == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].bTimeout_flag)
    {
    	s_sCEM_Data.sTimeout.bcs = (U8)TRUE;
    }
    else if (TRUE == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].bTimeout_flag)
    {
    	s_sCEM_Data.sTimeout.bcl = (U8)TRUE;
    }
    else if (TRUE == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].bTimeout_flag)
    {
    	s_sCEM_Data.sTimeout.bst = (U8)TRUE;
    }
    else if (TRUE == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].bTimeout_flag)
    {
    	s_sCEM_Data.sTimeout.bsd = (U8)TRUE;
    }
    else
    {
    	//不处理
    }

    (void)memcpy(ucBuf, &s_sCEM_Data, sizeof(CEM_Data_t));

	sFrame.dataByte0 = ucBuf[0];
	sFrame.dataByte1 = ucBuf[1];
	sFrame.dataByte2 = ucBuf[2];
	sFrame.dataByte3 = ucBuf[3];

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_CEM_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = AS_CEM_DATA_LEN;

	if ((ucGun_id == GUN_A))
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}

    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CEM].uiSend_cnt++;
}

/**
 * @brief SECC报文发送控制函数
 * @param ucGun_id:枪id
 * @return
*/
void SeccMessageSend(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	static const Msg2bms_Info_t sAS_Msg2bms_info[CMAX] =
	{
	    {SEND_BMS_CHM_PACK,250,&Send2SeccCHM},
	    {SEND_BMS_CRM_PACK,250,&Send2SeccCRM},
	    {SEND_BMS_CTS_PACK,500,&Send2SeccCTS},
	    {SEND_BMS_CML_PACK,250,&Send2SeccCML},
	    {SEND_BMS_CRO_PACK,250,&Send2SeccCRO},
	    {SEND_BMS_CCS_PACK,50,&Send2SeccCCS},
	    {SEND_BMS_CST_PACK,10,&Send2SeccCST},
	    {SEND_BMS_CSD_PACK,250,&Send2SeccCSD},
	    {SEND_BMS_CEM_PACK,250,&Send2SeccCEM},
	};

	U32 uiNow_tick = xTaskGetTickCount();

	for (U8 i = 0; i < (sizeof(g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl)/sizeof(g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[0])); i++)
	{
			if ((TRUE == g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[i].bSend_flag) &&
					((uiNow_tick - g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[i].uiLast_send_tick) >= sAS_Msg2bms_info[i].unPeriod_time))
			{
				sAS_Msg2bms_info[i].pSend_fun(ucGun_id);
				g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[i].uiLast_send_tick = uiNow_tick;
//            		WDI_FEED_DOG();
			}
	}
}

/**
 * @brief 获取预充状态
 * @param ucGun_id：枪id
 * @return 预充状态
 */
U8 AS_GetGunPrechargeStatus(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return TRUE;
	}

	return g_sGun_data[ucGun_id].sTemp.ucPrecharge_status;
}

/**
 * @brief 获取枪状态
 * @param ucGun_id：枪id
 * @return 枪状态
 */
U8 GetGunStatus(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return FALSE;
	}

	return g_sGun_data[ucGun_id].ucConnect_status;
}

void CheckAnotherGunStatus(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	if (GUN_A == ucGun_id)
	{
		if ((U8)STA_START_CHARGE == g_sGun_data[GUN_B].ucGun_common_status)
		{
			vTaskDelay(2000);
		}
	}
	else
	{
		if ((U8)STA_START_CHARGE == g_sGun_data[GUN_A].ucGun_common_status)
		{
			vTaskDelay(2000);
		}
	}
}
/**
 * @brief 清除充电各类数据和状态
 * @param ucGun_id：枪id
 * @return
 */
void ClearChargingData(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	(void)memset(&g_sAS_Bms_data[ucGun_id], 0, sizeof(g_sAS_Bms_data[ucGun_id]));
	(void)memset(&g_sJwt_stop_data[ucGun_id], 0, sizeof(g_sJwt_stop_data[ucGun_id]));
	(void)memset(&g_sRkn_stop_data[ucGun_id], 0, sizeof(g_sRkn_stop_data[ucGun_id]));
	(void)memset(&g_sBms_ctrl[ucGun_id], 0, sizeof(g_sBms_ctrl[ucGun_id]));
	(void)memset(&g_sAS_Charger_data[ucGun_id], 0, sizeof(g_sAS_Charger_data[ucGun_id]));
	(void)memset(&g_sGun_data[ucGun_id].sTemp, 0, sizeof(g_sGun_data[ucGun_id].sTemp));
	(void)memset(&g_sGun_id_status, 0, sizeof(g_sGun_id_status));
}

//解析相关
void SendGunConnectAbort(U32 uiPgn, U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	flexcan_frame_t sFrame = {0};

	sFrame.dataByte0 = 255U;
	sFrame.dataByte1 = 0xFFU;
	sFrame.dataByte2 = 0xFFU;
	sFrame.dataByte3 = 0xFFU;
	sFrame.dataByte4 = 0xFFU;
	sFrame.dataByte5 = (U8)((uiPgn >> 16U) & 0xFFU);
	sFrame.dataByte6 = (U8)((uiPgn >> 8U) & 0xFFU);
	sFrame.dataByte7 = (U8)(uiPgn & 0xFFU);

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_EC_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = SEND_BMS_BYTE_NUM;

	if (ucGun_id == GUN_A)
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

void SendGunRecBRMRequestPack(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	flexcan_frame_t sFrame = {0};

    sFrame.dataByte0 = 0x11U;
    sFrame.dataByte1 = g_ucRec_packnum[ucGun_id][REC_MUL_BRM];
    sFrame.dataByte2 = 0x01U;
    sFrame.dataByte3 = 0xffU;
    sFrame.dataByte4 = 0xffU;
    sFrame.dataByte5 = 0U;
    sFrame.dataByte6 = 0x02U;
    sFrame.dataByte7 = 0U;

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_EC_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = SEND_BMS_BYTE_NUM;

	if (ucGun_id == GUN_A)
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

void SendGunRecBRMEndPack(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	flexcan_frame_t sFrame = {0};

	sFrame.dataByte0 = 0x13U;
	sFrame.dataByte1 = g_ucRec_multipack_len[ucGun_id][REC_MUL_BRM];
	sFrame.dataByte2 = (U8)((U16)g_ucRec_multipack_len[ucGun_id][REC_MUL_BRM]>>8U);
	sFrame.dataByte3 = g_ucRec_packnum[ucGun_id][REC_MUL_BRM];
	sFrame.dataByte4 = 0xffU;
	sFrame.dataByte5 = 0U;
	sFrame.dataByte6 = 0x02U;
	sFrame.dataByte7 = 0U;

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_EC_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = SEND_BMS_BYTE_NUM;

	if (ucGun_id == GUN_A)
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

void SendGunRecBCPRequestPack(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	flexcan_frame_t sFrame = {0};

	sFrame.dataByte0 = 0x11U;
	sFrame.dataByte1 = g_ucRec_packnum[ucGun_id][REC_MUL_BCP];
	sFrame.dataByte2 = 0x01U;
	sFrame.dataByte3 = 0xFFU;
	sFrame.dataByte4 = 0xFFU;
	sFrame.dataByte5 = 0U;
	sFrame.dataByte6 = 0x06U;
	sFrame.dataByte7 = 0U;

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_EC_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = SEND_BMS_BYTE_NUM;

	if (ucGun_id == GUN_A)
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

void SendGunRecBCPEndPack(const U8 ucGun_id)
{
	flexcan_frame_t sFrame = {0};

	sFrame.dataByte0 = 0x13U;
	sFrame.dataByte1 = g_ucRec_multipack_len[ucGun_id][REC_MUL_BCP];
	sFrame.dataByte2 = (U8)((U16)g_ucRec_multipack_len[ucGun_id][REC_MUL_BCP]>>8);
	sFrame.dataByte3 = g_ucRec_packnum[ucGun_id][REC_MUL_BCP];
	sFrame.dataByte4 = 0xFFU;
	sFrame.dataByte5 = 0U;
	sFrame.dataByte6 = 0x06U;
	sFrame.dataByte7 = 0U;

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_EC_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = SEND_BMS_BYTE_NUM;

	if (ucGun_id == GUN_A)
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

void SendGunRecBCSRequestPack(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	flexcan_frame_t sFrame = {0};

	sFrame.dataByte0 = 0x11U;
	sFrame.dataByte1 = g_ucRec_packnum[ucGun_id][REC_MUL_BCS];
	sFrame.dataByte2 = 0x01U;
	sFrame.dataByte3 = 0xFFU;
	sFrame.dataByte4 = 0xFFU;
	sFrame.dataByte5 = 0U;
	sFrame.dataByte6 = 0x11U;
	sFrame.dataByte7 = 0U;

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_EC_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = SEND_BMS_BYTE_NUM;

	if (ucGun_id == GUN_A)
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

void SendGunRecBCSEndPack(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	flexcan_frame_t sFrame = {0};

	sFrame.dataByte0 = 0x13U;
	sFrame.dataByte1 = g_ucRec_multipack_len[ucGun_id][REC_MUL_BCS];
	sFrame.dataByte2 = (U8)((U16)g_ucRec_multipack_len[ucGun_id][REC_MUL_BCS]>>8);
	sFrame.dataByte3 = g_ucRec_packnum[ucGun_id][REC_MUL_BCS];
	sFrame.dataByte4 = 0xFFU;
	sFrame.dataByte5 = 0U;
	sFrame.dataByte6 = 0x11U;
	sFrame.dataByte7 = 0U;

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_EC_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = SEND_BMS_BYTE_NUM;

	if (ucGun_id == GUN_A)
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

void SendGunRecBSPRequestPack(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	flexcan_frame_t sFrame = {0};

	sFrame.dataByte0 = 0x11U;
	sFrame.dataByte1 = g_ucRec_packnum[ucGun_id][REC_MUL_BSP];
	sFrame.dataByte2 = 0x01U;
	sFrame.dataByte3 = 0xFFU;
	sFrame.dataByte4 = 0xFFU;
	sFrame.dataByte5 = 0U;
	sFrame.dataByte6 = 0x15U;
	sFrame.dataByte7 = 0U;

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_EC_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = SEND_BMS_BYTE_NUM;

	if (ucGun_id == GUN_A)
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

void SendGunRecBSPEndPack(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	flexcan_frame_t sFrame = {0};

	sFrame.dataByte0 = 0x13U;
	sFrame.dataByte1 = g_ucRec_multipack_len[ucGun_id][REC_MUL_BSP];
	sFrame.dataByte2 = (U8)((U16)g_ucRec_multipack_len[ucGun_id][REC_MUL_BSP]>>8);
	sFrame.dataByte3 = g_ucRec_packnum[ucGun_id][REC_MUL_BSP];
	sFrame.dataByte4 = 0xFFU;
	sFrame.dataByte5 = 0U;
	sFrame.dataByte6 = 0x15U;
	sFrame.dataByte7 = 0U;

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_EC_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = SEND_BMS_BYTE_NUM;

	if (ucGun_id == GUN_A)
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

void SendGunRecBMVRequestPack(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	flexcan_frame_t sFrame = {0};

	sFrame.dataByte0 = 0x11U;
	sFrame.dataByte1 = g_ucRec_packnum[ucGun_id][REC_MUL_BMV];
	sFrame.dataByte2 = 0x01U;
	sFrame.dataByte3 = 0xFFU;
	sFrame.dataByte4 = 0xFFU;
	sFrame.dataByte5 = 0U;
	sFrame.dataByte6 = 0x16U;
	sFrame.dataByte7 = 0U;

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_EC_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = SEND_BMS_BYTE_NUM;

	if (ucGun_id == GUN_A)
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

void SendGunRecBMVEndPack(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	flexcan_frame_t sFrame = {0};

	sFrame.dataByte0 = 0x13U;
	sFrame.dataByte1 = g_ucRec_multipack_len[ucGun_id][REC_MUL_BMV];
	sFrame.dataByte2 = (U8)((U16)g_ucRec_multipack_len[ucGun_id][REC_MUL_BMV]>>8);
	sFrame.dataByte3 = g_ucRec_packnum[ucGun_id][REC_MUL_BMV];
	sFrame.dataByte4 = 0xFFU;
	sFrame.dataByte5 = 0U;
	sFrame.dataByte6 = 0x16U;
	sFrame.dataByte7 = 0U;

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_EC_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = SEND_BMS_BYTE_NUM;

	if (ucGun_id == GUN_A)
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

void SendGunRecBMTRequestPack(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	flexcan_frame_t sFrame = {0};

	sFrame.dataByte0 = 0x11U;
	sFrame.dataByte1 = g_ucRec_packnum[ucGun_id][REC_MUL_BMT];
	sFrame.dataByte2 = 0x01U;
	sFrame.dataByte3 = 0xFFU;
	sFrame.dataByte4 = 0xFFU;
	sFrame.dataByte5 = 0U;
	sFrame.dataByte6 = 0x17U;
	sFrame.dataByte7 = 0U;

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_EC_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = SEND_BMS_BYTE_NUM;

	if (ucGun_id == GUN_A)
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

void SendGunRecBMTEndPack(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	flexcan_frame_t sFrame = {0};

	sFrame.dataByte0 = 0x13U;
	sFrame.dataByte1 = g_ucRec_multipack_len[ucGun_id][REC_MUL_BMT];
	sFrame.dataByte2 = (U8)((U16)g_ucRec_multipack_len[ucGun_id][REC_MUL_BMT]>>8);
	sFrame.dataByte3 = g_ucRec_packnum[ucGun_id][REC_MUL_BMT];
	sFrame.dataByte4 = 0xFFU;
	sFrame.dataByte5 = 0U;
	sFrame.dataByte6 = 0x17U;
	sFrame.dataByte7 = 0U;

	sFrame.type = (U8)kFLEXCAN_FrameTypeData;
	sFrame.id = SEND_BMS_EC_PACK;
	sFrame.format = (U8)kFLEXCAN_FrameFormatExtend;
	sFrame.length = SEND_BMS_BYTE_NUM;

	if (ucGun_id == GUN_A)
	{
		(void)ChgACanSendFrame(&sFrame);
	}
	else
	{
		(void)ChgBCanSendFrame(&sFrame);
	}
}

/**
 * @brief 发送充电停止报文
 * @param ucGun_id:枪id
 * @return
 */
void SendStopChargeMsg(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	U16 unDelay_cnt = 0;

	//防止异常导致死循环，增加超时判定
	while (unDelay_cnt < 300U)
	{
		//兼容SECC发送停止报文时前面报文的停止原因为0，等待EVCC真实停止后发送真实停充原因，此时不能直接上传原因给PCU
		//因为此时停止原因为0，需等待SECC传入真实充电停止原因再上传PCU
		//桩端主动停止不受影响
		if (0U != g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type)
		{
			break;
		}
		vTaskDelay(10);
		unDelay_cnt++;
	}

	if (0U == g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type)
	{
		g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = (U8)FAULT_STOP_MODE;
	}

	g_sGun_id_status.ucFinish_control_id = g_sStorage_data.sPublic_data[ucGun_id].ucGun_id;
	//发送停止充电报文
	TcpSendControl(&g_sGun_id_status.bStop_charge_flag);
}

/**
 * @brief 释放信号量，启动充电
 * @param ucGun_id:枪id
 * @return
 */
void StartChargeControl(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	if (NULL != g_sStart_charge_sem[ucGun_id])
	{
		//启动交互
		(void)xSemaphoreGive(g_sStart_charge_sem[ucGun_id]);
		g_bStart_charge_flag[ucGun_id] = TRUE;
	}
}

/**
 * @brief 切换充电步骤，结束充电
 * @param ucGun_id:枪id
 * @return
 */
void StopChargeControl(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].unRecv_cnt = 0U;
    g_sBms_ctrl[ucGun_id].sStage.sNow_status = STOPCHARGE;
    //填充CST
    g_sGB_Charger_data[ucGun_id].sCST.sStop_reason.sItem.ucPerson_stop = (U8)TRUE;
    g_sAS_Charger_data[ucGun_id].sCST.CF_Secc_ArtificialSuspend = (U8)TRUE;
    my_printf(USER_INFO, "%s Execute stop charging control\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
}

/**
 * @brief 充电步骤切换显示
 * @param
 * @return
 */
BOOL BmsStageSwitchCheck(Com_Stage_t* psStage)
{
	CHECK_PTR_NULL(psStage);

	static const char* s_pcBms_stage_name[STAGEMAX]=
	{
	    "INIT",
	    "HANDSHAKE",
	    "CONFIGURE",
	    "CHARGING",
	    "STOPCHARGE",
	    "TIMEOUT"
	};

    if (psStage->sNow_status != psStage->sLast_status)
    {
        my_printf(USER_INFO, "BMS sStage switch form %s --> %s\n", s_pcBms_stage_name[psStage->sLast_status], s_pcBms_stage_name[psStage->sNow_status]);
        psStage->sLast_status = psStage->sNow_status;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief 充电继电器控制函数
 * @param sMod 继电器
 * @param bFlag 闭合和断开
 * @return
 */
void RelayControl_A(const RELAY_CONTROL_t sMode, BOOL ucFlag)
{
	switch (sMode)
	{
	case PRECHARGE_RELAY:
		if (CLOSE_CONTROL == ucFlag)
		{
			PRECHARGE_RELAY_A_CLOSE();
			g_sGun_data[GUN_A].bPrecharge_relay_status = TRUE;
			(void)SetSigVal(SIGNAL_STATUS_PRECHARGE_A_DO, (S32)TRUE);
		}
		else
		{
			PRECHARGE_RELAY_A_OPEN();
			g_sGun_data[GUN_A].bPrecharge_relay_status = FALSE;
			(void)SetSigVal(SIGNAL_STATUS_PRECHARGE_A_DO, (S32)FALSE);
		}
		break;
	case POSITIVE_RELAY:
		if (CLOSE_CONTROL == ucFlag)
		{
			POSITIVE_RELAY_A_CLOSE();
			g_sGun_data[GUN_A].bPositive_relay_status = TRUE;
			(void)SetSigVal(SIGNAL_STATUS_CONNECTOR_A_POSITIVE_DO, (S32)TRUE);
		}
		else
		{
			POSITIVE_RELAY_A_OPEN();
			g_sGun_data[GUN_A].bPositive_relay_status = FALSE;
			(void)SetSigVal(SIGNAL_STATUS_CONNECTOR_A_POSITIVE_DO, (S32)FALSE);
		}
		break;
	case NEGATIVE_RELAY:
		if (CLOSE_CONTROL == ucFlag)
		{
			NEGATIVE_RELAY_A_CLOSE();
			g_sGun_data[GUN_A].bNegative_relay_status = TRUE;
			(void)SetSigVal(SIGNAL_STATUS_CONNECTOR_A_NEGATIVE_DO, (S32)TRUE);
		}
		else
		{
			NEGATIVE_RELAY_A_OPEN();
			g_sGun_data[GUN_A].bNegative_relay_status = FALSE;
			(void)SetSigVal(SIGNAL_STATUS_CONNECTOR_A_NEGATIVE_DO, (S32)FALSE);
		}
		break;
	default:
		my_printf(USER_ERROR, "%s:%d GUN_A relay param control error\n", __FILE__, __LINE__);
		break;
	}
}

/**
 * @brief 结束充电继电器控制
 * @param
 * @return
 */
void StopRelayControl_A(void)
{
	//发送请求降功率报文
	SendStopChargeMsg(GUN_A);

	if (((g_sGun_data[GUN_A].uiOutput_cur/10U) < SAFE_CUR_LIMIT)
			&& ((0U == GET_POSITIVE_RELAY_A_FB_STATUS()) || (0U == GET_NEGATIVE_RELAY_A_FB_STATUS())))
	{
		//断开高压继电器
		RelayControl_A(POSITIVE_RELAY, OPEN_CONTROL);
		vTaskDelay(10);
		RelayControl_A(NEGATIVE_RELAY, OPEN_CONTROL);
	    vTaskDelay(10);
	    RelayControl_A(PRECHARGE_RELAY, OPEN_CONTROL);
	}
}

/**
 * @brief 充电继电器控制函数
 * @param sMode：继电器 bFlag：闭合和断开
 * @param bFlag：闭合和断开
 * @return
 */
void RelayControl_B(const RELAY_CONTROL_t sMode, U8 ucFlag)
{
	switch (sMode)
	{
	case PRECHARGE_RELAY:
		if (CLOSE_CONTROL == ucFlag)
		{
			PRECHARGE_RELAY_B_CLOSE();
			g_sGun_data[GUN_B].bPrecharge_relay_status = TRUE;
			(void)SetSigVal(SIGNAL_STATUS_PRECHARGE_B_DO, (S32)TRUE);
		}
		else
		{
			PRECHARGE_RELAY_B_OPEN();
			g_sGun_data[GUN_B].bPrecharge_relay_status = FALSE;
			(void)SetSigVal(SIGNAL_STATUS_PRECHARGE_B_DO, (S32)FALSE);
		}
		break;
	case POSITIVE_RELAY:
		if (CLOSE_CONTROL == ucFlag)
		{
			POSITIVE_RELAY_B_CLOSE();
			g_sGun_data[GUN_B].bPositive_relay_status = TRUE;
			(void)SetSigVal(SIGNAL_STATUS_CONNECTOR_B_POSITIVE_DO, (S32)TRUE);
		}
		else
		{
			POSITIVE_RELAY_B_OPEN();
			g_sGun_data[GUN_B].bPositive_relay_status = FALSE;
			(void)SetSigVal(SIGNAL_STATUS_CONNECTOR_B_POSITIVE_DO, (S32)FALSE);
		}
		break;
	case NEGATIVE_RELAY:
		if (CLOSE_CONTROL == ucFlag)
		{
			NEGATIVE_RELAY_B_CLOSE();
			g_sGun_data[GUN_B].bNegative_relay_status = TRUE;
			(void)SetSigVal(SIGNAL_STATUS_CONNECTOR_B_NEGATIVE_DO, (S32)TRUE);
		}
		else
		{
			NEGATIVE_RELAY_B_OPEN();
			g_sGun_data[GUN_B].bNegative_relay_status = FALSE;
			(void)SetSigVal(SIGNAL_STATUS_CONNECTOR_B_NEGATIVE_DO, (S32)FALSE);
		}
		break;
	default:
		my_printf(USER_ERROR, "%s:%d GUN_B relay param control error\n", __FILE__, __LINE__);
		break;
	}
}

/**
 * @brief 结束充电继电器控制
 * @param
 * @return
 */
void StopRelayControl_B(void)
{
	//发送请求降功率报文
	SendStopChargeMsg(GUN_B);

	if (((g_sGun_data[GUN_B].uiOutput_cur/10U) < SAFE_CUR_LIMIT)
			&& ((0U == GET_POSITIVE_RELAY_B_FB_STATUS()) || (0U == GET_NEGATIVE_RELAY_B_FB_STATUS())))
	{
		//断开高压继电器
		RelayControl_B(POSITIVE_RELAY, OPEN_CONTROL);
		vTaskDelay(10);
		RelayControl_B(NEGATIVE_RELAY, OPEN_CONTROL);
	}
}

/**
 * @brief 结束充电继电器控制
 * @param ucGun_id；枪id
 * @return
 */
static void StopChargeRelayControl(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	if (GUN_A == ucGun_id)
	{
		//断开高压继电器
		RelayControl_A(POSITIVE_RELAY, OPEN_CONTROL);
		vTaskDelay(10);
		RelayControl_A(NEGATIVE_RELAY, OPEN_CONTROL);
	}
	else
	{
		//断开高压继电器
		RelayControl_B(POSITIVE_RELAY, OPEN_CONTROL);
		vTaskDelay(10);
		RelayControl_B(NEGATIVE_RELAY, OPEN_CONTROL);
	}
}

/**
 * @brief: 预充开始继电器控制
 * @param: ucGun_id；枪id
 * @return
 */
void StartPrechargeRelayControl(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	if (GUN_A == ucGun_id)
	{
		//闭合主负继电器和预充继电器
		RelayControl_A(NEGATIVE_RELAY, CLOSE_CONTROL);
		vTaskDelay(10);
		RelayControl_A(PRECHARGE_RELAY, CLOSE_CONTROL);
	}
	else
	{
		//闭合主负继电器和预充继电器
		RelayControl_B(NEGATIVE_RELAY, CLOSE_CONTROL);
		vTaskDelay(10);
		RelayControl_B(PRECHARGE_RELAY, CLOSE_CONTROL);
	}
}

/**
 * @brief: 预充失败继电器控制
 * @param: ucGun_id；枪id
 * @return
 */
void PrechargeFailRelayControl(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	if (GUN_A == ucGun_id)
	{
		//断开继电器
		RelayControl_A(NEGATIVE_RELAY, OPEN_CONTROL);
		vTaskDelay(10);
		RelayControl_A(PRECHARGE_RELAY, OPEN_CONTROL);
	}
	else
	{
		//断开继电器
		RelayControl_B(NEGATIVE_RELAY, OPEN_CONTROL);
		vTaskDelay(10);
		RelayControl_B(PRECHARGE_RELAY, OPEN_CONTROL);
	}
}

/**
 * @brief: 预充完成继电器控制
 * @param: ucGun_id；枪id
 * @return
 */
void PrechargeFinishRelayControl(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	if (GUN_A == ucGun_id)
	{
		//闭合主回路正极继电器
		RelayControl_A(POSITIVE_RELAY, CLOSE_CONTROL);
		vTaskDelay(10);
		//断开预充继电器
		RelayControl_A(PRECHARGE_RELAY, OPEN_CONTROL);
	}
	else
	{
		//闭合主回路正极继电器
		RelayControl_B(POSITIVE_RELAY, CLOSE_CONTROL);
		vTaskDelay(10);
		//断开预充继电器
		RelayControl_B(PRECHARGE_RELAY, OPEN_CONTROL);
	}
}

/**
 * @brief: 开始充电继电器控制
 * @param: ucGun_id；枪id
 * @return
 */
static void StartChargeRelayControl(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	if (GUN_A == ucGun_id)
	{
		//闭合高压继电器
		RelayControl_A(NEGATIVE_RELAY, CLOSE_CONTROL);
		vTaskDelay(10);
		RelayControl_A(POSITIVE_RELAY, CLOSE_CONTROL);
	}
	else
	{
		//闭合高压继电器
		RelayControl_B(NEGATIVE_RELAY, CLOSE_CONTROL);
		vTaskDelay(10);
		RelayControl_B(POSITIVE_RELAY, CLOSE_CONTROL);
	}
}

/**
 * @brief 充电中故障检测
 * @param ucGun_id；枪id
 * @return pdFAIL：有故障  pdPASS：正常
 */
BOOL AS_CheckSysChargingErr(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return FALSE;
	}

	Alarm_Status_t eError_status = NORMAL_STATUS;

	eError_status = FaultCodeCheck(ucGun_id);

	if (ALARM_STATUS == eError_status)
	{
		my_printf(USER_ERROR, "%s:%d %s fault detected, stop charging\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");
        g_sBms_ctrl[ucGun_id].sStage.sNow_status = STOPCHARGE;
        g_sAS_Charger_data[ucGun_id].sCST.CF_Secc_FaultSpended = (U8)TRUE;

        return FALSE;
	}

	if ((U8)UNPLUGGED == GetGunStatus(ucGun_id))
    {
        my_printf(USER_ERROR, "%s:%d %s gun disconnected during charging\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");
        g_sBms_ctrl[ucGun_id].sStage.sNow_status = STOPCHARGE;
        g_sAS_Charger_data[ucGun_id].sCST.CF_Secc_FaultSpended = (U8)TRUE;
        return FALSE;
    }

	if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BEM].unRecv_cnt > 0U)
	{
        my_printf(USER_ERROR, "%s:%d %s BEM detected during charging\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");
        return FALSE;
	}

	if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].unRecv_cnt > 0U)
	{
        my_printf(USER_ERROR, "%s:%d %s BST detected during charging\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");
		g_sAS_Charger_data[ucGun_id].sCST.CF_Secc_BmsSuspended = (U8)TRUE;
        return FALSE;
	}

	if (g_sBms_ctrl[ucGun_id].sStage.sNow_status == STOPCHARGE)
	{
		my_printf(USER_ERROR, "%s:%d %s STOPCHARGE detected during charging\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");
		return FALSE;
	}

    return TRUE;
}

/**
 * @brief 充电中故障检测 过压
 * @param uiNow_tick:时间
 * @param ucGun_id：枪id
 * @return 是否有故障
 */
static BOOL BmsOverVolCheck(const U32 uiNow_tick, U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return TRUE;
	}

	U16 unTemp_limit_vol = g_sGun_data[ucGun_id].sTemp.unEV_max_limit_vol;
	U16 unTemp_target_vol = g_sGun_data[ucGun_id].sTemp.unEV_target_vol;
	U16 unTemp_imd_vol = g_psIMD_data->sPublic_data[ucGun_id].unBus_vol;
	U16 unTemp_meter_vol = (g_sGun_data[ucGun_id].uiOutput_vol / 10U);

	//取最小电压(需最终确定)
	U16 unMin_vol = (unTemp_imd_vol < unTemp_meter_vol) ? unTemp_imd_vol : unTemp_meter_vol;

    //判断是否过压
	if (unMin_vol > unTemp_limit_vol)
	{
		//过压比例
		U16 unDiff_vol = (unMin_vol - unTemp_limit_vol);
		U16 unOver_vol_ratio = (unDiff_vol * 100U) / unTemp_limit_vol;

		U16 unOver_vol_hold_time = 0U;
		U16 unTemp_vol = 0U;

		if (unTemp_target_vol >= unTemp_limit_vol)
		{
			//需求电压和限制电压相同，过压判定+10V
			unTemp_vol = unTemp_limit_vol + OVER_VOL_LIMIT + 10U;
		}
		else
		{
			//超过车端最大允许电压15V
			unTemp_vol = unTemp_limit_vol + OVER_VOL_LIMIT;
		}

		if ((unTemp_meter_vol > unTemp_vol)
			&& (unTemp_imd_vol > unTemp_vol))
		{
			unOver_vol_hold_time = FAULT_OVER_VOL_TIMEOUT;
		}
		else
		{
			//高于需求电压过压
//			if (unOver_vol_ratio >= FAULT_OVER_VOL_RATIO)
//			{
//				unOver_vol_hold_time = ABNORMAL_OVER_VOL_TIMEOUT;
//			}
//			else
			{
				//过压恢复
				if (0U != s_uiOver_vol_timeout[ucGun_id])
				{
					if (g_sGun_warn[ucGun_id].sRecoverable_warn.sItem.gun_over_vol != (U8)FALSE)
					{
						GunAlarmReset(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_GUN_OVER_VOL);
						my_printf(USER_INFO, "%s restore over VOL warning\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
					}

					s_uiOver_vol_timeout[ucGun_id] = 0U;
				}
				return FALSE;
			}
		}

		if (unOver_vol_hold_time > 0U)
		{
			if ((uiNow_tick - s_uiOver_vol_timeout[ucGun_id]) < ALARM_TRIGGER_DELAY)
			{
				return FALSE;
			}
			//过压告警
			if (0U == s_uiOver_vol_timeout[ucGun_id])
			{
				s_uiOver_vol_timeout[ucGun_id] = uiNow_tick;
				if (g_sGun_warn[ucGun_id].sRecoverable_warn.sItem.gun_over_vol != (U8)TRUE)
				{
					GunAlarmSet(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_GUN_OVER_VOL);
					my_printf(USER_ERROR, "%s:%d %s over VOL warning:imd_vol = %d meter_vol = %d bms_target = %d start_tick = %d\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B",
							unTemp_imd_vol/10U, unTemp_meter_vol/10U,
							unTemp_limit_vol/10U, s_uiOver_vol_timeout[ucGun_id]);
				}
			}
			else
			{
				//过压故障
				if ((uiNow_tick - s_uiOver_vol_timeout[ucGun_id]) > unOver_vol_hold_time)
				{
					//填充CST
					g_sGB_Charger_data[ucGun_id].sCST.sErr_reason.sItem.ucVol_err = TRUE;
					if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.gun_over_vol != (U8)TRUE)
					{
						GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_GUN_OVER_VOL);
						GunAlarmReset(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_GUN_OVER_VOL);
						my_printf(USER_ERROR, "%s:%d %s over VOL fault:imd_vol = %d meter_vol = %d bms_target = %d now_tick = %d\n", __FILE__, __LINE__,
								(ucGun_id==GUN_A)?"GUN_A":"GUN_B", unTemp_imd_vol/10U, unTemp_meter_vol/10U,
										unTemp_limit_vol/10U, uiNow_tick);
					}

					g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CCS].bSend_flag = FALSE;
					s_uiOver_vol_timeout[ucGun_id] = 0U;

					return TRUE;
				}
			}
		}
	}
	else
	{
		//过压恢复
		if (0U != s_uiOver_vol_timeout[ucGun_id])
		{
			if (g_sGun_warn[ucGun_id].sRecoverable_warn.sItem.gun_over_vol != (U8)FALSE)
			{
				GunAlarmReset(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_GUN_OVER_VOL);
				my_printf(USER_INFO, "%s restore over VOL warning\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
			}

			s_uiOver_vol_timeout[ucGun_id] = 0U;
		}
	}

	return FALSE;
}

/**
 * @brief 充电中故障检测 欠压
 * @param uiNow_tick:时间
 * @param ucGun_id：枪id
 * @return 是否有故障
 */
static BOOL BmsUnderVolCheck(const U32 uiNow_tick, U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return TRUE;
	}

	U16 unTemp_target_vol = g_sGun_data[ucGun_id].sTemp.unEV_target_vol;
	U16 unTemp_imd_vol = g_psIMD_data->sPublic_data[ucGun_id].unBus_vol;
	U16 unTemp_meter_vol = (g_sGun_data[ucGun_id].uiOutput_vol/10U);

	//判定是否欠压
	if ((unTemp_meter_vol < unTemp_target_vol) && (unTemp_imd_vol < unTemp_target_vol))
	{
		//取最小压差(需最终确定)
		U16 unDiff_vol = (unTemp_target_vol - unTemp_imd_vol) < (unTemp_target_vol - unTemp_meter_vol)
				?(unTemp_target_vol - unTemp_imd_vol):(unTemp_target_vol - unTemp_meter_vol);

		//欠压比例
		U16 unUnder_vol_ratio = (unDiff_vol * 100U) / unTemp_target_vol;
		//故障处理时间
		U16 unUnder_vol_hold_time = 0U;

		//欠压
		if (unUnder_vol_ratio >= FAULT_UNDER_VOL_RATIO)
		{
			unUnder_vol_hold_time = FAULT_UNDER_VOL_TIMEOUT;
		}
		else
		{
			//恢复
			if (0U != s_uiUnder_vol_timeout[ucGun_id])
			{
				if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.gun_under_vol != (U8)FALSE)
				{
					GunAlarmReset(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_GUN_UNDER_VOL);
					my_printf(USER_INFO, "%s restore under VOL fault\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
				}

				s_uiUnder_vol_timeout[ucGun_id] = 0U;
			}
			return FALSE;
		}

		if (unUnder_vol_hold_time > 0U)
		{
			if ((uiNow_tick - s_uiUnder_vol_timeout[ucGun_id]) < ALARM_TRIGGER_DELAY)
			{
				return FALSE;
			}

			if (0U == s_uiUnder_vol_timeout[ucGun_id])
			{
				s_uiUnder_vol_timeout[ucGun_id] = uiNow_tick;

				my_printf(USER_ERROR, "%s:%d %s under VOL warning: meter_vol = %d imd_vol = %d target_vol = %d start_tick = %d\n", __FILE__, __LINE__,
						(ucGun_id==GUN_A)?"GUN_A":"GUN_B", unTemp_meter_vol/10U, unTemp_imd_vol/10U,
								unTemp_target_vol/10U, s_uiUnder_vol_timeout[ucGun_id]);
			}
			else
			{
				//欠压故障
				if ((uiNow_tick - s_uiUnder_vol_timeout[ucGun_id]) > unUnder_vol_hold_time)
				{
					//填充CST
					g_sGB_Charger_data[ucGun_id].sCST.sErr_reason.sItem.ucVol_err = TRUE;
					if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.gun_under_vol != (U8)TRUE)
					{
						GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_GUN_UNDER_VOL);
						my_printf(USER_ERROR, "%s:%d %s under VOL fault: meter_vol = %d imd_vol = %d target_vol = %d start_tick = %d now_tick = %d\n",
								__FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B", unTemp_meter_vol/10U, unTemp_imd_vol/10U,
										unTemp_target_vol/10U, s_uiUnder_vol_timeout[ucGun_id], uiNow_tick);
					}

					s_uiUnder_vol_timeout[ucGun_id] = 0U;

					return TRUE;
				}
			}
		}
	}
	else
	{
		//恢复
		if (0U != s_uiUnder_vol_timeout[ucGun_id])
		{
			if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.gun_under_vol != (U8)FALSE)
			{
				GunAlarmReset(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_GUN_UNDER_VOL);
				my_printf(USER_INFO, "%s restore under VOL fault\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
			}

			s_uiUnder_vol_timeout[ucGun_id] = 0U;
		}
	}

	return FALSE;
}

/**
 * @brief 充电中故障检测 过流
 * @param uiNow_tick:时间
 * @param ucGun_id：枪id
 * @return 是否有故障
 */
static BOOL BmsOverCurCheck(const U32 uiNow_tick, U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return TRUE;
	}

    U16 unTarget_cur = g_sGun_data[ucGun_id].sTemp.unEV_target_cur;
    //目标电流不为0
    if (0U != unTarget_cur)
    {
		U16 unMeter_cur = g_psMeter_data->sPublic_data[ucGun_id].uiDc_cur/10U;

		//实际电流差大于3A（防止停止充电的时候需求为0，模块有残留电流导致过流）
		if ((unMeter_cur > unTarget_cur) && ((unMeter_cur - unTarget_cur) > OVER_CUR_DIFF))
		{
			//故障处理时间
			U16 unOver_cur_hold_time = 0U;
			//严重过流大于需求电流1.3倍
			if ((unMeter_cur *100U) > (unTarget_cur*FAULT_OVER_CUR_LIMIT))
			{
				unOver_cur_hold_time = FAULT_OVER_CUR_TIMEOUT;
			}
			else if ((unMeter_cur *100U) > (unTarget_cur*OVER_CUR_LIMIT))
			{
				//普通过流大于需求电流1.1倍
				unOver_cur_hold_time = OVER_CUR_TIMEOUT;
			}
			else
			{
			    //过流恢复
				if (0U != s_uiOver_cur_timeout[ucGun_id])
				{
					if (g_sGun_warn[ucGun_id].sRecoverable_warn.sItem.gun_over_cur != (U8)FALSE)
					{
						GunAlarmReset(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_GUN_OVER_CUR);
						my_printf(USER_INFO, "%s restore charge over cur warning\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
					}

					s_uiOver_cur_timeout[ucGun_id] = 0U;
				}
				return FALSE;
			}

			if (unOver_cur_hold_time > 0U)
			{
				//告警滤波
				if ((uiNow_tick - s_uiOver_cur_timeout[ucGun_id]) < ALARM_TRIGGER_DELAY)
				{
					return FALSE;
				}
				//过流告警
				if (0U == s_uiOver_cur_timeout[ucGun_id])
				{
					s_uiOver_cur_timeout[ucGun_id] = uiNow_tick;
					if (g_sGun_warn[ucGun_id].sRecoverable_warn.sItem.gun_over_cur != (U8)TRUE)
					{
						GunAlarmSet(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_GUN_OVER_CUR);
						my_printf(USER_ERROR, "%s:%d %s over cur warning, meter_cur = %dA bms_target = %dA start_tick = %d\n", __FILE__, __LINE__,
								(ucGun_id==GUN_A)?"GUN_A":"GUN_B", unMeter_cur/10U, unTarget_cur/10U,
								s_uiOver_cur_timeout[ucGun_id]);
					}
				}
				//过流故障
				if ((uiNow_tick - s_uiOver_cur_timeout[ucGun_id]) > unOver_cur_hold_time)
				{
					//填充CST
					g_sGB_Charger_data[ucGun_id].sCST.sErr_reason.sItem.ucOver_cur = TRUE;
					if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.gun_over_cur != (U8)TRUE)
					{
						GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_GUN_OVER_CUR);
						GunAlarmReset(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_GUN_OVER_CUR);
						my_printf(USER_ERROR, "%s:%d %s over cur fault, meter_cur = %dA bms_target = %dA now_tick = %d\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B",
								unMeter_cur/10U, unTarget_cur/10U, uiNow_tick);
					}

					g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CCS].bSend_flag = FALSE;
					s_uiOver_cur_timeout[ucGun_id] = 0U;

					return TRUE;
				}
			}
		}
	    else
	    {
			//过流恢复
			if (0U != s_uiOver_cur_timeout[ucGun_id])
			{
				if (g_sGun_warn[ucGun_id].sRecoverable_warn.sItem.gun_over_cur != (U8)FALSE)
				{
					GunAlarmReset(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_GUN_OVER_CUR);
					my_printf(USER_INFO, "%s restore charge over cur warning\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
				}

				s_uiOver_cur_timeout[ucGun_id] = 0U;
			}
	    }
    }

	return FALSE;
}

/**
 * @brief 充电中故障检测 过压/欠压/过流
 * @param uiNow_tick:时间
 * @param ucGun_id：枪id
 * @return 是否有故障
 */
BOOL BmsChargingAlarmCheck(const U32 uiNow_tick, const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return TRUE;
	}

    return BmsOverVolCheck(uiNow_tick, ucGun_id)   || //过压
           BmsUnderVolCheck(uiNow_tick, ucGun_id) || //欠压
           BmsOverCurCheck(uiNow_tick, ucGun_id); //过流
}

/**
 * @brief 充电握手交互
 * @param ucGun_id:枪id
 * @return 下一步操作：配置/故障/超时
 */
Com_Stage_Def_t AS_BmsHandshakeTreat(const U8 ucGun_id)
{
    U32 uiNow_tick = 0;
    U8 handshake_step = HANDSHAKE_BHM;

    //状态变更，发送充电实时数据上传报文
    g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = HANDSHAKE_BHM;
    TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
    my_printf(USER_INFO, "%s CCU request SECC start charge\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");

    while(1)
    {
        uiNow_tick = xTaskGetTickCount();
        //查错，查异常
        if (TRUE != AS_CheckSysChargingErr(ucGun_id))
        {
    		return STOPCHARGE;
        }
        switch(handshake_step)
        {
            case HANDSHAKE_BHM:
                if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BHM].unRecv_cnt > 0U)
                {
                    if (FALSE == g_sAS_Bms_data[ucGun_id].sBHM.ucCR_Plc_Aagvalue)
                    {
                    	if ((U8)PLUGGED_IN == GetGunStatus(ucGun_id))
                    	{
                    		//充电桩更改标志位，请求启动
                    		g_ucCF_Secc_StartPlc[ucGun_id] = PLC_START_CHARGING_REQ;
                    	}
                    }
                    else//收到SECC不为0的信号
                    {
                        g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CHM].bSend_flag = FALSE;
                        //恢复标志位
                        g_ucCF_Secc_StartPlc[ucGun_id] = PLC_WAIT_TO_START;
                        g_sAS_Charger_data[ucGun_id].ucCrm_status = 0U;
                        g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRM].bSend_flag = TRUE;
						my_printf(USER_INFO, "%s stop send CHM, start send CRM 00\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
                        handshake_step = HANDSHAKE_BRM;
                        //状态变更，发送充电实时数据上传报文
                        g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = HANDSHAKE_BRM;
                        TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
                    }
                }
                break;
            case HANDSHAKE_BRM:
                if ((0U == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRM].unRecv_cnt)
                    && (0U != (uiNow_tick-g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRM].uiStart_tick))
                    && ((uiNow_tick-g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRM].uiStart_tick) > AS_BRM_TIMEOUT_TICK))
                {
                    my_printf(USER_ERROR, "%s:%d %s BRM message timeout start_tick = %d now = %d\n", __FILE__, __LINE__,
                    		(ucGun_id==GUN_A)?"GUN_A":"GUN_B", uiNow_tick-g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRM].uiStart_tick, uiNow_tick);
    	    		if (g_sSecc_fault[ucGun_id].sSecc_fault_second.sItem.gq_can_brm_timeout_error != (U8)TRUE)
    	    		{
    	    			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_BRM_TIMEOUT_ERROR);
    	    		}

            		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRM].bSend_flag = FALSE;
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRM].bTimeout_flag = TRUE;

                    return TIMEOUT;
                }
                else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRM].unRecv_cnt > 0U)
                {
                    g_sAS_Charger_data[ucGun_id].ucCrm_status = 0xAA;
                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRM].bSend_flag = TRUE;
					my_printf(USER_INFO, "%s stop send CRM 00 start send CRM AA\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCP].uiStart_tick = uiNow_tick;
                    handshake_step = CONFIG_BCP;
					 //状态变更，发送充电实时数据上传报文
					g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = CONFIG_BCP;
					TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
                }
                else
                {
                	//不处理
                }
                break;
            case CONFIG_BCP:
                if ((0U == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCP].unRecv_cnt)
                    && (0U != g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCP].uiStart_tick)
                    && ((uiNow_tick-g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCP].uiStart_tick) > AS_BCP_TIMEOUT_TICK))
                {
                    my_printf(USER_ERROR, "%s:%d %s BCP message timeout start_tick = %d now = %d\n", __FILE__, __LINE__,
                    		(ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCP].uiStart_tick, uiNow_tick);
    	    		if (g_sSecc_fault[ucGun_id].sSecc_fault_second.sItem.gq_can_bcp_timeout_error != (U8)TRUE)
    	    		{
    	    			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_BCP_TIMEOUT_ERROR);
    	    		}

                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCP].bTimeout_flag = TRUE;
                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRM].bSend_flag = FALSE;

                    return TIMEOUT;
                }
                else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCP].unRecv_cnt > 0U)
                {
                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRM].bSend_flag = FALSE;

                    return CONFIGURE;
                }
                else
                {
                	//不处理
                }
                break;
            default:
            	my_printf(USER_ERROR, "%s:%d %s handshake step error step = %d\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B", handshake_step);

            	return STOPCHARGE;
        }
        vTaskDelay(50);
    }
}

/**
 * @brief 绝缘检测升压
 * @param ucGun_id:枪id
 * @return 升压成功/失败
 */
static BOOL InsulationVolBoost(U8 ucGun_id, U32 uiStart_tick)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return FALSE;
	}

	static U32 uiOverVol_StartTick[2] = {0};
	U16 unEV_max_imd_vol = g_sGun_data[ucGun_id].sTemp.unEV_max_imd_vol;

	//告知PCU升压、电压为绝缘检测电压
	g_sGun_data[ucGun_id].sTemp.ucImd_control_flag = STEPUP_VOLTAGE;
	//绝缘电压小于桩最大输出电压
	if (unEV_max_imd_vol < (g_sStorage_data.sPublic_data[ucGun_id].unGun_max_vol*10U))
	{
		g_sGun_data[ucGun_id].sTemp.unEV_target_vol = unEV_max_imd_vol;
	}
	else
	{
		g_sGun_data[ucGun_id].sTemp.unEV_target_vol = (g_sStorage_data.sPublic_data[ucGun_id].unGun_max_vol*10U);
	}

	TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
	my_printf(USER_INFO, "%s start IMD VOL boost tick = %d target_vol = %dV\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B", uiStart_tick,
			unEV_max_imd_vol/10U);

	StartChargeRelayControl(ucGun_id);

	while(1)
	{
		U32 uiNow_tick = xTaskGetTickCount();
		//0.1V
		U16 unTemp_imd_vol = g_psIMD_data->sPublic_data[ucGun_id].unBus_vol;
		U16 unTemp_meter_vol = g_sGun_data[ucGun_id].uiOutput_vol/10U;
        //check err and fault
        if(TRUE != AS_CheckSysChargingErr(ucGun_id))
        {
        	g_psIMD_data->sPublic_data[ucGun_id].ucCheck_flag = (U8)IMD_DISABLE;
        	uiOverVol_StartTick[ucGun_id] = 0;

            return FALSE;
        }
		//绝缘监测升压超时
        if ((uiNow_tick - uiStart_tick) > IMD_TIMEOUT)
        {
            my_printf(USER_ERROR, "%s:%d %s IMD VOL boost timeout start_tick = %d now = %d VOL = %dV\n", __FILE__, __LINE__,
            		(ucGun_id==GUN_A)?"GUN_A":"GUN_B", uiStart_tick, uiNow_tick, unTemp_imd_vol/10U);
    		if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.insulation_timeout != (U8)TRUE)
    		{
    			GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_INSULATION_TIMEOUT);
    		}
    		uiOverVol_StartTick[ucGun_id] = 0;

            return FALSE;
        }

        if (0U != unEV_max_imd_vol)
        {
        	if ((unTemp_imd_vol > unEV_max_imd_vol) && (unTemp_meter_vol > unEV_max_imd_vol))
        	{
				if (((unTemp_imd_vol - unEV_max_imd_vol) > (3*IMD_VOL_DIFF))
						&& ((unTemp_meter_vol - unEV_max_imd_vol) > (3*IMD_VOL_DIFF)))
				{
					if (0U == uiOverVol_StartTick[ucGun_id])
					{
						uiOverVol_StartTick[ucGun_id] = uiNow_tick;
					}
					if ((uiNow_tick - uiOverVol_StartTick[ucGun_id]) > FAULT_OVER_VOL_TIMEOUT)
					{
						my_printf(USER_ERROR, "%s:%d %s IMD over VOL fault:imd_vol = %dV meter_vol = %dV bms_limit = %dV\n", __FILE__, __LINE__,
									(ucGun_id==GUN_A)?"GUN_A":"GUN_B", unTemp_imd_vol/10U, unTemp_meter_vol/10U, unEV_max_imd_vol/10U);
						if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.gun_over_vol != (U8)TRUE)
						{
							GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_GUN_OVER_VOL);
							GunAlarmReset(&g_sGun_warn[ucGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_GUN_OVER_VOL);
						}
						uiOverVol_StartTick[ucGun_id] = 0;

			            return FALSE;
					}
				}
        	}
        	if ((FALSE == my_unsigned_abs(unEV_max_imd_vol, unTemp_imd_vol, IMD_VOL_DIFF))
        			|| (FALSE == my_unsigned_abs(unEV_max_imd_vol, unTemp_meter_vol, IMD_VOL_DIFF)))
			{
        		uiOverVol_StartTick[ucGun_id] = 0;
				my_printf(USER_INFO, "%s VOL boost success,start IMD check: imd_vol = %dV target_vol = %dV tick = %d\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B",
						unTemp_imd_vol/10U, unEV_max_imd_vol/10U, uiNow_tick);
				break;
			}
			else
			{
				vTaskDelay(50);
			}
        }
        else
        {
        	uiOverVol_StartTick[ucGun_id] = 0;
        	my_printf(USER_ERROR, "%s:%d %s IMD target VOL = 0V\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B");

        	return FALSE;
        }
	}

	return TRUE;
}

/**
 * @brief 绝缘检测泄压
 * @param ucGun_id:枪id
 * @return 泄压成功/失败
 */
static BOOL InsulationPressureRelief(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return FALSE;
	}

	//关闭绝缘检测
	g_psIMD_data->sPublic_data[ucGun_id].ucCheck_flag = (U8)IMD_DISABLE;

	U32 uiXF_start_tick = xTaskGetTickCount();
	//告知PCU泄压
	g_sGun_data[ucGun_id].sTemp.ucImd_control_flag = STEPDOWN_VOLTAGE;
	g_sGun_data[ucGun_id].sTemp.unEV_target_vol = 0U;
	TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
	my_printf(USER_INFO, "%s receive IMD check complete, start Pressure relief tick = %d\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B", uiXF_start_tick);

	while (1)
	{
        //check err and fault
        if(TRUE != AS_CheckSysChargingErr(ucGun_id))
        {
        	g_psIMD_data->sPublic_data[ucGun_id].ucCheck_flag = (U8)IMD_DISABLE;

            return FALSE;
        }
		if ((g_psIMD_data->sPublic_data[ucGun_id].unBus_vol > SAFE_VOL_LIMIT)
			&& ((g_sGun_data[ucGun_id].uiOutput_vol/10U) > SAFE_VOL_LIMIT))
		{
			if ((xTaskGetTickCount() - uiXF_start_tick) > IMD_DISCHARGE_TIMEOUT)
			{
				my_printf(USER_ERROR, "%s:%d %s IMD Pressure relief timeout: tick = %d imd_vol = %dV meter_vol = %dV\n", __FILE__, __LINE__,
						(ucGun_id==GUN_A)?"GUN_A":"GUN_B", xTaskGetTickCount(), g_psIMD_data->sPublic_data[ucGun_id].unBus_vol/10U, g_sGun_data[ucGun_id].uiOutput_vol/100U);
	    		if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.imd_discharge_timeout != (U8)TRUE)
	    		{
	    			GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_IMD_DISCHARGE_TIMEOUT);
	    		}

				return FALSE;
			}
		}
		else
		{
			my_printf(USER_INFO, "%s IMD Pressure relief complete: tick = %d imd_vol = %dV meter_vol = %dV\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B", xTaskGetTickCount(),
					g_psIMD_data->sPublic_data[ucGun_id].unBus_vol/10U, g_sGun_data[ucGun_id].uiOutput_vol/100U);
			//恢复绝缘监测标志位
			g_sGun_data[ucGun_id].sTemp.ucImd_control_flag = DEFAULT;

			StopChargeRelayControl(ucGun_id);

			return TRUE;
		}
		vTaskDelay(50);
	}
}

/**
 * @brief 绝缘检测
 * @param ucGun_id:枪id
 * @return 绝缘检测是否成功
 */
BOOL BmsInsulationCheck(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return FALSE;
	}

	CHECK_PTR_NULL(g_psIMD_data);

    if (g_sGun_data[ucGun_id].sTemp.unEV_max_imd_vol < (g_sStorage_data.sPublic_data[ucGun_id].unGun_min_vol*10U))
    {
    	my_printf(USER_ERROR, "%s:%d %s IMD handshake VOL %d < DCDC minimum output VOL %d\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B",
    			g_sGun_data[ucGun_id].sTemp.unEV_max_imd_vol/10U, g_sStorage_data.sPublic_data[ucGun_id].unGun_min_vol);
		if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.imd_vol_less_than_dcdc_min_vol != (U8)TRUE)
		{
			GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_IMD_VOL_LESS_THAN_DCDC_MIN_VOL);
		}

		g_sGB_Charger_data[ucGun_id].sCST.sErr_reason.sItem.ucParam_dismatch = TRUE;

    	return FALSE;
    }

    if (g_sGun_data[ucGun_id].sTemp.unEV_max_imd_vol > (g_sStorage_data.sPublic_data[ucGun_id].unGun_max_vol*10U))
    {
    	my_printf(USER_ERROR, "%s:%d %s IMD handshake VOL %d > DCDC max output VOL %d \n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B",
    			g_sGun_data[ucGun_id].sTemp.unEV_max_imd_vol/10U, g_sStorage_data.sPublic_data[ucGun_id].unGun_max_vol);
    	g_sGun_data[ucGun_id].sTemp.unEV_max_imd_vol = g_sStorage_data.sPublic_data[ucGun_id].unGun_max_vol*10U;
    }

	U32 uiStart_tick = xTaskGetTickCount();

    //升压
    if (TRUE != InsulationVolBoost(ucGun_id, uiStart_tick))
    {
    	return FALSE;
    }

	//启动绝缘监测
	g_psIMD_data->sPublic_data[ucGun_id].ucCheck_flag = (U8)IMD_ENABLE;

	while ((xTaskGetTickCount() - uiStart_tick) < IMD_TIMEOUT)
    {
        //check err and fault
        if(TRUE != AS_CheckSysChargingErr(ucGun_id))
        {
        	g_psIMD_data->sPublic_data[ucGun_id].ucCheck_flag = (U8)IMD_DISABLE;

            return FALSE;
        }

        switch(g_sGun_data[ucGun_id].sTemp.ucImd_status)
        {
        case IMD_CHECK_INVALID:
        	break;
        case IMD_CHECKING:
        	break;
        case IMD_CHECK_WARNING:
            if (TRUE != InsulationPressureRelief(ucGun_id))
            {
            	return FALSE;
            }
            else
            {
            	return TRUE;
            }
        case IMD_CHECK_VALID:
            if (TRUE != InsulationPressureRelief(ucGun_id))
            {
            	return FALSE;
            }
            else
            {
            	return TRUE;
            }
        case IMD_CHECK_FAULT:
        	g_psIMD_data->sPublic_data[ucGun_id].ucCheck_flag = (U8)IMD_DISABLE;

            return FALSE;
        default:
			my_printf(USER_ERROR, "%s:%d %s IMD check status error:%d\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B",
					g_sGun_data[ucGun_id].sTemp.ucImd_status);
			g_psIMD_data->sPublic_data[ucGun_id].ucCheck_flag = (U8)IMD_DISABLE;

			return FALSE;
        }

		vTaskDelay(50);
	}

    my_printf(USER_ERROR, "%s:%d %s IMD check timeout start_tick = %d now_tick = %d\n", __FILE__, __LINE__,
    		(ucGun_id==GUN_A)?"GUN_A":"GUN_B", uiStart_tick, xTaskGetTickCount());
	if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.insulation_timeout != (U8)TRUE)
	{
		GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_INSULATION_TIMEOUT);
	}

    return FALSE;
}

/**
 * @brief 充电配置交互
 * @param ucGun_id:枪id
 * @return下一步操作：预充/故障/超时
 */
Com_Stage_Def_t AS_BmsConfigTreat(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return STOPCHARGE;
	}

    U32 uiNow_tick = 0;
    U8 ucConfig_step = CONFIG_BRO;
    U8 ucBcl_bcs_recv_flag = 0;

	//状态变更，发送充电实时数据上传报文
	g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = CONFIG_BRO;
	TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);

    while (1)
    {
        uiNow_tick = xTaskGetTickCount();

        if (TRUE != AS_CheckSysChargingErr(ucGun_id))
        {
            return STOPCHARGE;
        }

        switch(ucConfig_step)
        {
            case CONFIG_BRO:
                if ((0U == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].unRecv_cnt)
                    && (0U != g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].uiStart_tick)
                    && ((uiNow_tick-g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].uiStart_tick) > AS_BRO_00_TIMEOUT_TICK))
                {
                    my_printf(USER_ERROR, "%s:%d %s BRO 00 message timeout start_tick = %d now = %d\n", __FILE__, __LINE__,
                    		(ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].uiStart_tick, uiNow_tick);
    	    		if (g_sSecc_fault[ucGun_id].sSecc_fault_second.sItem.gq_can_bro_timeout_error != (U8)TRUE)
    	    		{
    	    			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_BRO_TIMEOUT_ERROR);
    	    		}

                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CTS].bSend_flag = FALSE;
                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CML].bSend_flag = FALSE;
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].bTimeout_flag = TRUE;

                    return TIMEOUT;
                }
                else if (0U != g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_00].unRecv_cnt)
                {
                   //
                }
                else
                {
                	//
                }
                if ((0U == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_AA].unRecv_cnt) &&
                  ((uiNow_tick-g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_AA].uiStart_tick) > AS_BRO_AA_TIMEOUT_TICK))
                {
                    my_printf(USER_ERROR, "%s:%d %s BRO AA message timeout\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
    	    		if (g_sSecc_fault[ucGun_id].sSecc_fault_second.sItem.gq_can_bro_timeout_error != (U8)TRUE)
    	    		{
    	    			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_BRO_TIMEOUT_ERROR);
    	    		}

                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CTS].bSend_flag = FALSE;
                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CML].bSend_flag = FALSE;
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_AA].bTimeout_flag = TRUE;

                    return TIMEOUT;
                }
                else
                {
                    if ((g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BRO_AA].unRecv_cnt) > 0U)
                    {
                        //停止
                        g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CTS].bSend_flag = FALSE;
                        g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CML].bSend_flag = FALSE;
                        g_sAS_Charger_data[ucGun_id].ucCro_status = 0;
                        g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRO].bSend_flag = TRUE;
                        ucConfig_step = CONFIG_ISO_CHECK;
                        my_printf(USER_INFO, "%s start send CRO 00\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
                    	//状态变更，发送充电实时数据上传报文
                    	g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = CONFIG_ISO_CHECK;
                    	TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
                    }
                }
                break;
            case CONFIG_ISO_CHECK:
            	//绝缘监测
                if (TRUE == BmsInsulationCheck(ucGun_id))
                {
                    g_sAS_Charger_data[ucGun_id].ucCro_status = 0xAA;
                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRO].bSend_flag = TRUE;
                    g_sGun_data[ucGun_id].sTemp.bAllow_charge_flag = TRUE;
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].uiStart_tick = xTaskGetTickCount();
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].uiStart_tick = xTaskGetTickCount();
                    ucConfig_step = CONFIG_PRECHARGE;
                	//状态变更，发送充电实时数据上传报文
                	g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = CONFIG_PRECHARGE;
                	TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
					my_printf(USER_INFO, "%s IMD check complete,start send CRO AA\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
                }
                else
                {
                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRO].bSend_flag = FALSE;

					return STOPCHARGE;
                }
                break;
            case CONFIG_PRECHARGE:
                if ((0U == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].unRecv_cnt)
                    && (0U != g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].uiStart_tick)
                    && ((uiNow_tick-g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].uiStart_tick) > AS_BCL_TIMEOUT_TICK))
                {
                    my_printf(USER_ERROR, "%s:%d %s BCL message timeout start_tick = %d now = %d\n", __FILE__, __LINE__,
                    		(ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].uiStart_tick, uiNow_tick);
    	    		if (g_sSecc_fault[ucGun_id].sSecc_fault_second.sItem.gq_can_bcl_timeout_error != (U8)TRUE)
    	    		{
    	    			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_BCL_TIMEOUT_ERROR);
    	    		}

                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRO].bSend_flag = FALSE;
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].bTimeout_flag = TRUE;

                    return TIMEOUT;
                }
                else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].unRecv_cnt > 0U)
                {
                	g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRO].bSend_flag = FALSE;
                	ucBcl_bcs_recv_flag |= 0x01U;
                	StartPrechargeRelayControl(ucGun_id);
                }
                else
                {
                	//
                }
                if ((0U == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].unRecv_cnt)
                    && (0U != g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].uiStart_tick)
                    && ((uiNow_tick-g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].uiStart_tick) > AS_BCS_TIMEOUT_TICK))
                {
                    my_printf(USER_ERROR, "%s:%d %s BCS message timeout start_tick = %d now = %d\n", __FILE__, __LINE__,
                    		(ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].uiStart_tick, uiNow_tick);
    	    		if (g_sSecc_fault[ucGun_id].sSecc_fault_second.sItem.gq_can_bcs_timeout_error != (U8)TRUE)
    	    		{
    	    			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_BCS_TIMEOUT_ERROR);
    	    		}

                    g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRO].bSend_flag = FALSE;
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].bTimeout_flag = TRUE;

                    return TIMEOUT;
                }
                else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].unRecv_cnt > 0U)
                {
                	g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CRO].bSend_flag = FALSE;
                	ucBcl_bcs_recv_flag |= 0x02U;
                }
                else
                {
                	//
                }

                if (0x03U == ucBcl_bcs_recv_flag)
                {
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].uiStart_tick = uiNow_tick;  //更新时间
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].uiStart_tick = uiNow_tick;
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].unRecv_cnt = 0U;
                    g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].unRecv_cnt = 0U;

                    return CHARGING;
                }
                break;
            default:
            	my_printf(USER_ERROR, "%s:%d %s configure type error :ucConfig_step = %d\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B", ucConfig_step);

            	return STOPCHARGE;
        }
        vTaskDelay(50);
    }
}

/**
 * @brief 预充失败处理函数
 * @param ucGun_id 枪id
 * @return
 */
void PrechargeFailProcess(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

    U32 uiOutputVol = g_sGun_data[ucGun_id].uiOutput_vol;
    U16 unImdVol = g_psIMD_data->sPublic_data[ucGun_id].unBus_vol;
    U16 unTargetVol = g_sGun_data[ucGun_id].sTemp.unEV_target_vol;

	//EV侧电压大于安全电压，预充继电器正常闭合
	if ((uiOutputVol/10U) > SAFE_VOL_LIMIT)
	{
		PrechargeFailRelayControl(ucGun_id);
		//预充超时
		if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.precharge_timeout != (U8)TRUE)
		{
			GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_PRECHARGE_TIMEOUT);
		}

		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CCS].bSend_flag = FALSE;
		my_printf(USER_ERROR, "%s:%d %s precharge timeout:tick = %d meter_vol = %dV imd_vol = %dV target_vol = %dV\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B", xTaskGetTickCount(),
				uiOutputVol/100U, unImdVol/10U, unTargetVol/10U);
	}
	//电压小于安全电压，预充继电器回路故障
	else
	{
		PrechargeFailRelayControl(ucGun_id);
		//预充回路故障
		if (g_sGun_fault[ucGun_id].sGeneral_fault.sItem.precharge_circuit != (U8)TRUE)
		{
			GunAlarmSet(&g_sGun_fault[ucGun_id].sGeneral_fault.uiWhole_flag, BIT32_STRUCT, FAULT_PRECHARGE_CIRCUIT);
		}

		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CCS].bSend_flag = FALSE;
		my_printf(USER_ERROR, "%s:%d %s precharge timeout:tick = %d meter_vol = %d imd_vol = %d target_vol = %d\n", __FILE__, __LINE__, (ucGun_id==GUN_A)?"GUN_A":"GUN_B", xTaskGetTickCount(),
				uiOutputVol/100U, unImdVol/10U, unTargetVol/10U);
	}

	return;
}

/**
 * @brief 预充
 * @param ucGun_id 枪id
 * @return 成功/失败
 */
BOOL AS_BmsPrechargeTreat(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return FALSE;
	}

	U32 uiPrecharge_start_tick = xTaskGetTickCount();

	while (1)
	{
		//检测到故障
        if (TRUE != AS_CheckSysChargingErr(ucGun_id))
        {
        	return FALSE;
        }

    	//预充未超时
    	if ((xTaskGetTickCount() - uiPrecharge_start_tick) < PRECHARGE_TIMEOUT)
    	{
    		//判定电压是否达到车端目标电压
    		U16 unTemp_vol = g_sGun_data[ucGun_id].sTemp.unEV_target_vol - AS_STANDARD_PRECHARGE_LIMIT_VOL;
            if ( ((g_sGun_data[ucGun_id].uiOutput_vol/10U) >= (U32)unTemp_vol)
            		|| (g_psIMD_data->sPublic_data[ucGun_id].unBus_vol >= unTemp_vol) )
            {
        		my_printf(USER_INFO, "%s precharge complete, wait SECC response tick = %d\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B", xTaskGetTickCount());

            	while (1)
            	{
        			//SECC反馈预充完成(1->0)
        			if (FALSE == g_sAS_Bms_data[ucGun_id].sBCL.ucCR_Plc_PreCharge_Stauts)
        			{
        				PrechargeFinishRelayControl(ucGun_id);
        				g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = CHARGING_BCL_BCS;
        				//状态变更，发送充电实时数据上传报文
        				TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
        				g_psIMD_data->sPublic_data[ucGun_id].ucCheck_flag = (U8)IMD_ENABLE;
        				my_printf(USER_INFO, "%s precharge complete:meter_vol = %dV IMD_VOL = %dV target_vol = %dV tick = %d\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B",
        						g_sGun_data[ucGun_id].uiOutput_vol/100U, g_psIMD_data->sPublic_data[ucGun_id].unBus_vol/10U, g_sGun_data[ucGun_id].sTemp.unEV_target_vol/10U, xTaskGetTickCount());

        				return TRUE;
        			}
        			else//兼容升压完成但是SECC未回复预充完成情况
        			{
        				//20s仍未回复完成
        				if ((xTaskGetTickCount() - uiPrecharge_start_tick) > (PRECHARGE_TIMEOUT+13000U))
        				{
        					my_printf(USER_INFO, "%s precharge complete but SECC response failed\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
        		    		PrechargeFailProcess(ucGun_id);

        		    		return FALSE;
        				}
        			}
        			vTaskDelay(20);
            	}
            }
    	}
    	else
    	{
    		PrechargeFailProcess(ucGun_id);

    		return FALSE;
    	}
		vTaskDelay(50);
	}
}

/**
 * @brief 充电中交互
 * @param ucGun_id 枪id
 * @return 超时/故障/继续充电
 */
Com_Stage_Def_t AS_BmsChargingTreat(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return STOPCHARGE;
	}

	U32 uiNow_tick = xTaskGetTickCount();

	if (TRUE != AS_CheckSysChargingErr(ucGun_id))
	{
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CCS].bSend_flag = FALSE;

		return STOPCHARGE;
	}

	if ((0U == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].unRecv_cnt) &&
		((uiNow_tick - g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].uiStart_tick) > AS_BCL_TIMEOUT_TICK))
	{
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].bTimeout_flag = TRUE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CCS].bSend_flag = FALSE;

		if (g_sSecc_fault[ucGun_id].sSecc_fault_second.sItem.gq_can_bcl_timeout_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_BCL_TIMEOUT_ERROR);
		}
		my_printf(USER_ERROR, "%s:%d %s BCL timeout start_tick = %d now = %d\n", __FILE__, __LINE__,
				(ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].uiStart_tick, uiNow_tick);

		return TIMEOUT;
	}
	else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].unRecv_cnt > 0U)
	{
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].uiStart_tick = uiNow_tick;
		//重置CNT，防止越界
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCL].unRecv_cnt = 0U;

		g_sGun_data[ucGun_id].sTemp.unAlready_charge_time = (U16)((uiNow_tick - g_sGun_data[ucGun_id].sTemp.uiStart_time)/1000U);

		if (0U == g_sGun_data[ucGun_id].sTemp.unEV_target_vol)
		{
			if (0U == g_uiErr_vol_tick[ucGun_id])
			{
				g_uiErr_vol_tick[ucGun_id] = uiNow_tick;
			}
			if ((uiNow_tick - g_uiErr_vol_tick[ucGun_id]) > FAULT_OVER_VOL_TIMEOUT)
			{
				if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.unBms_vol_err != TRUE)
				{
					GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_BMS_VOL_ERR);
					my_printf(USER_ERROR, "%s check target_vol = 0\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B");
				}
				g_uiErr_vol_tick[ucGun_id] = 0U;

				return STOPCHARGE;
			}
		}
		else
		{
			g_uiErr_vol_tick[ucGun_id] = 0U;

			if (TRUE == BmsChargingAlarmCheck(uiNow_tick, ucGun_id))
			{
				return STOPCHARGE;
			}
		}
	}
	else
	{
		//
	}

	if ((0U == g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].unRecv_cnt) &&
			((uiNow_tick - g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].uiStart_tick) > AS_BCS_TIMEOUT_TICK)
	  )
	{
		if (g_sSecc_fault[ucGun_id].sSecc_fault_second.sItem.gq_can_bcs_timeout_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_BCS_TIMEOUT_ERROR);
		}

		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].bTimeout_flag = TRUE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CCS].bSend_flag = FALSE;
		my_printf(USER_ERROR, "%s:%d %s BCS timeout start_tick = %d now = %d\n", __FILE__, __LINE__,
				(ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].uiStart_tick, uiNow_tick);

		return TIMEOUT;
	}
	else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].unRecv_cnt > 0U)
	{
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].uiStart_tick = uiNow_tick;
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BCS].unRecv_cnt = 0U;
	}
	else
	{
		//
	}

	return (Com_Stage_Def_t)TRUE;
}

/**
 * @brief 充电过程中报文超时
 * @param ucGun_id: 枪id
 * @return TRUE：超时处理完成  FALSE:超时处理中
 */
BOOL AS_BmsTimeoutTreat(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return TRUE;
	}

	U32 uiNow_tick = xTaskGetTickCount();

	if ((2U > g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].unRecv_cnt) &&
			((uiNow_tick - g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].uiStart_tick) > AS_BST_TIMEOUT_TICK))
	{
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].bTimeout_flag = TRUE;
		if (g_sSecc_fault[ucGun_id].sSecc_fault_second.sItem.gq_can_bst_timeout_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_BST_TIMEOUT_ERROR);
		}
		my_printf(USER_ERROR, "%s:%d TIMEOUT: %s BST timeout start_tick = %d now = %d\n", __FILE__, __LINE__,
				(ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].uiStart_tick, uiNow_tick);

		return TRUE;
	}
	else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].unRecv_cnt >= 2U)
	{
		//状态变更，发送充电实时数据上传报文
		if (END_OF_CHARGE_CSD != g_sGun_data[ucGun_id].sTemp.ucCharge_current_step)
		{
			g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = END_OF_CHARGE_CSD;
			TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
		}

		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CEM].bSend_flag = FALSE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CSD].bSend_flag = TRUE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CST].bSend_flag = FALSE;

		if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].uiStart_tick ==
					g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].uiStart_tick)
		{
			g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].uiStart_tick = uiNow_tick;
			my_printf(USER_INFO, "STOPCHARGE: %s receive EV BST CNT = %d，start send CSD\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].unRecv_cnt);
		}
	}
	else
	{
		//
	}

	if ((2U > g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].unRecv_cnt) &&
			((uiNow_tick - g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].uiStart_tick) > AS_BSD_TIMEOUT_TICK))
	{
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].bTimeout_flag = TRUE;
		if (g_sSecc_fault[ucGun_id].sSecc_fault_second.sItem.gq_can_bsd_timeout_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_BSD_TIMEOUT_ERROR);
		}
		my_printf(USER_ERROR, "%s:%d TIMEOUT: %s BSD timeout start_tick = %d now = %d\n", __FILE__, __LINE__,
				(ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].uiStart_tick, uiNow_tick);

		return TRUE;
	}
	else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].unRecv_cnt >= 2U)
	{
		//状态变更，发送充电实时数据上传报文
		if (END_OF_CHARGE_CSD != g_sGun_data[ucGun_id].sTemp.ucCharge_current_step)
		{
			g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = END_OF_CHARGE_CSD;
			TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
		}

		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CEM].bSend_flag = FALSE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CST].bSend_flag = FALSE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CSD].bSend_flag = TRUE;
		vTaskDelay(1000);

		return TRUE;
	}
	else
	{
		//
	}

	return FALSE;
}

/**
 * @brief 充电结束
 * @param ucGun_id: 枪id
 * @return TRUE:结束充电处理完成  FALSE:结束充电处理中
 */
BOOL AS_BmsStopChargeTreat(const U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return TRUE;
	}

	U32 uiNow_tick = xTaskGetTickCount();

	if ((2U > g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].unRecv_cnt) &&
			((uiNow_tick - g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].uiStart_tick) > AS_BST_TIMEOUT_TICK))
	{
        //状态变更，发送充电实时数据上传报文
		g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = END_OF_CHARGE_CEM;
		TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].bTimeout_flag = TRUE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CEM].bSend_flag = TRUE;

		if (g_sSecc_fault[ucGun_id].sSecc_fault_second.sItem.gq_can_bst_timeout_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_BST_TIMEOUT_ERROR);
		}
		g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = FAULT_STOP_MODE;
		my_printf(USER_ERROR, "%s:%d STOPCHARGE: %s BST timeout start_tick = %d now = %d\n", __FILE__, __LINE__,
				(ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].uiStart_tick, uiNow_tick);

		return TRUE;
	}
	else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].unRecv_cnt >= 2U)
	{
    	//状态变更，发送充电实时数据上传报文
		if (END_OF_CHARGE_CSD != g_sGun_data[ucGun_id].sTemp.ucCharge_current_step)
		{
			g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = END_OF_CHARGE_CSD;
			TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
		}

		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CEM].bSend_flag = FALSE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CST].bSend_flag = FALSE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CSD].bSend_flag = TRUE;

		if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].uiStart_tick ==
					g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].uiStart_tick)
		{
			g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].uiStart_tick = uiNow_tick;
			my_printf(USER_INFO, "STOPCHARGE: %s receive EV BST CNT = %d，start send CSD\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B",
					g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BST].unRecv_cnt);
		}
	}
	else
	{
		//
	}

	if ((2U > g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].unRecv_cnt) &&
			((uiNow_tick - g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].uiStart_tick) > AS_BSD_TIMEOUT_TICK))
	{
        //状态变更，发送充电实时数据上传报文
		g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = END_OF_CHARGE_CEM;
		TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
		g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].bTimeout_flag = TRUE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CEM].bSend_flag = TRUE;

		if (g_sSecc_fault[ucGun_id].sSecc_fault_second.sItem.gq_can_bsd_timeout_error != (U8)TRUE)
		{
			GunAlarmSet(&g_sSecc_fault[ucGun_id].sSecc_fault_second.u64Whole_flag, BIT64_STRUCT, SItem_GQ_CAN_BSD_TIMEOUT_ERROR);
		}
		g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type = FAULT_STOP_MODE;
		my_printf(USER_ERROR, "%s:%d STOPCHARGE: %s BSD timeout start_tick = %d now = %d\n", __FILE__, __LINE__,
				(ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].uiStart_tick, uiNow_tick);

		return TRUE;
	}
	else if (g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].unRecv_cnt >= 2U)
	{
    	//状态变更，发送充电实时数据上传报文
		if (END_OF_CHARGE_CSD != g_sGun_data[ucGun_id].sTemp.ucCharge_current_step)
		{
			g_sGun_data[ucGun_id].sTemp.ucCharge_current_step = END_OF_CHARGE_CSD;
			TcpSendControl(&g_sCMD_msg_control.sCharging_control.bSend_charging_data_flag);
		}
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CEM].bSend_flag = FALSE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CST].bSend_flag = FALSE;
		g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl[CSD].bSend_flag = TRUE;

		my_printf(USER_INFO, "STOPCHARGE: %s receive BSD CNT = %d send CSD\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sBms_ctrl[ucGun_id].sRecv_bms_timeout[BSD].unRecv_cnt);
		vTaskDelay(1000);

		return TRUE;
	}
	else
	{
		//
	}

	return FALSE;
}

/**
 * @brief 充电结束关闭辅源和断开枪锁
 * @param ucGun_id: 枪id
 * @return
 */
void CloseLockAndAssist(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	U8 ucRealy_timeout_cnt = 0;

	if (GUN_A == ucGun_id)
	{
		CigContrlDataInput(CIG_OPEN, OPEN_GUNA_LOCK_RELAY_AND_12V_ASSIST);
		//CigContrlDataInput(CIG_OPEN, OPEN_GUNA_LOCK_RELAY_AND_24V_ASSIST);
		my_printf(USER_INFO, "open GUN_A lock relay and 12V assist\n");
	}
	else
	{
		CigContrlDataInput(CIG_OPEN, OPEN_GUNB_LOCK_RELAY_AND_12V_ASSIST);
		//CigContrlDataInput(CIG_OPEN, OPEN_GUNB_LOCK_RELAY_AND_24V_ASSIST);
		my_printf(USER_INFO, "open GUN_B lock relay and 12V assist\n");
	}

	while (1)
	{
		//等待反馈
		if ((CIG_ASSIST_OPEN == g_sGun_data[ucGun_id].ucBms_assist_power_feedback_status)
				&& (CIG_LOCK_RELAY_OPEN == g_sGun_data[ucGun_id].ucGun_lock_relay_feedback_status))
		{
			//正常断开
			break;
		}
		else
		{
			if (ucRealy_timeout_cnt > (CIG_RELAY_TIMEOUT/CIG_RELAY_RES_CYCLE))
			{
				if (CIG_ASSIST_CLOSE == g_sGun_data[ucGun_id].ucBms_assist_power_feedback_status)
				{
					//辅源继电器粘连失败
					if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.unCig_assist_relay_sticking != TRUE)
					{
						GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_CIG_ASSIST_RELAY_STICKING);
						my_printf(USER_ERROR, "%s:%d %s open assist power supply relay failed:relay sticking\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");
					}
					break;
				}
				if (CIG_LOCK_RELAY_CLOSE == g_sGun_data[ucGun_id].ucGun_lock_relay_feedback_status)
				{
					//枪锁继电器粘连失败
					if (g_sGun_fault[ucGun_id].sRecoverable_fault.sItem.unCig_gun_relay_sticking != TRUE)
					{
						GunAlarmSet(&g_sGun_fault[ucGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_CIG_GUN_RELAY_CLOSE);
						my_printf(USER_ERROR, "%s:%d %s open gun lock relay failed:relay sticking\n", __FILE__, __LINE__, (ucGun_id == GUN_A)?"GUN_A":"GUN_B");
					}
					break;
				}
			}
			vTaskDelay(CIG_RELAY_RES_CYCLE);
			ucRealy_timeout_cnt++;
		}
	}
}

/**
 * @brief 结束充电处理
 * @param ucGun_id 枪id
 */
static void ChargeEnd(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	//国标需要关闭辅源，解锁电子锁
	if (g_sStorage_data.sPublic_data[ucGun_id].ucGun_type == (U8)GB)
	{
		CloseLockAndAssist(ucGun_id);
	}
	StopChargeRelayControl(ucGun_id);

	//清除当前充电数据
    (void)memset(&g_sBms_ctrl[ucGun_id].sMsg2bms_ctrl, 0, sizeof(Msg2bms_Ctrl_t)*(U8)CMAX);
    s_uiOver_vol_timeout[ucGun_id] = 0U;
    s_uiUnder_vol_timeout[ucGun_id] = 0U;
    s_uiOver_cur_timeout[ucGun_id] = 0U;
    g_uiErr_vol_tick[ucGun_id] = 0U;

    if ((U8)FAULT_STOP_MODE != g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type)
	{
    	GunStatusChange(ucGun_id, STA_CHARG_FINISHED, FALSE, FALSE);
	}
    else
    {
    	GunStatusChange(ucGun_id, STA_FAULT, FALSE, FALSE);
    }
	//未启动充电成功
	if (FALSE == g_bStart_charge_success_flag[ucGun_id])
	{
		//如果启动方式是POS启动，发送取消指令
		if (g_sGun_data[ucGun_id].sTemp.ucStart_Charge_type == (U8)POS_START_MODE)
		{
			if (NULL != g_psPOS_data)
			{
				g_psPOS_data->POS_Transaction[ucGun_id].bHmi_TransactionReversal_flag = TRUE;
			}
		}
	}
	g_bStart_charge_success_flag[ucGun_id] = FALSE;
    my_printf(USER_INFO, "%s charge complete stop charge type = %d (error:0xFF)\n", (ucGun_id==GUN_A)?"GUN_A":"GUN_B", g_sGun_data[ucGun_id].sTemp.ucStop_Charge_type);
}

/**
 * @brief 充电结束处理函数
 * @param ucGun_id 枪id
 * @param bFlag 特殊故障标志位
 * @return
 */
void ChargeFinishProcess(const U8 ucGun_id, const BOOL bFlag)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return;
	}

	U16 unCnt = 0;
	//未进入充电交互
	if (TRUE == bFlag)
	{
		//发送请求降功率报文
		SendStopChargeMsg(ucGun_id);
	}
	while (unCnt < 200U)
	{
		if (NULL != g_psMeter_data)
		{
			if (((g_sGun_data[ucGun_id].uiOutput_cur/10U) <= SAFE_CUR_LIMIT) || ((g_sGun_data[ucGun_id].uiOutput_vol/10U) <= SAFE_VOL_LIMIT))
			{
				break;
			}
		}
		unCnt++;
		vTaskDelay(25);
	}
	StopChargeRelayControl(ucGun_id);

	if (200 == unCnt)
	{
		my_printf(USER_ERROR, "stop charge PressureRelief timeout > 5S\n");
	}

	ChargeEnd(ucGun_id);
}

/**
 * @brief 通用充电控制子函数（仅处理控制指针相关逻辑，信号量留任务内）
 * @param pCtrl：当前模式下的控制指针（统一→全局，混搭→A/B专属）
 * @param gunType：枪型（GUN_A/GUN_B，用于ChargeControl调用）
 * @param gunName：枪名称（"GUN_A"/"GUN_B"，用于日志）
 */
static void CommonChargeControl(g_psCharge_Control_t* pCtrl, U8 gunType, const char* gunName)
{
    if (NULL != pCtrl)
    {
        if (NULL != pCtrl->ChargeControl)
        {
            pCtrl->ChargeControl(gunType);
        }
        else
        {
            my_printf(USER_ERROR, "%s:%d %s ChargeControl function is NULL\n", __FILE__, __LINE__, gunName);
        }
    }
    else
    {
        my_printf(USER_ERROR, "%s:%d %s control pointer is NULL\n", __FILE__, __LINE__, gunName);
    }
}

/**
 * @brief A枪充电控制任务
 * @param NULL
*/
static void Charge_CanA_Control_Task(void *parameter)
{
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
    volatile UBaseType_t uxHighWaterMark;
#endif

    while (1)
    {
        if (NULL != g_sStart_charge_sem[GUN_A])
        {
            if (pdTRUE == xSemaphoreTake(g_sStart_charge_sem[GUN_A], portMAX_DELAY))
            {
                if (g_sStorage_data.sPublic_data[GUN_A].ucGun_type == g_sStorage_data.sPublic_data[GUN_B].ucGun_type)
                {
                    CommonChargeControl(g_psCharge_control, GUN_A, "GUN_A");
                }
                else
                {
                    CommonChargeControl(g_psCharge_control_A, GUN_A, "GUN_A");
                }
            }
            else
            {
                my_printf(USER_ERROR, "%s:%d GUN_A wait for start charge TIMEOUT\n", __FILE__, __LINE__);
            }
        }
        else
        {
            my_printf(USER_ERROR, "%s:%d GUN_A g_sStart_charge_sem[GUN_A] = NULL\n", __FILE__, __LINE__);
            vTaskDelay(2000);
        }

#ifdef FREE_STACK_SPACE_CHECK_ENABLE
        uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
#endif
    }
}

/**
 * @brief B枪充电控制任务
 * @param NULL
 * @return
*/
static void Charge_CanB_Control_Task(void *parameter)
{
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
    volatile UBaseType_t uxHighWaterMark;
#endif

    while (1)
    {
        if (NULL != g_sStart_charge_sem[GUN_B])
        {
            if (pdTRUE == xSemaphoreTake(g_sStart_charge_sem[GUN_B], portMAX_DELAY))
            {
                if (g_sStorage_data.sPublic_data[GUN_A].ucGun_type == g_sStorage_data.sPublic_data[GUN_B].ucGun_type)
                {
                    CommonChargeControl(g_psCharge_control, GUN_B, "GUN_B");
                }
                else
                {
                    CommonChargeControl(g_psCharge_control_B, GUN_B, "GUN_B");
                }
            }
            else
            {
                my_printf(USER_ERROR, "%s:%d GUN_B wait for start charge TIMEOUT\n", __FILE__, __LINE__);
            }
        }
        else
        {
            my_printf(USER_ERROR, "%s:%d GUN_B g_sStart_charge_sem[GUN_B] = NULL\n", __FILE__, __LINE__);
            vTaskDelay(2000);
        }

#ifdef FREE_STACK_SPACE_CHECK_ENABLE
        uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
#endif
    }
}

/**
 * @brief 通用 CAN 报文发送子函数
 * @param pCtrl：控制指针
 * @param gunType：枪型
 * @param errMsg
 */
static void CommonCanMsgSend(g_psCharge_Control_t* pCtrl, U8 gunType, const char* errMsg)
{
    if (NULL != pCtrl)
    {
        if (NULL != pCtrl->CANMessageSend)
        {
            pCtrl->CANMessageSend(gunType);
            vTaskDelay(10);
            return;
        }
    }

    my_printf(USER_ERROR, "%s:%d %s = NULL\n", __FILE__, __LINE__, errMsg);
    vTaskDelay(1000);
}

/**
 * @brief A枪报文发送任务（调用通用子函数，逻辑极简）
 */
static void Charge_CanA_Msg_Send_Task(void *pvParameters)
{
    while (1)
    {
        if (g_sStorage_data.sPublic_data[GUN_A].ucGun_type == g_sStorage_data.sPublic_data[GUN_B].ucGun_type)
        {
            CommonCanMsgSend(g_psCharge_control, GUN_A, "g_psCharge_control");
        }
        else
        {
            CommonCanMsgSend(g_psCharge_control_A, GUN_A, "g_psCharge_control_A");
        }
    }
}

/**
 * @brief B枪报文发送任务（调用通用子函数，逻辑极简）
 */
static void Charge_CanB_Msg_Send_Task(void *pvParameters)
{
    while (1)
    {
        if (g_sStorage_data.sPublic_data[GUN_A].ucGun_type == g_sStorage_data.sPublic_data[GUN_B].ucGun_type)
        {
            CommonCanMsgSend(g_psCharge_control, GUN_B, "g_psCharge_control");
        }
        else
        {
            CommonCanMsgSend(g_psCharge_control_B, GUN_B, "g_psCharge_control_B");
        }
    }
}

void Charge_Init_Task(void * pvParameters)
{
	//外设初始化
	RKNSeccInit();
	JWTSeccInit();
	GB2015Init();
	//外设选择
	SeccModuleSelect();

	CheckServerConfig();

	g_sStart_charge_sem[GUN_B] = xSemaphoreCreateBinary();
	g_sStart_charge_sem[GUN_A] = xSemaphoreCreateBinary();
	if ((NULL == g_sStart_charge_sem[GUN_B]) || (NULL == g_sStart_charge_sem[GUN_A]))
	{
		my_printf(USER_ERROR, "%s:%d Charge_Init_Task error\n", __FILE__, __LINE__);
		vTaskSuspend(NULL);
	}

	taskENTER_CRITICAL();

	//add other meter_init task here
	(void)xTaskCreate(&Charge_CanA_Msg_Send_Task, "CHARGE_CANA_MSG_SEND", 700U/4U, NULL, IMPORTENT_TASK_TASK_PRIO, NULL);
	(void)xTaskCreate(&Charge_CanB_Msg_Send_Task, "CHARGE_CANB_MSG_SEND", 700U/4U, NULL, IMPORTENT_TASK_TASK_PRIO, NULL);
	(void)xTaskCreate(&Charge_CanA_Control_Task, "CHARGE_CANA_CONTROL", 1000U/4U, NULL, IMPORTENT_TASK_TASK_PRIO, NULL);
	(void)xTaskCreate(&Charge_CanB_Control_Task, "CHARGE_CANB_CONTROL", 1000U/4U, NULL, IMPORTENT_TASK_TASK_PRIO, NULL);
	vTaskDelete(NULL);

	taskEXIT_CRITICAL();
}
