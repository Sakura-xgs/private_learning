/*
 * uart_comm.c
 *
 *  Created on: 2024年8月28日
 *      Author: Bono
 */

#include "hal_ext_rtc.h"
#include "SignalManage.h"
#include "uart_comm.h"
#include "tcp_client_IF.h"
#include "imd_IF.h"
#include "rfid_IF.h"
#include "hmi_IF.h"
#include "pos_IF.h"
#include "meter_IF.h"
#include "emergency_fault_IF.h"
#include "hal_eth_IF.h"
#include "boot.h"
#include "uart_boot.h"
#include "hal_sys_IF.h"
#include "cig_IF.h"
#include "uart_boot.h"

SemaphoreHandle_t uartDebugMsgMutexSemaphore = NULL;
__BSS(SRAM_OC) Test_t g_Test = {0};

/**
 * @brief 检测两个数的差值是否大于目标值
 * @param uiFirst_num：数据1
 * @param uiSecond_num：数据2
 * @param uiDiff_data：目标差值
 * @return TRUE：差值大于目前值 FLASE：差值小于目标值
 */
BOOL my_unsigned_abs(const U32 uiFirst_num, const U32 uiSecond_num, const U32 uiDiff_data)
{
	U32 uiTemp = (uiFirst_num > uiSecond_num) ? (uiFirst_num - uiSecond_num) : (uiSecond_num - uiFirst_num);
	return (uiTemp > uiDiff_data);
}

void my_printf(LogLevel level, const char *format, ...)
{
	if(g_Mbms_sam_update_state == (CCU_UPGRADE_BY_UART1 + DBG_UART))
	{
		return;
	}

	static __BSS(SRAM_OC) U8 u8MyDebugBuff[UART_COMM_BUF_LEN] = {0};

    if (level < USER_DEBUG_LEVEL)
    {
        return; // 日志级别低于当前启用的级别，不输出
    }

    if (NULL == uartDebugMsgMutexSemaphore)
    {
    	return;
    }
	(void)xSemaphoreTake(uartDebugMsgMutexSemaphore, portMAX_DELAY);

    // 先输出日志级别
    const char *levelString = NULL;
    switch (level)
    {
        case USER_DEBUG:
            levelString = "DEBUG";
            break;
        case USER_INFO:
            levelString = "INFO";
            break;
        case USER_ERROR:
            levelString = "ERROR";
            break;
        default:
            levelString = "UNKNOWN";
            break;
    }
    U32 u32MsgSize = 0;
    int ret = snprintf(u8MyDebugBuff, sizeof(u8MyDebugBuff), "[%s] ", levelString);
    if ((ret < 0) || (ret >= (int)sizeof(u8MyDebugBuff))) // 异常处理：写入失败直接返回
    {
    	(void)xSemaphoreGive(uartDebugMsgMutexSemaphore);
    	return;
    }

    u32MsgSize = ret;

    va_list args;
    va_start(args, format);

    // 接着输出格式化字符串
    U32 remainingSpace = sizeof(u8MyDebugBuff) - u32MsgSize - 1U; // 保留一个字符给空终止符
    (void)vsnprintf(&u8MyDebugBuff[u32MsgSize], remainingSpace, format, args);
    u32MsgSize = strlen(u8MyDebugBuff); // 更新总消息大小

    va_end(args);

    // 确保末尾终止符
    u8MyDebugBuff[sizeof(u8MyDebugBuff) - 1U] = '\0';

#ifdef CCU_LOG_UPLOAD
	if (-1 != sockfd)
	{
		// 上传PCU，去掉末尾的 '\n'
		if ((u32MsgSize > 0U) && (u8MyDebugBuff[u32MsgSize - 1U] == '\n'))
		{
			u32MsgSize--;
		}

		u8MyDebugBuff[u32MsgSize] = '\0';
		g_sMsg_control.sCCU_log_control.uiLog_len = u32MsgSize;
		(void)memcpy(g_sMsg_control.sCCU_log_control.ucLog_data, u8MyDebugBuff, u32MsgSize);
		TcpSendControl(&g_sMsg_control.sCCU_log_control.bSend_log_flag);
	}
#else
	u8MyDebugBuff[u32MsgSize] = '\0';
	Uart_Dma_Send(DBG_UART, u32MsgSize, u8MyDebugBuff);
#endif
	// -------------------------------------------------------------------
	memset(u8MyDebugBuff, 0, UART_COMM_BUF_LEN);

	(void)xSemaphoreGive(uartDebugMsgMutexSemaphore);
	vTaskDelay(10);
}

