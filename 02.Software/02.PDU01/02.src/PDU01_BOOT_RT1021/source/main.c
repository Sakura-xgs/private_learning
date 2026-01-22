/*
 * Copyright 2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_romapi.h"
#include "fsl_debug_console.h"
#include "fsl_cache.h"

#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_common.h"
#include "gpio_config.h"
#include "PublicDefine.h"


#define FAST_FLASH_OPERATE	(1)
/*******************************************************************************
 * Definitions
 ******************************************************************************/
typedef void(*JumpToPtr)(void);
#define APP_TOTAL_SIZE		(0x80000)	//APP total size is 512KB
#define APP_INTEG_FG_OFFSET	(0x100)		//APP total size is 512KB
#define APP_INTEG_SIZE		(8)			//APP integ_flag's length 8 byte
#define APP_FLASH_ADDR_START  			(0x60010000)//APP begin address
#define APP_RUN_ADDR_OFFSET  			(0x400)//APP run begin address
#define APP_INTEG_FG_FLASH_ADDR_START	(APP_FLASH_ADDR_START + APP_INTEG_FG_OFFSET)//APP integ_flag address
#define APP_DATA_FLASH_ADDR_START		(APP_FLASH_ADDR_START)//APP integ_flag address
#define BACK_APP_FLASH_ADDR_START  		(APP_FLASH_ADDR_START + APP_TOTAL_SIZE)
#define BACK_APP_INTEG_FLASH_ADDR_START (BACK_APP_FLASH_ADDR_START + APP_INTEG_FG_OFFSET)
#define BACK_APP_DATA_FLASH_ADDR_START 	(BACK_APP_FLASH_ADDR_START)
#define NEW_APP_FLASH_ADDR_START  		(BACK_APP_FLASH_ADDR_START + APP_TOTAL_SIZE)
#define NEW_APP_INTEG_FLASH_ADDR_START  (NEW_APP_FLASH_ADDR_START + APP_INTEG_FG_OFFSET)
#define NEW_APP_DATA_FLASH_ADDR_START  	(NEW_APP_FLASH_ADDR_START)

#define FlexSpiInstance           		0U
#define EXAMPLE_FLEXSPI_AMBA_BASE 		FlexSPI_AMBA_BASE
#define FLASH_SIZE                		0x800000UL /* 8MBytes */
#define FLASH_PAGE_SIZE           		256UL      /* 256Bytes */
#define FLASH_SECTOR_SIZE         		0x1000UL   /* 4KBytes */
#define FLASH_BLOCK_SIZE          		0x10000UL  /* 64KBytes */
#define APP_FLASH_BASE  				(0x60000000UL)
#define CLEAR_BASE(A)					(A - APP_FLASH_BASE)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
#if !(defined(XIP_EXTERNAL_FLASH) && (XIP_EXTERNAL_FLASH == 1))
/* Configure clock for FlexSPI peripheral */
void BOARD_SetupFlexSpiClock(void);
#endif
/* Get FlexSPI NOR Configuration Block */
void FLEXSPI_NorFlash_GetConfig(flexspi_nor_config_t *config);
void error_trap(void);
void app_finalize(void);
void FeedWatchDog(void);
status_t FLEXSPI_NorFlash_GetVendorID(uint32_t instance, uint32_t *vendorID);
/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief FLEXSPI NOR flash driver Structure */
static flexspi_nor_config_t norConfig;
/*! @brief Buffer for program */
static uint8_t s_buffer[FLASH_SECTOR_SIZE];
/*! @brief Buffer for readback */
static uint8_t s_buffer_rbc[FLASH_SECTOR_SIZE];

static uint32_t g_systickCounter;

static uint8_t u8AppIntegBuf[FLASH_PAGE_SIZE] = {0};

/*******************************************************************************
 * Code
 ******************************************************************************/
