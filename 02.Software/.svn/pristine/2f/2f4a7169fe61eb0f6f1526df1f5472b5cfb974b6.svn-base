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
#include "hal_sys_IF.h"
#include "fsl_cache.h"
#include "string.h"
#include "Signal.h"
#include "SignalManage.h"

static flexspi_nor_config_t norConfig = {0};

void SaveSignal2FlashFunc(U32 u32SigId, S32 s32SigVal)
{
	return;
}


void FLEXSPI_NorFlash_GetConfig(flexspi_nor_config_t *config)
{
    config->memConfig.tag              = FLEXSPI_CFG_BLK_TAG;
    config->memConfig.version          = FLEXSPI_CFG_BLK_VERSION;
    config->memConfig.readSampleClkSrc = kFLEXSPIReadSampleClk_LoopbackFromDqsPad;
    config->memConfig.serialClkFreq =
        kFLEXSPISerialClk_133MHz;          /* Serial Flash Frequencey.See System Boot Chapter for more details */
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
    config->ipcmdSerialClkFreq      = kFLEXSPISerialClk_30MHz; /* Clock frequency for IP command */

    /* Fast Read Quad I/O */
    config->memConfig.lookupTable[4U * NOR_CMD_LUT_SEQ_IDX_READ + 0U] =
        FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0xebU, RADDR_SDR, FLEXSPI_4PAD, 0x18U);
    config->memConfig.lookupTable[4U * NOR_CMD_LUT_SEQ_IDX_READ + 1U] =
        FSL_ROM_FLEXSPI_LUT_SEQ(DUMMY_SDR, FLEXSPI_4PAD, 0x06U, READ_SDR, FLEXSPI_4PAD, 0x4U);

    /* Read Status */
    config->memConfig.lookupTable[4U * NOR_CMD_LUT_SEQ_IDX_READSTATUS] =
        FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x05U, READ_SDR, FLEXSPI_1PAD, 0x1U);

    /* Write Enable */
    config->memConfig.lookupTable[4U * NOR_CMD_LUT_SEQ_IDX_WRITEENABLE] =
        FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x06U, STOP, FLEXSPI_1PAD, 0x0U);

    /* Page Program - quad mode */
    config->memConfig.lookupTable[4U * NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM + 0U] =
        FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x32U, RADDR_SDR, FLEXSPI_1PAD, 0x18U);
    config->memConfig.lookupTable[4U * NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM + 1U] =
        FSL_ROM_FLEXSPI_LUT_SEQ(WRITE_SDR, FLEXSPI_4PAD, 0x04U, STOP, FLEXSPI_1PAD, 0x0U);

    /* Sector Erase */
    config->memConfig.lookupTable[4U * NOR_CMD_LUT_SEQ_IDX_ERASESECTOR] =
        FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0xD7U, RADDR_SDR, FLEXSPI_1PAD, 0x18U);

    /* Block Erase */
    config->memConfig.lookupTable[4U * NOR_CMD_LUT_SEQ_IDX_ERASEBLOCK] =
        FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0xD8U, RADDR_SDR, FLEXSPI_1PAD, 0x18U);
}

status_t FLEXSPI_NorFlash_GetVendorID(uint32_t instance, uint32_t *vendorID)
{
    uint32_t lut_seq[4];
    memset(lut_seq, 0, sizeof(lut_seq));
    // Read manufacturer ID
    lut_seq[0] = FSL_ROM_FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x9F, READ_SDR, FLEXSPI_1PAD, 4);
    ROM_FLEXSPI_NorFlash_UpdateLut(instance, NOR_CMD_LUT_SEQ_IDX_READID, (const uint32_t *)lut_seq, 1U);

    flexspi_xfer_t xfer;
    xfer.operation            = kFLEXSPIOperation_Read;
    xfer.seqId                = NOR_CMD_LUT_SEQ_IDX_READID;
    xfer.seqNum               = 1U;
    xfer.baseAddress          = 0U;
    xfer.isParallelModeEnable = false;
    xfer.rxBuffer             = vendorID;
    xfer.rxSize               = 1U;

    uint32_t status = ROM_FLEXSPI_NorFlash_CommandXfer(instance, &xfer);
    if (*vendorID != kSerialFlash_ISSI_ManufacturerID)
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
    memset(&norConfig, 0U, sizeof(flexspi_nor_config_t));

    /* Setup FLEXSPI NOR Configuration Block */
    FLEXSPI_NorFlash_GetConfig(&norConfig);

