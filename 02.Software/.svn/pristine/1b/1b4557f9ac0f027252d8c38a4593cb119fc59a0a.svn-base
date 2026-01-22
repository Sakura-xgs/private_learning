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


static const IRQn_Type Lpuart_IRQ[e_UART_NUM] = {
												#if UART_1
												LPUART1_IRQn
												#endif
												#if UART_2
												,LPUART2_IRQn
												#endif
												#if UART_3
												,LPUART3_IRQn
												#endif
												#if UART_4
												,LPUART4_IRQn
												#endif
												#if UART_5
												,LPUART5_IRQn
												#endif
												#if UART_6
												,LPUART6_IRQn
												#endif
												#if UART_7
												,LPUART7_IRQn
												#endif
												#if UART_8
												,LPUART8_IRQn
												#endif
												};
static const U32 Lpuart_srcCLock[e_UART_NUM] = {
												#if UART_1
												LPUART1_CLOCK_SOURCE
												#endif
												#if UART_2
												,LPUART2_CLOCK_SOURCE
												#endif
												#if UART_3
												,LPUART3_CLOCK_SOURCE
												#endif
												#if UART_4
												,LPUART4_CLOCK_SOURCE
												#endif
												#if UART_5
												,LPUART5_CLOCK_SOURCE
												#endif
												#if UART_6
												,LPUART6_CLOCK_SOURCE
												#endif
												#if UART_7
												,LPUART7_CLOCK_SOURCE
												#endif
												#if UART_8
												,LPUART8_CLOCK_SOURCE
												#endif
												};

static LPUART_Type *const Lpuart_Base[e_UART_NUM] = {
												#if UART_1
												LPUART1
												#endif
												#if UART_2
												,LPUART2
												#endif
												#if UART_3
												,LPUART3
												#endif
												#if UART_4
												,LPUART4
												#endif
												#if UART_5
												,LPUART5
												#endif
												#if UART_6
												,LPUART6
												#endif
												#if UART_7
												,LPUART7
												#endif
												#if UART_8
												,LPUART8
												#endif
												};
static lpuart_edma_handle_t *const Lpuart_edma_handle[e_UART_NUM] = {
												#if UART_1
												&LPUART1_LPUART_eDMA_Handle
												#endif
												#if UART_2
												,&LPUART2_LPUART_eDMA_Handle
												#endif
												#if UART_3
												,&LPUART3_LPUART_eDMA_Handle
												#endif
												#if UART_4
												,&LPUART4_LPUART_eDMA_Handle
												#endif
												#if UART_5
												,&LPUART5_LPUART_eDMA_Handle
												#endif
												#if UART_6
												,&LPUART6_LPUART_eDMA_Handle
												#endif
												#if UART_7
												,&LPUART7_LPUART_eDMA_Handle
												#endif
												#if UART_8
												,&LPUART8_LPUART_eDMA_Handle
												#endif
												};
AT_NONCACHEABLE_SECTION_INIT(UartConfig user_UartConfig[e_UART_NUM]) = {0};

/*************************upgrade check************************/
BOOL blCheckUsartUpdateMsg(UART_LIST uart)
{
	BOOL blRet = FALSE;
	U16 u16ReadCrc = 0;
	U16 u16CalCrc = 0;
	U16 u16DataType = 0;
	S32 s32UpdateType = 0;
	S32 s32CcuAddr = 0;

	(void)GetSigVal(PDU_SAM_SIG_ID_UPDATE_STATE, &s32UpdateType);

	if((s32UpdateType != NO_UPGRADE)
	&& (s32UpdateType != (UPGRADE_BY_UART1 + uart)))
	{
		return blRet;
	}

	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32CcuAddr);

	if((0xAA == user_UartConfig[uart].rxBuffer[0])	//head
	&& (0x55 == user_UartConfig[uart].rxBuffer[1])	//head
	&& (s32CcuAddr == user_UartConfig[uart].rxBuffer[2])	//dev addr
	&& (0x50 == user_UartConfig[uart].rxBuffer[3])	//upgrade order code
	&& (user_UartConfig[uart].recLength > 7)
	&& (user_UartConfig[uart].recLength < 300))
	{
		u16CalCrc = crc16_ccitt_xmodem(&user_UartConfig[uart].rxBuffer[3], (user_UartConfig[uart].recLength-7), 0);
		u16ReadCrc = (U16)user_UartConfig[uart].rxBuffer[user_UartConfig[uart].recLength-1-3] | ((U16)user_UartConfig[uart].rxBuffer[user_UartConfig[uart].recLength-1-2]<<8);

		if(u16CalCrc == u16ReadCrc)
		{
			u16DataType = (U16)user_UartConfig[uart].rxBuffer[5] | ((U16)user_UartConfig[uart].rxBuffer[6])<<8;

			//判断升级报文是否按照升级流程要求
			if(TRUE != blDetcetUsartUpdataStep((USART_BOOT_STEP_CMD)u16DataType))
			{
				return blRet;
			}

			blRet = TRUE;

			if(FALSE == g_blUsartUpdataFuncEnFg)
			{
				g_blUsartUpdataFuncEnFg = TRUE;
				(void)SetSigVal(PDU_SAM_SIG_ID_UPDATE_STATE, UPGRADE_BY_UART1 + uart);
//				xTaskResumeFromISR(NormalUpdataTask_Handler);
			}
		}
	}

	return blRet;
}

