/*
 * ATQ_hmi.c
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

#include "hmi.h"
#include "XRD_hmi.h"
#include "SignalManage.h"
#include "calculate_crc.h"
#include "uart_comm.h"
#include "tcp_client_IF.h"
#include "boot.h"

static __BSS(SRAM_OC) HMI_data_t g_sXRD_HMI = {0};

/**
 * @brief HMI数据接受解析函数
 * @param pucBuf 数据缓存
 * @param unInput_len：数据长度
 * @return
 */
static void XRDHmiParse(const U8 *pucBuf, U16 unInput_len)
{
	CHECK_MSG_LEN_NO_RETURN(unInput_len, 7U);
	CHECK_PTR_NULL_NO_RETURN(g_psHMI_data);

	if ((HEAD_NUM == pucBuf[HEAD_INDEX]) && (TAIL_NUM == pucBuf[unInput_len-3U]))//帧头帧尾
	{
		//解析
		U16 unCrc_data = CalCrc16(pucBuf, unInput_len - CRC_LEN);
		unCrc_data = (((unCrc_data << 8) & 0xFF00U) | ((unCrc_data >> 8) & 0xFFU));

		switch (unInput_len)
		{
		case SET_PILE_NUM_DATA_LEN:
			(void)memcpy(&g_HMI_set_num, pucBuf, unInput_len);
			if (unCrc_data == g_HMI_set_num.unCrc)
			{
				if ((U8)PILE_CONFIG == g_HMI_set_num.ucCmd)
				{
					if ((PILE_NUM_MIN <= g_HMI_set_num.ucPile_num) && (g_HMI_set_num.ucPile_num <= PILE_NUM_MAX))//桩编号1-4
					{
						(void)SetSigVal(CCU_SET_SIG_ID_GUN_NUM, (S32)g_HMI_set_num.ucPile_num);
						Uart_Dma_Send(HMI_UART, unInput_len, pucBuf);
						my_printf(USER_INFO, "receive HMI pile number config:%d\n", g_HMI_set_num.ucPile_num);
						TcpClose();
						vTaskDelay(2000);
						SystemResetFunc();
					}
					else
					{
						my_printf(USER_ERROR, "%s:%d receive HMI pile number error:%d\n", __FILE__, __LINE__, g_HMI_set_num.ucPile_num);
					}
				}
			}
			else
			{
				my_printf(USER_ERROR, "%s:%d unInput_len CRC error\n", __FILE__, __LINE__);
			}
			break;
		case CHARGE_CONTROL_DATA_LEN:
			(void)memcpy(&g_HMI_charge_control, pucBuf, unInput_len);
			if (unCrc_data == g_HMI_charge_control.unCrc)
			{
				if ((GUN_NUM_MIN <= g_HMI_charge_control.eGun_id) && (g_HMI_charge_control.eGun_id <= GUN_NUM_MAX))//枪id1-8
				{
					U8 GunId = g_HMI_charge_control.eGun_id - ((2U * g_sStorage_data.ucPile_num) - 1U);
					//判定id是否有效
					if (GunId < GUN_MAX_NUM)
					{
						//复位鉴权状态
						g_psHMI_data->ucAuthorize_flag[GUN_A] = 0;
						g_psHMI_data->ucAuthorize_flag[GUN_B] = 0;

						switch (g_HMI_charge_control.ucCmd)
						{
						case START_CHARGE:
							HmiStartCharge(GunId);
							Uart_Dma_Send(HMI_UART, unInput_len, pucBuf);
							break;
						case CANCEL_CHARGE:
							HmiCancelCharge(GunId);
							Uart_Dma_Send(HMI_UART, unInput_len, pucBuf);
							break;
						case STOP_CHARGE:
							//判定与启动方式是否相同
							if (g_sGun_data[GunId].sTemp.eStart_Charge_type == (Start_Charge_Type_e)g_HMI_charge_control.ucCharge_type)
							{
								HmiStopCharge(GunId);
								Uart_Dma_Send(HMI_UART, unInput_len, pucBuf);
							}
							else
							{
								my_printf(USER_ERROR, "%s:%d receive HMI stop type error start = %d stop = %d\n", __FILE__, __LINE__,
										g_sGun_data[GunId].sTemp.eStart_Charge_type, g_HMI_charge_control.ucCharge_type);
							}
							break;
						default:
							my_printf(USER_ERROR, "%s:%d ucCmd error:%d\n", __FILE__, __LINE__, g_HMI_charge_control.ucCmd);
							break;
						}
					}
					else
					{
						my_printf(USER_ERROR, "%s:%d receive HMI invalid gun id dismatch pile_num\n", __FILE__, __LINE__);
					}
				}
				else
				{
					my_printf(USER_ERROR, "%s:%d HMI control gun id error: %d\n", __FILE__, __LINE__, g_HMI_charge_control.eGun_id);
				}
			}
			else
			{
				my_printf(USER_ERROR, "%s:%d unInput_len CRC error\n", __FILE__, __LINE__);
			}
			break;
		case GET_STATUS_DATA_LEN:
			(void)memcpy(&g_HMI_get_status, pucBuf, unInput_len);
			if (unCrc_data == g_HMI_get_status.unCrc)
			{
				if ((U8)PILE_STATUS == g_HMI_get_status.ucCmd)
				{
					if ((PILE_NUM_MIN <= g_HMI_get_status.ucPile_num) && (g_HMI_get_status.ucPile_num <= PILE_NUM_MAX))//桩编号
					{
						//获取版本号
						(void)memcpy(g_psHMI_data->ucHmi_version, g_HMI_get_status.ucVersion, sizeof(g_HMI_get_status.ucVersion));
						PileStatusResponse();
					}
					else
					{
						my_printf(USER_ERROR, "%s:%d receive HMI invalid pile number = %d\n", __FILE__, __LINE__, g_HMI_get_status.ucPile_num);
					}
				}
			}
			else
			{
				my_printf(USER_ERROR, "%s:%d unInput_len CRC error\n", __FILE__, __LINE__);
			}
			break;
		default:
			//用户获取账单方式不限制数据长度
			for (U32 i = 0; i < unInput_len; i++)
			{
				uPRINTF("%02x ", pucBuf[i]);
			}
			uPRINTF("\n");
			U16 unRecv_crc = 0;
			unRecv_crc = pucBuf[unInput_len-2U];
			unRecv_crc |= (U16)(pucBuf[unInput_len-1U]) << 8;
			if (unCrc_data == unRecv_crc)
			{
				if ((U8)ORDER_GET_CHANNEL == pucBuf[1])
				{
					my_printf(USER_INFO, "receive HMI user order type\n");
					if (pucBuf[4] == g_sStorage_data.sPublic_data[GUN_A].eGun_id)
					{
//						g_psHMI_data->sOrder_data[GUN_A].ucOrder_type = g_HMI_set_order.uccharge_type;
//						(void)memcpy(g_psHMI_data->sOrder_data[GUN_A].ucOrder_data, g_HMI_set_order.ucData, sizeof(g_HMI_set_order.ucData));
						Uart_Dma_Send(HMI_UART, unInput_len, pucBuf);
					}
					else if (pucBuf[4] == g_sStorage_data.sPublic_data[GUN_B].eGun_id)
					{
//						g_psHMI_data->sOrder_data[GUN_B].ucOrder_type = g_HMI_set_order.uccharge_type;
//						(void)memcpy(g_psHMI_data->sOrder_data[GUN_B].ucOrder_data, g_HMI_set_order.ucData, sizeof(g_HMI_set_order.ucData));
						Uart_Dma_Send(HMI_UART, unInput_len, pucBuf);
					}
					else
					{
						my_printf(USER_ERROR, "%s:%d eGun_id error:%d\n", __FILE__, __LINE__, pucBuf[0]);
					}
				}
			}
			else
			{
				my_printf(USER_ERROR, "%s:%d unInput_len CRC error %d!=%d\n", __FILE__, __LINE__, unCrc_data, unRecv_crc);
			}
			break;
		}
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d receive data tail or head error\n", __FILE__, __LINE__);
	}

	return;
}

HMI_data_t* GetXRDHmiModel(void)
{
	return &g_sXRD_HMI;
}

void XRDHmiInit(void)
{
	g_sXRD_HMI.HmiDataParse = &XRDHmiParse;
}
