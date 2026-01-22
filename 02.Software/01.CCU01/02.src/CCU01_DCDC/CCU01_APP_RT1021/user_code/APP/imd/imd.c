/*
 * iso.c
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

#include "GY_imd.h"
#include "imd.h"
#include "hal_uart_IF.h"
#include "uart_comm.h"
#include "tcp_client_IF.h"
#include "SignalManage.h"
#include "emergency_fault_IF.h"

U32 g_uiNow_tick_imd[GUN_MAX_NUM] = {0};
IMD_data_t *g_psIMD_data = NULL;
static BOOL g_bImd_Send_flag = 0;
static BOOL s_bImd_version_get[GUN_MAX_NUM] = {FALSE, FALSE};

/**
 * @brief 兼容性函数，用于选择不同厂家的控制函数
 * @param
 * @return
 */
static void ImdModuleSelect(void)
{
	//获取IMD型号
	S32 uiModule_flag = 0;

	(void)GetSigVal(CCU_SET_SIG_ID_IMD_MODEL_A, &uiModule_flag);

	if (GY_IMD_MODULE_FLAG == uiModule_flag)
	{
		g_psIMD_data = GetGYImdModel();
		my_printf(USER_INFO, "get imd config model: GY\n");
	}
	else
	{
		g_psIMD_data = GetGYImdModel();
		my_printf(USER_INFO, "use default model :GY\n");
	}
}

/**
 * @brief 绝缘数据发送函数，防止总线数据冲突，等待收到响应后再次发送
 * @param uart: 串口
 * @param byte_num:数据长度
 * @param buff：数据
 * @return
 */
void ImdUartSend(const UART_LIST uart, const U32 byte_num, U8 *buff)
{
	CHECK_PTR_NULL_NO_RETURN(buff);

	U8 ucCnt = 0;
	g_bImd_Send_flag = FALSE;

	U16 unCrc_temp = CalCrc16(buff, (U16)(byte_num - CRC_LEN));
	buff[CRC_OFFSET] = (U8)(unCrc_temp >> 8);
	buff[CRC_OFFSET+1U] = (U8)(unCrc_temp & 0xFFU);

	Uart_Dma_Send(uart, byte_num, buff);

	//等待数据响应
	while(ucCnt < (IMD_SEND_TIMEOUT/IMD_WAIT_TIMEOUT))
	{
		if(g_bImd_Send_flag)
		{
			break;
		}
		vTaskDelay(IMD_WAIT_TIMEOUT);
		ucCnt++;
	}

	return;
}

/**
 * @brief 绝缘监测板通讯检测函数
 * @param
 * @return
 */