/* Get FLEXSPI NOR Configuration Block */
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
#if !(defined(XIP_EXTERNAL_FLASH) && (XIP_EXTERNAL_FLASH == 1))
/* Configure clock for FlexSPI peripheral */
void BOARD_SetupFlexSpiClock(void)
{
    /* Disable FlexSPI peripheral */
    FLEXSPI->MCR0 |= FLEXSPI_MCR0_MDIS_MASK;
    CLOCK_DisableClock(kCLOCK_FlexSpi);

    /* Init Usb1 PLL3 to 480MHZ. */
    CLOCK_InitUsb1Pll(&usb1PllConfig_BOARD_BootClockRUN);
    /* Init Usb1 PLL3->pfd0 360MHZ. */
    CLOCK_InitUsb1Pfd(kCLOCK_Pfd0, 24);
    /* Enable Usb1 PLL output for USBPHY1. */
    CCM_ANALOG->PLL_USB1 |= CCM_ANALOG_PLL_USB1_EN_USB_CLKS_MASK;

    /* Set FLEXSPI_PODF, FlexSPI out clock is 60MHZ. */
    CLOCK_SetDiv(kCLOCK_FlexspiDiv, 5);
    /* Set flexspi clock source to PLL3->pfd0 */
    CLOCK_SetMux(kCLOCK_FlexspiMux, 3);

    CLOCK_EnableClock(kCLOCK_FlexSpi);
    /* Enable FlexSPI peripheral */
    FLEXSPI->MCR0 &= ~FLEXSPI_MCR0_MDIS_MASK;
}
#endif

void SysTick_DelayTicks(uint32_t n)
{
    g_systickCounter = n;
    while (g_systickCounter != 0U)
    {
    	g_systickCounter--;
    }
}


/*
 * @brief Gets called when an error occurs.
 *
 * @details Print error message and trap forever.
 */
void error_trap(void)
{
    PRINTF("\r\n\r\n\r\n\t---- HALTED DUE TO FLEXSPI NOR ERROR! ----");
    while (1)
    {
    	/* Delay about 200 ms */
		SysTick_DelayTicks(200*100000U);
		FeedWatchDog();
    }
}

/*
 * @brief Gets called when the app is complete.
 *
 * @details Print finshed message and trap forever.
 */
void app_finalize(void)
{
    /* Print finished message. */
    PRINTF("\r\n End of FLEXSPI NOR Example! \r\n");
    while (1)
    {
    	/* Delay about 200 ms */
    	SysTick_DelayTicks(200*100000U);
    	FeedWatchDog();
    }
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

void SBL_DisablePeripherals(void)
{
    DbgConsole_Deinit();
    SCB_DisableDCache();
    SCB_DisableICache();
    ARM_MPU_Disable();
}

void cleanup(void)
{
    SBL_DisablePeripherals();
}

void Jump_To_App(void)
{
	JumpToPtr	pJumpTo;

	pJumpTo = (JumpToPtr)*(uint32_t*)(APP_FLASH_ADDR_START + APP_RUN_ADDR_OFFSET + 4);

	cleanup();
	__set_CONTROL(0);
	__set_MSP(APP_FLASH_ADDR_START);
	__ISB();

	pJumpTo();

	app_finalize();
}
BOOL CheckAppIntegFg(const U32 u32FlashAddr)
{
	U8 i = 0U;
	U8 temp_data[APP_INTEG_SIZE]={0U};
	U8 app_integ_tab[APP_INTEG_SIZE] = {'A','P','P','I','N','T','E','G'};

    DCACHE_InvalidateByRange(u32FlashAddr, APP_INTEG_SIZE);
    memcpy(temp_data, (void *)(u32FlashAddr), APP_INTEG_SIZE);

    for(i = 0; i < APP_INTEG_SIZE; i++)
    {
    	if(app_integ_tab[i] != temp_data[i])
    	{
    		return FALSE;
    	}
    }

	return TRUE;
}

void FeedWatchDog(void)
{
	WDOG_WDI_TOGGLE();
	RUN_LED_TOGGLE();
}

BOOL BackupRunningAppArea(void)
{
    status_t status;
    uint32_t i        = 0U;
    uint32_t j        = 0U;
    uint32_t AppSize = 0U;
    uint32_t AppSectorNum = APP_TOTAL_SIZE/FLASH_SECTOR_SIZE;

#if FAST_FLASH_OPERATE
    memcpy(s_buffer, (void *)(APP_INTEG_FG_FLASH_ADDR_START + APP_INTEG_SIZE), 4);
    AppSize = (s_buffer[0]<<24) | (s_buffer[1]<<16) | (s_buffer[2]<<8) | (s_buffer[3]);
    if(AppSize == 0
    || AppSize > APP_TOTAL_SIZE)
    {
    	AppSize = APP_TOTAL_SIZE;
    }

    AppSectorNum = (AppSize/FLASH_SECTOR_SIZE)+1;
    AppSectorNum = (AppSectorNum < (APP_TOTAL_SIZE/FLASH_SECTOR_SIZE)) ? (AppSectorNum) : (APP_TOTAL_SIZE/FLASH_SECTOR_SIZE);
#endif
	//back up the original APP
	for(i = 0; i < AppSectorNum; i++)
	{
		FeedWatchDog();
		//Refresh the corresponding data in the D-cache before reading, otherwise may read the values that were previously written in the D-cacheֵ
		DCACHE_InvalidateByRange(APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
		//Read the data from original APP, reading one sector at a time
		memcpy(s_buffer, (void *)(APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE)), FLASH_SECTOR_SIZE);

    	//Erase the sector of backup area
        status = ROM_FLEXSPI_NorFlash_Erase(FlexSpiInstance, &norConfig, CLEAR_BASE(BACK_APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE)), FLASH_SECTOR_SIZE);
        if (status != kStatus_Success)
        {
            PRINTF("\r\n Erase backup sector failure!\r\n");
            return FALSE;
        }

        //Write the data from original APP area into the backup APP area
        for(j = 0; j < FLASH_SECTOR_SIZE/FLASH_PAGE_SIZE; j++)
        {
        	FeedWatchDog();
        	//only one page of 256 bytes of data can be written each time, one sector requires 16 writes
        	status = ROM_FLEXSPI_NorFlash_ProgramPage(FlexSpiInstance, &norConfig, CLEAR_BASE(BACK_APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE) + (j * FLASH_PAGE_SIZE)),
        			(const uint32_t *)&s_buffer[j * FLASH_PAGE_SIZE]);
        	if (status != kStatus_Success)
        	{
        		PRINTF("\r\n Page program failure!\r\n");
        		return FALSE;
        	}
        }

        //Refresh D-cacheֵ
        DCACHE_InvalidateByRange(BACK_APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
        //Read the written data
		memcpy(s_buffer_rbc, (void *)(BACK_APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE)), FLASH_SECTOR_SIZE);
        if (memcmp(s_buffer_rbc, s_buffer, FLASH_SECTOR_SIZE) != 0)
        {
            PRINTF("\r\n Program data -  read out data value incorrect!\r\n ");
            return FALSE;
        }
	}

