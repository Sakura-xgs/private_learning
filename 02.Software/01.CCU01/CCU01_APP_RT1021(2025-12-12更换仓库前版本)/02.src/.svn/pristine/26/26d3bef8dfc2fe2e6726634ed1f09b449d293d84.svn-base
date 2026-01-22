/*
 * rfid.c
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "cr_section_macros.h"

#include "rfid.h"
#include "HW_rfid.h"
#include "hal_uart_IF.h"
#include "hmi_IF.h"
#include "tcp_client_IF.h"
#include "emergency_fault_IF.h"
#include "SignalManage.h"
#include "uart_comm.h"

static U16 g_unRfid_timeout_cnt = 0;
RFID_data_t *g_psRFID_data = NULL;
TimerHandle_t g_sTimer_rfid = NULL;

/**
 * @brief 兼容性函数，用于选择不同厂家的控制函数
 * @param
 * @return
 */
static void RfidModuleSelect(void)
{
	//获取RFID型号
	S32 uiModule_flag = 0;

	(void)GetSigVal(CCU_SET_SIG_ID_RFID_MODEL, &uiModule_flag);

	if (HW_RFID_MODULE_FLAG == uiModule_flag)
	{
		g_psRFID_data = GetHwRfidModel();
		my_printf(USER_INFO, "get RFID configure model: HW\n");
	}
	else
	{
		g_psRFID_data = GetHwRfidModel();
		my_printf(USER_INFO, "use default configure model: HW\n");
	}
}

/**
 * @brief 刷卡板启动充电控制
 * @param
 * @return
 */
static void RfidStartCharge(void)
{
	CHECK_PTR_NULL_NO_RETURN(g_psHMI_data);

	if ((g_psHMI_data->eGun_id == g_sStorage_data.sPublic_data[GUN_A].eGun_id)
			|| (g_psHMI_data->eGun_id == g_sStorage_data.sPublic_data[GUN_B].eGun_id))
	{
		U8 GunId = g_psHMI_data->eGun_id - ((2U * g_sStorage_data.ucPile_num) - 1U);

		if (GunId < GUN_MAX_NUM)
		{
			(void)memcpy(g_sGun_data[GunId].sTemp.ucIdtag, g_psRFID_data->ucId_tag, sizeof(g_sGun_data[GunId].sTemp.ucIdtag));
			g_sGun_data[GunId].sTemp.eStart_Charge_type = RFID_START_MODE;
			g_sMsg_control.sGun_charge_control.ucStart_id = g_psHMI_data->eGun_id;
			g_sGun_data[GunId].sTemp.ucAuth_type = START_CHARGE_AUTH;
			my_printf(USER_INFO, "%s RFID request start charge auth id = %s\n", (GunId==GUN_A)?"GUN_A":"GUN_B", g_sGun_data[GunId].sTemp.ucIdtag);
			//发送启动鉴权报文
			TcpSendControl(&g_sMsg_control.sGun_charge_control.bSend_auth_flag);
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d START: gun id error, ucPile_num = %d\n", __FILE__, __LINE__, g_sStorage_data.ucPile_num);
		}
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d START:HMI set gun id error = %d\n", __FILE__, __LINE__, g_psHMI_data->eGun_id);
	}
}

/**
 * @brief 刷卡板结束充电控制
 * @param
 * @return
 */
