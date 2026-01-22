#include "gd32f4xx.h"
#include <stdio.h>
#include "string.h"
#include "PublicDefine.h"
#include "flash_program.h"



/******************************************************************************
* 名  	称:  ClearBackupFlashPage
* 功  	能:  清除升级包存放区域T的FLASH page
* 入口参数:  无
* 出口参数:  TRUE:操作成功 FALSE:操作失败
*******************************************************************************/

BOOL FlashClearBackupApp(void)
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
    for(EraseCounter = 0; EraseCounter < UPDATA_FLASH_PAGE_NUM; EraseCounter++)
    {   
        fmc_state = fmc_page_erase(UPDATA_FLASH_WR_BEGIN_ADDR + (FMC_PAGE_SIZE * EraseCounter));

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


BOOL FlashBufferWrite(uint32_t addr, uint8_t *data, uint16_t len)
{
	U16 index = 0;
	fmc_state_enum fmc_state = FMC_READY;
	BOOL blRet = TRUE;

	/* unlock the flash program/erase controller */
    fmc_unlock();

    /* clear all pending flags */
	fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR 
                    | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);

	for(index = 0; index < len; index++)
	{
		__set_PRIMASK(1);
        fmc_state = fmc_byte_program(addr+index, data[index]);
		__set_PRIMASK(0);

		/* clear all pending flags */
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


BOOL FlashBufferRead(uint32_t addr, uint8_t *data, uint16_t len)
{
	U16 index = 0;

	for(index = 0; index < len; index++)
	{
		data[index] = *(__IO uint8_t*)(addr + index);
	}

	return TRUE;
}