void debug_printf(const char *format, ...)
{
    if(g_Mbms_sam_update_state == (CCU_UPGRADE_BY_UART1 + DBG_UART))
    {
    	return;
    }

	__BSS(SRAM_OC) static U8 u8DebugBuff[UART_COMM_BUF_LEN] = {0};

#if UART_DEBUG_MSG
	S32 s32MsgSize = 0;
	va_list args;

	(void)xSemaphoreTake(uartDebugMsgMutexSemaphore, portMAX_DELAY);

	va_start(args, format);

	s32MsgSize = vsnprintf((char *)u8DebugBuff, sizeof(u8DebugBuff), format, args);
	va_end(args);

	if (s32MsgSize > 0)
	{
		if ((U32)s32MsgSize >= sizeof(u8DebugBuff))
		{
			s32MsgSize = (S32)(sizeof(u8DebugBuff) - 1U);
			u8DebugBuff[s32MsgSize] = (U8)'\0';
		}

		Uart_Dma_Send(DBG_UART, (U32)s32MsgSize, u8DebugBuff);
	}

	(void)xSemaphoreGive(uartDebugMsgMutexSemaphore);
#endif
}

static void DebugControl(const U8 *ucBuf)
{
	//调试模式
	//if (DEBUG_MODE == g_sPile_data.ucPile_config_mode)
	{
		switch (ucBuf[0])
		{
		case (U8)'1':
			RelayControl(GUN_A, POSITIVE_RELAY, RELAY_ON);
			my_printf(USER_INFO, "close GUN_A positive relay\n");
			break;
		case (U8)'2':
			RelayControl(GUN_A, POSITIVE_RELAY, RELAY_OFF);
			my_printf(USER_INFO, "open GUN_A positive relay\n");
			break;
		case (U8)'3':
			RelayControl(GUN_A, NEGATIVE_RELAY, RELAY_ON);
			my_printf(USER_INFO, "close GUN_A negative relay\n");
			break;
		case (U8)'4':
		    RelayControl(GUN_A, NEGATIVE_RELAY, RELAY_OFF);
			my_printf(USER_INFO, "open GUN_A negative relay\n");
			break;
		case (U8)'5':
			RelayControl(GUN_B, POSITIVE_RELAY, RELAY_ON);
			my_printf(USER_INFO, "close GUN_B positive relay\n");
			break;
		case (U8)'6':
			RelayControl(GUN_B, POSITIVE_RELAY, RELAY_OFF);
			my_printf(USER_INFO, "open GUN_B positive relay\n");
			break;
		case (U8)'7':
			RelayControl(GUN_B, NEGATIVE_RELAY, RELAY_ON);
			my_printf(USER_INFO, "close GUN_B negative relay\n");
			break;
		case (U8)'8':
			RelayControl(GUN_B, NEGATIVE_RELAY, RELAY_OFF);
			my_printf(USER_INFO, "open GUN_B negative relay\n");
			break;
		case (U8)'9':
		    RelayControl(GUN_A, PRECHARGE_RELAY, RELAY_ON);
			my_printf(USER_INFO, "close GUN_A precharge relay\n");
			break;
		case (U8)'a':
		    RelayControl(GUN_A, PRECHARGE_RELAY, RELAY_OFF);
			my_printf(USER_INFO, "open GUN_A precharge relay\n");
			break;
		case (U8)'b':
			RelayControl(GUN_B, PRECHARGE_RELAY, RELAY_ON);
			my_printf(USER_INFO, "close GUN_B precharge relay\n");
			break;
		case (U8)'c':
			RelayControl(GUN_B, PRECHARGE_RELAY, RELAY_OFF);
			my_printf(USER_INFO, "open GUN_B precharge relay\n");
			break;
		case (U8)'d':
			CHECK_PTR_NULL_NO_RETURN(g_psRFID_data);
			CHECK_PTR_NULL_NO_RETURN(g_psHMI_data);
			g_psHMI_data->eGun_id = ((2U*g_sStorage_data.ucPile_num)-1U);
			g_psHMI_data->ucCharge_control_type = RFID_START_MODE;
			//使能刷卡板记时
			g_psRFID_data->eRfid_start_timing_flag = TIMER_OPEN_STATUS;
			//使能刷卡板寻卡
			g_psRFID_data->bRfid_search_flag = TRUE;
			g_psHMI_data->eHmi_control_type = START_CHARGE;
			my_printf(USER_INFO, "GUN_A: use RFID start charge\n");
			break;
		case (U8)'e':
			CHECK_PTR_NULL_NO_RETURN(g_psRFID_data);
			CHECK_PTR_NULL_NO_RETURN(g_psHMI_data);
			g_psHMI_data->eGun_id = ((2U*g_sStorage_data.ucPile_num)-1U);
			g_psHMI_data->ucCharge_control_type = RFID_STOP_MODE;
			//使能刷卡板记时
			g_psRFID_data->eRfid_start_timing_flag = TIMER_OPEN_STATUS;
			//使能刷卡板寻卡
			g_psRFID_data->bRfid_search_flag = TRUE;
			g_psHMI_data->eHmi_control_type = STOP_CHARGE;
			my_printf(USER_INFO, "GUN_A: use RFID stop charge\n");
			break;
		case (U8)'f':
			CHECK_PTR_NULL_NO_RETURN(g_psRFID_data);
			CHECK_PTR_NULL_NO_RETURN(g_psHMI_data);
			g_psHMI_data->eGun_id = 2U*g_sStorage_data.ucPile_num;
			g_psHMI_data->ucCharge_control_type = RFID_START_MODE;
			//使能刷卡板记时
			g_psRFID_data->eRfid_start_timing_flag = TIMER_OPEN_STATUS;
			//使能刷卡板寻卡
			g_psRFID_data->bRfid_search_flag = TRUE;
			g_psHMI_data->eHmi_control_type = START_CHARGE;
			my_printf(USER_INFO, "GUN_B: use RFID start charge\n");
			break;
		case (U8)'g':
			CHECK_PTR_NULL_NO_RETURN(g_psRFID_data);
			CHECK_PTR_NULL_NO_RETURN(g_psHMI_data);
			g_psHMI_data->eGun_id = 2U*g_sStorage_data.ucPile_num;
			g_psHMI_data->ucCharge_control_type = RFID_STOP_MODE;
			//使能刷卡板记时
			g_psRFID_data->eRfid_start_timing_flag = TIMER_OPEN_STATUS;
			//使能刷卡板寻卡
			g_psRFID_data->bRfid_search_flag = TRUE;
			g_psHMI_data->eHmi_control_type = STOP_CHARGE;
			my_printf(USER_INFO, "GUN_B: use RFID stop charge\n");
			break;
		case (U8)'h':
			//取消刷卡板定时
			CHECK_PTR_NULL_NO_RETURN(g_psRFID_data);
			CHECK_PTR_NULL_NO_RETURN(g_psHMI_data);
			g_psRFID_data->eRfid_start_timing_flag = TIMER_CLOSE_STATUS;
			//取消刷卡板寻卡
			g_psRFID_data->bRfid_search_flag = FALSE;
			g_psHMI_data->ucCharge_control_type = UKNOWN_STOP_MODE;
			g_psHMI_data->eHmi_control_type = UKNOWN_STATUS;
			my_printf(USER_INFO, "cancel RFID start charge\n");
			break;
		case (U8)'i':
			g_psHMI_data->eGun_id = ((2U*g_sStorage_data.ucPile_num)-1U);
			g_psHMI_data->eHmi_control_type = START_CHARGE;
			g_psHMI_data->ucCharge_control_type = POS_START_MODE;
			//g_psHMI_data->ucPrepayment_amount = pucBuf[6];
			//告知POS启动充电
			g_psPOS_data->POS_Transaction[0].bHmi_TransactionStart_flag = TRUE;
			my_printf(USER_INFO, "GUN_A: POS start charge\n");
			break;
		case (U8)'j':
			g_psHMI_data->eGun_id = ((2U*g_sStorage_data.ucPile_num)-1U);
			g_psHMI_data->ucCharge_control_type = POS_STOP_MODE;
			g_psHMI_data->eHmi_control_type = STOP_CHARGE;
			//告知POS充电完成，发送寻卡指令
			g_psPOS_data->POS_Transaction[0].bHmi_GetCardInfo_flag = TRUE;
			my_printf(USER_INFO, "GUN_A: POS stop charge\n");
			break;
		case (U8)'k':
			g_psHMI_data->eGun_id = 2U*g_sStorage_data.ucPile_num;
			g_psHMI_data->eHmi_control_type = START_CHARGE;
			g_psHMI_data->ucCharge_control_type = POS_START_MODE;
			//g_psHMI_data->ucPrepayment_amount = pucBuf[6];
			//告知POS启动充电
			g_psPOS_data->POS_Transaction[1].bHmi_TransactionStart_flag = TRUE;
			my_printf(USER_INFO, "GUN_B: POS start charge\n");
			break;
		case (U8)'l':
			g_psHMI_data->eGun_id = 2U*g_sStorage_data.ucPile_num;
			g_psHMI_data->ucCharge_control_type = POS_STOP_MODE;
			g_psHMI_data->eHmi_control_type = STOP_CHARGE;
			//告知POS充电完成，发送寻卡指令
			g_psPOS_data->POS_Transaction[1].bHmi_GetCardInfo_flag = TRUE;
			my_printf(USER_INFO, "GUN_B: POS stop charge\n");
			break;
		case (U8)'m':
			g_psHMI_data->eGun_id = ((2U*g_sStorage_data.ucPile_num)-1U);
			g_psHMI_data->ucCharge_control_type = UKNOWN_STOP_MODE;
			g_psHMI_data->eHmi_control_type = UKNOWN_STATUS;
			//告知POS用户取消使用
			g_psPOS_data->POS_Transaction[0].bHmi_Abort_flag = TRUE;
			my_printf(USER_INFO, "GUN_A: cancel POS start charge\n");
			break;
		case (U8)'n':
			g_psHMI_data->eGun_id = 2U*g_sStorage_data.ucPile_num;
			g_psHMI_data->ucCharge_control_type = UKNOWN_STOP_MODE;
			g_psHMI_data->eHmi_control_type = UKNOWN_STATUS;
			//告知POS用户取消使用
			g_psPOS_data->POS_Transaction[1].bHmi_Abort_flag = TRUE;
			my_printf(USER_INFO, "GUN_B: cancel POS start charge\n");
			break;
		case (U8)'o':
			g_Test.PosTempGunA = ucBuf[1];
			g_Test.NegTempGunA = ucBuf[2];
			my_printf(USER_INFO, "GUN_A: test add positive temp = %d negative temp = %d\n", ucBuf[1], ucBuf[2]);
			break;
		case (U8)'p':
			g_Test.PosTempGunB = ucBuf[1];
			g_Test.NegTempGunB = ucBuf[2];
			my_printf(USER_INFO, "GUN_B: test add positive temp = %d negative temp = %d\n", ucBuf[1], ucBuf[2]);
			break;
		case (U8)'q':
			g_Test.CurGunA = 120;
			g_Test.bTest_flag = TRUE;
			my_printf(USER_INFO, "GUN_A: test control over cur\n");
			break;
		case (U8)'r':
			g_Test.CurGunB = 120;
			g_Test.bTest_flag = TRUE;
			my_printf(USER_INFO, "GUN_B: test control over cur\n");
			break;
		case (U8)'s':
			g_Test.VolGunA = 700;
			g_Test.bTest_flag = TRUE;
			my_printf(USER_INFO, "GUN_A: test control over vol\n");
			break;
		case (U8)'t':
			g_Test.VolGunB = 700;
			g_Test.bTest_flag = TRUE;
			my_printf(USER_INFO, "GUN_B: test control over vol\n");
			break;
		case (U8)'u':
			my_printf(USER_INFO, "reboot\n");
			TcpClose();
			g_psHMI_data->bRestore_flag = TRUE;
			vTaskDelay(2000);
			SystemResetFunc();
			break;
		case (U8)'v':
			if (1U == (ucBuf[1]-48U))
			{
				g_sPile_data.ucPile_config_mode = DEBUG_MODE;
				my_printf(USER_INFO, "Change the mode to debug mode\n");
			}
			else
			{
				g_sPile_data.ucPile_config_mode = RELEASE_MODE;
				my_printf(USER_INFO, "Change the mode to factory mode\n");
			}
			break;
		case (U8)'w':
			//配置电表地址
			U8 ucBuff[] = {0x1, 0x10, 2, 0x5c, 0, 1, 2, 2, 2, 0, 0};
			MeterUartSend(METER_UART, sizeof(ucBuff) , ucBuff);
			break;
		case (U8)'x':
			(void)SetSigVal(CCU_SET_SIG_ID_ADVANCE_CHARGE, 10000);
			(void)SetSigVal(CCU_SET_SIG_ID_GRACE_PERIOD, 1);
			(void)SetSigVal(CCU_SET_SIG_ID_CHARGE_PRICE_UNIT_PRICE, 100);
			(void)SetSigVal(CCU_SET_SIG_ID_OCCUPATION_FEE_UNIT_PRICE, 100);
			my_printf(USER_INFO, "set default advance charge = 10000(0.01$), grace time = 1(min), unit price = 100(0.01$)\n");
			break;
		case (U8)'y':
			S32 iCurrent_gun_num = 0;
			S32 iTarget_gun_num = (S32)((S8)ucBuf[1]-'0');
			(void)GetSigVal(CCU_SET_SIG_ID_GUN_NUM, &iCurrent_gun_num);
			my_printf(USER_INFO, "current pile number = %d change pile number = %d\n", iCurrent_gun_num, iTarget_gun_num);
			(void)SetSigVal(CCU_SET_SIG_ID_GUN_NUM, iTarget_gun_num);
			break;
		case (U8)'z':
			(void)SetSigVal(CCU_SET_SIG_ID_CHARGE_PRICE_A, 0);
			vTaskDelay(3);
			(void)SetSigVal(CCU_SET_SIG_ID_OCCUPATION_FEE_A, 0);
			vTaskDelay(3);
			(void)SetSigVal(CCU_SAM_SIG_ID_OCCUPATION_FEE_A, 0);
			vTaskDelay(3);
			(void)SetSigVal(CCU_SAM_SIG_ID_CHARGE_PRICE_A, 0);
			vTaskDelay(3);
			(void)SetSigVal(CCU_SET_SIG_ID_CHARGE_PRICE_B, 0);
			vTaskDelay(3);
			(void)SetSigVal(CCU_SET_SIG_ID_OCCUPATION_FEE_B, 0);
			vTaskDelay(3);
			(void)SetSigVal(CCU_SAM_SIG_ID_OCCUPATION_FEE_B, 0);
			vTaskDelay(3);
			(void)SetSigVal(CCU_SAM_SIG_ID_CHARGE_PRICE_B, 0);
			vTaskDelay(3);
			my_printf(USER_INFO, "clear charge price\n");
			break;
		case 'A':
			g_sPile_data.ucPile_config_mode = PRODUCTION_MODE;
			USER_LED_TOGGLE();
			my_printf(USER_INFO, "device led toggle\n");
			break;
		case 'B':
			CigContrlDataInput(CIG_CLOSE, CLOSE_GUNA_LOCK_RELAY);
			my_printf(USER_INFO, "close CIG GUNA lock relay\n");
			break;
		case 'C':
			CigContrlDataInput(CIG_CLOSE, CLOSE_GUNB_LOCK_RELAY);
			my_printf(USER_INFO, "close CIG GUNB lock relay\n");
			break;
		case 'D':
			CigAssistRelayClose(GUN_A);
			break;
		case 'E':
			CigAssistRelayClose(GUN_B);
			break;
		case 'F':
			CigRelayOpenContrl(GUN_A);
			my_printf(USER_INFO, "open CIG GUNA ASSIST/lock relay\n");
			break;
		case 'G':
			CigRelayOpenContrl(GUN_B);
			my_printf(USER_INFO, "open CIG GUNB ASSIST/lock relay\n");
			break;
		case 'H':
			GunAlarmSet(&g_sSecc_fault[GUN_B].sSecc_fault_fourth.u64Whole_flag, BIT64_STRUCT, CCS2_CP_Error_Bit);
			my_printf(USER_INFO, "flag = %lld\n", g_sSecc_fault[GUN_B].sSecc_fault_fourth.u64Whole_flag);
			break;
		default:
			my_printf(USER_ERROR, "%s:%d UART set control error = %d\n", __FILE__, __LINE__, ucBuf[0]);
			break;
		}
	}
}

