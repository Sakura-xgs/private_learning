/*
 * AKR_meter.c
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "FreeRTOS.h"
#include "task.h"

#include "AKR_meter.h"
#include "meter.h"
#include "calculate_crc.h"
#include "hal_uart_IF.h"
#include "emergency_fault_IF.h"
#include "uart_comm.h"

__BSS(SRAM_OC) static Meter_data_t g_sAKR_Meter = {0};
//AKR_private_alarm_t g_sAKR_private_alarm = {0};
__BSS(SRAM_OC) static AKR_Receive_Data_t g_sAKR_receive_data = {0};
__BSS(SRAM_OC) static AKR_Receive_Version_t g_sAKR_receive_version = {0};
__BSS(SRAM_OC)static  AKR_Receive_Energy_t g_sAKR_receive_energy = {0};

/**
 * @brief 电表发送请求数据
 * @param ucAddr；电表地址
 * @return
 */
static void AkrMeterRequest(U8 ucAddr)
{
	static U8 ucCnt[2] = {0};

	if (ucAddr == METER_ADD_A)
	{
		if (0U != g_sAKR_Meter.sPublic_data[GUN_A].unMeter_version)
		{
			//高速采集电压、电流、功率寄存器
			U8 ucBuf_A[] = {METER_ADD_A, AKR_READ_CMD, FAST_AKR_START_REG_ADD_H, FAST_AKR_START_REG_ADD_L, 0, FAST_AKR_READ_REG_NUM, 0, 0};
			MeterUartSend(METER_UART, sizeof(ucBuf_A) , ucBuf_A);
			//减少低俗寄存器的读取频率
			if (ucCnt[GUN_A] > 4)
			{
				vTaskDelay(50);
				//电量
				U8 ucEnergy_A[] = {METER_ADD_A, AKR_READ_CMD, AKR_ENERGY_REG_ADD_H, AKR_ENERGY_REG_ADD_L, 0, AKR_ENERGY_REG_NUM, 0, 0};
				MeterUartSend(METER_UART, sizeof(ucEnergy_A) , ucEnergy_A);
				ucCnt[GUN_A] = 0;
			}
			ucCnt[GUN_A]++;
		}
		else
		{
			U8 ucBuf_A[] = {METER_ADD_A, AKR_READ_CMD, AKR_VERSION_REG_ADD_H, AKR_VERSION_REG_ADD_L, 0, AKR_VERSION_REG_NUM, 0, 0};
			MeterUartSend(METER_UART, sizeof(ucBuf_A) , ucBuf_A);
		}
	}
	else if (ucAddr == METER_ADD_B)
	{
		if (0U != g_sAKR_Meter.sPublic_data[GUN_B].unMeter_version)
		{
			//高速采集电压、电流、功率寄存器
			U8 ucBuf_B[] = {METER_ADD_B, AKR_READ_CMD, FAST_AKR_START_REG_ADD_H, FAST_AKR_START_REG_ADD_L, 0, FAST_AKR_READ_REG_NUM, 0, 0};
			MeterUartSend(METER_UART, sizeof(ucBuf_B) , ucBuf_B);
			//减少低俗寄存器的读取频率
			if (ucCnt[GUN_B] > 4)
			{
				vTaskDelay(50);
				//电量
				U8 ucEnergy_B[] = {METER_ADD_B, AKR_READ_CMD, AKR_ENERGY_REG_ADD_H, AKR_ENERGY_REG_ADD_L, 0, AKR_ENERGY_REG_NUM, 0, 0};
				MeterUartSend(METER_UART, sizeof(ucEnergy_B) , ucEnergy_B);
				ucCnt[GUN_B] = 0;
			}
			ucCnt[GUN_B]++;
		}
		else
		{
			U8 ucBuf_B[] = {METER_ADD_B, AKR_READ_CMD, AKR_VERSION_REG_ADD_H, AKR_VERSION_REG_ADD_L, 0, AKR_VERSION_REG_NUM, 0, 0};
			MeterUartSend(METER_UART, sizeof(ucBuf_B) , ucBuf_B);
		}
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d ucAddr error:%d\n", __FILE__, __LINE__, ucAddr);
	}
}

/**
 * @brief 电表版本响应解析
 * @param
 * @return
 */