#if FAST_FLASH_OPERATE
	for(i = AppSectorNum; i < APP_TOTAL_SIZE/FLASH_SECTOR_SIZE; i++)
	{
		FeedWatchDog();
		status = ROM_FLEXSPI_NorFlash_Erase(FlexSpiInstance, &norConfig, CLEAR_BASE(BACK_APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE)), FLASH_SECTOR_SIZE);
		if (status != kStatus_Success)
		{
			PRINTF("\r\n Erase sector failure!\r\n");
			return FALSE;
		}
	}
#endif

	return TRUE;
}

BOOL TransferNewApp2RunningArea(void)
{
    status_t status;
    uint32_t i        = 0U;
    uint32_t j        = 0U;
    uint32_t AppSize = 0U;
    uint32_t AppSectorNum = APP_TOTAL_SIZE/FLASH_SECTOR_SIZE;

#if FAST_FLASH_OPERATE
    DCACHE_InvalidateByRange(NEW_APP_INTEG_FLASH_ADDR_START, FLASH_SECTOR_SIZE);
    memcpy(s_buffer, (void *)(NEW_APP_INTEG_FLASH_ADDR_START), APP_INTEG_SIZE+4);
    AppSize = (s_buffer[APP_INTEG_SIZE]<<24) | (s_buffer[APP_INTEG_SIZE+1]<<16) | (s_buffer[APP_INTEG_SIZE+2]<<8) | (s_buffer[APP_INTEG_SIZE+3]);
    if(AppSize == 0
    || AppSize > APP_TOTAL_SIZE)
    {
    	AppSize = APP_TOTAL_SIZE;
    }

    AppSectorNum = AppSize/FLASH_SECTOR_SIZE+1;
    AppSectorNum = (AppSectorNum < (APP_TOTAL_SIZE/FLASH_SECTOR_SIZE)) ? (AppSectorNum) : (APP_TOTAL_SIZE/FLASH_SECTOR_SIZE);
#endif
	for(i = 0; i < AppSectorNum; i++)
	{
		FeedWatchDog();
		//Refresh the corresponding data in the D-cache before reading, otherwise may read the values that were previously written in the D-cacheֵ
		DCACHE_InvalidateByRange(NEW_APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
		//Read the data from new APP, reading one sector at a time
		memcpy(s_buffer, (void *)(NEW_APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE)), FLASH_SECTOR_SIZE);

		//Erase the data of the original APP sector for the new APP
		status = ROM_FLEXSPI_NorFlash_Erase(FlexSpiInstance, &norConfig, CLEAR_BASE(APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE)), FLASH_SECTOR_SIZE);
		if (status != kStatus_Success)
		{
			PRINTF("\r\n Erase sector failure!\r\n");
			return FALSE;
		}

		//Write the data from new APP area into the running APP area
		for(j = 0; j < FLASH_SECTOR_SIZE/FLASH_PAGE_SIZE; j++)
		{
			FeedWatchDog();
			if((APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE) + (j * FLASH_PAGE_SIZE)) == APP_INTEG_FG_FLASH_ADDR_START)
			{
				memcpy(u8AppIntegBuf, &s_buffer[j * FLASH_PAGE_SIZE], FLASH_PAGE_SIZE);
				continue;
			}
			//only one page of 256 bytes of data can be written each time, one sector requires 16 writes
			status = ROM_FLEXSPI_NorFlash_ProgramPage(FlexSpiInstance, &norConfig, CLEAR_BASE(APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE) + (j * FLASH_PAGE_SIZE)),
					(const uint32_t *)&s_buffer[j * FLASH_PAGE_SIZE]);
			if (status != kStatus_Success)
			{
				PRINTF("\r\n Page program failure!\r\n");
				return FALSE;
			}
		}

		//Refresh D-cacheֵ
		DCACHE_InvalidateByRange(APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
		//Read the written data
		memcpy(s_buffer_rbc, (void *)(APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE)), FLASH_SECTOR_SIZE);
		if(i == 0)
		{
			memcpy(&s_buffer_rbc[APP_INTEG_FG_OFFSET], u8AppIntegBuf, FLASH_PAGE_SIZE);
		}
		if (memcmp(s_buffer_rbc, s_buffer, FLASH_SECTOR_SIZE) != 0)
		{
			PRINTF("\r\n Program data - read out data value incorrect!\r\n ");
			return FALSE;
		}
	}

