/*
 * flash_data.c
 *
 *  Created on: 2024年8月19日
 *      Author: Bono
 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include "string.h"
#include "PublicDefine.h"
#include "flash_data_IF.h"
#include "string.h"
#include "Signal.h"
#include "SignalManage.h"

#include "clock_config.h"
#include "fsl_common.h"

#include "hal_sys_IF.h"

/*! @brief FLEXSPI NOR flash driver Structure */
static flexspi_nor_config_t norConfig={0};

void SaveSignal2FlashFunc(U32 u32SigId, S32 s32SigVal)
{
	return;
}

/* Get FLEXSPI NOR Configuration Block */
static void FLEXSPI_NorFlash_GetConfig(flexspi_nor_config_t *config)
{
    config->memConfig.tag              = FLEXSPI_CFG_BLK_TAG;
    config->memConfig.version          = FLEXSPI_CFG_BLK_VERSION;
    config->memConfig.readSampleClkSrc = (U8)kFLEXSPIReadSampleClk_LoopbackFromDqsPad;
    config->memConfig.serialClkFreq =
        (U8)kFLEXSPISerialClk_133MHz;          /* Serial Flash Frequencey.See System Boot Chapter for more details */
    config->memConfig.sflashA1Size   = FLASH_SIZE;
    config->memConfig.csHoldTime     = 3U; /* Data hold time, default value: 3 */
    config->memConfig.csSetupTime    = 3U; /* Date setup time, default value: 3 */
    config->memConfig.deviceType     = kFLEXSPIDeviceType_SerialNOR; /* Flash device type default type: Serial NOR */
    config->memConfig.deviceModeType = kDeviceConfigCmdType_Generic;
    config->memConfig.columnAddressWidth  = 0U;
    config->memConfig.deviceModeCfgEnable = 0U;

    config->memConfig.waitTimeCfgCommands = 0U;
    config->memConfig.configCmdEnable     = 0U;
    /* Always enable Safe configuration Frequency */
    config->memConfig.controllerMiscOption = FSL_ROM_FLEXSPI_BITMASK(kFLEXSPIMiscOffset_SafeConfigFreqEnable);
    config->memConfig.sflashPadType = kSerialFlash_4Pads; /* Pad Type: 1 - Single, 2 - Dual, 4 - Quad, 8 - Octal */
    config->pageSize                = FLASH_PAGE_SIZE;
    config->sectorSize              = FLASH_SECTOR_SIZE;
    config->blockSize               = FLASH_BLOCK_SIZE;
    config->ipcmdSerialClkFreq      = (U8)kFLEXSPISerialClk_30MHz; /* Clock frequency for IP command */

    /* Fast Read Quad I/O */
    config->memConfig.lookupTable[(4U * NOR_CMD_LUT_SEQ_IDX_READ) + 0U] =
        FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0xebU, RADDR_SDR, FLEXSPI_4PAD, 0x18U);
    config->memConfig.lookupTable[(4U * NOR_CMD_LUT_SEQ_IDX_READ) + 1U] =
        FSL_ROM_FLEXSPI_LUT_SEQ(DUMMY_SDR, FLEXSPI_4PAD, 0x06U, READ_SDR, FLEXSPI_4PAD, 0x4U);

    /* Read Status */
    config->memConfig.lookupTable[4U * NOR_CMD_LUT_SEQ_IDX_READSTATUS] =
        FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x05U, READ_SDR, FLEXSPI_1PAD, 0x1U);

    /* Write Enable */
    config->memConfig.lookupTable[4U * NOR_CMD_LUT_SEQ_IDX_WRITEENABLE] =
        FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x06U, STOP, FLEXSPI_1PAD, 0x0U);

    /* Page Program - quad mode */
    config->memConfig.lookupTable[(4U * NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM) + 0U] =
        FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x32U, RADDR_SDR, FLEXSPI_1PAD, 0x18U);

    config->memConfig.lookupTable[(4U * NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM) + 1U] =
        FSL_ROM_FLEXSPI_LUT_SEQ(WRITE_SDR, FLEXSPI_4PAD, 0x04U, STOP, FLEXSPI_1PAD, 0x0U);

    /* Sector Erase */
    config->memConfig.lookupTable[4U * NOR_CMD_LUT_SEQ_IDX_ERASESECTOR] =
        FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0xD7U, RADDR_SDR, FLEXSPI_1PAD, 0x18U);

    /* Block Erase */
    config->memConfig.lookupTable[4U * NOR_CMD_LUT_SEQ_IDX_ERASEBLOCK] =
        FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0xD8U, RADDR_SDR, FLEXSPI_1PAD, 0x18U);
}

static status_t FLEXSPI_NorFlash_GetVendorID(uint32_t instance, uint32_t *vendorID)
{
    uint32_t lut_seq[4];
    (void)memset(lut_seq, 0, sizeof(lut_seq));
    // Read manufacturer ID
    lut_seq[0] = FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x9F, READ_SDR, FLEXSPI_1PAD, 4);
    (void)ROM_FLEXSPI_NorFlash_UpdateLut(instance, NOR_CMD_LUT_SEQ_IDX_READID, (const uint32_t *)lut_seq, 1U);

    flexspi_xfer_t xfer;
    xfer.operation            = kFLEXSPIOperation_Read;
    xfer.seqId                = NOR_CMD_LUT_SEQ_IDX_READID;
    xfer.seqNum               = 1U;
    xfer.baseAddress          = 0U;
    xfer.isParallelModeEnable = false;
    xfer.rxBuffer             = vendorID;
    xfer.rxSize               = 1U;

    status_t status = ROM_FLEXSPI_NorFlash_CommandXfer(instance, &xfer);
    if (*vendorID != (U32)kSerialFlash_ISSI_ManufacturerID)
    {
        status = kStatus_ROM_FLEXSPINOR_Flash_NotFound;
        return status;
    }

    return status;
}