static void ImdComCheck(Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return;
	}

	if ((xTaskGetTickCount() - g_uiNow_tick_imd[eGun_id]) > DEVICE_TIMEOUT_MS)
	{
		//置位通讯超时
		if (g_sGun_fault[eGun_id].sGeneral_fault.sItem.imd_comm_lost != TRUE)
		{
			GunAlarmSet(&g_sGun_fault[eGun_id].sGeneral_fault.uiWhole_flag, BIT32_STRUCT, FAULT_IMD_COMM_LOST);
			my_printf(USER_ERROR, "%s:%d %s trigger IMD timeout fault\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		}
	}
	else
	{
		//恢复
		if (g_sGun_fault[eGun_id].sGeneral_fault.sItem.imd_comm_lost != FALSE)
		{
			GunAlarmReset(&g_sGun_fault[eGun_id].sGeneral_fault.uiWhole_flag, BIT32_STRUCT, FAULT_IMD_COMM_LOST);
			my_printf(USER_INFO, "%s restore IMD timeout fault\n", (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		}
	}
}

/**
 * @brief 绝缘监测板控制函数
 * @param
 * @return
 */
static void GunImdControl(const Gun_Id_e eGun_id, U8 ucDevice_addr)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return;
	}

	//设置为自动电桥模式
	if (g_psIMD_data->ucImd_mode[eGun_id] != GY_AUTO_MODE)
	{
		g_psIMD_data->ImdControl(AUTO_MODE_CHANGE, ucDevice_addr);
	}

	if (IMD_ENABLE == g_psIMD_data->sPublic_data[eGun_id].eCheck_flag)//启动绝缘监测
	{
		if (TRUE != g_psIMD_data->sPublic_data[eGun_id].bImd_enable_status)
		{
			if (g_psIMD_data->sPublic_data[eGun_id].unBus_vol > IMD_WORKING_VOL)
			{
				g_psIMD_data->ImdControl(IMD_ENABLE, ucDevice_addr);
				vTaskDelay(50);
			}
			else
			{
				my_printf(USER_DEBUG, "%s bus bar < 100V\n", (eGun_id==GUN_A)?"GUN_A":"GUN_B");
			}
		}
	}
	//关闭绝缘监测
	else
	{
		if (FALSE != g_psIMD_data->sPublic_data[eGun_id].bImd_enable_status)
		{
			g_psIMD_data->ImdControl(IMD_DISABLE, ucDevice_addr);
			vTaskDelay(50);
		}
	}

	g_psIMD_data->ImdControl(IMD_CHECK_DATA, ucDevice_addr);
}

/**
 * @brief 绝缘阻值判定
 * @param ucTemp_positive: 正极绝缘阻值
 * @param ucTemp_negative：负极绝缘阻值
 * @param eGun_id：枪id
 * @return
 */
static void ImdDataCheck(U32 uiTemp_positive, U32 uiTemp_negative, const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	//故障
	if (uiTemp_positive < MIN_INSULATION_VALUE)
	{
		g_sGun_data[eGun_id].sTemp.ucImd_status = IMD_CHECK_FAULT;
		if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.positive_insulation != (U8)TRUE)
		{
			GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_POSITIVE_INSULATION);
			GunAlarmReset(&g_sGun_warn[eGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_POSITIVE_INSULATION);
			my_printf(USER_ERROR, "%s:%d %s positive IMD check fault <100 OM/V\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		}
	}
	//告警
	else if ((uiTemp_positive >= MIN_INSULATION_VALUE) && (uiTemp_positive < NORMAL_INSULATION_VALUE))
	{
		g_sGun_data[eGun_id].sTemp.ucImd_status = IMD_CHECK_WARNING;
		if (g_sGun_warn[eGun_id].sRecoverable_warn.sItem.positive_insulation != (U8)TRUE)
		{
			GunAlarmSet(&g_sGun_warn[eGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_POSITIVE_INSULATION);
			my_printf(USER_INFO, "%s positive IMD check warning:(>=100 OM/V, <500 OM/V)\n", (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		}
	}
	//故障
	if (uiTemp_negative < MIN_INSULATION_VALUE)
	{
		g_sGun_data[eGun_id].sTemp.ucImd_status = IMD_CHECK_FAULT;
		if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.negative_insulation != (U8)TRUE)
		{
			GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_NEGATIVE_INSULATION);
			GunAlarmReset(&g_sGun_warn[eGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_NEGATIVE_INSULATION);
			my_printf(USER_ERROR, "%s:%d %s negative IMD check fault <100 OM/V\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		}
	}
	//告警
	else if ((uiTemp_negative >= MIN_INSULATION_VALUE) && (uiTemp_negative < NORMAL_INSULATION_VALUE))
	{
		g_sGun_data[eGun_id].sTemp.ucImd_status = IMD_CHECK_WARNING;
		if (g_sGun_warn[eGun_id].sRecoverable_warn.sItem.negative_insulation != (U8)TRUE)
		{
			GunAlarmSet(&g_sGun_warn[eGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_NEGATIVE_INSULATION);
			my_printf(USER_INFO, "%s negative IMD check warning:(>=100 OM/V, <500 OM/V)\n", (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		}
	}
	//正常
	if ((uiTemp_positive >= NORMAL_INSULATION_VALUE) && (uiTemp_negative >= NORMAL_INSULATION_VALUE))
	{
		g_sGun_data[eGun_id].sTemp.ucImd_status = IMD_CHECK_VALID;
		if (g_sGun_warn[eGun_id].sRecoverable_warn.sItem.positive_insulation != (U8)FALSE)
		{
			GunAlarmReset(&g_sGun_warn[eGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_POSITIVE_INSULATION);
			my_printf(USER_INFO, "%s:%d %s restore positive IMD warning:(>=100 OM/V, <500 OM/V)\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		}
		if (g_sGun_warn[eGun_id].sRecoverable_warn.sItem.negative_insulation != (U8)FALSE)
		{
			GunAlarmReset(&g_sGun_warn[eGun_id].sRecoverable_warn.uiWhole_flag, BIT32_STRUCT, WARNING_NEGATIVE_INSULATION);
			my_printf(USER_INFO, "%s:%d %s restore negative IMD warning:(>=100 OM/V, <500 OM/V)\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		}
		my_printf(USER_DEBUG, "%s IMD check valid: uiTemp_positive = %d uiTemp_negative = %d\n", (eGun_id==GUN_A)?"GUN_A":"GUN_B", uiTemp_positive, uiTemp_negative);
	}
}

/**
 * @brief 绝缘监测不同状态处理
 * @param eGun_id:枪id
 * @return
 */
static void ImdStatusParse(const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	if (FALSE == s_bImd_version_get[eGun_id])
	{
		(void)memcpy(g_sGun_data[eGun_id].ucImd_version, &g_psIMD_data->sPublic_data[eGun_id].ucImd_version, sizeof(g_sGun_data[eGun_id].ucImd_version));
		s_bImd_version_get[eGun_id] = TRUE;
	}

	U32 uiTemp_positive = 0, uiTemp_negative = 0;

	switch (g_psIMD_data->sPublic_data[eGun_id].ucImd_status)
	{
	case IMD_CHECK_INVALID:
		g_sGun_data[eGun_id].sTemp.ucImd_status = IMD_CHECK_INVALID;
		break;
	case IMD_CHECKING:
		g_sGun_data[eGun_id].sTemp.ucImd_status = IMD_CHECKING;
		//my_printf(USER_DEBUG, "%s start IMD check\n", (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		break;
	case IMD_CHECK_VALID:
		//阻值计算：正极
		if (GY_INVALID_RESISTANCE_VALUE != g_psIMD_data->sPublic_data[eGun_id].unPositive_R)
		{
			//检测电阻大于10兆欧
			if (GY_MAX_RESISTANCE_VALUE == g_psIMD_data->sPublic_data[eGun_id].unPositive_R)
			{
				uiTemp_positive = 600U*1000U;
			}
			else
			{
				if ((0U != g_psIMD_data->sPublic_data[eGun_id].unPositive_R) && (0U != g_psIMD_data->sPublic_data[eGun_id].unBus_vol))
				{
					uiTemp_positive = ((U32)g_psIMD_data->sPublic_data[eGun_id].unPositive_R*1000U) / ((U32)g_psIMD_data->sPublic_data[eGun_id].unBus_vol/10U);
				}
			}
		}
		else//无效阻值
		{
			g_sGun_data[eGun_id].sTemp.ucImd_status = IMD_CHECK_FAULT;
			if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.positive_insulation != (U8)TRUE)
			{
				GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_POSITIVE_INSULATION);
				my_printf(USER_ERROR, "%s:%d %s trigger positive IMD check invalid fault\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
			}
		}
		//阻值计算：负极
		if (GY_INVALID_RESISTANCE_VALUE != g_psIMD_data->sPublic_data[eGun_id].unNegative_R)
		{
			//检测电阻大于10兆欧
			if (GY_MAX_RESISTANCE_VALUE == g_psIMD_data->sPublic_data[eGun_id].unNegative_R)
			{
				uiTemp_negative = 600U*1000U;
			}
			else
			{
				if ((0U != g_psIMD_data->sPublic_data[eGun_id].unNegative_R) && (0U != g_psIMD_data->sPublic_data[eGun_id].unBus_vol))
				{
					uiTemp_negative = ((U32)g_psIMD_data->sPublic_data[eGun_id].unNegative_R*1000U) / ((U32)g_psIMD_data->sPublic_data[eGun_id].unBus_vol/10U);
				}
			}
		}
		else//无效阻值
		{
			g_sGun_data[eGun_id].sTemp.ucImd_status = IMD_CHECK_FAULT;
			if (g_sGun_fault[eGun_id].sRecoverable_fault.sItem.negative_insulation != (U8)TRUE)
			{
				GunAlarmSet(&g_sGun_fault[eGun_id].sRecoverable_fault.uiWhole_flag, BIT32_STRUCT, FAULT_NEGATIVE_INSULATION);
				my_printf(USER_ERROR, "%s:%d %s trigger negative IMD check invalid fault\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
			}
		}
		//绝缘阻值判定
		ImdDataCheck(uiTemp_positive, uiTemp_negative, eGun_id);
		break;
	case IMD_CHECK_FAULT:
		//自检失败
		g_sGun_data[eGun_id].sTemp.ucImd_status = IMD_CHECK_FAULT;
		if (g_sGY_fault[eGun_id].sItem.ucImd_self_test != (U8)TRUE)
		{
			GunAlarmSet(&g_sGY_fault[eGun_id].ucWhole_flag, BIT8_STRUCT, IMD_SELF_TEST);
		}
		my_printf(USER_ERROR, "%s:%d %s IMD self-check failed\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		break;
	default:
		my_printf(USER_ERROR, "%s:%d %s IMD check status error\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		break;
	}
}

static void ImdDataProcess(void)
{
	__BSS(SRAM_OC) static U8 s_ucIMD_data[UART_COMM_BUF_LEN] = {0};
	U16 u16RecByteNum = 0;

	u16RecByteNum = RecUartData(IMD_UART, s_ucIMD_data, UART_COMM_BUF_LEN);

	if (UART_COMM_BUF_LEN < u16RecByteNum)
	{
		return;
	}

	if(u16RecByteNum == 0U)
	{
		ReconfigUart7();
		Uart_Dma_Init(IMD_UART);
		Uart_Init(IMD_UART);
		my_printf(USER_ERROR, "%s:%d reconfig imd uart\n", __FILE__, __LINE__);
		return;
	}

	//接收到数据，置位发送标志位
	g_bImd_Send_flag = TRUE;

	CHECK_PTR_NULL_NO_RETURN(g_psIMD_data);

	//解析
	g_psIMD_data->ImdDataParse(s_ucIMD_data, u16RecByteNum);

	switch(g_psIMD_data->eDevice_switch)
	{
	case GUN_A:
		ImdStatusParse(GUN_A);
		break;
	case GUN_B:
		ImdStatusParse(GUN_B);
		break;
	default:
		my_printf(USER_ERROR, "%s:%d ucDevice_switch_flag error = %d\n", __FILE__, __LINE__, g_psIMD_data->eDevice_switch);
		break;
	}
}

static void Imd_Parse_Task(void *pvParameter)
{
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
	volatile UBaseType_t uxHighWaterMark;
#endif
	BaseType_t err = pdFALSE;

	while(1)
	{
		if (NULL != Uart_recBinarySemaphore(IMD_UART))
		{
			err = xSemaphoreTake(Uart_recBinarySemaphore(IMD_UART), portMAX_DELAY);
			if (pdTRUE == err)
			{
				ImdDataProcess();
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

static void Imd_Request_Task(void *pvParameter)
{
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
	volatile UBaseType_t uxHighWaterMark;
#endif
    // 获取当前的tick数
	g_uiNow_tick_imd[GUN_A] = xTaskGetTickCount();
	g_uiNow_tick_imd[GUN_B] = g_uiNow_tick_imd[GUN_A];

	while(1)
	{
		if (NULL != g_psIMD_data)
		{
			ImdComCheck(GUN_A);
			//获取到版本号则查询数据
			if (TRUE != g_sSelf_check.sItem.imd_comm_flag_A)
			{
				g_psIMD_data->ImdControl(IMD_VERSION, IMD_ADD_A);
			}
			else
			{
				GunImdControl(GUN_A, IMD_ADD_A);
			}

			//两个设备之间间隔读取
			vTaskDelay(40);

			ImdComCheck(GUN_B);
			//获取到版本号则查询数据
			if (TRUE != g_sSelf_check.sItem.imd_comm_flag_B)
			{
				g_psIMD_data->ImdControl(IMD_VERSION, IMD_ADD_B);
			}
			else
			{
				GunImdControl(GUN_B, IMD_ADD_B);
			}

			vTaskDelay(WORKING_STATUS_TASK_PERIOD_MS);
		}
		else
		{
			vTaskDelay(2000);
			my_printf(USER_ERROR, "%s:%d  g_psIMD_data = NULL!\n", __FILE__, __LINE__);
		}
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
#endif
	}
}

void Imd_Init_Task(void * pvParameters)
{
    //外设初始化
	GYImdInit();
	//外设选择
	ImdModuleSelect();
    vTaskDelay(500);
	taskENTER_CRITICAL();

	//add other iso_init task here
	(void)xTaskCreate(&Imd_Request_Task, "IMD_REQUEST", 600U/4U, NULL, GENERAL_TASK_PRIO, NULL);

	(void)xTaskCreate(&Imd_Parse_Task, "IMD_PARSE", 800U/4U, NULL, GENERAL_TASK_PRIO, NULL);

	vTaskDelete(NULL);

	taskEXIT_CRITICAL();
}
