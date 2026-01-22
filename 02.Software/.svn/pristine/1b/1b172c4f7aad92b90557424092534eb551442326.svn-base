/*
 * hal_uart_IF.h
 *
 *  Created on: 2024年8月28日
 *      Author: Bono
 */

#ifndef HAL_HAL_UART_HAL_UART_IF_H_
#define HAL_HAL_UART_HAL_UART_IF_H_

#include "hal_uart.h"

typedef enum
{
	e_UART_1 = 0,
	e_UART_2,
	e_UART_3,
	e_UART_4,
	e_UART_5,
	e_UART_6,
	e_UART_7,
	e_UART_8,
	e_UART_NUM,
}UART_LIST;

typedef enum
{
	baud_9600 = 0,
	baud_115200,
	baud_19200
}BAUDRATE;

#define FIND_INDEX(lpuart) ((lpuart) == LPUART1 ? 0 : \
                            (lpuart) == LPUART2 ? 1 : \
                            (lpuart) == LPUART3 ? 2 : \
                            (lpuart) == LPUART4 ? 3 : \
                            (lpuart) == LPUART5 ? 4 : \
                            (lpuart) == LPUART6 ? 5 : \
                            (lpuart) == LPUART7 ? 6 : \
                            (lpuart) == LPUART8 ? 7 : -1)

typedef void (*Control485PinFunc)(void);
typedef void (*InitFunc)(UART_LIST Uart);

typedef struct {
    SemaphoreHandle_t recBinarySemaphore;
    SemaphoreHandle_t sendBinarySemaphore;
    SemaphoreHandle_t edmaMutexSemaphore;
    uint8_t txBuffer[UART_COMM_BUF_LEN];
    uint8_t rxBuffer[UART_COMM_BUF_LEN];
    lpuart_transfer_t sendXfer;
    lpuart_transfer_t recXfer;
    __IO uint32_t recLength;
    Control485PinFunc set485High;
	Control485PinFunc set485Low;
} UartConfig;

extern void Uart_Dma_Send(UART_LIST uart, uint32_t byte_num, const uint8_t *buff);
extern void Hal_Uart_Init(void);
extern void LPUART_IRQ_User_handle(UART_LIST uart);
extern void Uart_BaudRate_Reinit(UART_LIST uart, BAUDRATE baudrate);
extern SemaphoreHandle_t Uart_recBinarySemaphore(UART_LIST uart);
extern SemaphoreHandle_t Uart_sendBinarySemaphore(UART_LIST uart);
extern SemaphoreHandle_t Uart_commMutexSemaphore(UART_LIST uart);
extern uint16_t RecUartData(UART_LIST uart, uint8_t *rec_buf, uint16_t rec_buf_length);
extern void Uart_Init(UART_LIST uart);
void Uart_Dma_Init(UART_LIST uart);

#endif /* HAL_HAL_UART_HAL_UART_IF_H_ */