static void MeterVersionParse(void)
{
	if (AKR_READ_CMD == g_sAKR_receive_version.ucCmd)
	{
		if (METER_ADD_A == g_sAKR_receive_version.ucSlave_addr)
		{
			//A电表地址自检完成
			if ((U16)FALSE == g_sSelf_check.sItem.meter_comm_flag_A)
			{
				g_sSelf_check.sItem.meter_comm_flag_A = (U16)TRUE;
			}
			//计时复位
			g_uiNow_tick_meter[GUN_A] = xTaskGetTickCount();
			//置位解析标志位为A电表
			g_sAKR_Meter.ucDevice_switch_flag = GUN_A;
			(void)memcpy(&g_sAKR_Meter.sPublic_data[GUN_A].unMeter_version, &g_sAKR_receive_version.unVersion, sizeof(g_sAKR_Meter.sPublic_data[GUN_A].unMeter_version));
		}
		else if (METER_ADD_B == g_sAKR_receive_version.ucSlave_addr)
		{
			//B电表地址自检完成
			if ((U16)FALSE == g_sSelf_check.sItem.meter_comm_flag_B)
			{
				g_sSelf_check.sItem.meter_comm_flag_B = (U16)TRUE;
			}
			//计时复位
			g_uiNow_tick_meter[GUN_B] = xTaskGetTickCount();
			//置位解析标志位为B电表
			g_sAKR_Meter.ucDevice_switch_flag = GUN_B;
			(void)memcpy(&g_sAKR_Meter.sPublic_data[GUN_B].unMeter_version, &g_sAKR_receive_version.unVersion, sizeof(g_sAKR_Meter.sPublic_data[GUN_B].unMeter_version));
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d ucSlave_addr error:%d\n", __FILE__, __LINE__, g_sAKR_receive_version.ucSlave_addr);
		}
	}
}

/**
 * @brief 电表电压、电流、电量解析
 * @param
 * @return
 */
static void MeterRequestDataParse(void)
{
	if (AKR_READ_CMD == g_sAKR_receive_data.ucCmd)
	{
		//处理预充时出现微量负值电流
		S32 sTemp_cur = (gHexToFloat(g_sAKR_receive_data.cCur,0) * 100.0f);
		if (METER_ADD_A == g_sAKR_receive_data.ucSlave_addr)
		{
			//计时复位
			g_uiNow_tick_meter[GUN_A] = xTaskGetTickCount();
			//置位解析标志位为A电表
			g_sAKR_Meter.ucDevice_switch_flag = GUN_A;
			g_sAKR_Meter.sPublic_data[GUN_A].uiDc_vol = (U32)(gHexToFloat(g_sAKR_receive_data.cVol,0) * 100.0f);
			g_sAKR_Meter.sPublic_data[GUN_A].uiDc_cur = (U32)((sTemp_cur > 0)?sTemp_cur:-sTemp_cur);
			g_sAKR_Meter.sPublic_data[GUN_A].uiDc_pwr = (U32)(gHexToFloat(g_sAKR_receive_data.cPower,0) * 10000.0f);
		}
		else if (METER_ADD_B == g_sAKR_receive_data.ucSlave_addr)
		{
			//计时复位
			g_uiNow_tick_meter[GUN_B] = xTaskGetTickCount();
			//置位解析标志位为B电表
			g_sAKR_Meter.ucDevice_switch_flag = GUN_B;
			g_sAKR_Meter.sPublic_data[GUN_B].uiDc_vol = (U32)(gHexToFloat(g_sAKR_receive_data.cVol,0) * 100.0f);
			g_sAKR_Meter.sPublic_data[GUN_B].uiDc_cur = (U32)((sTemp_cur > 0)?sTemp_cur:-sTemp_cur);
			g_sAKR_Meter.sPublic_data[GUN_B].uiDc_pwr = (U32)(gHexToFloat(g_sAKR_receive_data.cPower,0) * 10000.0f);
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d ucCmd error:%d\n", __FILE__, __LINE__, g_sAKR_receive_data.ucCmd);
		}
	}
}

/**
 * @brief 电表功率解析
 * @param
 * @return
 */
