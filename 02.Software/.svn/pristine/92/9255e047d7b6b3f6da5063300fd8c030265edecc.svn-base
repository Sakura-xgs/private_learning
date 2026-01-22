#include "hal_eeprom.h"
#include "gd32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hard_i2c.h"

/***********************************变量定义*******************************/
U8 commCnt = 0;
/************************************函数定义********************************/

/******************************************************************************
* 名  	称:  FM24C64_IIC_READ_ARRAY
* 功  	能:  读取多个字节
              返回：
                  读取正确：  1
                  读取错误：  0
* 入口参数:  无
* 出口参数:  无
*******************************************************************************/
I2C_Status FM24C64_IIC_READ_ARRAY(U8* rd_data_buf, U16 addr, U16 bwn)
{
	I2C_Status blRet = I2C_FAIL;
	
	vTaskDelay(2);

	xSemaphoreTake(g_HARD_I2C1_MutexSemaphore, portMAX_DELAY);

  	blRet = I2Cx_Read_NBytes(I2C1, EXT_EEPROM_ADDR, addr, bwn, rd_data_buf, 2);

	xSemaphoreGive(g_HARD_I2C1_MutexSemaphore);
	
	if(I2C_FAIL == blRet)
	{
		commCnt++;
	}
		
	vTaskDelay(2);
	
	return blRet;
}

/******************************************************************************
* 名  	称:  FM24C64_IIC_WRITE_ARRAY
* 功  	能:  写入多个字节
             返回值：
                    正确写入：1
                    错误写入：0
* 入口参数:  无
* 出口参数:  无
*******************************************************************************/
I2C_Status FM24C64_IIC_WRITE_ARRAY(U8* wr_data_buf, U16 addr, U16 bwn)
{
	I2C_Status blRet = I2C_FAIL;
	
	vTaskDelay(2);

	xSemaphoreTake(g_HARD_I2C1_MutexSemaphore, portMAX_DELAY);

	blRet = I2Cx_Write_NBytes(I2C1, EXT_EEPROM_ADDR, addr, bwn, wr_data_buf, 2);

	if(I2C_FAIL == blRet)
	{
		commCnt++;
	}

	xSemaphoreGive(g_HARD_I2C1_MutexSemaphore);
	
	vTaskDelay(2);
	
	return blRet;
}


