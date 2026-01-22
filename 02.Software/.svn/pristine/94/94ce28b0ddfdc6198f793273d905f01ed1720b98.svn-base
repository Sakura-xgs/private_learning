#ifndef __FLASH_PROGRAM_H
#define __FLASH_PROGRAM_H

#include "PublicDefine.h"


/**** 定义常量区域 ****/


/**** 定义宏表达式区域 ****/
#define FMC_PAGE_SIZE								((uint16_t)0x1000U)


#define APP_SOFTWARE_VERSION_ADDR       (0x08004000)                        //APP版本信息存放地址
#define APP_INTEG_FG_FLASH_ADDR         (0x08004020)                        //APP完整标志存放地址
#define APP_PROJECT_VERSION_ADDR		(0x08004030)				        //项目软件版本信息
#define	ApplicationAddress              (0x08004200)                        //APP入口地址
#define FLASH_WR_BEGIN_ADDR             (APP_SOFTWARE_VERSION_ADDR)         //允许操作的FLASH地址开始地址
#define FLASH_WR_END_ADDR               (0x08022000)				        //允许操作的FLASH地址结束地址

#define UPDATA_FLASH_WR_BEGIN_ADDR      (0x08022000)				        //允许升级下载的FLASH地址开始地址
#define UPDATA_APP_INTEG_FG_FLASH_ADDR  (0x08022020)				        //升级APP完整标志存放地址
#define UPDATA_FLASH_WR_END_ADDR        (0x08080000)				        //允许升级下载的FLASH地址结束地址

/******升级时允许操作的FLASH总页数********/
#define APP_FLASH_PAGE_NUM 							((FLASH_WR_END_ADDR - FLASH_WR_BEGIN_ADDR) / FMC_PAGE_SIZE)

/**** 定义类型区域 ****/
/**** 声明变量区域 ****/
/**** 声明函数区域 ****/
extern BOOL EraseUpdateFlashPage(const U32 u32FlashAddr);
extern BOOL WriteAppInterFg(U32 u32FlashAddr);
extern BOOL CheckAppIntegFg(const U32 u32FlashAddr);
extern BOOL Write32BitsDataToIntFlash(U32 addr, P_U8 pu8WrBuf, U32 len);
extern void ClearUpdataFg(void);
extern BOOL CheckUpdataFg(void);
extern void WriteIntCanUpdataFg(void);
extern void WritePcsCanUpdataFg(void);

#endif