USART_BOOT_STEP_CMD RecUpgradeData(UART_LIST uart, uint8_t *rec_buf, uint16_t rec_buf_length)
{
	xSemaphoreTake(user_UartConfig[uart].edmaMutexSemaphore, portMAX_DELAY);

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

	xSemaphoreGive(user_UartConfig[uart].edmaMutexSemaphore);
	return user_UartConfig[uart].recLength;
}

/**************************************************/

uint16_t RecUartData(UART_LIST uart, uint8_t *rec_buf, uint16_t rec_buf_length)
{
	xSemaphoreTake(user_UartConfig[uart].edmaMutexSemaphore, portMAX_DELAY);

	if(rec_buf_length < user_UartConfig[uart].recLength)
	{
		(void)memcpy(rec_buf, user_UartConfig[uart].rxBuffer, rec_buf_length);
	}
	else
	{
		(void)memcpy(rec_buf, user_UartConfig[uart].rxBuffer, user_UartConfig[uart].recLength);
	}

	xSemaphoreGive(user_UartConfig[uart].edmaMutexSemaphore);
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

void Rs485_1_ConnPin_Enable(void)
{
	RS485_EN1_ENABLE();
}

void Rs485_1_ConnPin_Disable(void)
{
	RS485_EN1_DISABLE();
}

void Uart_Dma_Send(UART_LIST uart, uint32_t byte_num, uint8_t *buff)
{
	if((byte_num > UART_COMM_BUF_LEN)
	|| (buff == NULL)
	|| (uart < e_UART_1)
	|| (uart >= e_UART_NUM))
	{
		return;
	}

	xSemaphoreTake(user_UartConfig[uart].edmaMutexSemaphore, portMAX_DELAY);

	memcpy(user_UartConfig[uart].txBuffer, buff, byte_num);

	user_UartConfig[uart].sendXfer.dataSize = byte_num;

	if(user_UartConfig[uart].set485High)
	{
		user_UartConfig[uart].set485High();
	}

    /* 设置DMA传输 */
    LPUART_SendEDMA(Lpuart_Base[uart], Lpuart_edma_handle[uart], &user_UartConfig[uart].sendXfer);

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

	if(user_UartConfig[uart].set485Low)
	{
		user_UartConfig[uart].set485Low();
	}

	xSemaphoreGive(user_UartConfig[uart].edmaMutexSemaphore);
}

void LPUART_DmaCallback(LPUART_Type *base, lpuart_edma_handle_t *handle, status_t status, void *userData)
{
	BaseType_t xHigherPriorityTaskWoken;

	if (kStatus_LPUART_TxIdle == status)
	{
		UART_LIST uart = (UART_LIST)FIND_INDEX(base);

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
	BaseType_t xHigherPriorityTaskWoken;
	/* 获取当前已接收字节数 */
	LPUART_TransferGetReceiveCountEDMA(Lpuart_Base[uart], Lpuart_edma_handle[uart], (uint32_t*)&user_UartConfig[uart].recLength);
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

	/* 重新开始DMA接收传输 */
	LPUART_ReceiveEDMA(Lpuart_Base[uart], Lpuart_edma_handle[uart], &user_UartConfig[uart].recXfer);
}

void Uart_BaudRate_Reinit(UART_LIST uart, BAUDRATE baudrate)
{
	switch(baudrate)
	{
		case baud_9600:
			LPUART_SetBaudRate(Lpuart_Base[uart], 9600, Lpuart_srcCLock[uart]);
			break;
		case baud_115200:
			LPUART_SetBaudRate(Lpuart_Base[uart], 115200, Lpuart_srcCLock[uart]);
			break;
		default:
			break;
	}
}


void Uart_Dma_Init(UART_LIST uart)
{
	user_UartConfig[uart].sendXfer.data = user_UartConfig[uart].txBuffer;
	user_UartConfig[uart].recXfer.dataSize = UART_COMM_BUF_LEN;
	user_UartConfig[uart].recXfer.data = user_UartConfig[uart].rxBuffer;
	LPUART_ReceiveEDMA(Lpuart_Base[uart], Lpuart_edma_handle[uart], &user_UartConfig[uart].recXfer);
	LPUART_TransferAbortSendEDMA(Lpuart_Base[uart], Lpuart_edma_handle[uart]);
}

void Uart_Init(UART_LIST uart)
{
	NVIC_SetPriority(Lpuart_IRQ[uart], 3);
	LPUART_ClearStatusFlags(Lpuart_Base[uart], (uint32_t)kLPUART_IdleLineFlag);
	LPUART_EnableInterrupts(Lpuart_Base[uart], (uint32_t)kLPUART_IdleLineInterruptEnable);
	EnableIRQ(Lpuart_IRQ[uart]);
}


void Hal_Uart_Init(void)
{
	U8 i = 0;

	for(i = 0; i < e_UART_NUM; i++)
	{
		user_UartConfig[i].recBinarySemaphore = xSemaphoreCreateBinary();
		user_UartConfig[i].sendBinarySemaphore = xSemaphoreCreateBinary();
		user_UartConfig[i].edmaMutexSemaphore = xSemaphoreCreateMutex();
		Uart_Dma_Init(i);
		Uart_Init(i);
	}

	/*Registration 485_EN pin*/
	user_UartConfig[e_UART_1].set485High = Rs485_1_ConnPin_Enable;
	user_UartConfig[e_UART_1].set485Low = Rs485_1_ConnPin_Disable;

	#if UART_DEBUG_MSG
	uartDebugMsgMutexSemaphore = xSemaphoreCreateMutex();
	#endif
}
