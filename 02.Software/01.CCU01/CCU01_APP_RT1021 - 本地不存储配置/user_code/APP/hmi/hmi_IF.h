/*
 * hmi_IF.h
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */

#ifndef APP_HMI_HMI_IF_H_
#define APP_HMI_HMI_IF_H_

#include "PublicDefine.h"

#define HMI_COM_TIMEOUT           7000U

#define HEAD_NUM				  0xAAU
#define TAIL_NUM                  0x55U

#define	FAULT_STATUS_RESPONSE_LEN 81U
#define	PILE_STATUS_RESPONSE_LEN  157U

#define XRD_HMI_MODULE_FLAG       1
#define WX_HMI_MODULE_FLAG        2

//鉴权类型
enum
{
	IDLE_AUTH 		  = 0,
	START_CHARGE_AUTH = 1,
	STOP_CHARGE_AUTH  = 2
};

//鉴权状态
enum
{
	AUTHORIZE_FREE		= 0,
	AUTHORIZE_SUCCESS	= 1,
	AUTHORIZE_FAILED	= 2
};

enum
{
	UKNOWN_STATUS 	= 0,
	PILE_CONFIG 	= 1,
	START_CHARGE	= 2,
	CANCEL_CHARGE	= 3,
	STOP_CHARGE		= 4,
	FAULT_STATUS	= 5,
	PILE_STATUS		= 6,
	ORDER_GET_CHANNEL = 7
};

#pragma pack(1)

typedef struct
{
	U8 ucOrder_type;
	U8 ucOrder_data[16];
}Order_Data_t;

typedef struct
{
	BOOL bRestore_flag;
	U8 ucHmi_version[16];
	U8 ucGun_id;
	U8 ucAuthorize_flag[2];
	U8 ucCharge_control_type;
	U8 ucHmi_control_type;
	Order_Data_t sOrder_data[2];
	void (*HmiDataParse)(const U8 *buf, U16 unData_len);
}HMI_data_t;

#pragma pack(0)

extern HMI_data_t *g_psHMI_data;

void Hmi_Init_Task(void * pvParameters);

#endif /* APP_HMI_HMI_IF_H_ */
