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



U8 u8EEHQBackupIndex[PDU_SET_SIG_ID_HIGH_FREQ_END_FLAG - PDU_SET_SIG_ID_BEGIN_FLAG] = {0};  //从0开始
U8 u8EENLBackupIndex[PDU_SET_SIG_ID_END_FLAG - PDU_SET_SIG_ID_HIGH_FREQ_END_FLAG] = {0};
U32 g_u32EepCommErrCnt = 0;
U8 u8CommFailCnt = 0;


BOOL eep_compare(uint8_t* src, uint8_t* dst, uint16_t length)
{
    while(length --){
        if(*src++ != *dst++)
            return FALSE;
    }
    return TRUE;
}

status_t FM24C64_IIC_READ_ARRAY(U8* read_Buffer, U16 start_Addr, U16 number_Bytes)
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

status_t FM24C64_IIC_WRITE_ARRAY(U8* write_Buffer, U16 start_Addr, U16 number_Bytes)
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

	assert(g_CtlExtEeprom_MutexSemaphore);
	xSemaphoreTake(g_CtlExtEeprom_MutexSemaphore, portMAX_DELAY);

	if(u32SigId < PDU_SET_SIG_ID_HIGH_FREQ_END_FLAG)
	{
		u32SigIdNo = u32SigId - PDU_SET_SIG_ID_BEGIN_FLAG;

		u32StartAddr = EXT_EEPROM_HQ_PAR_SAVE_BEGIN_ADDR+(u32SigIdNo-1)*EXT_EEPROM_HQ_PAR_BACKUP_NUM*EXT_EEPROM_PAR_DATA_LEN;

		u8ParaBackupNum = EXT_EEPROM_HQ_PAR_BACKUP_NUM;

		pu8Buf = u8EEHQBackupIndex;
	}
	else
	{
		u32SigIdNo = u32SigId - PDU_SET_SIG_ID_HIGH_FREQ_END_FLAG;

		u32StartAddr = EXT_EEPROM_NL_PAR_SAVE_BEGIN_ADDR+(u32SigIdNo-1)*EXT_EEPROM_NL_PAR_BACKUP_NUM*EXT_EEPROM_PAR_DATA_LEN;

		u8ParaBackupNum = EXT_EEPROM_NL_PAR_BACKUP_NUM;

		pu8Buf = u8EENLBackupIndex;
	}

	u8BackupLoc = pu8Buf[u32SigIdNo-1];

	if(0 == u8BackupLoc)
	{
		u8ObandLoc = (u8ParaBackupNum-1);
	}
	else
	{
		u8ObandLoc = u8BackupLoc-1;
	}

	//废弃原本的备份参数
	if(u8ParaBackupNum >= 3)
	{
		vTaskDelay(4);

		//废弃原本的备份
		u8ReadByte = ABANDONED;
		u32CtlAddr = u32StartAddr+u8ObandLoc*EXT_EEPROM_PAR_DATA_LEN+7;
		if(kStatus_Success != FM24C64_IIC_WRITE_ARRAY(&u8ReadByte, u32CtlAddr, 1))
		{
			g_u32EepCommErrCnt++;
			xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
			return FALSE;
		}
	}

	//将原本的参数改为备份参数
	if(u8ParaBackupNum >= 2)
	{
		vTaskDelay(4);

		//将原本的参数改为备份参数
		u8ReadByte = ORIGINAL;
		u32CtlAddr = u32StartAddr+u8BackupLoc*EXT_EEPROM_PAR_DATA_LEN+7;
		if(kStatus_Success != FM24C64_IIC_WRITE_ARRAY(&u8ReadByte, u32CtlAddr, 1))
		{
			g_u32EepCommErrCnt++;
			xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
			return FALSE;
		}
	}

	u8SaveBuf[1] = s32SigVal>>24;
	u8SaveBuf[2] = s32SigVal>>16;
	u8SaveBuf[3] = s32SigVal>>8;
	u8SaveBuf[4] = s32SigVal;

	u16CalCrc = CalCrc16(u8SaveBuf, 5);

	u8SaveBuf[5] = ((U8)((u16CalCrc & 0xFF00) >> 8));
	u8SaveBuf[6] = ((U8)((u16CalCrc & 0x00FF)));

	/*将数据写入FLASH*/
	if((u8ParaBackupNum-1) == pu8Buf[u32SigIdNo-1])
	{
		pu8Buf[u32SigIdNo-1] = 0;
	}
	else
	{
		pu8Buf[u32SigIdNo-1]++;
	}

	vTaskDelay(3);

	u32CtlAddr = u32StartAddr+pu8Buf[u32SigIdNo-1]*EXT_EEPROM_PAR_DATA_LEN;
	if(kStatus_Success != FM24C64_IIC_WRITE_ARRAY(u8SaveBuf, u32CtlAddr, EXT_EEPROM_PAR_DATA_LEN))
	{
		g_u32EepCommErrCnt++;
		xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
		return FALSE;
	}

	vTaskDelay(3);

	//校验
	if(kStatus_Success != FM24C64_IIC_READ_ARRAY(u8ReadBuf, u32CtlAddr, EXT_EEPROM_PAR_DATA_LEN))
	{
		g_u32EepCommErrCnt++;
		xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
		return FALSE;
	}

	if(TRUE != eep_compare(u8SaveBuf, u8ReadBuf, EXT_EEPROM_PAR_DATA_LEN))
	{
		g_u32EepCommErrCnt++;
		xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
		return FALSE;
	}

	xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
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

	assert(g_CtlExtEeprom_MutexSemaphore);
	xSemaphoreTake(g_CtlExtEeprom_MutexSemaphore, portMAX_DELAY);

	if((PDU_SET_SIG_ID_BEGIN_FLAG == u32SigId)
	|| (PDU_SET_SIG_ID_HIGH_FREQ_END_FLAG == u32SigId)
	|| (PDU_SET_SIG_ID_END_FLAG == u32SigId))
	{
		xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
		return TRUE;
	}

	if(u32SigId < PDU_SET_SIG_ID_HIGH_FREQ_END_FLAG)
	{
		u32SigIdNo = u32SigId - PDU_SET_SIG_ID_BEGIN_FLAG;

		u32StartAddr = EXT_EEPROM_HQ_PAR_SAVE_BEGIN_ADDR+(u32SigIdNo-1)*EXT_EEPROM_HQ_PAR_BACKUP_NUM*EXT_EEPROM_PAR_DATA_LEN;

		u8ParaBackupNum = EXT_EEPROM_HQ_PAR_BACKUP_NUM;

		pu8Buf = u8EEHQBackupIndex;
	}
	else
	{
		u32SigIdNo = u32SigId - PDU_SET_SIG_ID_HIGH_FREQ_END_FLAG;

		u32StartAddr = EXT_EEPROM_NL_PAR_SAVE_BEGIN_ADDR+(u32SigIdNo-1)*EXT_EEPROM_NL_PAR_BACKUP_NUM*EXT_EEPROM_PAR_DATA_LEN;

		u8ParaBackupNum = EXT_EEPROM_NL_PAR_BACKUP_NUM;

		pu8Buf = u8EENLBackupIndex;
	}

	//读取该参数的全部存储值
	for(u8ReadCnt = 0; u8ReadCnt < u8ParaBackupNum; u8ReadCnt++)
	{
		vTaskDelay(1);

		u32CtlAddr = u32StartAddr+u8ReadCnt*EXT_EEPROM_PAR_DATA_LEN;

		if(kStatus_Success == FM24C64_IIC_READ_ARRAY(u8ReadBuf, u32CtlAddr, EXT_EEPROM_PAR_DATA_LEN))
		{
			u16GetCrc = ((U16)(u8ReadBuf[5]))<<8 | ((U16)(u8ReadBuf[6]))<<0;
			u16CalCrc = CalCrc16(u8ReadBuf, 5);

			if((u16GetCrc == u16CalCrc) && (0xA5 == u8ReadBuf[0]))
			{
				s32GetSigVal = ((U32)(u8ReadBuf[1]))<<24
								| ((U32)(u8ReadBuf[2]))<<16
								| ((U32)(u8ReadBuf[3]))<<8
								| ((U32)(u8ReadBuf[4]))<<0;

				if(NEWEST == u8ReadBuf[7])
				{
					blNewFindFg = TRUE;

					pu8Buf[u32SigIdNo-1] = u8ReadCnt;

					*ps32RetVal = s32GetSigVal;

					blRet = TRUE;

					break;
				}
				else if(ORIGINAL == u8ReadBuf[7])
				{
					blPreFindFg = TRUE;

					u8PreParaLoc = u8ReadCnt;

					u32PrePara = s32GetSigVal;
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

		pu8Buf[u32SigIdNo-1] = u8PreParaLoc;

		blRet = TRUE;
	}

	xSemaphoreGive(g_CtlExtEeprom_MutexSemaphore);
	return blRet;
}