static void DebugRead(const U8 *ucBuf)
{
	//调试模式
	//if (DEBUG_MODE == g_sPile_data.ucPile_config_mode)
	{
		switch (ucBuf[0])
		{
		case (U8)'1':
			my_printf(USER_INFO, "pile id = %d\n", g_sStorage_data.ucPile_num);
			break;
		case (U8)'2':
			my_printf(USER_INFO, "GUN_A id = %d status = %d GUN_B id = %d status = %d\n", g_sStorage_data.sPublic_data[GUN_A].eGun_id, g_sGun_data[GUN_A].eGun_common_status,
					g_sStorage_data.sPublic_data[GUN_B].eGun_id, g_sGun_data[GUN_B].eGun_common_status);
			break;
		case (U8)'3':
			U8 ucTemp[6] = {0};
			U8 ucData[18] = {0};
			GetMacAddress(ucTemp);
			(void)snprintf(ucData, 18, "%02x:%02x:%02x:%02x:%02x:%02x", ucTemp[0], ucTemp[1], ucTemp[2],ucTemp[3], ucTemp[4], ucTemp[5]);
			my_printf(USER_INFO, "mac address = %s\n", ucData);
			break;
		case (U8)'4':
			my_printf(USER_INFO, "GUN_A imd_vol=%dV meter_vol=%dV meter_cur=%dA power=%d GUN_B:imd_vol=%dV meter_vol=%dV meter_cur=%dA power=%d\n",
					g_psIMD_data->sPublic_data[GUN_A].unBus_vol/10U, g_sGun_data[GUN_A].uiOutput_vol/100U,
					g_sGun_data[GUN_A].uiOutput_cur/100U, g_sGun_data[GUN_A].uiOutput_power,
					g_psIMD_data->sPublic_data[GUN_B].unBus_vol/10U, g_sGun_data[GUN_B].uiOutput_vol/100U,
					g_sGun_data[GUN_B].uiOutput_cur/100U, g_sGun_data[GUN_B].uiOutput_power);
			break;
		case (U8)'5':
			my_printf(USER_INFO, "Software = %s\n", g_sPile_data.ucSoftware);
			break;
		case (U8)'6':
			my_printf(USER_INFO, "Hardware = %s\n", g_sPile_data.ucHardware);
			break;
		case (U8)'7':

			break;
		case (U8)'8':
			my_printf(USER_INFO, "GUN_A DC+ = %d℃ DC- = %d℃ GUN_B DC+ = %d℃ DC- = %d℃(-60 offset)\n", g_sGun_data[GUN_A].nDC_positive_temp,
					g_sGun_data[GUN_A].nDC_negative_temp, g_sGun_data[GUN_B].nDC_positive_temp, g_sGun_data[GUN_B].nDC_negative_temp);
			break;
		case (U8)'9':
			U32 year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;

			SYS_GetDate(&year, &month, &day, &hour, &minute, &second);

			my_printf(USER_INFO, "time = %d/%d/%d/%d:%d:%d\n", year, month, day, hour, minute, second);
			break;
		case (U8)'a':
			my_printf(USER_INFO, "GUN_A status = %d GUN_B status = %d\n", g_sGun_data[GUN_A].eGun_common_status,
					g_sGun_data[GUN_B].eGun_common_status);
			break;
		default:
			my_printf(USER_ERROR, "%s:%d UART read control error = %d\n", __FILE__, __LINE__, ucBuf[0]);
			break;
		}
	}
}

