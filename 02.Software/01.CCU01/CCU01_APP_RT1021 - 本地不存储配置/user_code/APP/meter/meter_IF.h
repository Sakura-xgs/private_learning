/*
 * meter_IF.h
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */

#ifndef APP_METER_METER_IF_H_
#define APP_METER_METER_IF_H_

#include "PublicDefine.h"
#include "uart_comm.h"

#pragma pack(1)

#define DH_METER_MODULE_FLAG    	1
#define AKR_METER_MODULE_FLAG  	 	2

typedef struct
{
	U16 unMeter_version;
	U32 uiPos_act_dn;   //0.0001kw.h
	U32 uiRev_act_dn;   //0.0001kw.h
	U32 uiDc_pwr;    	//0.0001
	U32 uiDc_cur;    	//0.01
	U32 uiDc_vol;    	//0.01
}Meter_Public_Data_t;

typedef struct
{
	U8 ucDevice_switch_flag;
	Meter_Public_Data_t sPublic_data[2];
	void (*MeterDataRequest)(U8 ucAddr);
	void (*MeterDataParse)(const U8 *buf, U16 buf_len);
}Meter_data_t;

#pragma pack()

extern Meter_data_t *g_psMeter_data;

void Meter_Init_Task(void * pvParameters);
void MeterUartSend(const UART_LIST uart, const U32 byte_num, U8 *buff);

#endif /* APP_METER_METER_IF_H_ */
