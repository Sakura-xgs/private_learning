#ifndef _TCPBOOT_H_
#define _TCPBOOT_H_
#include "PublicDefine.h"

typedef struct Update_t UpdateTypeDef;

typedef enum
{
    TCP_UPDATE_UNKNOW_CMD     = 0x0000,	//未定义命令
    TCP_UPDATE_REQUEST_CMD    = 0x0001,	//升级请求
    TCP_UPDATE_READY_CMD	  = 0x0002,	//升级准备
    TCP_UPDATE_BEGIN_CMD      = 0x0003,	//起始包
    TCP_UPDATE_MIDDLE_CMD     = 0x0004,	//中间包
    TCP_UPDATE_END_CMD        = 0x0005,	//结束包
    TCP_UPDATE_CHECKSUM_CMD   = 0x0006,	//固件文件校验码包
}TCP_BOOT_STEP_CMD;


//extern U32 g_u32TcpSuccessCntFg;
extern UpdateTypeDef g_Updata_CommBuf;
extern SemaphoreHandle_t TCP_Updata_Binary_Semaphore;

BOOL blTcpCheckUpdateMsg(void);
void TcpUpdataFunc(void);
void TcpUpdataTimeOutFunc(void);

#endif
