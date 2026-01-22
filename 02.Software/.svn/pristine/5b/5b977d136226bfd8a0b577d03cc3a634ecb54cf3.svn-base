/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */  

/* Includes ------------------------------------------------------------------*/
#include "gd32f4xx.h"
#include <stdio.h>
#include "string.h"
#include "boot.h"
#include "flash_program.h"
#include "led.h"

typedef        void (*pFunction)(void);
pFunction       Jump_To_Application;
uint32_t       JumpAddress;

/******************************************************************************
* 名  	称:  SystemResetFunc
* 功  	能:  重启程序
* 入口参数:  无
* 出口参数:  无
*******************************************************************************/
void SystemResetFunc(void)
{
	__set_FAULTMASK(1);			//关闭所有中断
	__disable_irq();
	NVIC_SystemReset();			//复位
	for(;;);
}

void Boot2AppCtl(void)
{
	if (((*(__IO uint32_t*)ApplicationAddress) & 0x2FFE0000 ) == 0x20000000)
	{
		__disable_irq();

		/* 上锁Flash */	
		fmc_lock();

		JumpAddress = *(__IO uint32_t*) (ApplicationAddress + 4);

		Jump_To_Application = (pFunction) JumpAddress;  //APP地址写入

		__set_MSP(*(__IO uint32_t*) ApplicationAddress);

		/* 重定向中断向量表 */
		SCB->VTOR = ApplicationAddress;		

		Jump_To_Application();
	}
}

void Boot2AppFunc(void)
{
	//有升级请求标志 或 运行APP完整标志不存在 则进入更新APP流程
	if((TRUE == CheckUpdataFg())
		|| (TRUE != CheckAppIntegFg(APP_INTEG_FG_FLASH_ADDR)))
	{
		//升级APP完整标志存在时，将升级APP更新至运行APP中
		if (TRUE == CheckAppIntegFg(UPDATA_APP_INTEG_FG_FLASH_ADDR))
		{
			//擦除运行APP的FLASH段
			if(TRUE == EraseUpdateFlashPage(FLASH_WR_BEGIN_ADDR))
			{
				//更新APP
				if(TRUE == TransferFlashData(UPDATA_FLASH_WR_BEGIN_ADDR, FLASH_WR_BEGIN_ADDR, APP_FLASH_PAGE_NUM))
				{
					//清除升级请求标志
					ClearUpdataFg();
				}
			}
		}
	}

	if(TRUE == CheckAppIntegFg(APP_INTEG_FG_FLASH_ADDR))
	{
		Boot2AppCtl();
	}
}


/******************************************************************************
* 名  	称:	TransferFlashData
* 功  	能:	把源头FLASH地址数据转存到目标FLASH地址
* 入口参数:	u32SourceBeginAddr:源头FLASH开始地址
* 			u32TargetBeginAddr:目标FLASH开始地址
* 			u32FlashPageNum:要转移数据的FLASH页数
* 出口参数:	BOOL
*******************************************************************************/
BOOL TransferFlashData(const U32 u32SourceBeginAddr, const U32 u32TargetBeginAddr, const U32 u32FlashPageNum)
{
	BOOL blRet = TRUE;
	U8 u8FlashWrBuf[8] = {0};
	U16 u16index = 0;
	U16 u16RdWrFlashTimeNum = 0;
	U16 u16RdWrFlashTimeCnt = 0;
	U32 u32FlashRdAddr = 0;
	U32 u32FlashWrAddr = 0;
	U16 u16Data0xFFcnt = 0;
	U32 u32WrErrCnt = 0;

	u16RdWrFlashTimeNum = u32FlashPageNum * FMC_PAGE_SIZE / 8;

	for(u16RdWrFlashTimeCnt = 0; u16RdWrFlashTimeCnt < u16RdWrFlashTimeNum; u16RdWrFlashTimeCnt++)
	{
		fwdgt_counter_reload();
        LedToggle();
		
		u32FlashRdAddr = u32SourceBeginAddr + u16RdWrFlashTimeCnt*8;
		u32FlashWrAddr = u32TargetBeginAddr + u16RdWrFlashTimeCnt*8;

		memset(u8FlashWrBuf, 0xFF, 8);

		u16Data0xFFcnt = 0;

		for(u16index = 0; u16index < 8; u16index++)
		{
			u8FlashWrBuf[u16index] = *(__IO uint8_t*)(u32FlashRdAddr + u16index);

			if(0xFF == u8FlashWrBuf[u16index])
			{
				u16Data0xFFcnt++;
			}
		}

		if((UPDATA_APP_INTEG_FG_FLASH_ADDR != u32FlashRdAddr)
				&& (8 != u16Data0xFFcnt)
				)
		{
			if(TRUE != Write32BitsDataToIntFlash(u32FlashWrAddr, u8FlashWrBuf, 8))
			{
				u32WrErrCnt++;
			}
		}
	}

	//最后才写入完整标志位
	if(0 == u32WrErrCnt)
	{
		blRet = WriteAppInterFg(u32TargetBeginAddr + 0x20);
	}
	else
	{
		blRet = FALSE;
	}

	return blRet;
}

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
