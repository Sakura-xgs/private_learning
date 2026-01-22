/*
 * ex_eeprom.c
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
#include "hal_eeprom.h"
#include "hal_eeprom_IF.h"
#include "string.h"
#include "Signal.h"
#include "SignalManage.h"
#include "peripherals.h"
#include "calculate_crc.h"

static U8 u8EEHQBackupIndex[CCU_SET_SIG_ID_HIGH_FREQ_END_FLAG - CCU_SET_SIG_ID_BEGIN_FLAG] = {0};  //从0开始
static U8 u8EENLBackupIndex[CCU_SET_SIG_ID_END_FLAG - CCU_SET_SIG_ID_HIGH_FREQ_END_FLAG] = {0};
static U32 g_u32EepCommErrCnt = 0;
static U8 u8CommFailCnt = 0;

static BOOL eep_compare(const uint8_t* src, const uint8_t* dst, uint16_t length)
{
    while (0U != (length --)){
        if(*src++ != *dst++)
        {
            return FALSE;
        }
    }
    return TRUE;
}

static status_t FM24C64_IIC_READ_ARRAY(U8* read_Buffer, U16 start_Addr, U16 number_Bytes)
{
	status_t blRet = kStatus_Fail;

	assert(number_Bytes <= EXT_EEPROM_PAR_DATA_LEN);

	EEPROM_I2C_masterTransfer.flags          = kLPI2C_TransferDefaultFlag;
	EEPROM_I2C_masterTransfer.slaveAddress   = (EXT_EEPROM_ADDR >> 1);
	EEPROM_I2C_masterTransfer.direction      = kLPI2C_Read;
	EEPROM_I2C_masterTransfer.subaddress     = start_Addr;
	EEPROM_I2C_masterTransfer.subaddressSize = 2;
	EEPROM_I2C_masterTransfer.data           = read_Buffer;
	EEPROM_I2C_masterTransfer.dataSize       = number_Bytes;

  	blRet = LPI2C_RTOS_Transfer(&EEPROM_I2C_masterHandle, &EEPROM_I2C_masterTransfer);

	if(kStatus_Success != blRet)
	{
		u8CommFailCnt++;
	}

	return blRet;
}

static status_t FM24C64_IIC_WRITE_ARRAY(U8* write_Buffer, U16 start_Addr, U16 number_Bytes)
{
	status_t blRet = kStatus_Fail;

	assert(number_Bytes <= EXT_EEPROM_PAR_DATA_LEN);

	EEPROM_I2C_masterTransfer.flags          = kLPI2C_TransferDefaultFlag;
	EEPROM_I2C_masterTransfer.slaveAddress   = (EXT_EEPROM_ADDR >> 1);
	EEPROM_I2C_masterTransfer.direction      = kLPI2C_Write;
	EEPROM_I2C_masterTransfer.subaddress     = start_Addr;
	EEPROM_I2C_masterTransfer.subaddressSize = 2;
	EEPROM_I2C_masterTransfer.data           = write_Buffer;
	EEPROM_I2C_masterTransfer.dataSize       = number_Bytes;

	blRet = LPI2C_RTOS_Transfer(&EEPROM_I2C_masterHandle, &EEPROM_I2C_masterTransfer);

	if(kStatus_Success != blRet)
	{
		u8CommFailCnt++;
	}

	return blRet;
}

BOOL ExtEepromSaveParameter(U32 u32SigId, S32 s32SigVal)
{
	U8 u8SaveBuf[EXT_EEPROM_PAR_DATA_LEN] = {0xA5,
											0, 0, 0, 0,
											0, 0,
											NEWEST};
	U8 u8ReadBuf[EXT_EEPROM_PAR_DATA_LEN] = {0};
	U8 u8ReadByte = 0;
	U8 u8ParaBackupNum = EXT_EEPROM_HQ_PAR_BACKUP_NUM;
	U32 u32SigIdNo = 0;
	U32 u32StartAddr = 0;
	U32 u32CtlAddr = 0;
	U16 u16CalCrc = 0;
	P_U8 pu8Buf;
	U8 u8BackupLoc = 0;
	U8 u8ObandLoc = 0;

	assert(NULL != g_CtlExtEeprom_MutexSemaphore);
	(void)xSemaphoreTake(g_CtlExtEeprom_MutexSemaphore, portMAX_DELAY);

	if(u32SigId < CCU_SET_SIG_ID_HIGH_FREQ_END_FLAG)
	{
		u32SigIdNo = u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG;

		u32StartAddr = EXT_EEPROM_HQ_PAR_SAVE_BEGIN_ADDR + ((u32SigIdNo-1U)*EXT_EEPROM_HQ_PAR_BACKUP_NUM*EXT_EEPROM_PAR_DATA_LEN);

		u8ParaBackupNum = EXT_EEPROM_HQ_PAR_BACKUP_NUM;

		pu8Buf = u8EEHQBackupIndex;
	}
	else
	{
		u32SigIdNo = u32SigId - CCU_SET_SIG_ID_HIGH_FREQ_END_FLAG;

		u32StartAddr = EXT_EEPROM_NL_PAR_SAVE_BEGIN_ADDR + ((u32SigIdNo-1U)*EXT_EEPROM_NL_PAR_BACKUP_NUM*EXT_EEPROM_PAR_DATA_LEN);

		u8ParaBackupNum = EXT_EEPROM_NL_PAR_BACKUP_NUM;

		pu8Buf = u8EENLBackupIndex;
	}

	u8BackupLoc = pu8Buf[u32SigIdNo-1U];

	if(0U == u8BackupLoc)
	{
		u8ObandLoc = (u8ParaBackupNum-1U);
	}
	else
	{
		u8ObandLoc = u8BackupLoc-1U;
	}

	//废弃原本的备份参数
	if(u8ParaBackupNum >= 3U)
	{
		EepromDelay();

		//废弃原本的备份
		u8ReadByte = (U8)ABANDONED;
		u32CtlAddr = u32StartAddr + ((u8ObandLoc*EXT_EEPROM_PAR_DATA_LEN)+7U);
		if(kStatus_Success != FM24C64_IIC_WRITE_ARRAY(&u8ReadByte, u32CtlAddr, 1))
		{
			g_u32EepCommErrCnt++;
			(void)xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
			return FALSE;
		}
	}

	//将原本的参数改为备份参数
	if(u8ParaBackupNum >= 2U)
	{
		EepromDelay();

		//将原本的参数改为备份参数
		u8ReadByte = (U8)ORIGINAL;
		u32CtlAddr = u32StartAddr + ((u8BackupLoc*EXT_EEPROM_PAR_DATA_LEN)+7U);
		if(kStatus_Success != FM24C64_IIC_WRITE_ARRAY(&u8ReadByte, u32CtlAddr, 1))
		{
			g_u32EepCommErrCnt++;
			(void)xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
			return FALSE;
		}
	}

	u8SaveBuf[1] = (U8)(s32SigVal>>24U);
	u8SaveBuf[2] = (U8)(s32SigVal>>16U);
	u8SaveBuf[3] = (U8)(s32SigVal>>8U);
	u8SaveBuf[4] = (U8)s32SigVal;

	u16CalCrc = CalCrc16(u8SaveBuf, 5);

	u8SaveBuf[5] = (U8)((u16CalCrc & 0xFF00U) >> 8);
	u8SaveBuf[6] = (U8)(u16CalCrc & 0x00FFU);

	/*将数据写入FLASH*/
	if((u8ParaBackupNum-1U) == pu8Buf[u32SigIdNo-1U])
	{
		pu8Buf[u32SigIdNo-1U] = 0;
	}
	else
	{
		pu8Buf[u32SigIdNo-1U]++;
	}

	EepromDelay();

	u32CtlAddr = u32StartAddr + (pu8Buf[u32SigIdNo-1U]*EXT_EEPROM_PAR_DATA_LEN);
	if(kStatus_Success != FM24C64_IIC_WRITE_ARRAY(u8SaveBuf, u32CtlAddr, EXT_EEPROM_PAR_DATA_LEN))
	{
		g_u32EepCommErrCnt++;
		(void)xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
		return FALSE;
	}

	EepromDelay();

	//校验
	if(kStatus_Success != FM24C64_IIC_READ_ARRAY(u8ReadBuf, u32CtlAddr, EXT_EEPROM_PAR_DATA_LEN))
	{
		g_u32EepCommErrCnt++;
		(void)xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
		return FALSE;
	}

	//if(kStatus_Success != eep_compare(u8SaveBuf, u8ReadBuf, EXT_EEPROM_PAR_DATA_LEN))
	if(TRUE != eep_compare(u8SaveBuf, u8ReadBuf, EXT_EEPROM_PAR_DATA_LEN))
	{
		g_u32EepCommErrCnt++;
		(void)xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
		return FALSE;
	}

	(void)xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
	return TRUE;
}

