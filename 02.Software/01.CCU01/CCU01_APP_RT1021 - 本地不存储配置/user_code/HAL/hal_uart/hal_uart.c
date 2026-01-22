/*
 * hal_uart.c
 *
 *  Created on: 2024年8月28日
 *      Author: Bono
 */

#include "uart_comm.h"
#include "SignalManage.h"
#include "uart_boot.h"
#include "hal_uart.h"
#include "hal_uart_IF.h"
#include "hal_sys_IF.h"
#include "fsl_lpuart.h"
#include "fsl_lpuart_edma.h"
#include "cr_section_macros.h"
#include "PublicDefine.h"
#include "BF_pos.h"
#include "tcp_client_IF.h"
#include "APP/charge_process/charge_process_IF.h"

static BOOL blCheckUsartUpdateMsg(UART_LIST uart);

static LPUART_Type *const Lpuart_Base[e_UART_NUM] = {LPUART1, LPUART2, LPUART3, LPUART4, LPUART5, LPUART6, LPUART7, LPUART8 };
static lpuart_edma_handle_t *const Lpuart_edma_handle[e_UART_NUM] = {&LPUART1_LPUART_eDMA_Handle,
																	 &LPUART2_LPUART_eDMA_Handle,
																	 &LPUART3_LPUART_eDMA_Handle,
																	 &LPUART4_LPUART_eDMA_Handle,
																	 &LPUART5_LPUART_eDMA_Handle,
																	 &LPUART6_LPUART_eDMA_Handle,
																	 &LPUART7_LPUART_eDMA_Handle,
																	 &LPUART8_LPUART_eDMA_Handle};

AT_NONCACHEABLE_SECTION_INIT(UartConfig user_UartConfig[e_UART_NUM]) = {0};

S32 g_Mbms_sam_update_state = 0;

static void Rs485_1_ConnPin_Enable(void);
static void Rs485_1_ConnPin_Disable(void);
static void Rs485_2_ConnPin_Enable(void);
static void Rs485_2_ConnPin_Disable(void);
static void Rs485_3_ConnPin_Enable(void);
static void Rs485_3_ConnPin_Disable(void);
static void Rs485_4_ConnPin_Enable(void);
static void Rs485_4_ConnPin_Disable(void);

