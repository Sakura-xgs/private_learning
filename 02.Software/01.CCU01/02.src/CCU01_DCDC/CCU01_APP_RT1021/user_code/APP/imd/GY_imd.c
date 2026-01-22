/*
 * gy_iso.c
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */
#include "GY_imd.h"
#include "imd.h"
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

#include "calculate_crc.h"
#include "hal_uart_IF.h"
#include "uart_comm.h"
#include "emergency_fault_IF.h"

__BSS(SRAM_OC) GY_Fault_t g_sGY_fault[GUN_MAX_NUM] = {0};
__BSS(SRAM_OC) static IMD_data_t g_sGY_IMD = {0};
__BSS(SRAM_OC) static GY_Receive_Data_t g_sGY_receive_data = {0};
__BSS(SRAM_OC) static GY_Receive_Version_t g_sGY_receive_version = {0};
__BSS(SRAM_OC) static GY_Receive_Control_t g_sGY_receive_control = {0};

/**
 * @brief 发送绝缘监测继电器数据读取请求
 * @param ucAddr:从机地址
 * @param ucCmd:cmd
 * @param unReg_addr:寄存器地址
 * @param unReg_data:写入的数据/读取数据的长度
 * @return
*/
static void GYImdSendData(U8 ucAddr, U8 ucCmd, U16 unReg_addr, U16 unReg_data)
{
	U8 ucSend_buf[] = {ucAddr, ucCmd, (U8)((unReg_addr >> 8) & 0xFFU), (U8)(unReg_addr & 0xFFU), (U8)((unReg_data >> 8) & 0xFFU), (U8)(unReg_data & 0xFFU), 0, 0};

	ImdUartSend(IMD_UART, sizeof(ucSend_buf), ucSend_buf);
}

/**
 * @brief 绝缘检测通讯命令控制
 * @param sMode:命令功能
 * @param ucAddr：从机地址
 * @return
*/
static void GYImdControl(IMD_Control_e sMode, U8 ucAddr)
{
	switch (sMode)
	{
	case IMD_VERSION:
		GYImdSendData(ucAddr, GY_READ_CMD, GY_READ_VERSION_START_REG, GY_READ_VERSION_START_REG_LEN);
		break;
	case IMD_CHECK_DATA:
		//读取连续5个寄存器的值：母线电压：10 未使用值：11 正极绝缘阻值：12  负极绝缘阻值：13  状态位：14
		GYImdSendData(ucAddr, GY_READ_CMD, GY_READ_DATA_START_REG, GY_READ_DATA_START_REG_LEN);
		break;
	case FIX_MODE_CHANGE:
		GYImdSendData(ucAddr, GY_WRITE_CMD, GY_CHANGE_MODE_REG, GY_FIX_MODE_PARAM);
		break;
	case AUTO_MODE_CHANGE:
		GYImdSendData(ucAddr, GY_WRITE_CMD, GY_CHANGE_MODE_REG, GY_AUTO_MODE_PARAM);
		break;
	case IMD_ENABLE:
		GYImdSendData(ucAddr, GY_WRITE_CMD, GY_ENABLE_CONTROL_REG, GY_ENABLE_CONTROL_PARAM);
		break;
	case IMD_DISABLE:
		GYImdSendData(ucAddr, GY_WRITE_CMD, GY_ENABLE_CONTROL_REG, GY_DISABLE_CONTROL_PARAM);
		break;
	default:
		my_printf(USER_ERROR, "%s:%d sMode error:%d\n", __FILE__, __LINE__, sMode);
		break;
	}
}