#if FAST_FLASH_OPERATE
	for(i = AppSectorNum; i < APP_TOTAL_SIZE/FLASH_SECTOR_SIZE; i++)
	{
		FeedWatchDog();
		status = ROM_FLEXSPI_NorFlash_Erase(FlexSpiInstance, &norConfig, CLEAR_BASE(APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE)), FLASH_SECTOR_SIZE);
		if (status != kStatus_Success)
		{
			PRINTF("\r\n Erase sector failure!\r\n");
			return FALSE;
		}
	}
#endif

	return TRUE;
}

BOOL RecoverBackupApp2RunningArea(void)
{
    status_t status;
    uint32_t i        = 0U;
    uint32_t j        = 0U;
    uint32_t AppSize = 0U;
    uint32_t AppSectorNum = APP_TOTAL_SIZE/FLASH_SECTOR_SIZE;

#if FAST_FLASH_OPERATE
    memcpy(s_buffer, (void *)(BACK_APP_INTEG_FLASH_ADDR_START + APP_INTEG_SIZE), 4);
    AppSize = (s_buffer[0]<<24) | (s_buffer[1]<<16) | (s_buffer[2]<<8) | (s_buffer[3]);
    if(AppSize == 0
    || AppSize > APP_TOTAL_SIZE)
    {
    	AppSize = APP_TOTAL_SIZE;
    }

    AppSectorNum = AppSize/FLASH_SECTOR_SIZE+1;
    AppSectorNum = (AppSectorNum < (APP_TOTAL_SIZE/FLASH_SECTOR_SIZE)) ? (AppSectorNum) : (APP_TOTAL_SIZE/FLASH_SECTOR_SIZE);
#endif
    for(i = 0; i < AppSectorNum; i++)
	{
    	FeedWatchDog();
		DCACHE_InvalidateByRange(BACK_APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
		memcpy(s_buffer, (void *)(BACK_APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE)), FLASH_SECTOR_SIZE);

		//Erase the sector of running area
		status = ROM_FLEXSPI_NorFlash_Erase(FlexSpiInstance, &norConfig, CLEAR_BASE(APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE)), FLASH_SECTOR_SIZE);
		if (status != kStatus_Success)
		{
			PRINTF("\r\n Erase sector failure!\r\n");
			return FALSE;
		}

		for(j = 0; j < FLASH_SECTOR_SIZE/FLASH_PAGE_SIZE; j++)
		{
			FeedWatchDog();
			status = ROM_FLEXSPI_NorFlash_ProgramPage(FlexSpiInstance, &norConfig, CLEAR_BASE(APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE) + (j * FLASH_PAGE_SIZE)),
					(const uint32_t *)&s_buffer[j * FLASH_PAGE_SIZE]);
			if (status != kStatus_Success)
			{
				PRINTF("\r\n Page program failure!\r\n");
				return FALSE;
			}
		}

		DCACHE_InvalidateByRange(APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
		memcpy(s_buffer_rbc, (void *)(APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE)), FLASH_SECTOR_SIZE);
		if (memcmp(s_buffer_rbc, s_buffer, FLASH_SECTOR_SIZE) == 0)
		{
			PRINTF("\r\n Recover backup App to running area - read out data value incorrect!\r\n ");
			return FALSE;
		}
	}

