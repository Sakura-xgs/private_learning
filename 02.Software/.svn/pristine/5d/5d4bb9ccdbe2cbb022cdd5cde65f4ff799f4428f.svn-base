#ifndef __HAL_UART_H
#define __HAL_UART_H

#include "PublicDefine.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define USART_COMM_BUF_LEN 300

//USART\USAT等待发送完成超时时候
#define SEND_TIME_OUT_VAL	300	//300MS

typedef struct
{
	U16	u16RecvDataCnt;			            //接收数据个数
	U8	u8Recvbuf[USART_COMM_BUF_LEN];	    //接收缓冲区
}strUSART_MSG;

typedef struct
{
    uint32_t ConfigNo;
    uint32_t UsartComNo;
	uint32_t UsartComClk;
	uint32_t UsartComTxPin;
	uint32_t UsartComRxPin;
	uint32_t UsartComGpioPort;
	uint32_t UsartComGpioPortClk;
	uint32_t UsartBaudRate;
}strUSART_PARA;



extern strUSART_MSG g_Usart_0_CommBuf;
extern strUSART_MSG g_Usart_1_CommBuf;
extern strUSART_MSG g_Usart_2_CommBuf;

extern SemaphoreHandle_t g_USART_0_Recv_Binary_Semaphore;
extern SemaphoreHandle_t g_USART_1_Recv_Binary_Semaphore;
extern SemaphoreHandle_t g_USART_2_Recv_Binary_Semaphore;

extern SemaphoreHandle_t g_USART_0_Send_Binary_Semaphore;
extern SemaphoreHandle_t g_USART_1_Send_Binary_Semaphore;
extern SemaphoreHandle_t g_USART_2_Send_Binary_Semaphore;

extern SemaphoreHandle_t g_USART_0_Comm_MutexSemaphore;
extern SemaphoreHandle_t g_USART_1_Comm_MutexSemaphore;
extern SemaphoreHandle_t g_USART_2_Comm_MutexSemaphore;

extern SemaphoreHandle_t g_USART_1_Updata_Binary_Semaphore;
extern SemaphoreHandle_t g_USART_2_Updata_Binary_Semaphore;


void USART_INIT(void);
void usart0_dma_send(uint8_t* buffer, uint16_t len);
void usart1_dma_send(uint8_t* buffer, uint16_t len);
void usart2_dma_send(uint8_t* buffer, uint16_t len);


void Usart0_Switch_To_Send_Mode(void);
void Usart0_Switch_To_Recv_Mode(void);
void Usart1_Switch_To_Send_Mode(void);
void Usart1_Switch_To_Recv_Mode(void);
void Usart2_Switch_To_Send_Mode(void);
void Usart2_Switch_To_Recv_Mode(void);

#endif

