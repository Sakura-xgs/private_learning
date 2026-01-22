/*
 * flash_data_IF.h
 *
 *  Created on: 2024年8月19日
 *      Author: Bono
 */

#ifndef HAL_FLASH_DATA_FLASH_DATA_IF_H_
#define HAL_FLASH_DATA_FLASH_DATA_IF_H_
#include "fsl_romapi.h"
#include "PublicDefine.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define FlexSpiInstance           		0U
#define EXAMPLE_FLEXSPI_AMBA_BASE 		FlexSPI_AMBA_BASE
#define FLASH_SIZE                		0x800000UL /* 8MBytes */
#define FLASH_PAGE_SIZE           		256UL      /* 256Bytes */
#define FLASH_SECTOR_SIZE         		0x1000UL   /* 4KBytes */
#define FLASH_BLOCK_SIZE          		0x10000UL  /* 64KBytes */
#define BUFFER_LEN 						FLASH_PAGE_SIZE

#define APP_TOTAL_SIZE					(0x80000UL)	//APP total size is 512KB
#define APP_INTEG_FG_OFFSET				(0x100UL)
#define APP_INTEG_SIZE					(8)			//APP integ_flag's length 8 bytes
#define APP_FLASH_ADDR_START  			(0x60010000UL)//APP begin address
#define APP_INTEG_FG_FLASH_ADDR_START	(APP_FLASH_ADDR_START + APP_INTEG_FG_OFFSET)//APP integ_flag address
#define APP_DATA_FLASH_ADDR_START		(APP_FLASH_ADDR_START)//APP integ_flag address
#define BACK_APP_FLASH_ADDR_START  		(APP_FLASH_ADDR_START + APP_TOTAL_SIZE)
#define BACK_APP_INTEG_FLASH_ADDR_START (BACK_APP_FLASH_ADDR_START + APP_INTEG_FG_OFFSET)
#define BACK_APP_DATA_FLASH_ADDR_START 	(BACK_APP_FLASH_ADDR_START)
#define NEW_APP_FLASH_ADDR_START  		(BACK_APP_FLASH_ADDR_START + APP_TOTAL_SIZE)
#define NEW_APP_INTEG_FLASH_ADDR_START  (NEW_APP_FLASH_ADDR_START + APP_INTEG_FG_OFFSET)
#define NEW_APP_DATA_FLASH_ADDR_START  	(NEW_APP_FLASH_ADDR_START)

#define APP_FLASH_BASE  				(0x60000000UL)
#define CLEAR_BASE(A)					(A - APP_FLASH_BASE)


extern void SaveSignal2FlashFunc(U32 u32SigId, S32 s32SigVal);

extern BOOL Hal_Norflash_Init(void);
extern void FLEXSPI_NorFlash_GetConfig(flexspi_nor_config_t *config);
extern status_t FLEXSPI_NorFlash_GetVendorID(uint32_t instance, uint32_t *vendorID);
extern BOOL flexspi_flash_buffer_erase_sectors(uint32_t FlashBeginAddr,uint16_t serialNorSectorNum);
extern void flexspi_flash_buffer_read(uint8_t *FlashRdBuf, uint32_t FlashBeginAddr,uint16_t DataLen);
extern BOOL flexspi_flash_buffer_write_a_page(uint8_t *FlashWrBuf, uint32_t FlashBeginAddr);
extern BOOL ExtFlashClrBackupApp(void);

#endif /* HAL_FLASH_DATA_FLASH_DATA_IF_H_ */
