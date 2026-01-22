/*******************************************************************************
Copyright (C), 2009, PowerWise Tech. Co., Ltd.
-------------------------------------------------------------------------------|
ProjectName:	
-------------------------------------------------------------------------------|
Description:	Driver layer
-------------------------------------------------------------------------------|
Version:		0.1
-------------------------------------------------------------------------------|
Function List:
	1. 
-------------------------------------------------------------------------------|
History:
	<author>		<time>		<version >		<desc>
			    2010-04		0.1	            build this moudle
*******************************************************************************/
#include "gd32f4xx.h"
#include <stdio.h>
#include "string.h"
#include "PublicDefine.h"
#include "flash_program.h"
#include "led.h"

/***********************************变量定义*******************************/
/************************************函数定义********************************/

/******************************************************************************
* 名  	称:  EraseUpdateFlashPage
* 功  	能:  清除所有非BOOT的FLASH Sector
* 入口参数:  无
* 出口参数:  TRUE:操作成功 FALSE:操作失败
*******************************************************************************/
BOOL EraseUpdateFlashPage(const U32 u32FlashAddr)
{
	BOOL blRet = TRUE;	
	fmc_state_enum fmc_state = FMC_READY;
    uint32_t EraseCounter;
    
    /* unlock the flash program/erase controller */
    fmc_unlock();

    /* clear all pending flags */
	fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR 
                    | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);

    /* erase the flash pages */
    for(EraseCounter = 0; EraseCounter < APP_FLASH_PAGE_NUM; EraseCounter++)
    {
        LedToggle();
        
        fmc_state = fmc_page_erase(u32FlashAddr + (FMC_PAGE_SIZE * EraseCounter));


        fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR 
                        | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);

		if(FMC_READY != fmc_state)
		{
			blRet = FALSE;
			break;
		}
    }

    /* lock the main FMC after the erase operation */
    fmc_lock();

    return blRet;
}

/******************************************************************************
*名称:		WriteAppInterFg
*功能:		写入程序升级成功标记
*入口参数:	Flash写入地址
*出口参数:	操作标志
*******************************************************************************/
BOOL WriteAppInterFg(U32 u32FlashAddr)
{
	U8 blRet = FALSE;
	U8 app_integ_tab[8] = {'A','P','P','I','N','T','E','G'};

	blRet = Write32BitsDataToIntFlash(u32FlashAddr, app_integ_tab, 8);

	return blRet;
}

/******************************************************************************
* 名  	称:  CheckAppIntegFg
* 功  	能: 获取APP完整标志
* 入口参数:  无
* 出口参数:  无
*******************************************************************************/
BOOL CheckAppIntegFg(const U32 u32FlashAddr)
{
	U8 i=0;
	uint32_t temp=0;
	U8 temp_data[8]={0};
	U8 app_integ_tab[8] = {'A','P','P','I','N','T','E','G'};

	temp=*(__IO uint16_t*)u32FlashAddr;
	temp_data[0]=temp;
	temp_data[1]=temp>>8;

	temp=*(__IO uint16_t*)(u32FlashAddr+2);
	temp_data[2]=temp;
	temp_data[3]=temp>>8;

	temp=*(__IO uint16_t*)(u32FlashAddr+4);
	temp_data[4]=temp;
	temp_data[5]=temp>>8;

	temp=*(__IO uint16_t*)(u32FlashAddr+6);
	temp_data[6]=temp;
	temp_data[7]=temp>>8;

	for(i=0;i<8;i++)
	{
		if(app_integ_tab[i]!=temp_data[i])
		{
			return FALSE;
		}
	}

	return TRUE;
}

