/*
 * uart_comm.h
 *
 *  Created on: 2024年8月28日
 *      Author: Bono
 */

#ifndef APP_UART_COMM_UART_COMM_H_
#define APP_UART_COMM_UART_COMM_H_

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "cr_section_macros.h"
#include "queue.h"
#include "semphr.h"
#include "hal_uart_IF.h"
#include "PublicDefine.h"


#define USART_COMM_TASK_PRIO	(4)

#define DBG_UART				(e_UART_1)
#define METER_UART				(e_UART_2)
#define RFID_UART				(e_UART_3)
#define ISO_UART				(e_UART_4)
#define HMI_UART				(e_UART_5)
#define DBG6_UART				(e_UART_6)
#define POS_UART				(e_UART_7)
#define DBG8_UART				(e_UART_8)


extern SemaphoreHandle_t uartDebugMsgMutexSemaphore;

extern void debug_printf(const char *format, ...);
extern void Uart1_Comm_Task(void * pvParameters);
extern void Uart_Init_Task(void * pvParameters);

#endif /* APP_UART_COMM_UART_COMM_H_ */
