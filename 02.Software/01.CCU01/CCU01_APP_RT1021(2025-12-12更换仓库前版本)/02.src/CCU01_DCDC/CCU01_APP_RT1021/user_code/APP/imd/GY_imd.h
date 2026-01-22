/*
 * gy_iso.h
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */

#ifndef APP_IMD_GY_IMD_H_
#define APP_IMD_GY_IMD_H_

#include <APP/imd/imd_IF.h>

#define GY_READ_VERSION_START_REG   	0x001A
#define GY_READ_VERSION_START_REG_LEN   1
#define GY_READ_DATA_START_REG			0x0010
#define GY_READ_DATA_START_REG_LEN		5
#define GY_CHANGE_MODE_REG				0x0100
#define GY_AUTO_MODE_PARAM				0x0004
#define GY_FIX_MODE_PARAM				0
#define GY_ENABLE_CONTROL_REG			0x0102
#define GY_ENABLE_CONTROL_PARAM			0x0011
#define GY_DISABLE_CONTROL_PARAM		0

#define GY_FIXED_MODE          			0U//固定电阻模式，监测对地电压
#define GY_AUTO_MODE           	 		1U//自动电阻模式，切换内部电阻监测对地电阻

#define GY_READ_CMD            	 		0x03U
#define GY_WRITE_CMD            		0x06U
//无线电阻值
#define GY_INVALID_RESISTANCE_VALUE 	0xFFFFU
//最大电阻值
#define GY_MAX_RESISTANCE_VALUE     	0xEA60U

//读取版本响应数据长度
#define VERSION_DATA_LEN				7
//采样值响应数据长度
#define REQUEST_DATA_LEN            	15
//控制响应
#define IMD_CONTROL_LEN        			8

#pragma pack(1)

typedef struct
{
	U8 ucSlave_addr;	//从机地址
	U8 ucCmd;			//cmd
	U8 ucData_len;		//数据长度
	U16 unBus_vol;		//母线电压
	U16 reserve;		//预留
	U16 unPositive_R;	//正极电阻,千欧
	U16 unNegative_R;	//负极电阻,千欧
	U8 ucStatus[2];		//检测状态
	U16 unCrc;			//crc校验
}GY_Receive_Data_t;

typedef struct
{
	U8 ucSlave_addr;	//从机地址
	U8 ucCmd;			//cmd
	U8 ucData_len;		//数据长度
	U16 unVersion;		//版本号
	U16 unCrc;			//crc校验
}GY_Receive_Version_t;

typedef struct
{
	U8 ucSlave_addr;	//从机地址
	U8 ucCmd;			//cmd
	U8 ucAddr_H;		//寄存器地址高字节
	U8 ucAddr_L;		//寄存器地址低字节
	U8 ucData_H;		//数据高字节
	U8 ucData_L;		//数据低字节
	U16 unCrc;			//crc校验
}GY_Receive_Control_t;

#pragma pack()

IMD_data_t* GetGYImdModel(void);
void GYImdInit(void);

#endif /* APP_IMD_GY_IMD_H_ */