static void RfidStopCharge(void)
{
	CHECK_PTR_NULL_NO_RETURN(g_psHMI_data);

	if ((g_psHMI_data->eGun_id == g_sStorage_data.sPublic_data[GUN_A].eGun_id)
			|| (g_psHMI_data->eGun_id == g_sStorage_data.sPublic_data[GUN_B].eGun_id))
	{
		U8 GunId = g_psHMI_data->eGun_id - ((2U * g_sStorage_data.ucPile_num) - 1U);

		if (GunId < GUN_MAX_NUM)
		{
			(void)memcpy(g_sGun_data[GunId].sTemp.ucIdtag, g_psRFID_data->ucId_tag, sizeof(g_sGun_data[GunId].sTemp.ucIdtag));
			g_sGun_data[GunId].sTemp.eStop_Charge_type = RFID_STOP_MODE;
			g_sMsg_control.sGun_charge_control.ucFinish_id = g_sStorage_data.sPublic_data[GunId].eGun_id;
			g_sGun_data[GunId].sTemp.ucAuth_type = STOP_CHARGE_AUTH;
			my_printf(USER_INFO, "%s RFID request stop charge auth id = %s\n", (GunId==GUN_A)?"GUN_A":"GUN_B", g_sGun_data[GunId].sTemp.ucIdtag);
			//发送结束鉴权报文
			TcpSendControl(&g_sMsg_control.sGun_charge_control.bSend_auth_flag);
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d STOP: gun id error, ucPile_num = %d\n", __FILE__, __LINE__, g_sStorage_data.ucPile_num);
		}
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d STOP:HMI set gun id error = %d\n", __FILE__, __LINE__, g_psHMI_data->eGun_id);
	}
}

static void RfidDataProcess(void)
{
	__BSS(SRAM_OC) static U8 s_ucRFID_data[UART_COMM_BUF_LEN] = {0};
	U16 u16RecByteNum = 0;

	u16RecByteNum = RecUartData(RFID_UART, s_ucRFID_data, UART_COMM_BUF_LEN);

	if (UART_COMM_BUF_LEN < u16RecByteNum)
	{
		return;
	}

	CHECK_PTR_NULL_NO_RETURN(g_psRFID_data);
	CHECK_PTR_NULL_NO_RETURN(g_psHMI_data);

	g_psRFID_data->RfidDataParse(s_ucRFID_data, u16RecByteNum);

	if (0U == strlen(g_sPile_data.ucRfid_version))
	{
		(void)memcpy(g_sPile_data.ucRfid_version, g_psRFID_data->ucVersion, sizeof(g_sPile_data.ucRfid_version));
	}

	if (TRUE == g_psRFID_data->bGet_data_flag)
	{
		switch (g_psHMI_data->eHmi_control_type)
		{
		case START_CHARGE:
			RfidStartCharge();
			break;
		case STOP_CHARGE:
			RfidStopCharge();
			break;
		default:
			my_printf(USER_ERROR, "%s:%d HMI control type error\n", __FILE__, __LINE__);
			break;
		}
		g_psRFID_data->bGet_data_flag = FALSE;
	}

	g_unRfid_timeout_cnt = 0;
}