/*************************upgrade check************************/
static BOOL blCheckUsartUpdateMsg(UART_LIST uart)
{
	if(g_sPile_data.ucPile_config_mode == PRODUCTION_MODE)//生产测试模式
	{
		if((user_UartConfig[uart].rxBuffer[0] == 0x05U) && (user_UartConfig[uart].rxBuffer[1] == 0xAAU))//生产测试串口功能
		{
			if(uart > e_UART_8)
			{
				return FALSE;
			}
			U8 UartFunTest[9] = {0};
			U8 SendLen = sizeof(UartFunTest);

			UartFunTest[0] = 0x05U;
			UartFunTest[1 + uart] = 1U;

			if(user_UartConfig[uart].set485High)
			{
				user_UartConfig[uart].set485High();
			}
			switch(uart)
			{
				case e_UART_1:
					(void)LPUART_WriteBlocking(LPUART1, UartFunTest, SendLen);
					break;
				case e_UART_2:
					(void)LPUART_WriteBlocking(LPUART2, UartFunTest, SendLen);
					break;
				case e_UART_3:
					(void)LPUART_WriteBlocking(LPUART3, UartFunTest, SendLen);
					break;
				case e_UART_4:
					(void)LPUART_WriteBlocking(LPUART4, UartFunTest, SendLen);
					break;
				case e_UART_5:
					(void)LPUART_WriteBlocking(LPUART5, UartFunTest, SendLen);
					break;
				case e_UART_6:
					(void)LPUART_WriteBlocking(LPUART6, UartFunTest, SendLen);
					break;
				case e_UART_7:
					(void)LPUART_WriteBlocking(LPUART7, UartFunTest, SendLen);
					break;
				case e_UART_8:
					(void)LPUART_WriteBlocking(LPUART8, UartFunTest, SendLen);
					break;

				default:
					break;
			}
			if(NULL != user_UartConfig[uart].set485Low)
			{
				user_UartConfig[uart].set485Low();
			}
		}
	}

	if(uart != DBG_UART)
	{
		return FALSE;
	}

	BOOL blRet = FALSE;
	U16 u16ReadCrc = 0;
	U16 u16CalCrc = 0;
	U16 u16DataType = 0;
	S32 s32UpdateType = 0;

	s32UpdateType = g_Mbms_sam_update_state;
	//(void)GetSigVal(CCU_SAM_SIG_ID_UPDATE_STATE, &s32UpdateType);

	if((s32UpdateType != (S32)NO_UPGRADE)
	&& (s32UpdateType != (CCU_UPGRADE_BY_UART1 + uart)))
	{
		return blRet;
	}

	if((0xAAU == user_UartConfig[uart].rxBuffer[0])	//head
	&& (0x55U == user_UartConfig[uart].rxBuffer[1])	//head
	&& (CCU_ADDR == user_UartConfig[uart].rxBuffer[2])	//dev addr
	&& (0x50U == user_UartConfig[uart].rxBuffer[3])	//upgrade order code
	&& (user_UartConfig[uart].recLength > 7U)
	&& (user_UartConfig[uart].recLength < 300U))
	{
		u16CalCrc = crc16_ccitt_xmodem(&user_UartConfig[uart].rxBuffer[3], (user_UartConfig[uart].recLength-7U), 0);
		u16ReadCrc = (U16)user_UartConfig[uart].rxBuffer[user_UartConfig[uart].recLength-1U-3U] | ((U16)user_UartConfig[uart].rxBuffer[user_UartConfig[uart].recLength-1U-2U]<<8);

		if(u16CalCrc == u16ReadCrc)
		{
			u16DataType = (U16)user_UartConfig[uart].rxBuffer[6] | (((U16)user_UartConfig[uart].rxBuffer[7])<<8U);

			//判断升级报文是否按照升级流程要求
			if(TRUE != blDetcetUsartUpdataStep((USART_BOOT_STEP_CMD)u16DataType))
			{
				return blRet;
			}

			//转存数据
			g_Usart_2_CommBuf.u16RecvDataCnt = ((U16)user_UartConfig[uart].rxBuffer[5] << 8) + user_UartConfig[uart].rxBuffer[4] + 10U;
			if(g_Usart_2_CommBuf.u16RecvDataCnt < 300U)
			{
				(void)memcpy(g_Usart_2_CommBuf.u8Recvbuf,user_UartConfig[uart].rxBuffer,g_Usart_2_CommBuf.u16RecvDataCnt);
			}
			else
			{
				return blRet;
			}

			blRet = TRUE;

			if(FALSE == g_blUsartUpdataFuncEnFg)
			{
				if ((FALSE == CheckChargingStatus(GUN_A)) && (FALSE == CheckChargingStatus(GUN_B)))
				{
					g_blUsartUpdataFuncEnFg = TRUE;
					//(void)SetSigVal(CCU_SAM_SIG_ID_UPDATE_STATE, CCU_UPGRADE_BY_UART1 + uart);
					g_Mbms_sam_update_state = CCU_UPGRADE_BY_UART1 + uart;

					//状态变更处理
					SpecialStatusSetProcess(GUN_A, (U8)STA_UPDATE);
					SpecialStatusSetProcess(GUN_B, (U8)STA_UPDATE);

					(void)xTaskResumeFromISR(NormalUpdataTask_Handler);
				}
				else
				{
					blRet = FALSE;
				}
			}
		}
	}

	return blRet;
}