#if FAST_FLASH_OPERATE
	for(i = AppSectorNum; i < APP_TOTAL_SIZE/FLASH_SECTOR_SIZE; i++)
	{
		FeedWatchDog();
		status = ROM_FLEXSPI_NorFlash_Erase(FlexSpiInstance, &norConfig, CLEAR_BASE(APP_FLASH_ADDR_START + (i * FLASH_SECTOR_SIZE)), FLASH_SECTOR_SIZE);
		if (status != kStatus_Success)
		{
			PRINTF("\r\n Erase sector failure!\r\n");
			return FALSE;
		}
	}
#endif

	return TRUE;
}

BOOL flexspi_flash_buffer_write_a_page(uint8_t *FlashWrBuf, uint32_t FlashBeginAddr)
{
	status_t status;

	status = ROM_FLEXSPI_NorFlash_ProgramPage(FlexSpiInstance, &norConfig, CLEAR_BASE(FlashBeginAddr), (const uint32_t *)FlashWrBuf);

	if(status != kStatus_Success)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL WriteAppInterFg2Flash(void)
{
	return flexspi_flash_buffer_write_a_page(u8AppIntegBuf, APP_INTEG_FG_FLASH_ADDR_START);
}

void EarseNewAppInteg(void)
{
	/* erase the new app INTEG */
	ROM_FLEXSPI_NorFlash_Erase(FlexSpiInstance, &norConfig, CLEAR_BASE(NEW_APP_INTEG_FLASH_ADDR_START), FLASH_SECTOR_SIZE);
}

BOOL User_NorFlashInit(void)
{
	status_t status;
	uint32_t vendorID = 0U;

    /* Clean up FLEXSPI NOR flash driver Structure */
    memset(&norConfig, 0U, sizeof(flexspi_nor_config_t));

    /* Setup FLEXSPI NOR Configuration Block */
    FLEXSPI_NorFlash_GetConfig(&norConfig);

    /* Initializes the FLEXSPI module for the other FLEXSPI APIs */
    status = ROM_FLEXSPI_NorFlash_Init(FlexSpiInstance, &norConfig);
    if (status != kStatus_Success)
    {
        PRINTF("\r\n ROM_FLEXSPI_NorFlash_Init failed !\r\n");
        return FALSE;
    }

    /* Perform software reset after initializing flexspi module */
    ROM_FLEXSPI_NorFlash_ClearCache(FlexSpiInstance);

    /* Probe device presence by verifying Manufacturer ID */
    status = FLEXSPI_NorFlash_GetVendorID(FlexSpiInstance, &vendorID);
    if (status != kStatus_Success)
    {
        PRINTF("\r\n FLEXSPI_NorFlash_GetVendorID failed!\r\n");
        return FALSE;
    }

    return TRUE;
}

int main(void)
{
    BOARD_ConfigMPU();
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
#if !(defined(XIP_EXTERNAL_FLASH) && (XIP_EXTERNAL_FLASH == 1))
    BOARD_SetupFlexSpiClock();
#endif

//    Jump_To_App();

    FeedWatchDog();

    if(FALSE == User_NorFlashInit())
    {
    	error_trap();
    }

    if(TRUE == CheckAppIntegFg(NEW_APP_INTEG_FLASH_ADDR_START))
    {
    	if(TRUE == BackupRunningAppArea())
    	{
    		if(TRUE == TransferNewApp2RunningArea())
    		{
    			WriteAppInterFg2Flash();
    			EarseNewAppInteg();
    		}
    		else
    		{
    			RecoverBackupApp2RunningArea();
    		}
    	}
    }
    else
    {
    	if(FALSE == CheckAppIntegFg(APP_INTEG_FG_FLASH_ADDR_START))
    	{
    		if(TRUE == CheckAppIntegFg(BACK_APP_INTEG_FLASH_ADDR_START))
			{
    			RecoverBackupApp2RunningArea();
			}
    		else
			{
    			PRINTF("\r\n App is damaged and can not find integed backup app!\r\n");
				error_trap();
			}
    	}
    }

    if(TRUE == CheckAppIntegFg(APP_INTEG_FG_FLASH_ADDR_START))
    {
    	Jump_To_App();
    }
    else
    {
    	app_finalize();
    }

    return 0;
}