static void Rfid_Parse_Task(void *pvParameter)
{
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
	volatile UBaseType_t uxHighWaterMark;
#endif
	BaseType_t err = pdFALSE;

	while(1)
	{
		if (NULL != Uart_recBinarySemaphore(RFID_UART))
		{
			err = xSemaphoreTake(Uart_recBinarySemaphore(RFID_UART), portMAX_DELAY);
			if (pdPASS == err)
			{
				RfidDataProcess();
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

//60s超时，取消寻卡
static void RfidTimeoutCallback(TimerHandle_t sTimer)
{
	CHECK_PTR_NULL_NO_RETURN(g_psHMI_data);

	g_psRFID_data->bRfid_search_flag = FALSE;
	U8 GunId = g_psHMI_data->eGun_id - ((2U * g_sStorage_data.ucPile_num) - 1U);

	(void)xTimerStop(g_sTimer_rfid, 0);

	if ((GunId == GUN_A) || (GunId == GUN_B))
	{
		g_psHMI_data->ucAuthorize_flag[GunId] = AUTHORIZE_FAILED;
		my_printf(USER_INFO, "cancel RFID search: timeout(60s) auth failed\n");
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d cancel RFID search: timeout(60s) auth id error = %d\n", __FILE__, __LINE__, GunId);
	}
}

/**
 * @brief RFID通讯检测函数
 * @param
 * @return
 */
static void RfidComCheck(void)
{
	S32 iTmp_flag = 0;

	if (g_unRfid_timeout_cnt < 255U)
	{
		g_unRfid_timeout_cnt++;
	}

	if (g_unRfid_timeout_cnt > (DEVICE_TIMEOUT_MS/WORKING_STATUS_TASK_PERIOD_MS))
	{
		(void)GetSigVal(ALARM_ID_RFID_COMM_LOST_ALARM, &iTmp_flag);
		if (iTmp_flag != COMM_LOST)
		{
			//通讯超时
			PileAlarmSet(ALARM_ID_RFID_COMM_LOST_ALARM);
			my_printf(USER_ERROR, "%s:%d trigger RFID comm lost warning\n", __FILE__, __LINE__);
		}
	}
	else
	{
		(void)GetSigVal(ALARM_ID_RFID_COMM_LOST_ALARM, &iTmp_flag);
		if (iTmp_flag == COMM_LOST)
		{
			//恢复
			PileAlarmReset(ALARM_ID_RFID_COMM_LOST_ALARM);
			my_printf(USER_INFO, "GUN_A restore RFID timeout fault\n");
		}
	}
}

static void Rfid_Request_Task(void *pvParameter)
{
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
	volatile UBaseType_t uxHighWaterMark;
#endif
	U8 ucHeart_cnt = 0;

	vTaskDelay(1000);

	while(1)
	{
		//心跳
		if (NULL != g_psRFID_data)
		{
			RfidComCheck();

			//收到hmi启动刷卡，开始记时
			if (TIMER_OPEN_STATUS == g_psRFID_data->eRfid_start_timing_flag)
			{
				my_printf(USER_INFO, "HMI control into search card mode\n");
				(void)xTimerStart(g_sTimer_rfid, 0);
				g_psRFID_data->eRfid_start_timing_flag = TIMER_IDLE_STATUS;
			}
			else if (TIMER_CLOSE_STATUS == g_psRFID_data->eRfid_start_timing_flag)
			{
				my_printf(USER_INFO, "HMI control cancel search card timer\n");
				(void)xTimerStop(g_sTimer_rfid, 0);
				g_psRFID_data->eRfid_start_timing_flag = TIMER_IDLE_STATUS;
			}
			else
			{
				//默认状态不处理
			}
			//收到HMI寻卡指令,持续寻卡
			if (TRUE == g_psRFID_data->bRfid_search_flag)
			{
				//my_printf(USER_INFO, "send RFID search cmd tick = %d\n", xTaskGetTickCount());
				g_psRFID_data->SendRfidSearchRequest();
				vTaskDelay(WORKING_STATUS_TASK_PERIOD_MS);
			}
			if (ucHeart_cnt >= (IDLE_STATUS_TASK_PERIOD_MS/WORKING_STATUS_TASK_PERIOD_MS))
			{
				g_psRFID_data->SendRfidHeartRequest();
				ucHeart_cnt = 0;
			}
			ucHeart_cnt++;
			vTaskDelay(WORKING_STATUS_TASK_PERIOD_MS);
		}
		else
		{
			vTaskDelay(2000);
			my_printf(USER_ERROR, "%s:%d g_psRFID_data = NULL!\n", __FILE__, __LINE__);
		}
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
#endif
	}
}

void Rfid_Init_Task(void * pvParameters)
{
    //外设初始化
	HwRfidInit();
    //外设选择
	RfidModuleSelect();

    vTaskDelay(500);

	taskENTER_CRITICAL();

	if (TRUE == g_sStorage_data.bUse_ad_rfid)
	{
		//寻卡定时器
	    g_sTimer_rfid = xTimerCreate("RfidSerachTimer",         /* Text name. */
									 RFID_TIMER_PERIOD_MS, 		/* Timer period. */
									 pdTRUE,             		/* Enable auto reload. */
									 "RfidSerach",              /* ID is not used. */
									 &RfidTimeoutCallback);   	/* The callback function. */
		//add other rfid_init task here
		(void)xTaskCreate(&Rfid_Request_Task, 	"RFID_REQUEST", 600U/4U, NULL, GENERAL_TASK_PRIO, NULL);

		(void)xTaskCreate(&Rfid_Parse_Task, 	"RFID_PARSE", 800U/4U, NULL, GENERAL_TASK_PRIO, NULL);
	}

	vTaskDelete(NULL);

	taskEXIT_CRITICAL();
}