BOOL Hal_Norflash_Init(void)
{
    status_t status;
    uint32_t vendorID = 0U;

    /* Clean up FLEXSPI NOR flash driver Structure */
    (void)memset(&norConfig, 0U, sizeof(flexspi_nor_config_t));

    /* Setup FLEXSPI NOR Configuration Block */
    FLEXSPI_NorFlash_GetConfig(&norConfig);

#if (1)
    /* Initializes the FLEXSPI module for the other FLEXSPI APIs */
    status = ROM_FLEXSPI_NorFlash_Init(FlexSpiInstance, &norConfig);
    if (status == kStatus_Success)
    {
       // uPRINTF("\r\n Successfully init flash\r\n ");
    }
    else
    {
       // uPRINTF("\r\n Erase sector failure !\r\n");
    }
    /* Perform software reset after initializing flexspi module */
    ROM_FLEXSPI_NorFlash_ClearCache(FlexSpiInstance);
#endif
    /* Probe device presence by verifying Manufacturer ID */
    status = FLEXSPI_NorFlash_GetVendorID(FlexSpiInstance, &vendorID);
    if (status == kStatus_Success)
    {
    	//uPRINTF("Flash vendor ID: 0x%x\r\n", vendorID);
        return TRUE;
    }

    //uPRINTF("\r\n Serial flash verndorID read failed!\r\n");
    return FALSE;

}

bool flexspi_flash_buffer_write(const uint8_t *FlashWrBuf, uint32_t FlashBeginAddr,uint16_t DATALEN)
{
	status_t status;

	__disable_irq();

	if((FlashBeginAddr%4096U) == 0U)
	{
		status = ROM_FLEXSPI_NorFlash_Erase(FlexSpiInstance, &norConfig,FlashBeginAddr, norConfig.sectorSize);

		if(status != kStatus_Success)
		{
			__enable_irq();
			//擦除失败
			return FALSE;
		}
	}

	status = ROM_FLEXSPI_NorFlash_ProgramPage(FlexSpiInstance, &norConfig, FlashBeginAddr, (const uint32_t *)FlashWrBuf);

	__enable_irq();

	if(status != kStatus_Success)
	{
		//写失败
		return FALSE;
	}
	return TRUE;
}

void flexspi_flash_buffer_read(uint8_t *FlashRdBuf, uint32_t FlashBeginAddr,uint16_t DATALEN)
{
	taskENTER_CRITICAL();

	//DCACHE_InvalidateByRange(FlashBeginAddr, DATALEN);

	(void)memcpy(FlashRdBuf, (void *)(FlashBeginAddr), DATALEN);

	taskEXIT_CRITICAL();
}

BOOL flexspi_flash_buffer_erase_backup_app_area(uint32_t FlashBeginAddr)
{
	status_t status = kStatus_Success;
	uint32_t minSize = FLASH_BLOCK_SIZE;
	uint32_t serialNorBlockNum = APP_TOTAL_SIZE/minSize;

	if((FlashBeginAddr%4096U) == 0U)	/*不能擦除不对齐的片区*/
	{
		__disable_irq();
		for(uint16_t i = 0;i < serialNorBlockNum;i++)
		{
			FeedWatchDog();
			status = ROM_FLEXSPI_NorFlash_Erase(FlexSpiInstance, &norConfig, CLEAR_BASE(FlashBeginAddr + (i*minSize)), minSize);

			if(status != kStatus_Success)
			{
				__enable_irq();
				return FALSE;
			}
			AllLedToggle();
		}
		__enable_irq();
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}

BOOL flexspi_flash_buffer_erase_sectors(uint32_t FlashBeginAddr,uint16_t serialNorSectorNum)
{
	status_t status = kStatus_Success;

	if((FlashBeginAddr%4096U) == 0U)	/*不能擦除不对齐的片区*/
	{
		__disable_irq();

		for(uint16_t i = 0;i < serialNorSectorNum;i++)
		{
			status = ROM_FLEXSPI_NorFlash_Erase(FlexSpiInstance, &norConfig, CLEAR_BASE(FlashBeginAddr + (i*FLASH_SECTOR_SIZE)), norConfig.sectorSize);
			if(status != kStatus_Success)
			{
				__enable_irq();

				return FALSE;
			}
		}
		__enable_irq();
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}

BOOL flexspi_flash_buffer_write_a_page(const uint8_t *FlashWrBuf, uint32_t FlashBeginAddr)
{
	status_t status;

	__disable_irq();

	status = ROM_FLEXSPI_NorFlash_ProgramPage(FlexSpiInstance, &norConfig, CLEAR_BASE(FlashBeginAddr), (const uint32_t *)FlashWrBuf);

	if(status != kStatus_Success)
	{
		__enable_irq();

		return FALSE;
	}

	__enable_irq();

	return TRUE;
}