BOOL ExtEepromReadParameter(U32 u32SigId, P_S32 ps32RetVal)
{
	S32 s32GetSigVal = 0;
	BOOL blRet = FALSE;
	U8 u8ReadBuf[EXT_EEPROM_PAR_DATA_LEN] = {0};
	U32 u32SigIdNo = 0;
	U32 u32StartAddr = 0;
	U32 u32CtlAddr = 0;
	U8 u8ParaBackupNum = EXT_EEPROM_HQ_PAR_BACKUP_NUM;
	U32 u32PrePara = 0;
	BOOL blNewFindFg = FALSE;
	BOOL blPreFindFg = FALSE;
	U8 u8ReadCnt = 0;
	U8 u8PreParaLoc = 0;
	U16 u16GetCrc = 0;
	U16 u16CalCrc = 0;
	P_U8 pu8Buf;

	assert(NULL != g_CtlExtEeprom_MutexSemaphore);
	(void)xSemaphoreTake(g_CtlExtEeprom_MutexSemaphore, portMAX_DELAY);

	if((CCU_SET_SIG_ID_BEGIN_FLAG == u32SigId)
	|| (CCU_SET_SIG_ID_HIGH_FREQ_END_FLAG == u32SigId)
	|| (CCU_SET_SIG_ID_END_FLAG == u32SigId))
	{
		(void)xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
		return TRUE;
	}

	if(u32SigId < CCU_SET_SIG_ID_HIGH_FREQ_END_FLAG)
	{
		u32SigIdNo = u32SigId - CCU_SET_SIG_ID_BEGIN_FLAG;

		u32StartAddr = EXT_EEPROM_HQ_PAR_SAVE_BEGIN_ADDR + ((u32SigIdNo-1U)*EXT_EEPROM_HQ_PAR_BACKUP_NUM*EXT_EEPROM_PAR_DATA_LEN);

		u8ParaBackupNum = EXT_EEPROM_HQ_PAR_BACKUP_NUM;

		pu8Buf = u8EEHQBackupIndex;
	}
	else
	{
		u32SigIdNo = u32SigId - CCU_SET_SIG_ID_HIGH_FREQ_END_FLAG;

		u32StartAddr = EXT_EEPROM_NL_PAR_SAVE_BEGIN_ADDR + ((u32SigIdNo-1U)*EXT_EEPROM_NL_PAR_BACKUP_NUM*EXT_EEPROM_PAR_DATA_LEN);

		u8ParaBackupNum = EXT_EEPROM_NL_PAR_BACKUP_NUM;

		pu8Buf = u8EENLBackupIndex;
	}

	//读取该参数的全部存储值
	for(u8ReadCnt = 0; u8ReadCnt < u8ParaBackupNum; u8ReadCnt++)
	{
		EepromDelay();

		u32CtlAddr = u32StartAddr + (u8ReadCnt*EXT_EEPROM_PAR_DATA_LEN);

		if(kStatus_Success == FM24C64_IIC_READ_ARRAY(u8ReadBuf, u32CtlAddr, EXT_EEPROM_PAR_DATA_LEN))
		{
			u16GetCrc = (((U16)(u8ReadBuf[5]))<<8) | (((U16)(u8ReadBuf[6]))<<0);
			u16CalCrc = CalCrc16(u8ReadBuf, 5);

			if((u16GetCrc == u16CalCrc) && (0xA5U == u8ReadBuf[0]))
			{
				s32GetSigVal = (((U32)(u8ReadBuf[1]))<<24U)
								| (((U32)(u8ReadBuf[2]))<<16U)
								| (((U32)(u8ReadBuf[3]))<<8U)
								| (((U32)(u8ReadBuf[4]))<<0U);

				if((U8)NEWEST == u8ReadBuf[7])
				{
					blNewFindFg = TRUE;

					pu8Buf[u32SigIdNo-1U] = u8ReadCnt;

					*ps32RetVal = s32GetSigVal;

					blRet = TRUE;

					break;
				}
				else if((U8)ORIGINAL == u8ReadBuf[7])
				{
					blPreFindFg = TRUE;

					u8PreParaLoc = u8ReadCnt;

					u32PrePara = (U32)s32GetSigVal;
				}
				else
				{
					//
				}
			}
		}
		else
		{
			g_u32EepCommErrCnt++;
		}
	}

	//最新的没读到，但是读到之前的
	if((FALSE == blNewFindFg)
		&& (TRUE == blPreFindFg))
	{
		*ps32RetVal = u32PrePara;

		pu8Buf[u32SigIdNo-1U] = u8PreParaLoc;

		blRet = TRUE;
	}

	(void)xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
	return blRet;
}

