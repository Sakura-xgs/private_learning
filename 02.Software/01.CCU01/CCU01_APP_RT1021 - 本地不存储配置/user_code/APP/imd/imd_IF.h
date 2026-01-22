/*
 * iso_IF.h
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */

#ifndef APP_IMD_IMD_IF_H_
#define APP_IMD_IMD_IF_H_

#include "PublicDefine.h"
#include "uart_comm.h"

#pragma pack(1)

#define GY_IMD_MODULE_FLAG      1

enum
{
	IMD_CHECK_INVALID 	= 0,//无效阻值
	IMD_CHECK_VALID		= 1,//有效阻值
	IMD_CHECK_WARNING	= 2,//告警
	IMD_CHECK_FAULT		= 3,//故障
	IMD_CHECKING		= 4	//检测中
};

typedef enum
{
    AUTO_MODE_CHANGE = 0,//自动模式
	FIX_MODE_CHANGE	 = 1,//固定模式
    IMD_ENABLE		 = 2,//使能imd
    IMD_DISABLE		 = 3,//失能imd
    IMD_CHECK_DATA	 = 4,//imd检测数据
	IMD_VERSION		 = 5//imd版本
}IMD_CONTROL_t;

typedef struct
{
	U8 ucCheck_flag;
	U8 ucImd_version[16];
	U8 ucImd_status;
	U16 unPositive_R;		//千欧
	U16 unNegative_R;		//千欧
	U16 unPositive_Vol;		//0.1V
	U16 unNegative_Vol;		//0.1V
	U16 unBus_vol;			//0.1V
	BOOL bImd_enable_status;//使能状态
	BOOL bImd_data_valid;	//数据有效状态
	BOOL bImd_checkself_status;//自检状态
}Imd_Public_Data_t;

typedef struct
{
	U8 ucDevice_switch_flag;		//只进行接受到对应imd的数据解析
	Imd_Public_Data_t sPublic_data[2];
	U8 ucImd_mode[2];				//模式
	void (*ImdControl)(IMD_CONTROL_t sMode, U8 ucAddr);
	void (*ImdDataParse)(const U8 *buf, U16 unData_len);
}IMD_data_t;

#pragma pack()

extern IMD_data_t *g_psIMD_data;

void Imd_Init_Task(void * pvParameters);
void ImdUartSend(const UART_LIST uart, const U32 byte_num, U8 *buff);

#endif /* APP_IMD_IMD_IF_H_ */