/******************************************************************************
* 名  	称:  ClearIntFlashSector
* 功  	能:  清除指定的sector
* 入口参数:  无
* 出口参数:  TRUE:操作成功 FALSE:操作失败
*******************************************************************************/
BOOL ClearIntFlashSector(U32 u32FlashAddr)
{
    BOOL blRet = TRUE;
	fmc_state_enum fmc_state = FMC_READY;
    
    /* unlock the flash program/erase controller */
    fmc_unlock();

    /* clear all pending flags */
	fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR 
                    | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);

	fmc_state = fmc_page_erase(u32FlashAddr);

	fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR 
                    | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);

	if(FMC_READY != fmc_state)
	{
		blRet = FALSE;
	}

    /* lock the main FMC after the erase operation */
    fmc_lock();

    return blRet;
}

/******************************************************************************
* 名  	称:  Write16BitsDataToIntFlash
* 功  	能:  在内部FLASH中写入数据
* 入口参数:
* 出口参数:
*******************************************************************************/
BOOL Write16BitsDataToIntFlash(U32 addr, P_U8 pu8WrBuf, U32 len)
{
	U8 index = 0;
	fmc_state_enum fmc_state = FMC_READY;
	BOOL blRet = TRUE;
	uint64_t u64WrData = 0;

	/* unlock the flash program/erase controller */
    fmc_unlock();

    /* clear all pending flags */
	fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR 
                    | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);

	/* program flash */
	for(index = 0; index < len/2; index++)
	{
		u64WrData = (((uint64_t)pu8WrBuf[index*2+1]) << 8)  |
					(((uint64_t)pu8WrBuf[index*2+0]) << 0);

        fmc_state = fmc_halfword_program(addr+index*2, u64WrData);
		
		fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR 
                        | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);

		if(FMC_READY != fmc_state)
		{
			blRet = FALSE;
			break;
		}
	}

    /* lock the main FMC after the program operation */
    fmc_lock();

	return blRet;
}

/******************************************************************************
* 名  	称:  Write32BitsDataToIntFlash
* 功  	能:  在内部FLASH中写入数据
* 入口参数:
* 出口参数:
*******************************************************************************/
BOOL Write32BitsDataToIntFlash(U32 addr, P_U8 pu8WrBuf, U32 len)
{
	U8 index = 0;
	fmc_state_enum fmc_state = FMC_READY;
	BOOL blRet = TRUE;
	uint64_t u64WrData = 0;

	/* unlock the flash program/erase controller */
    fmc_unlock();

    /* clear all pending flags */
	fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR 
                    | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);

	for(index = 0; index < len/4; index++)
	{
		u64WrData = (((uint64_t)pu8WrBuf[index*4+3]) << 24) |
					(((uint64_t)pu8WrBuf[index*4+2]) << 16) |
					(((uint64_t)pu8WrBuf[index*4+1]) << 8)  |
					(((uint64_t)pu8WrBuf[index*4+0]) << 0);

        fmc_state = fmc_word_program(addr+index*4, u64WrData);

		fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR 
                        | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);

		if(FMC_READY != fmc_state)
		{
			blRet = FALSE;
			break;
		}
	}

	/* lock the main FMC after the program operation */
    fmc_lock();

	return blRet;
}

/******************************************************************************
* 名  	称:  ClearUpdataFg
* 功  	能:  清除升级标记
* 入口参数:
* 出口参数:
*******************************************************************************/
void ClearUpdataFg(void)
{
    /* wait until last write operation on RTC registers has finished */
    rtc_lwoff_wait();
    bkp_write_data(RTC_BKP10, (uint16_t)0xFFFF);
}

/******************************************************************************
* 名  	称:  CheckUpdataFg
* 功  	能:  获取升级标记
* 入口参数:
* 出口参数:
*******************************************************************************/
BOOL CheckUpdataFg(void)
{
	BOOL blRet = FALSE;
	U32 u32GetVal = 0;

	/* wait until last write operation on RTC registers has finished */
    rtc_lwoff_wait();
	u32GetVal = bkp_read_data(RTC_BKP10);

	if(0x55AA == u32GetVal)
	{
		blRet = TRUE;
	}

	return blRet;
}
/****************************************/
