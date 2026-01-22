#ifndef __FLASH_PROGRAM_H
#define __FLASH_PROGRAM_H

#include "PublicDefine.h"


/**** 定义常量区域 ****/


/**** 定义宏表达式区域 ****/
#define FMC_PAGE_SIZE								((uint16_t)0x1000U)

// #define	ApplicationAddress              (0x08004200)				//APP入口地址
// #define APP_SOFTWARE_VERSION_ADDR 		(0x08004000)				//APP版本信息存放地址
#define str_APP_SOFTWARE_VERSION_ADDR 	"0x08004000"				//APP版本信息存放地址
// #define APP_INTEG_FG_FLASH_ADDR 	    (0x08004020)				//APP完整标志存放地址
#define str_APP_INTEG_FG_FLASH_ADDR 	"0x08004020"				//APP完整标志存放地址
// #define APP_PROJECT_VERSION_ADDR 		(0x08004030)				//项目软件版本
// #define FLASH_WR_BEGIN_ADDR				(APP_SOFTWARE_VERSION_ADDR)	//允许操作的FLASH地址开始地址
// #define FLASH_WR_END_ADDR               (0x08022000)			    //允许操作的FLASH地址结束地址

#define UPDATA_FLASH_WR_BEGIN_ADDR      (0x08022000)				        //允许升级下载的FLASH地址开始地址
#define UPDATA_APP_INTEG_FG_FLASH_ADDR  (0x08022020)				        //升级APP完整标志存放地址
#define UPDATA_FLASH_WR_END_ADDR        (0x08080000)				        //允许升级下载的FLASH地址结束地址

#define UPDATA_FLASH_PAGE_NUM           ((UPDATA_FLASH_WR_END_ADDR - UPDATA_FLASH_WR_BEGIN_ADDR) / FMC_PAGE_SIZE)  //升级时允许操作的FLASH总页数


BOOL FlashClearBackupApp(void);
BOOL FlashBufferWrite(uint32_t addr, uint8_t *data, uint16_t len);
BOOL FlashBufferRead(uint32_t addr, uint8_t *data, uint16_t len);

#endif