static USART_BOOT_STEP_CMD RecUpgradeData(UART_LIST uart, uint8_t *rec_buf, uint16_t rec_buf_length)
{
	(void)xSemaphoreTake(user_UartConfig[uart].edmaMutexSemaphore, portMAX_DELAY);

//	if(s32UpdateType == UART2_BMS_UPGRADE)
//	{
//		u16DataType = (U16)WIFI_RecBuf.u8Recvbuf[5] | ((U16)WIFI_RecBuf.u8Recvbuf[6])<<8;
//	}
//	else if(s32UpdateType == UART0_BMS_UPGRADE)
//	{
//		u16DataType = (U16)g_Usart_0_CommBuf.u8Recvbuf[5] | ((U16)g_Usart_0_CommBuf.u8Recvbuf[6])<<8;
//	}
//
//	//进行升级步骤判断，一次升级过程只能依次处理完整的升级流程
//	if (u16DataType < enumUsartPreUpdataStep)
//	{
//		return USART_UPDATE_CHECKSUM_CMD;
//	}

//	if((u16DataType > USART_UPDATE_CHECKSUM_CMD) || u16DataType <= USART_UPDATE_UNKNOW_CMD)
//	{
//		return USART_UPDATE_UNDEFINED_CMD;
//	}

	if(rec_buf_length < user_UartConfig[uart].recLength)
	{
		(void)memcpy(rec_buf, user_UartConfig[uart].rxBuffer, rec_buf_length);
	}
	else
	{
		(void)memcpy(rec_buf, user_UartConfig[uart].rxBuffer, user_UartConfig[uart].recLength);
	}

	(void)xSemaphoreGive(user_UartConfig[uart].edmaMutexSemaphore);
	return user_UartConfig[uart].recLength;
}

/**************************************************/

uint16_t RecUartData(UART_LIST uart, uint8_t *rec_buf, uint16_t rec_buf_length)
{
	if ((uart <= e_UART_1) && (uart >= e_UART_8))
	{
		return 0;
	}

	(void)xSemaphoreTake(user_UartConfig[uart].edmaMutexSemaphore, portMAX_DELAY);

	if(rec_buf_length < user_UartConfig[uart].recLength)
	{
		(void)memcpy(rec_buf, user_UartConfig[uart].rxBuffer, rec_buf_length);
	}
	else
	{
		(void)memcpy(rec_buf, user_UartConfig[uart].rxBuffer, user_UartConfig[uart].recLength);
	}

	(void)xSemaphoreGive(user_UartConfig[uart].edmaMutexSemaphore);
	return user_UartConfig[uart].recLength;
}

SemaphoreHandle_t Uart_recBinarySemaphore(UART_LIST uart)
{
	return user_UartConfig[uart].recBinarySemaphore;
}

SemaphoreHandle_t Uart_sendBinarySemaphore(UART_LIST uart)
{
	return user_UartConfig[uart].sendBinarySemaphore;
}

SemaphoreHandle_t Uart_commMutexSemaphore(UART_LIST uart)
{
	return user_UartConfig[uart].edmaMutexSemaphore;
}

static void Rs485_1_ConnPin_Enable(void)
{
	RS485_EN1_ENABLE();
}
static void Rs485_1_ConnPin_Disable(void)
{
	RS485_EN1_DISABLE();
}

static void Rs485_2_ConnPin_Enable(void)
{
	RS485_EN2_ENABLE();
}
static void Rs485_2_ConnPin_Disable(void)
{
	RS485_EN2_DISABLE();
}

static void Rs485_3_ConnPin_Enable(void)
{
	RS485_EN3_ENABLE();
}
static void Rs485_3_ConnPin_Disable(void)
{
	RS485_EN3_DISABLE();
}

static void Rs485_4_ConnPin_Enable(void)
{
	RS485_EN4_ENABLE();
}
static void Rs485_4_ConnPin_Disable(void)
{
	RS485_EN4_DISABLE();
}

