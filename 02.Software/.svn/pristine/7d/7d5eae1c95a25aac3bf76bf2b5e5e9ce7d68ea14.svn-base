/*
 * uart_comm.h
 *
 *  Created on: 2024年8月28日
 *      Author: Bono
 */

#ifndef APP_UART_COMM_UART_COMM_H_
#define APP_UART_COMM_UART_COMM_H_

#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "hal_uart_IF.h"
#include "cr_section_macros.h"
#include "PublicDefine.h"

#define uPRINTF  				debug_printf
//日志上传CPU
#define CCU_LOG_UPLOAD

#define DBG_UART				(e_UART_1)//调试口，485_1
#define RFID_UART				(e_UART_2)//用作232_1
#define POS_UART				(e_UART_3)//用作232_2
#define GB_GUNA_UART			(e_UART_4)//备用,用作232_3  //新板子修改为232-1 V1.1RFID_UART
#define GB_GUNB_UART			(e_UART_5)//备用,用作232_4
#define HMI_UART				(e_UART_6)//用作485_2
#define IMD_UART				(e_UART_7)//用作485_3
#define METER_UART				(e_UART_8)//用作485_4

#define CHECK_PTR_NULL_NO_RETURN(ptr) do { \
    if ((ptr) == NULL) {          \
        uPRINTF("Error: %s:%d Pointer is null\n", __FILE__, __LINE__);  \
        return;       			  \
    }                             \
} while(0)

#define CHECK_PTR_NULL(ptr) do {  \
    if ((ptr) == NULL) {          \
        uPRINTF("Error: %s:%d Pointer is null\n", __FILE__, __LINE__);  \
        return 0;       		  \
    }                             \
} while(0)

#define CHECK_MSG_LEN(limit_len, input_len) do { \
    if ((limit_len) < (input_len)) {                 \
        uPRINTF("Error: %s:%d input length error %d < %d\n", \
                 __FILE__, __LINE__, limit_len, input_len); \
        return 0;                     \
    }                                 \
} while(0)

#define CHECK_MSG_LEN_NO_RETURN(limit_len, input_len) do { \
    if ((limit_len) < (input_len)) {                 \
        uPRINTF("Error: %s:%d input length error %d < %d\n", \
                 __FILE__, __LINE__, limit_len, input_len); \
        return;                       \
    }                                 \
} while(0)

typedef enum
{
	USER_DEBUG	= 0,
	USER_INFO	= 1,
	USER_ERROR	= 2
}LogLevel;

#pragma pack(1)

typedef struct
{
	BOOL bTest_flag;
	U8 PosTempGunA;
	U8 NegTempGunA;
	U8 PosTempGunB;
	U8 NegTempGunB;
	U8 CurGunA;
	U8 CurGunB;
	U32 VolGunA;
	U32 VolGunB;
}Test_t;

#pragma pack()

extern Test_t g_Test;
extern SemaphoreHandle_t uartDebugMsgMutexSemaphore;

extern void debug_printf(const char *format, ...);
extern void my_printf(LogLevel level, const char *format, ...);
extern void Uart_Init_Task(void * pvParameters);
extern BOOL my_unsigned_abs(const U32 uiFirst_num, const U32 uiSecond_num, const U32 uiDiff_data);

#endif /* APP_UART_COMM_UART_COMM_H_ */
