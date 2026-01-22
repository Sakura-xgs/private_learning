/*
 * meter.c
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "cr_section_macros.h"
#include "calculate_crc.h"

#include "AKR_meter.h"
#include "DH_meter.h"
#include "meter.h"
#include "hal_uart_IF.h"
#include "uart_comm.h"
#include "tcp_client_IF.h"
#include "SignalManage.h"
#include "emergency_fault_IF.h"
#include "charge_process_IF.h"
#include "imd_IF.h"

U32 g_uiNow_tick_meter[2] = {0};
Meter_data_t *g_psMeter_data = NULL;
static BOOL g_bMeter_send_flag = TRUE;

/**
 * @brief 兼容性函数，用于选择不同厂家的控制函数
 * @param
 * @return
 */
static void MeterModuleSelect(void)
{
	//获取METER型号
	S32 uiModule_flag = 0;

	(void)GetSigVal(CCU_SET_SIG_ID_METER_MODEL_A, &uiModule_flag);

	if (uiModule_flag == AKR_METER_MODULE_FLAG)
	{
		g_psMeter_data = GetAkrMeterModel();
		my_printf(USER_INFO, "get meter config model: AKR\n");
	}
	else if (uiModule_flag == DH_METER_MODULE_FLAG)
	{
		g_psMeter_data = GetDhMeterModel();
		my_printf(USER_INFO, "get meter config model: DH\n");
	}
	else
	{
		g_psMeter_data = GetAkrMeterModel();
		my_printf(USER_INFO, "use default meter model: AKR\n");
	}
}

/**
 * @brief 电表数据发送函数，防止总线数据冲突，等待收到响应后再次发送
 * @param uart: 串口
 * @param byte_num:数据长度
 * @param buff：数据
 * @return
 */
void MeterUartSend(const UART_LIST uart, const U32 byte_num, U8 *buff)
{
	CHECK_PTR_NULL_NO_RETURN(buff);

	U8 ucCnt = 0;
	//电表发送标志
	g_bMeter_send_flag = FALSE;

	U16 unCrc_temp = CalCrc16(buff, (U16)(byte_num - CRC_LEN));
	buff[CRC_OFFSET] = (U8)(unCrc_temp >> 8);
	buff[CRC_OFFSET+1U] = (U8)(unCrc_temp & 0xFFU);
	Uart_Dma_Send(uart, byte_num, buff);

	//等待数据响应
	while(ucCnt < (METER_SEND_TIMEOUT/METER_WAIT_TIMEOUT))
	{
		//接受到响应
		if(g_bMeter_send_flag)
		{
			break;
		}
		vTaskDelay(METER_WAIT_TIMEOUT);
		ucCnt++;
	}

	return;
}

/**
 * @brief 浮点数转换
 * @param ucBuf_A：数据
 * @param ucArrIndex：有效数据下标
 * @return
 */
float gHexToFloat(const S8 *cBuf_A, U8 ucArrIndex)
{
	if (NULL == cBuf_A)
	{
		return 0.0f;
	}

	U8 ucValueTransferByte[4] = {0x66,0x66,0x3E,0x41};
	float fValueTransfer = 0.0f;

	for (U8 i = 0; i < 4U; i++)
	{
		ucValueTransferByte[i] = (U8)cBuf_A[ucArrIndex + 3U - i];
	}

	(void)memcpy(&fValueTransfer, ucValueTransferByte, 4);

	return fValueTransfer;
}

/**
 * @brief 电表通讯检测函数
 * @param
 * @return
 */