void Uart_Dma_Send(UART_LIST uart, uint32_t byte_num, const uint8_t *buff)
{
	if(g_sPile_data.ucPile_config_mode != PRODUCTION_MODE)//生产测试模式
	{
		if((byte_num > UART_COMM_BUF_LEN)
		|| (buff == NULL)
		|| (uart < e_UART_1)
		|| (uart >= e_UART_NUM))
		{
			return;
		}

		(void)xSemaphoreTake(user_UartConfig[uart].edmaMutexSemaphore, portMAX_DELAY);

		(void)memcpy(user_UartConfig[uart].txBuffer, buff, byte_num);

		user_UartConfig[uart].sendXfer.dataSize = byte_num;

		if(NULL != user_UartConfig[uart].set485High)
		{
			user_UartConfig[uart].set485High();
		}
		/* 设置DMA传输 */
		(void)LPUART_SendEDMA(Lpuart_Base[uart], Lpuart_edma_handle[uart], &user_UartConfig[uart].sendXfer);

		/* waiting for the transfer to complete*/
		if(NULL != user_UartConfig[uart].sendBinarySemaphore)
		{
			(void)xSemaphoreTake(user_UartConfig[uart].sendBinarySemaphore, SEND_TIME_OUT_VAL);
		}
		else
		{
			vTaskDelay(SEND_TIME_OUT_VAL);
		}
		LPUART_TransferAbortSendEDMA(Lpuart_Base[uart], Lpuart_edma_handle[uart]);

		if(NULL != user_UartConfig[uart].set485Low)
		{
			user_UartConfig[uart].set485Low();
		}

		(void)xSemaphoreGive(user_UartConfig[uart].edmaMutexSemaphore);
	}
}

void LPUART_DmaCallback(LPUART_Type *base, lpuart_edma_handle_t *handle, status_t status, void *userData)
{
	BaseType_t xHigherPriorityTaskWoken;

	if (kStatus_LPUART_TxIdle == status)
	{
		UART_LIST uart = (UART_LIST)FIND_INDEX(base);

		if ((uart < e_UART_1)
			|| (uart >= e_UART_NUM))
		{
			return;
		}

		if(NULL != user_UartConfig[uart].sendBinarySemaphore)
		{
			/* release binary semaphores. */
			(void)xSemaphoreGiveFromISR(user_UartConfig[uart].sendBinarySemaphore, &xHigherPriorityTaskWoken);

			/* make a task switch if necessary. */
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		}
	}
}