/**
 * @brief 解析枪绝缘监测状态数据以及电阻数据
 * @param eGun_id:枪id
 * @return
*/
static void GYDataParse(const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	//当前工作模式
	if (0U != ((g_sGY_receive_data.ucStatus[0] >> GY_STATUS_IMD_MODE_BIT) & 0x01U))
	{
		g_sGY_IMD.ucImd_mode[eGun_id] = GY_AUTO_MODE;
	}
	else
	{
		g_sGY_IMD.ucImd_mode[eGun_id] = GY_FIXED_MODE;
	}

	//当前状态
	g_sGY_IMD.sPublic_data[eGun_id].bImd_data_valid = (BOOL)((g_sGY_receive_data.ucStatus[1] >> GY_STATUS_IMD_DATA_VALID_BIT) & 0x01U);
	g_sGY_IMD.sPublic_data[eGun_id].bImd_enable_status = (BOOL)((g_sGY_receive_data.ucStatus[1] >> GY_STATUS_IMD_ENABLE_BIT) & 0x01U);
	g_sGY_IMD.sPublic_data[eGun_id].bImd_checkself_status = (BOOL)((g_sGY_receive_data.ucStatus[1] >> GY_STATUS_IMD_SELFCHECK_BIT) & 0x01U);

	//母线状态
	if (0U != ((g_sGY_receive_data.ucStatus[1] >> GY_STATUS_IMD_BUS_BIT) & 0x01U))
	{
		//母线反接故障
		if ((U8)TRUE != g_sGY_fault[eGun_id].sItem.ucImd_bus_bar_reverse)
		{
			GunAlarmSet(&g_sGY_fault[eGun_id].ucWhole_flag, BIT8_STRUCT, IMD_BUS_BAR_REVERSE);
			my_printf(USER_ERROR, "%s:%d %s trigger IMD bus bar reverse fault\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		}
	}
	else
	{
		if ((U8)FALSE != g_sGY_fault[eGun_id].sItem.ucImd_bus_bar_reverse)
		{
			GunAlarmReset(&g_sGY_fault[eGun_id].ucWhole_flag, BIT8_STRUCT, IMD_BUS_BAR_REVERSE);
			my_printf(USER_ERROR, "%s:%d %s restore IMD bus bar reverse fault\n", __FILE__, __LINE__, (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		}
	}
	//总线电压
	g_sGY_IMD.sPublic_data[eGun_id].unBus_vol = (((g_sGY_receive_data.unBus_vol << 8) & 0xFF00U) | ((g_sGY_receive_data.unBus_vol >> 8) & 0xFFU));
	//空闲
	if (g_sGY_IMD.sPublic_data[eGun_id].bImd_checkself_status && (FALSE == g_sGY_IMD.sPublic_data[eGun_id].bImd_enable_status))
	{
		g_sGY_IMD.sPublic_data[eGun_id].ucImd_status = IMD_CHECK_INVALID;
	}
	//自检失败
	if (FALSE == g_sGY_IMD.sPublic_data[eGun_id].bImd_checkself_status)
	{
		g_sGY_IMD.sPublic_data[eGun_id].ucImd_status = IMD_CHECK_FAULT;
	}
	//绝缘监测中
	if (g_sGY_IMD.sPublic_data[eGun_id].bImd_checkself_status && g_sGY_IMD.sPublic_data[eGun_id].bImd_enable_status
			&& (FALSE == g_sGY_IMD.sPublic_data[eGun_id].bImd_data_valid))
	{
		g_sGY_IMD.sPublic_data[eGun_id].ucImd_status = IMD_CHECKING;
	}
	//监测完成，数据有效
	if (g_sGY_IMD.sPublic_data[eGun_id].bImd_checkself_status && g_sGY_IMD.sPublic_data[eGun_id].bImd_data_valid
			&& g_sGY_IMD.sPublic_data[eGun_id].bImd_enable_status)
	{
		g_sGY_IMD.sPublic_data[eGun_id].ucImd_status = IMD_CHECK_VALID;
		//正极绝缘电阻
		g_sGY_IMD.sPublic_data[eGun_id].unPositive_R = (((g_sGY_receive_data.unPositive_R << 8) & 0xFF00U) | ((g_sGY_receive_data.unPositive_R >> 8) & 0xFFU));
		//负极绝缘电阻
		g_sGY_IMD.sPublic_data[eGun_id].unNegative_R = (((g_sGY_receive_data.unNegative_R << 8) & 0xFF00U) | ((g_sGY_receive_data.unNegative_R >> 8) & 0xFFU));
	}
}

/**
 * @brief 解析绝缘监测板的控制响应数据
 * @param eGun_id:枪id
 * @return
*/
static void ImdControlResponse(const Gun_Id_e eGun_id)
{
	if (FALSE == GunIdValidCheck(eGun_id))
	{
		return ;
	}

	//计时复位
	g_uiNow_tick_imd[eGun_id] = xTaskGetTickCount();
	//置位解析标志位
	g_sGY_IMD.eDevice_switch = eGun_id;
	//绝缘检测开启/关闭控制响应
	if ((0x01U == g_sGY_receive_control.ucAddr_H)
			&& (0x02U == g_sGY_receive_control.ucAddr_L))
	{
		if (0x11U == g_sGY_receive_control.ucData_L)
		{
			my_printf(USER_INFO, "%s:receive start IMD control response\n", (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		}
		else if (0U == g_sGY_receive_control.ucData_L)
		{
			my_printf(USER_INFO, "%s:receive stop IMD control response\n", (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d ucData_L error:%d\n", __FILE__, __LINE__, g_sGY_receive_control.ucData_L);
		}
	}
	//绝缘检测模式控制响应
	if ((0x10U == g_sGY_receive_control.ucAddr_H)
			&& (0U == g_sGY_receive_control.ucAddr_L))
	{
		if (0x04U == g_sGY_receive_control.ucData_L)
		{
			my_printf(USER_INFO, "%s:receive switch IMD model response: Auto bridge\n", (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		}
		else if (0U == g_sGY_receive_control.ucData_L)
		{
			my_printf(USER_INFO, "%s:receive switch IMD model response: Fixed bridge\n", (eGun_id==GUN_A)?"GUN_A":"GUN_B");
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d ucData_L error:%d\n", __FILE__, __LINE__, g_sGY_receive_control.ucData_L);
		}
	}
}

/**
 * @brief 解析绝缘监测板的状态数据
 * @param
 * @return
*/
static void ImdRequestDataParse(void)
{
	if (GY_READ_CMD == g_sGY_receive_data.ucCmd)
	{
		if (IMD_ADD_A == g_sGY_receive_data.ucSlave_addr)
		{
			//计时复位
			g_uiNow_tick_imd[GUN_A] = xTaskGetTickCount();
			//置位解析标志位为A绝缘检测模块
			g_sGY_IMD.eDevice_switch = GUN_A;

			GYDataParse(GUN_A);
		}
		else if (IMD_ADD_B == g_sGY_receive_data.ucSlave_addr)
		{
			//计时复位
			g_uiNow_tick_imd[GUN_B] = xTaskGetTickCount();
			//置位解析标志位为B绝缘检测模块
			g_sGY_IMD.eDevice_switch = GUN_B;

			GYDataParse(GUN_B);
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d ucSlave_addr error:%d\n", __FILE__, __LINE__, g_sGY_receive_data.ucSlave_addr);
		}
	}
}

/**
 * @brief 解析绝缘监测板版本数据
 * @param
 * @return
*/
static void ImdVersionParse(void)
{
	if (GY_READ_CMD == g_sGY_receive_version.ucCmd)
	{
		if (IMD_ADD_A == g_sGY_receive_version.ucSlave_addr)
		{
			//A绝缘检测模块地址自检完成
			if ((U16)FALSE == g_sSelf_check.sItem.imd_comm_flag_A)
			{
				g_sSelf_check.sItem.imd_comm_flag_A = (U16)TRUE;
			}
			//计时复位
			g_uiNow_tick_imd[GUN_A] = xTaskGetTickCount();
			//置位解析标志位为A绝缘检测模块
			g_sGY_IMD.eDevice_switch = GUN_A;
			(void)snprintf(g_sGY_IMD.sPublic_data[GUN_A].ucImd_version, sizeof(g_sGY_IMD.sPublic_data[GUN_A].ucImd_version), "%d", g_sGY_receive_version.unVersion);
		}
		else if (IMD_ADD_B == g_sGY_receive_version.ucSlave_addr)
		{
			//B绝缘检测模块地址自检完成
			if ((U16)FALSE == g_sSelf_check.sItem.imd_comm_flag_B)
			{
				g_sSelf_check.sItem.imd_comm_flag_B = (U16)TRUE;
			}
			//计时复位
			g_uiNow_tick_imd[GUN_B] = xTaskGetTickCount();
			//置位解析标志位为B绝缘检测模块
			g_sGY_IMD.eDevice_switch = GUN_B;
			(void)snprintf(g_sGY_IMD.sPublic_data[GUN_B].ucImd_version, sizeof(g_sGY_IMD.sPublic_data[GUN_B].ucImd_version), "%d", g_sGY_receive_version.unVersion);
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d ucSlave_addr error:%d\n", __FILE__, __LINE__, g_sGY_receive_version.ucSlave_addr);
		}
	}
}

/**
 * @brief 解析绝缘监测状态数据以及电阻数据  数据类型以大端存储,校验位为小端
 * @param *pucBuf：绝缘监测相应数据
 * @param unInput_len:数据长度
 * @return
*/
static void GYImdParse(const U8 *pucBuf, U16 unInput_len)
{
	//是否满足标准协议长度
	CHECK_PTR_NULL_NO_RETURN(pucBuf);
	CHECK_MSG_LEN_NO_RETURN(unInput_len, 5U);

	//解析
	U16 unCrc_data = CalCrc16(pucBuf, (U16)(unInput_len - CRC_LEN));
	unCrc_data = (((unCrc_data << 8) & 0xFF00U) | ((unCrc_data >> 8) & 0xFFU));

	switch(unInput_len)
	{
	case VERSION_DATA_LEN:
		(void)memcpy(&g_sGY_receive_version, pucBuf, unInput_len);

		if (unCrc_data == g_sGY_receive_version.unCrc)
		{
			ImdVersionParse();
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d unInput_len CRC error\n", __FILE__, __LINE__);
		}
		break;
	case REQUEST_DATA_LEN:
		(void)memcpy(&g_sGY_receive_data, pucBuf, unInput_len);

//		uPRINTF("IMD data = ");
//		for (int i = 0; i < unInput_len; i++)
//		{
//			uPRINTF("%d ", pucBuf[i]);
//		}
//		uPRINTF("\n");

		if (unCrc_data == g_sGY_receive_data.unCrc)
		{
			ImdRequestDataParse();
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d unInput_len CRC error\n", __FILE__, __LINE__);
		}
		break;
	case IMD_CONTROL_LEN:
		(void)memcpy(&g_sGY_receive_control, pucBuf, unInput_len);

		if (unCrc_data == g_sGY_receive_control.unCrc)
		{
			if (GY_WRITE_CMD == g_sGY_receive_control.ucCmd)
			{
				if (IMD_ADD_A == g_sGY_receive_control.ucSlave_addr)
				{
					ImdControlResponse(GUN_A);
				}
				else if (IMD_ADD_B == g_sGY_receive_control.ucSlave_addr)
				{
					ImdControlResponse(GUN_B);
				}
				else
				{
					my_printf(USER_ERROR, "%s:%d ucSlave_addr error:%d\n", __FILE__, __LINE__, g_sGY_receive_control.ucSlave_addr);
				}
			}
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d unInput_len CRC error\n", __FILE__, __LINE__);
		}
		break;
	default:
		my_printf(USER_ERROR, "%s:%d IMD parse data len error = %d\n", __FILE__, __LINE__, unInput_len);
		break;
	}
}

IMD_data_t* GetGYImdModel(void)
{
	//绝缘检测同步共元
	g_pImd_fault[GUN_A] = (U8 *)&g_sGY_fault[GUN_A];
	g_pImd_fault[GUN_B] = (U8 *)&g_sGY_fault[GUN_B];

	return &g_sGY_IMD;
}

void GYImdInit(void)
{
	g_sGY_IMD.ucImd_mode[GUN_A] = GY_AUTO_MODE;
	g_sGY_IMD.ucImd_mode[GUN_B] = GY_AUTO_MODE;
	g_sGY_IMD.ImdDataParse = &GYImdParse;
	g_sGY_IMD.ImdControl = &GYImdControl;
}
