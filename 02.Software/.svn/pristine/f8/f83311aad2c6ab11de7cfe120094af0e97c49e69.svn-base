#ifndef __BOOT_H
#define __BOOT_H

#include "PublicDefine.h"


//升级通信超时计时，单位：1ms
#define COMM_TIME_OUT_VAL                           (1000*10)				    //10秒超时

#define BOOT_DEVICE_ADDR     0x00

#define MSG_UPDATE_MAX_DATALEN                      (248)					    //升级包中数据数据最大字节数（起始包与中间包固定为248，结束包不定）

typedef enum
{
    USART_UPDATE_UNKNOW_CMD     = 0x0000,	//未定义命令
    USART_UPDATE_REQUEST_CMD    = 0x0001,	//升级请求
    USART_UPDATE_READY_CMD		= 0x0002,	//升级准备
    USART_UPDATE_BEGIN_CMD      = 0x0003,	//起始包
    USART_UPDATE_MIDDLE_CMD     = 0x0004,	//中间包
    USART_UPDATE_END_CMD        = 0x0005,	//结束包
    USART_UPDATE_CHECKSUM_CMD   = 0x0006,	//固件文件校验码包
}USART_BOOT_STEP_CMD;


void UpdataFuncInitTask(void *pvParameters);
BOOL blCheck_Usart1_UpdateMsg(void);
BOOL blCheck_Usart2_UpdateMsg(void);

void SystemResetFunc(void);

#endif 