static void DebugUartRec(void)
{
	__BSS(SRAM_OC) static U8 u8Recdata[UART_COMM_BUF_LEN] = {0};
	static U16 unRec_index = 0; // 当前接收到的字符索引
	U16 u16RecByteNum = 0;

	//防止越界
	if (unRec_index > UART_COMM_BUF_LEN)
	{
		(void)memset(u8Recdata, 0, UART_COMM_BUF_LEN);
		unRec_index = 0;
	}
	u16RecByteNum = RecUartData(DBG_UART, &u8Recdata[unRec_index], UART_COMM_BUF_LEN - unRec_index);
	//uPRINTF("uart-%d has %d bytes %s received.\n", DBG_UART, u16RecByteNum, u8Recdata);
	unRec_index += u16RecByteNum;
    // 检查是否接收到了换行符作为输入结束标志
    if ((unRec_index > 0U) && (u8Recdata[unRec_index - 1U] == (U8)'\n'))
    {
        // 将换行符替换为字符串结束符
        u8Recdata[unRec_index - 1U] = (U8)'\0';

    	if(u8Recdata[0] == (U8)baud_115200)
    	{
    		//uPRINTF("Please change the uart-%d baudrate to 115200.", DBG_UART);
    		Uart_BaudRate_Reinit(DBG_UART, baud_115200);
    	}
    	else if(u8Recdata[0] == (U8)baud_9600)
    	{
    		//uPRINTF("Please change the uart-%d baudrate to 9600.", DBG_UART);
    		Uart_BaudRate_Reinit(DBG_UART, baud_9600);
    	}
    	else if (((U8)'A' == u8Recdata[0]) && ((U8)'A' == u8Recdata[1]))
    	{
    		if (18U == u16RecByteNum)
    		{
    			S32 iTemp = 0;
    			(void)memcpy(&iTemp, &u8Recdata[1], 4);
    			(void)SetSigVal(CCU_SET_SIG_ID_SN_NUMBER_1_2_3_4, iTemp);
    			vTaskDelay(10);
    			(void)memcpy(&iTemp, &u8Recdata[5], 4);
    			(void)SetSigVal(CCU_SET_SIG_ID_SN_NUMBER_5_6_7_8, iTemp);
    			vTaskDelay(10);
    			(void)memcpy(&iTemp, &u8Recdata[9], 4);
    			(void)SetSigVal(CCU_SET_SIG_ID_SN_NUMBER_9_10_11_12, iTemp);
    			vTaskDelay(10);
    			(void)memcpy(&iTemp, &u8Recdata[13], 4);
    			(void)SetSigVal(CCU_SET_SIG_ID_SN_NUMBER_13_14_15_16, iTemp);
    		}
    	}
    	//控制功能
    	else if (((U8)'A' == u8Recdata[0]) && ((U8)'5' == u8Recdata[1]))
    	{
    		DebugControl(&u8Recdata[2]);
    	}
    	//读取功能
    	else if (((U8)'B' == u8Recdata[0]) && ((U8)'5' == u8Recdata[1]))
    	{
    		DebugRead(&u8Recdata[2]);
    	}
    	else
    	{
    		//不处理
    	}

        // 重置接收状态以便下次接收
    	unRec_index = 0;
    	(void)memset(u8Recdata, 0, UART_COMM_BUF_LEN);
    }
}

