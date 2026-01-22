#ifndef __HARD_IIC_H
#define __HARD_IIC_H

#include "PublicDefine.h"
#include "FreeRTOS.h"
#include "semphr.h"

#define I2C_SHORT_TIMEOUT 0x0fff//0x2ffff
#define I2C_LONG_TIMEOUT  0x7fff//0x5ffff


extern SemaphoreHandle_t g_HARD_I2C1_MutexSemaphore;


void I2C_DEINIT(void);
void HARD_I2C_INIT(void);

void Resume_IIC(uint32_t Timeout,uint32_t I2Cx );
I2C_Status I2Cx_Read_NBytes(uint32_t I2Cx,uint8_t driver_Addr, uint16_t start_Addr, uint8_t number_Bytes, uint8_t *read_Buffer,uint8_t ADDR_Length);
I2C_Status I2Cx_Write_NBytes(uint32_t I2Cx,uint8_t driver_Addr, uint16_t start_Addr, uint8_t number_Bytes, uint8_t *write_Buffer,uint8_t ADDR_Length);

#endif