static void MeterComCheck(void)
{
	if ((xTaskGetTickCount() - g_uiNow_tick_meter[GUN_A]) > DEVICE_TIMEOUT_MS)
	{
		//置位通讯超时
		if (g_sGun_fault[GUN_A].sGeneral_fault.sItem.meter_comm_lost != (U8)TRUE)
		{
			GunAlarmSet(&g_sGun_fault[GUN_A].sGeneral_fault.uiWhole_flag, BIT32_STRUCT, FAULT_METER_COMM_LOST);
			my_printf(USER_ERROR, "%s:%d GUN_A:trigger meter comm timeout\n", __FILE__, __LINE__);
		}
	}
	else
	{
		//恢复
		if (g_sGun_fault[GUN_A].sGeneral_fault.sItem.meter_comm_lost != (U8)FALSE)
		{
			GunAlarmReset(&g_sGun_fault[GUN_A].sGeneral_fault.uiWhole_flag, BIT32_STRUCT, FAULT_METER_COMM_LOST);
			my_printf(USER_INFO, "GUN_A:restore trigger meter comm timeout\n");
		}
	}

	if ((xTaskGetTickCount() - g_uiNow_tick_meter[GUN_B]) > DEVICE_TIMEOUT_MS)
	{
		//置位通讯超时
		if (g_sGun_fault[GUN_B].sGeneral_fault.sItem.meter_comm_lost != (U8)TRUE)
		{
			GunAlarmSet(&g_sGun_fault[GUN_B].sGeneral_fault.uiWhole_flag, BIT32_STRUCT, FAULT_METER_COMM_LOST);
			my_printf(USER_ERROR, "%s:%d GUN_B:trigger meter comm timeout\n", __FILE__, __LINE__);
		}
	}
	else
	{
		//恢复
		if (g_sGun_fault[GUN_B].sGeneral_fault.sItem.meter_comm_lost != (U8)FALSE)
		{
			GunAlarmReset(&g_sGun_fault[GUN_B].sGeneral_fault.uiWhole_flag, BIT32_STRUCT, FAULT_METER_COMM_LOST);
			my_printf(USER_INFO, "GUN_B:restore trigger meter comm timeout\n");
		}
	}
}

static void Meter_Request_Task(void *pvParameter)
{
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
	volatile UBaseType_t uxHighWaterMark;
#endif
	U8 ucTrigger_cnt = (IDLE_STATUS_TASK_PERIOD_MS/WORKING_STATUS_TASK_PERIOD_MS);
	//上电立刻读一次版本
	U8 ucMeter_cnt[2] = {ucTrigger_cnt, ucTrigger_cnt};

    //获取当前的tick数
	g_uiNow_tick_meter[GUN_A] = xTaskGetTickCount();
	g_uiNow_tick_meter[GUN_B] = xTaskGetTickCount();

	while(1)
	{
		if (NULL != g_psMeter_data)
		{
			MeterComCheck();
			//空闲查询频率降低
			if ((U8)UNPLUGGED == g_sGun_data[GUN_A].ucConnect_status)
			{
				if (ucMeter_cnt[GUN_A] >= ucTrigger_cnt)
				{
					g_psMeter_data->MeterDataRequest(METER_ADD_A);
					ucMeter_cnt[GUN_A] = 0;
				}
				ucMeter_cnt[GUN_A]++;
			}
			else
			{
				g_psMeter_data->MeterDataRequest(METER_ADD_A);
			}
			//两个设备之间间隔读取
			vTaskDelay(30);
			if ((U8)UNPLUGGED == g_sGun_data[GUN_B].ucConnect_status)
			{
				if (ucMeter_cnt[GUN_B] >= ucTrigger_cnt)
				{
					g_psMeter_data->MeterDataRequest(METER_ADD_B);
					ucMeter_cnt[GUN_B] = 0;
				}
				ucMeter_cnt[GUN_B]++;
			}
			else
			{
				g_psMeter_data->MeterDataRequest(METER_ADD_B);
			}

			vTaskDelay(WORKING_STATUS_TASK_PERIOD_MS);

			my_printf(USER_DEBUG, "GUN_A imd_vol=%dV meter_vol=%dV meter_cur=%dA power=%d GUN_B:imd_vol=%dV meter_vol=%dV meter_cur=%dA power=%d\n",
					g_psIMD_data->sPublic_data[GUN_A].unBus_vol/10U, g_sGun_data[GUN_A].uiOutput_vol/100U,
					g_sGun_data[GUN_A].uiOutput_cur/100U, g_sGun_data[GUN_A].uiOutput_power,
					g_psIMD_data->sPublic_data[GUN_B].unBus_vol/10U, g_sGun_data[GUN_B].uiOutput_vol/100U,
					g_sGun_data[GUN_B].uiOutput_cur/100U, g_sGun_data[GUN_B].uiOutput_power);
		}
		else
		{
			vTaskDelay(2000);
			my_printf(USER_ERROR, "%s:%d g_psIMD_data = NULL!\n", __FILE__, __LINE__);
		}
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
			uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
#endif
	}
}