static void MeterRequestEnergyParse(void)
{
	if (AKR_READ_CMD == g_sAKR_receive_energy.ucCmd)
	{
		if (METER_ADD_A == g_sAKR_receive_energy.ucSlave_addr)
		{
			//计时复位
			g_uiNow_tick_meter[GUN_A] = xTaskGetTickCount();
			//置位解析标志位为A电表
			g_sAKR_Meter.ucDevice_switch_flag = GUN_A;
			g_sAKR_Meter.sPublic_data[GUN_A].uiPos_act_dn = (U32)((((U32)g_sAKR_receive_energy.cEnergy[0] << 24UL) | ((U32)g_sAKR_receive_energy.cEnergy[1] << 16UL)
						| ((U32)g_sAKR_receive_energy.cEnergy[2] << 8UL) | g_sAKR_receive_energy.cEnergy[3])*10UL);
		}
		else if (METER_ADD_B == g_sAKR_receive_energy.ucSlave_addr)
		{
			//计时复位
			g_uiNow_tick_meter[GUN_B] = xTaskGetTickCount();
			//置位解析标志位为B电表
			g_sAKR_Meter.ucDevice_switch_flag = GUN_B;
			g_sAKR_Meter.sPublic_data[GUN_B].uiPos_act_dn = (U32)((((U32)g_sAKR_receive_energy.cEnergy[0] << 24UL) | ((U32)g_sAKR_receive_energy.cEnergy[1] << 16UL)
						| ((U32)g_sAKR_receive_energy.cEnergy[2] << 8UL) | g_sAKR_receive_energy.cEnergy[3])*10UL);
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d ucSlave_addr error:%d\n", __FILE__, __LINE__, g_sAKR_receive_energy.ucSlave_addr);
		}
	}
}

/**
 * @brief 解析电表数据 数据类型以大端存储,校验位为小端
 * @param pucBuf:待解析数据
 * @param unInput_len：数据长度
 * @return 无
 */
static void AkrMeterParse(const U8 *pucBuf, U16 unInput_len)
{
	//是否是满足标准协议长度
	CHECK_PTR_NULL_NO_RETURN(pucBuf);
	CHECK_MSG_LEN_NO_RETURN(unInput_len, 5U);

	//解析
	U16 unCrc_data = CalCrc16(pucBuf, (U16)(unInput_len - CRC_LEN));
	unCrc_data = (((unCrc_data << 8) & 0xFF00U) | ((unCrc_data >> 8) & 0xFFU));

	switch (unInput_len)
	{
	case VERSION_DATA_LEN:
		(void)memcpy(&g_sAKR_receive_version, pucBuf, unInput_len);
		if (unCrc_data == g_sAKR_receive_version.unCrc)
		{
			MeterVersionParse();
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d unInput_len CRC error\n", __FILE__, __LINE__);
		}
		break;
	case REQUEST_DATA_LEN:
		(void)memcpy(&g_sAKR_receive_data, pucBuf, unInput_len);
		if (unCrc_data == g_sAKR_receive_data.unCrc)
		{
			MeterRequestDataParse();
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d unInput_len CRC error\n", __FILE__, __LINE__);
		}
		break;
	case REQUEST_ENERGY_LEN:
		(void)memcpy(&g_sAKR_receive_energy, pucBuf, unInput_len);
		if (unCrc_data == g_sAKR_receive_energy.unCrc)
		{
			MeterRequestEnergyParse();
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d unInput_len CRC error\n", __FILE__, __LINE__);
		}
		break;
	default:
		my_printf(USER_ERROR, "%s:%d unInput_len error:%d\n", __FILE__, __LINE__, unInput_len);

		break;
	}
}

Meter_data_t* GetAkrMeterModel(void)
{
	static AKR_Fault_t g_sAKR_fault[2] = {0};

	//电表同步安科瑞
	g_pMeter_fault[GUN_A] = (U8 *)&g_sAKR_fault[GUN_A];
	g_pMeter_fault[GUN_B] = (U8 *)&g_sAKR_fault[GUN_B];
	return &g_sAKR_Meter;
}

void AkrMeterInit(void)
{
	g_sAKR_Meter.MeterDataRequest = &AkrMeterRequest;
	g_sAKR_Meter.MeterDataParse = &AkrMeterParse;
}
