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

//是否开启错误重发机制
#define UPGRADE_RESEND

#define NORMAL_UPDATA_TASK_PRIO                     ( tskIDLE_PRIORITY + 5U )
#define UPDATA_JUMP_TASK_PRIO                       ( tskIDLE_PRIORITY + 3U )

#define str_APP_INTEG_FLAG_ADDR			(NEW_APP_INTEG_FLASH_ADDR_START)

//升级通信超时计时，单位：1ms
#define COMM_TIME_OUT_VAL                           (1000*10)				    //10秒超时

#define MSG_UPDATE_MAX_DATALEN                      (256U)					    //升级包中数据数据最大字节数（起始包与中间包固定为256，结束包不定）

#define	ApplicationAddress                          (0x60010400)				//APP入口地址
#define CCU_FLASH_UPDATA_BEGIN_ADDR					(0x110000)
#define SECC_FLASH_UPDATA_BEGIN_ADDR				(0x190000)
#define APP_SOFTWARE_VERSION_ADDR 				 	(0x60010010)				//APP版本信息存放地址
#define APP_INTEG_FG_FLASH_ADDR 				 	(0x60110100)				//升级区APP完整标志存放地址


#define RESEND_PACK_TIME                            (5)                         //单包发送不成功，重发次数

#define UPDATE_MAX_DATALEN	280

typedef struct
{
	U8 sw_major_version[13];
	U8 sw_minor_version[3];
}SOFT_VER;

typedef enum
{
	NO_UPGRADE = 0,
	CCU_UPGRADE_BY_UART1,
	CCU_UPGRADE_BY_UART2,
	CCU_UPGRADE_BY_UART3,
	CCU_UPGRADE_BY_UART4,
	CCU_UPGRADE_BY_UART5,
	CCU_UPGRADE_BY_UART6,
	CCU_UPGRADE_BY_UART7,
	CCU_UPGRADE_BY_UART8,
	CCU_TCP_UPGRADE,
	SECC_UPGRADE,
}UPGRADE_TYPE;

typedef enum
{
	NO_UPGRADE_EQU = 0,
	TCP_CCU_UPGRADE = 1,
	TCP_SECC_UPGRADE,

}UPGRADE_EQUIPMENT_TYPE;

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
  M2S_UPDATE_UNKNOW_CMD     = 0x00,	//未定义命令
  M2S_UPDATE_REQUEST_CMD    = 0x01,	//升级请求
  M2S_UPDATE_READY_CMD      = 0x02,	//升级准备
  M2S_UPDATE_MIDDLE_CMD    	= 0x03,	//传输数据包
  M2S_UPDATE_CHECKSUM_CMD   = 0x04,	//升级校验
}M2S_UPDATA_STEP_CMD;

typedef enum
{
  UPDATA_UNKNOW   = 0x00,     //未定义
  UPDATA_SUCCESS  = 0xAA,     //升级操作成功
  UPDATA_FAIL     = 0x55,     //升级操作失败
}UPDATA_RET;

typedef enum
{
	M2S_MASTER  = 0x55AA,
	M2S_SLAVE   = 0xAA55
}M2S_UPDATA_FG;

typedef enum
{
  ACK_OK      = 0x00,
  ACK_ERR_EV1 = 0x01,
  ACK_ERR_EV2 = 0x02,
  ACK_ERR_EV3 = 0x03,
  ACK_ERR_EV4 = 0x04,
  ACK_ERR_EV5 = 0x05,
  ACK_ERR_EV6 = 0x06,
  ACK_ERR_EV7 = 0x07,
  ACK_ERR_EV8 = 0x08
}ACK_ERR_TYPE;

typedef struct
{
    U8 u8M2SUpdataBmsNo;
    M2S_UPDATA_STEP_CMD enumM2SUpdataStep;
    U32 u32AppTotalByteNum;
    U16 u16AppTotalBytePackNum;
    U16 u16CalSinglePackCrc;
    U32 u32M2SCalTotalPackCrc;
    U32 u32RecTotalPackCrc;
    UPDATA_RET eumSlaveAck;
    M2S_UPDATA_FG enumM2SUpdataModeFg;
    U8 u8SysBmsNum;
    U8 u8MsgSourceAddr;
    ACK_ERR_TYPE emuAckErrType;
}strM2S_UPDATA_TYPE;

typedef struct Update_t
{
	unsigned char u8Recvbuf[UPDATE_MAX_DATALEN];
	unsigned short u16RecvDataCnt;

}UpdateTypeDef;

extern TaskHandle_t NormalUpdataTask_Handler;
extern TaskHandle_t SeccUpgradeTask_Handler;
extern __RODATA(APP_VERSION) volatile const SOFT_VER cst_sw_no;

static void SuspendNormalUpdateTaskFunc(void);
void ResumeNormalUpdataTaskFunc(void);
extern void SystemResetFunc(void);
extern void UpdateFuncInitTask(void * pvParameters);
extern BOOL blDetcetUpdataStep(BOOT_STEP_CMD RecStep);
extern BOOL UpdataFlashFunc(const P_U8 pu8Data, const U16 u16DataLen, const U16 u16MsgNo);
extern BOOL CheckExtFlashAppIntegFg(void);
extern U32 compute_crc32(const U8 *data, U32 Len, U32 uCurCRC);
extern BOOL ExtFlashClrBackupApp(void);
extern U16 crc16_ccitt_xmodem(const U8* data, U32 len, U16 u16CrcInitVal);
BOOL WriteAppInterFg2Flash(U32 u32FlashAddr, U32 u32FileTotalBytes);

extern volatile const U8 g_boot_version[16];
extern SemaphoreHandle_t uartUpgradeBinarySemaphore;
extern void Boot_Init(void);

#endif /* HAL_BOOT_BOOT_H_ */