/**
  * @brief 向EEPROM写入不限量的数据
  * @param pBuffer:缓冲区指针
  * @param WriteAddr:写入的存储单元首地址
  * @param NumByteToWrite:要写的字节数
  */
status_t EepromBufferWrite(uint8_t* pBuffer,
						   uint16_t WriteAddr,
						   uint16_t NumByteToWrite)
{
	if(NumByteToWrite > 0U)
	{
		uint8_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0;
		uint8_t NumByteToWriteRest = NumByteToWrite;

		assert(NULL != g_CtlExtEeprom_MutexSemaphore);
		(void)xSemaphoreTake(g_CtlExtEeprom_MutexSemaphore, portMAX_DELAY);

		Addr = WriteAddr % EXT_EEPROM_PAR_DATA_LEN;
		count = EXT_EEPROM_PAR_DATA_LEN - Addr;

		NumByteToWriteRest = (NumByteToWrite > count) ? (NumByteToWrite - count) : NumByteToWrite;
		NumOfPage =  NumByteToWriteRest / EXT_EEPROM_PAR_DATA_LEN;
		NumOfSingle = NumByteToWriteRest % EXT_EEPROM_PAR_DATA_LEN;

		if ((count != 0U) && (NumByteToWrite > count))
		{
			if(kStatus_Success == FM24C64_IIC_WRITE_ARRAY(pBuffer, WriteAddr, count))
			{
				EepromDelay();
				WriteAddr += count;
				pBuffer += count;
			}
			else
			{
				(void)xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
				return kStatus_Fail;
			}
		}
		if(NumOfPage == 0U)
		{
			if(kStatus_Success == FM24C64_IIC_WRITE_ARRAY(pBuffer, WriteAddr, NumOfSingle))
			{
				EepromDelay();
			}
			else
			{
				(void)xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
				return kStatus_Fail;
			}
		}
		else
		{
			while(NumOfPage--)
			{
				if(kStatus_Success == FM24C64_IIC_WRITE_ARRAY(pBuffer, WriteAddr, EXT_EEPROM_PAR_DATA_LEN))
				{
					EepromDelay();
					WriteAddr +=  EXT_EEPROM_PAR_DATA_LEN;
					pBuffer += EXT_EEPROM_PAR_DATA_LEN;
				}
				else
				{
					(void)xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
					return kStatus_Fail;
				}
			}
			if(NumOfSingle != 0U)
			{
				if(kStatus_Success == FM24C64_IIC_WRITE_ARRAY(pBuffer, WriteAddr, NumOfSingle))
				{
					EepromDelay();
				}
				else
				{
					(void)xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
					return kStatus_Fail;
				}
			}
		}
		(void)xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
	}
	return kStatus_Success;
}

