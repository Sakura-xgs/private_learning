/*
 * DH_meter.c
 *
 *  Created on: 2024年10月21日
 *      Author: qjwu
 */
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "FreeRTOS.h"
#include "task.h"

#include "DH_meter.h"
#include "meter.h"
#include "calculate_crc.h"
#include "hal_uart_IF.h"
#include "uart_comm.h"

__BSS(SRAM_OC) static Meter_data_t g_sDH_Meter = {0};
//DH_private_alarm_t g_sDH_private_alarm = {0};

/**
 * @brief 电表发送请求数据
 * @param ucAddr；电表地址
 * @return
 */
static void DhMeterRequest(U8 ucAddr)
{
	if (ucAddr == METER_ADD_A)
	{
		U8 ucBuf_A[] = {METER_ADD_A, DH_READ_CMD, DH_START_REG_ADD_H, DH_START_REG_ADD_L, 0, DH_READ_REG_NUM, 0, 0};
		MeterUartSend(METER_UART, sizeof(ucBuf_A) , ucBuf_A);
	}
	else if (ucAddr == METER_ADD_B)
	{
		U8 ucBuf_B[] = {METER_ADD_B, DH_READ_CMD, DH_START_REG_ADD_H, DH_START_REG_ADD_L, 0, DH_READ_REG_NUM, 0, 0};
		MeterUartSend(METER_UART, sizeof(ucBuf_B) , ucBuf_B);
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d meter address error\n", __FILE__, __LINE__);
	}
}

/**
 * @brief 解析电表数据 数据类型以大端存储,校验位为小端
 * @param pucBuf:待解析数据
 * @param unInput_len：数据长度
 * @return 无
 */
static void DhMeterParse(const U8 *pucBuf, U16 unInput_len)
{
	//是否是满足标准协议长度
	CHECK_MSG_LEN_NO_RETURN(unInput_len, 5U);

	//解析
	U16 unCrc_data = CalCrc16(pucBuf, (U16)(unInput_len - CRC_LEN));
	//数据长度+地址1字节+cmd1字节+长度本身占1字节
	U8 ucCrc_offset = pucBuf[DATA_LEN_INDEX] + 3U;

	if ((((unCrc_data >> 8U) & 0xFFU) == pucBuf[ucCrc_offset]) && ((unCrc_data & 0xFFU) == pucBuf[ucCrc_offset+1U]))
	{
		if (METER_ADD_A == pucBuf[ADDR_INDEX])
		{
			if (DH_READ_CMD == pucBuf[CMD_INDEX])
			{
				//A电表地址自检完成
				if ((U16)FALSE == g_sSelf_check.sItem.meter_comm_flag_A)
				{
					g_sSelf_check.sItem.meter_comm_flag_A = (U16)TRUE;
				}
				//计时复位
				g_uiNow_tick_meter[GUN_A] = xTaskGetTickCount();
				//置位解析标志位为A电表
				g_sDH_Meter.ucDevice_switch_flag = GUN_A;
				//数据长度+地址1字节+cmd1字节+长度本身占1字节+crc2字节
				CHECK_MSG_LEN_NO_RETURN((U16)pucBuf[DATA_LEN_INDEX]+5U, unInput_len);
				if (unInput_len > (U16)DH_ENERGY_INDEX)
				{
					g_sDH_Meter.sPublic_data[GUN_A].uiDc_vol = (U32)gHexToFloat(pucBuf,DH_VOL_INDEX) * 100U;
					g_sDH_Meter.sPublic_data[GUN_A].uiDc_cur = (U32)gHexToFloat(pucBuf,DH_CUR_INDEX) * 100U;
					g_sDH_Meter.sPublic_data[GUN_A].uiDc_pwr = (U32)gHexToFloat(pucBuf,DH_POWER_INDEX) * 10U;
					g_sDH_Meter.sPublic_data[GUN_A].uiPos_act_dn = (U32)gHexToFloat(pucBuf,DH_ENERGY_INDEX) * 10000U;
				}

//				g_sDH_private_alarm.bCurrent_alarm_flag_A = (pucBuf[DH_CURRENT_ALARM_FLAG_INDEX] << 8)
//											| (pucBuf[DH_CURRENT_ALARM_FLAG_INDEX+1]);
			}
		}
		else if (METER_ADD_B == pucBuf[ADDR_INDEX])
		{
			if (DH_READ_CMD == pucBuf[CMD_INDEX])
			{
				//B电表地址自检完成
				if ((U16)FALSE == g_sSelf_check.sItem.meter_comm_flag_B)
				{
					g_sSelf_check.sItem.meter_comm_flag_B = (U16)TRUE;
				}
				//计时复位
				g_uiNow_tick_meter[GUN_B] = xTaskGetTickCount();
				//置位解析标志位为B电表
				g_sDH_Meter.ucDevice_switch_flag = GUN_B;
				//数据长度+地址1字节+cmd1字节+长度本身占1字节+crc2字节
				CHECK_MSG_LEN_NO_RETURN((U16)pucBuf[DATA_LEN_INDEX]+5U, unInput_len);

				if (unInput_len > (U16)DH_ENERGY_INDEX)
				{
					g_sDH_Meter.sPublic_data[GUN_B].uiDc_vol = (U32)gHexToFloat(pucBuf,DH_VOL_INDEX) * 100U;
					g_sDH_Meter.sPublic_data[GUN_B].uiDc_cur = (U32)gHexToFloat(pucBuf,DH_CUR_INDEX) * 100U;
					g_sDH_Meter.sPublic_data[GUN_B].uiDc_pwr = (U32)gHexToFloat(pucBuf,DH_POWER_INDEX) * 10U;
					g_sDH_Meter.sPublic_data[GUN_B].uiPos_act_dn = (U32)gHexToFloat(pucBuf,DH_ENERGY_INDEX) * 10000U;
				}

				//g_sDH_private_alarm.bCurrent_alarm_flag_B = (pucBuf[DH_CURRENT_ALARM_FLAG_INDEX] << 8)
				//							| (pucBuf[DH_CURRENT_ALARM_FLAG_INDEX+1]);
			}
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d meter data type error\n", __FILE__, __LINE__);
		}
	}
}

Meter_data_t* GetDhMeterModel(void)
{
	return &g_sDH_Meter;
}

void DhMeterInit(void)
{
	g_sDH_Meter.MeterDataRequest = &DhMeterRequest;
	g_sDH_Meter.MeterDataParse = &DhMeterParse;
}