void LPUART_IRQ_User_handle(UART_LIST uart)
{
	if (uart >= e_UART_NUM)
	{
		return;
	}

	BaseType_t xHigherPriorityTaskWoken;
	/* 获取当前已接收字节数 */
	(void)LPUART_TransferGetReceiveCountEDMA(Lpuart_Base[uart], Lpuart_edma_handle[uart], (uint32_t*)&user_UartConfig[uart].recLength);
	/* 关闭DMA传输 */
	LPUART_TransferAbortReceiveEDMA(Lpuart_Base[uart], Lpuart_edma_handle[uart]);

	if(blCheckUsartUpdateMsg(uart) == TRUE)
	{
		if(NULL != uartUpgradeBinarySemaphore)
		{
			/* release binary semaphores. */
			(void)xSemaphoreGiveFromISR(uartUpgradeBinarySemaphore, &xHigherPriorityTaskWoken);

			/* make a task switch if necessary. */
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		}
	}
	else if(NULL != user_UartConfig[uart].recBinarySemaphore)
	{
		/* release binary semaphores. */
		(void)xSemaphoreGiveFromISR(user_UartConfig[uart].recBinarySemaphore, &xHigherPriorityTaskWoken);

		/* make a task switch if necessary. */
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
	else
	{
		//
	}

	/* 重新开始DMA接收传输 */
	(void)LPUART_ReceiveEDMA(Lpuart_Base[uart], Lpuart_edma_handle[uart], &user_UartConfig[uart].recXfer);
}

void Uart_BaudRate_Reinit(UART_LIST uart, BAUDRATE baudrate)
{
	static const U32 Lpuart_srcCLock[e_UART_NUM] = {LPUART1_CLOCK_SOURCE, LPUART2_CLOCK_SOURCE, LPUART3_CLOCK_SOURCE, LPUART4_CLOCK_SOURCE,\
												    LPUART5_CLOCK_SOURCE, LPUART6_CLOCK_SOURCE, LPUART7_CLOCK_SOURCE, LPUART8_CLOCK_SOURCE};

	switch(baudrate)
	{
		case baud_9600:
			(void)LPUART_SetBaudRate(Lpuart_Base[uart], 9600, Lpuart_srcCLock[uart]);
			break;
		case baud_115200:
			(void)LPUART_SetBaudRate(Lpuart_Base[uart], 115200, Lpuart_srcCLock[uart]);
			break;
		case baud_19200:
			(void)LPUART_SetBaudRate(Lpuart_Base[uart], 19200, Lpuart_srcCLock[uart]);
			break;
		default:
			//
			break;
	}
}

void Uart_Dma_Init(UART_LIST uart)
{
	user_UartConfig[uart].sendXfer.data = user_UartConfig[uart].txBuffer;
	user_UartConfig[uart].recXfer.dataSize = UART_COMM_BUF_LEN;
	user_UartConfig[uart].recXfer.data = user_UartConfig[uart].rxBuffer;
	(void)LPUART_ReceiveEDMA(Lpuart_Base[uart], Lpuart_edma_handle[uart], &user_UartConfig[uart].recXfer);
	(void)LPUART_TransferAbortSendEDMA(Lpuart_Base[uart], Lpuart_edma_handle[uart]);
}

void Uart_Init(UART_LIST uart)
{
	static const IRQn_Type Lpuart_IRQ[e_UART_NUM] = {LPUART1_IRQn, LPUART2_IRQn, LPUART3_IRQn, LPUART4_IRQn, LPUART5_IRQn, LPUART6_IRQn, LPUART7_IRQn, LPUART8_IRQn};

	NVIC_SetPriority(Lpuart_IRQ[uart], 3);
	(void)LPUART_ClearStatusFlags(Lpuart_Base[uart], (uint32_t)kLPUART_IdleLineFlag);
	LPUART_EnableInterrupts(Lpuart_Base[uart], (uint32_t)kLPUART_IdleLineInterruptEnable);
	(void)EnableIRQ(Lpuart_IRQ[uart]);
}

void Hal_Uart_Init(void)
{
	U8 i = 0;

	for(i = 0; i < (U8)e_UART_NUM; i++)
	{
		user_UartConfig[i].recBinarySemaphore = xSemaphoreCreateBinary();
		user_UartConfig[i].sendBinarySemaphore = xSemaphoreCreateBinary();
		user_UartConfig[i].edmaMutexSemaphore = xSemaphoreCreateMutex();
		Uart_Dma_Init(i);
		Uart_Init(i);
	}

	/*Registration 485_EN pin*/
	user_UartConfig[e_UART_1].set485High = &Rs485_1_ConnPin_Enable;
	user_UartConfig[e_UART_1].set485Low = &Rs485_1_ConnPin_Disable;

	user_UartConfig[e_UART_6].set485High = &Rs485_2_ConnPin_Enable;
	user_UartConfig[e_UART_6].set485Low = &Rs485_2_ConnPin_Disable;

	user_UartConfig[e_UART_7].set485High = &Rs485_3_ConnPin_Enable;
	user_UartConfig[e_UART_7].set485Low = &Rs485_3_ConnPin_Disable;

	user_UartConfig[e_UART_8].set485High = &Rs485_4_ConnPin_Enable;
	user_UartConfig[e_UART_8].set485Low = &Rs485_4_ConnPin_Disable;

	#if UART_DEBUG_MSG
	uartDebugMsgMutexSemaphore = xSemaphoreCreateMutex();
	#endif
}
