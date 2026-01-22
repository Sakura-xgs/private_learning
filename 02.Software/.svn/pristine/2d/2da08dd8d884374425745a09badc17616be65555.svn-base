/*
 * uart_boot.h
 *
 *  Created on: 2024年10月25日
 *      Author: Bono
 */

#ifndef HAL_BOOT_UART_BOOT_H_
#define HAL_BOOT_UART_BOOT_H_

#include "hal_uart_IF.h"
#include "boot.h"

#define CCU_ADDR						(0x00U)

typedef enum
{
    USART_UPDATE_UNKNOW_CMD     = 0x0000,	//未定义命令
    USART_UPDATE_REQUEST_CMD    = 0x0001,	//升级请求
    USART_UPDATE_READY_CMD		= 0x0002,	//升级准备
    USART_UPDATE_BEGIN_CMD      = 0x0003,	//起始包
    USART_UPDATE_MIDDLE_CMD     = 0x0004,	//中间包
    USART_UPDATE_END_CMD        = 0x0005,	//结束包
    USART_UPDATE_CHECKSUM_CMD   = 0x0006,	//固件文件校验码包

	USART_UPDATE_SEQUENCE_ERROR	= 0x1001,	//未按顺序发送报文
	USART_UPDATE_UNDEFINED_CMD	= 0x1002,	//未定义的命令
}USART_BOOT_STEP_CMD;


extern BOOL g_blUsartUpdataFuncEnFg;
extern S32 g_Mbms_sam_update_state;
extern UpdateTypeDef g_Usart_2_CommBuf;

extern BOOL blDetcetUsartUpdataStep(USART_BOOT_STEP_CMD RecStep);
void UsartUpdataFunc(void);
void UsartUpdataTimeOutFunc(void);

#endif /* HAL_BOOT_UART_BOOT_H_ */