static void MeterDataProcess(void)
{
	__BSS(SRAM_OC) static U8 s_ucMeter_data[UART_COMM_BUF_LEN] = {0};
	U16 u16RecByteNum = 0;

	u16RecByteNum = RecUartData(METER_UART, s_ucMeter_data, UART_COMM_BUF_LEN);

	if (UART_COMM_BUF_LEN < u16RecByteNum)
	{
		return;
	}

	//接收到数据，置位发送标志位
	g_bMeter_send_flag = TRUE;

	CHECK_PTR_NULL_NO_RETURN(g_psMeter_data);

	//解析
	g_psMeter_data->MeterDataParse(s_ucMeter_data, u16RecByteNum);

	if (GUN_A == g_psMeter_data->ucDevice_switch_flag)
	{
		//输出电压
		g_sGun_data[GUN_A].uiOutput_vol = g_psMeter_data->sPublic_data[GUN_A].uiDc_vol;
		//输出电流
		g_sGun_data[GUN_A].uiOutput_cur = g_psMeter_data->sPublic_data[GUN_A].uiDc_cur;
		//输出功率
		g_sGun_data[GUN_A].uiOutput_power = g_psMeter_data->sPublic_data[GUN_A].uiDc_pwr;
		//当前电量
		g_sGun_data[GUN_A].uiCurrent_meter_dn = g_psMeter_data->sPublic_data[GUN_A].uiPos_act_dn;
	}
	else if (GUN_B == g_psMeter_data->ucDevice_switch_flag)
	{
		//输出电压
		g_sGun_data[GUN_B].uiOutput_vol = g_psMeter_data->sPublic_data[GUN_B].uiDc_vol;
		//输出电流
		g_sGun_data[GUN_B].uiOutput_cur = g_psMeter_data->sPublic_data[GUN_B].uiDc_cur;
		//输出功率
		g_sGun_data[GUN_B].uiOutput_power = g_psMeter_data->sPublic_data[GUN_B].uiDc_pwr;
		//当前电量
		g_sGun_data[GUN_B].uiCurrent_meter_dn = g_psMeter_data->sPublic_data[GUN_B].uiPos_act_dn;
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d ucDevice_switch_flag error = %d\n", __FILE__, __LINE__, g_psMeter_data->ucDevice_switch_flag);
	}
}

static void Meter_Parse_Task(void *parameter)
{
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
	volatile UBaseType_t uxHighWaterMark;
#endif
	BaseType_t err = pdFALSE;

	while(1)
	{
		if (NULL != Uart_recBinarySemaphore(METER_UART))
		{
			err = xSemaphoreTake(Uart_recBinarySemaphore(METER_UART), portMAX_DELAY);
			if (pdPASS == err)
			{
				MeterDataProcess();
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

void Meter_Init_Task(void * pvParameters)
{
    //外设初始化
	AkrMeterInit();
	DhMeterInit();
	//外设选择
	MeterModuleSelect();
    vTaskDelay(500);
	taskENTER_CRITICAL();

	//add other meter_init task here
    (void)xTaskCreate(&Meter_Request_Task,	"METER_REQUEST", 600U/4U, NULL, GENERAL_TASK_PRIO, NULL);

    (void)xTaskCreate(&Meter_Parse_Task,	"METER_PARSE",	800U/4U, NULL, GENERAL_TASK_PRIO, NULL);

	vTaskDelete(NULL);

	taskEXIT_CRITICAL();
}
