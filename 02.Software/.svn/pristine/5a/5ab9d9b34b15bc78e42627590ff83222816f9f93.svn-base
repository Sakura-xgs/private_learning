/*
 * boot.h
 *
 *  Created on: 2024年8月21日
 *      Author: Bono
 */

#ifndef HAL_BOOT_BOOT_H_
#define HAL_BOOT_BOOT_H_

#include "PublicDefine.h"
#include "cr_section_macros.h"
#include "peripherals.h"
#include "flash_data_IF.h"

#define _LINK_T(PRODUCT, AREA, INPUT, OUTPUT, YEAR, MONTH, DAY)  #PRODUCT#AREA#INPUT#OUTPUT#YEAR#MONTH#DAY
#define _SW_NUM(NAME)  #NAME

#define NORMAL_UPDATA_TASK_PRIO      	( tskIDLE_PRIORITY + 5 )
#define UPDATA_JUMP_TASK_PRIO         	( tskIDLE_PRIORITY + 3 )

#define str_APP_INTEG_FLAG_ADDR			(NEW_APP_INTEG_FLASH_ADDR_START)
#define str_APP_VERSION_ADDR			(APP_FLASH_ADDR_START+0x10)
#define str_BOOT_VERSION_ADDR			(APP_FLASH_ADDR_START)

#define UPGRADE_CAN						(CAN1)

#define RESPON_DELAY_TIME				(5)					//错峰回复间隔时间，单位ms
#define COMM_TIME_OUT_VAL				(1000*10)			//10秒超时
#define MSG_UPDATE_MAX_DATALEN			(FLASH_PAGE_SIZE)	//升级包中数据数据最大字节数（起始包与中间包固定为256，结束包不定）

#define CAN_RX_BUF_LEN					(300)

typedef struct
{
	U8 sw_major_version[13];
	U8 sw_minor_version[3];
}SOFT_VER;

typedef struct
{
	U8 hw_version[16];
}HARD_VER;

typedef enum
{
  UPDATE_UNKNOW_CMD         = 0x0000,	//未定义命令
  UPDATE_REQUEST_CMD		= 0x01A5,	//升级请求
  UPDATE_READY_CMD          = 0x02A5,	//升级准备
  UPDATE_MIDDLE_CMD         = 0x03A5,	//传输数据包
  UPDATE_CHECKSUM_CMD		= 0x04A5,	//升级校验
}BOOT_STEP_CMD;

typedef enum
{
  UPDATA_UNKNOW   = 0x00,     //未定义
  UPDATA_SUCCESS  = 0xAA,     //升级操作成功
  UPDATA_FAIL     = 0x55,     //升级操作失败
}UPDATA_RET;

typedef enum
{
	NO_UPGRADE	= 0,
	UPGRADE_BY_UART1,
	UPGRADE_BY_UART2,
	UPGRADE_BY_UART3,
	UPGRADE_BY_UART4,
	UPGRADE_BY_UART5,
	UPGRADE_BY_UART6,
	UPGRADE_BY_UART7,
	UPGRADE_BY_UART8,
	PDU_CAN1_UPGRADE,
}UPGRADE_TYPE;

typedef enum
{
  ACK_OK      = 0x00,
  ACK_ERR_FLASH_NOT_REARY = 0x01,
  ACK_ERR_SINGLE_PACK_CRC_ERR = 0x02,
  ACK_ERR_FLASH_WRITE_ERR = 0x03,
  ACK_ERR_NO_APPINTEG = 0x04,
  ACK_ERR_WRITE_APPINTEG_ERR = 0x05,
  ACK_ERR_TOTAL_PACK_CRC_ERR = 0x06,
  ACK_ERR_UNKNOW_ERR = 0x07,
  ACK_ERR_RELAY_CLOSED = 0x08,
}ACK_ERR_TYPE;

typedef struct
{
    U8              u8RxData[CAN_RX_BUF_LEN];
    U16             u16PackDataNum;
    U16             u16PackNum;
    U16             u16PackCrc;
    U32             u32FileCrc;
    U32				u32FileByteNums;
    U8              u8MsgSourceAddr;
    U8              u8MsgTargetAddr;
    BOOT_STEP_CMD   enumUpdataStep;
}strCAN_REC_BUF;

extern SemaphoreHandle_t uartUpgradeBinarySemaphore;
extern TaskHandle_t NormalUpdataTask_Handler;
extern volatile const U8 g_boot_version[16];

extern void Boot_Init(void);
extern U16 crc16_ccitt_xmodem(const U8* data, U32 len, U16 u16CrcInitVal);
extern void SystemResetFunc(void);
extern void Board_Addr_Init(void);

extern void SuspendNormalUpdateTaskFunc(void);
extern void UpdataFuncInitTask(void * pvParameters);
extern BOOL blDetcetUpdataStep(BOOT_STEP_CMD RecStep);

#endif /* HAL_BOOT_BOOT_H_ */