#if (1)
    /* Initializes the FLEXSPI module for the other FLEXSPI APIs */
    status = ROM_FLEXSPI_NorFlash_Init(FlexSpiInstance, &norConfig);
    if (status == kStatus_Success)
    {
        uPRINTF("\r\n Successfully init flash\r\n ");
    }
    else
    {
        uPRINTF("\r\n Erase sector failure !\r\n");
    }
    /* Perform software reset after initializing flexspi module */
    ROM_FLEXSPI_NorFlash_ClearCache(FlexSpiInstance);
#endif
    /* Probe device presence by verifying Manufacturer ID */
    status = FLEXSPI_NorFlash_GetVendorID(FlexSpiInstance, &vendorID);
    if (status == kStatus_Success)
    {
    	uPRINTF("Flash vendor ID: 0x%x\r\n", vendorID);
        return true;
    }

    uPRINTF("\r\n Serial flash verndorID read failed!\r\n");
    return false;
}

BOOL flexspi_flash_buffer_erase_backup_app_area(uint32_t FlashBeginAddr)
{
	status_t status = kStatus_Success;
	uint32_t minSize = FLASH_BLOCK_SIZE;
	uint32_t serialNorBlockNum = APP_TOTAL_SIZE/minSize;

	if(FlashBeginAddr%4096 == 0)	/*不能擦除不对齐的片区*/
	{
		taskENTER_CRITICAL();
		for(uint16_t i = 0;i < serialNorBlockNum;i++)
		{
			FeedWatchDog();
			status = ROM_FLEXSPI_NorFlash_Erase(FlexSpiInstance, &norConfig, CLEAR_BASE(FlashBeginAddr + (i*minSize)), minSize);
			if(status != kStatus_Success)
			{
				taskEXIT_CRITICAL();
				return FALSE;
			}
			AllLedToggle();
		}
		taskEXIT_CRITICAL();
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

	if(FlashBeginAddr%4096 == 0)	/*不能擦除不对齐的片区*/
	{
		taskENTER_CRITICAL();
		for(uint16_t i = 0;i < serialNorSectorNum;i++)
		{
			status = ROM_FLEXSPI_NorFlash_Erase(FlexSpiInstance, &norConfig, CLEAR_BASE(FlashBeginAddr + (i*FLASH_SECTOR_SIZE)), norConfig.sectorSize);
			if(status != kStatus_Success)
			{
				taskEXIT_CRITICAL();
				return FALSE;
			}
		}
		taskEXIT_CRITICAL();
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}

void flexspi_flash_buffer_read(uint8_t *FlashRdBuf, uint32_t FlashBeginAddr,uint16_t DataLen)
{
	taskENTER_CRITICAL();

	memcpy(FlashRdBuf, (void *)(FlashBeginAddr), DataLen);

	taskEXIT_CRITICAL();
}

BOOL flexspi_flash_buffer_write_a_page(uint8_t *FlashWrBuf, uint32_t FlashBeginAddr)
{
	status_t status;

	taskENTER_CRITICAL();

	status = ROM_FLEXSPI_NorFlash_ProgramPage(FlexSpiInstance, &norConfig, CLEAR_BASE(FlashBeginAddr), (const uint32_t *)FlashWrBuf);

	taskEXIT_CRITICAL();

	if(status != kStatus_Success)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL ExtFlashClrBackupApp(void)
{
	BOOL blRet = FALSE;

	//擦除新APP区整个区域
	blRet = flexspi_flash_buffer_erase_backup_app_area(NEW_APP_FLASH_ADDR_START);

    return blRet;
}
