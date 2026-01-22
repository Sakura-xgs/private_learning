/*
 * uart_comm.c
 *
 *  Created on: 2024年8月28日
 *      Author: Bono
 */


#include "uart_comm.h"
#include "SignalManage.h"

__BSS(SRAM_OC) static U8 u8DebugBuff[UART_COMM_BUF_LEN] = {0};
__BSS(SRAM_OC) static U8 u8Recdata[UART_COMM_BUF_LEN] = {0};
SemaphoreHandle_t uartDebugMsgMutexSemaphore = NULL;

void debug_printf(const char *format, ...)
{
#if UART_DEBUG_MSG
	U16 u16MsgSize = 0;
	va_list args;

	xSemaphoreTake(uartDebugMsgMutexSemaphore, portMAX_DELAY);

	va_start(args, format);
	u16MsgSize = vsnprintf((char *)u8DebugBuff, sizeof(u8DebugBuff), format, args);
	va_end(args);

	if (u16MsgSize >= sizeof(u8DebugBuff))
	{
		u16MsgSize = sizeof(u8DebugBuff) - 1;
		u8DebugBuff[u16MsgSize] = '\0';
	}

	Uart_Dma_Send(DBG_UART, u16MsgSize, u8DebugBuff);

	xSemaphoreGive(uartDebugMsgMutexSemaphore);
#endif
}

void DebugUartRec(void)
{
	U16 u16RecByteNum = 0;

	u16RecByteNum = RecUartData(DBG_UART, u8Recdata, UART_COMM_BUF_LEN);
	uPRINTF("uart-%d has %d bytes received.", DBG_UART, u16RecByteNum);
	if(TRUE == GetMsgVal(SIGNAL_STATUS_FACTORY_TEST))
	{
		U8 u8TempBuff[2] = {0};
		if((u8Recdata[0] == 0xAA) && (u8Recdata[1] == 0x55))
		{
			u8TempBuff[0] = 0xAA;
			Uart_Dma_Send(DBG_UART, 1 , u8TempBuff);
		}
		else
		{
			u8TempBuff[0] = 0x55;
			Uart_Dma_Send(DBG_UART, 1 , u8TempBuff);
		}
		return;
	}

	if(u8Recdata[0] == baud_115200)
	{
		uPRINTF("Please change the PC baudrate to 115200.", u16RecByteNum);
		Uart_BaudRate_Reinit(DBG_UART, baud_115200);
	}
	else if(u8Recdata[0] == baud_9600)
	{
		uPRINTF("Please change the PC baudrate to 9600.", u16RecByteNum);
		Uart_BaudRate_Reinit(DBG_UART, baud_9600);
	}
}

void ExampleUartSend(void)
{
	U8 u8TempBuff[UART_COMM_BUF_LEN] = {0};

	/*lock your mutex  here*/
	u8TempBuff[0] = 'H';
	u8TempBuff[1] = 'i';

	Uart_Dma_Send(DBG_UART, 2 , u8TempBuff);
	/*unlock your mutex here*/
}

/*!
    \brief      Rs485 UART COMM task
    \param[in]  pvParameters not used
    \param[out] none
    \retval     none
*/
void Debug_Uart_Task(void * pvParameters)
{
    BaseType_t err = pdFALSE;
	volatile UBaseType_t uxHighWaterMark;

	(void)pvParameters;

	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

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

		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    }
}

void Uart_Init_Task(void * pvParameters)
{
	(void)pvParameters;

	xTaskCreate(Debug_Uart_Task,	"DEBUG_UART_INIT",     configMINIMAL_STACK_SIZE,   	NULL,   USART_COMM_TASK_PRIO,    	NULL);

	vTaskDelete(NULL);
}

