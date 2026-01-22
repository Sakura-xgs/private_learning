/*
 * AKR_meter.h
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */

#ifndef APP_METER_AKR_METER_H_
#define APP_METER_AKR_METER_H_

#include <APP/meter/meter_IF.h>

#pragma pack(1)

#define AKR_READ_CMD            		0x03U
#define AKR_WRITE_CMD            		0x06U
//设置电表地址
#define ADDRESS_DATA_LEN			    7
#define AKR_ADDRESS_REG_ADD_L           0x5c
#define AKR_ADDRESS_REG_ADD_H           2
#define AKR_ADDRESS_REG_NUM             1
//读取版本响应数据长度
#define VERSION_DATA_LEN			    7
#define AKR_VERSION_REG_ADD_L           35
#define AKR_VERSION_REG_ADD_H           0
#define AKR_VERSION_REG_NUM             1
//高速采样频率读取电表数据 高速采集寄存器：电压、电流、功率 76-81
#define REQUEST_DATA_LEN                17
#define FAST_AKR_START_REG_ADD_L        76
#define FAST_AKR_START_REG_ADD_H        0
#define FAST_AKR_READ_REG_NUM           6
//电量
#define REQUEST_ENERGY_LEN              9
#define AKR_ENERGY_REG_ADD_L            12
#define AKR_ENERGY_REG_ADD_H            0
#define AKR_ENERGY_REG_NUM             	2

#define LOW_SPEED_REG_READ_FREQ         (4U)

typedef struct
{
	BOOL bBroken_flag_A;
	BOOL bBroken_flag_B;
}AKR_private_alarm_t;

typedef struct
{
	U8 ucSlave_addr;
	U8 ucCmd;
	U8 ucData_len;
	S8 cVol[4];
	S8 cCur[4];
	S8 cPower[4];
	U16 unCrc;
}AKR_Receive_Data_t;

typedef struct
{
	U8 ucSlave_addr;
	U8 ucCmd;
	U8 ucData_len;
	U16 unVersion;
	U16 unCrc;
}AKR_Receive_Version_t;

typedef struct
{
	U8 ucSlave_addr;
	U8 ucCmd;
	U8 ucData_len;
	U8 cEnergy[4];
	U16 unCrc;
}AKR_Receive_Energy_t;

typedef struct
{
	U8 reserve:8;
}AKR_Fault_t;

#pragma pack()

Meter_data_t* GetAkrMeterModel(void);
void AkrMeterInit(void);

#endif /* APP_METER_AKR_METER_H_ */