status_t EepromBufferRead(uint8_t* pBuffer,
						   uint16_t ReadAddr,
						   uint16_t NumByteToRead)
{
	if(NumByteToRead > 0U)
	{
		uint8_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0;
		uint8_t NumByteToReadRest = NumByteToRead;

		assert(g_CtlExtEeprom_MutexSemaphore);
		(void)xSemaphoreTake(g_CtlExtEeprom_MutexSemaphore, portMAX_DELAY);

		Addr = ReadAddr % EXT_EEPROM_PAR_DATA_LEN;
		count = EXT_EEPROM_PAR_DATA_LEN - Addr;

		NumByteToReadRest = (NumByteToRead > count) ? (NumByteToRead - count) : NumByteToRead;
		NumOfPage =  NumByteToReadRest / EXT_EEPROM_PAR_DATA_LEN;
		NumOfSingle = NumByteToReadRest % EXT_EEPROM_PAR_DATA_LEN;

		if ((count != 0U) && (NumByteToRead > count))
		{
			if(kStatus_Success == FM24C64_IIC_READ_ARRAY(pBuffer, ReadAddr, count))
			{
				EepromDelay();
				ReadAddr += count;
				pBuffer += count;
			}
			else
			{
				(void)xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
				return kStatus_Fail;
			}
		}
		if(NumOfPage == 0U)
		{
			if(kStatus_Success == FM24C64_IIC_READ_ARRAY(pBuffer, ReadAddr, NumOfSingle))
			{
				EepromDelay();
			}
			else
			{
				(void)xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
				return kStatus_Fail;
			}
		}
		else
		{
			while(NumOfPage--)
			{
				if(kStatus_Success == FM24C64_IIC_READ_ARRAY(pBuffer, ReadAddr, EXT_EEPROM_PAR_DATA_LEN))
				{
					EepromDelay();
					ReadAddr +=  EXT_EEPROM_PAR_DATA_LEN;
					pBuffer += EXT_EEPROM_PAR_DATA_LEN;
				}
				else
				{
					(void)xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
					return kStatus_Fail;
				}
			}
			if(NumOfSingle != 0U)
			{
				if(kStatus_Success == FM24C64_IIC_READ_ARRAY(pBuffer, ReadAddr, NumOfSingle))
				{
					EepromDelay();
				}
				else
				{
					(void)xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
					return kStatus_Fail;
				}
			}
		}
		(void)xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
	}
	return kStatus_Success;
}