static void ExampleUartSend(void)
{
	U8 u8TempBuff[UART_COMM_BUF_LEN] = {0};

	/*lock your mutex  here*/
	u8TempBuff[0] = (U8)'H';
	u8TempBuff[1] = (U8)'i';

	Uart_Dma_Send(DBG_UART, 2 , u8TempBuff);
	/*unlock your mutex here*/
}

/*!
    \brief      Rs485 UART COMM task
    \param[in]  pvParameters not used
    \param[out] none
    \retval     none
*/
static void Debug_Uart_Task(void * pvParameters)
{
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
	volatile UBaseType_t uxHighWaterMark;
#endif
    BaseType_t err = pdFALSE;

	(void)pvParameters;

    for( ;; )
    {
        if(NULL != Uart_recBinarySemaphore(DBG_UART))
        {
            err = xSemaphoreTake(Uart_recBinarySemaphore(DBG_UART), portMAX_DELAY);

            if(pdTRUE == err)
            {
            	DebugUartRec();
            }
        }
        else
        {
            vTaskDelay(1000);
        }

#ifdef FREE_STACK_SPACE_CHECK_ENABLE
		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
#endif
    }
}

void Uart_Init_Task(void * pvParameters)
{
	(void)pvParameters;

	(void)xTaskCreate(&Debug_Uart_Task,	"DEBUG_UART_INIT",     configMINIMAL_STACK_SIZE,   	NULL,   GENERAL_TASK_PRIO,    	NULL);

	vTaskDelete(NULL);
}
