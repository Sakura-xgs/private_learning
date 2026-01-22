/*
 * hmi.h
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */

#ifndef APP_HMI_HMI_H_
#define APP_HMI_HMI_H_

#include "PublicDefine.h"

#pragma pack(1)

#define HEAD_INDEX                  0
//充电控制报文数据长度
#define CHARGE_CONTROL_DATA_LEN		9
//状态获取报文数据长度
#define GET_STATUS_DATA_LEN     	24
//桩编号配置报文数据长度
#define SET_PILE_NUM_DATA_LEN       8
//用户账单获取方式报文数据长度
#define SET_ORDER_DATA_LEN       	25

#define CRC_LEN              		2U
//是否使能占位费
//#define IDLE_FEE

//#define HMI_DEBUG

typedef struct
{
	U8 ucHead_num;		//包头
	U8 ucCmd;			//cmd
	U16 ucData_len;		//数据长度
	U8 ucGun_id;		//枪id
	U8 ucCharge_type;	//充电控制类型
	U8 ucTail_num;		//包尾
	U16 unCrc;			//crc校验
}HMI_Charge_Control_t;

typedef struct
{
	U8 ucHead_num;		//包头
	U8 ucCmd;			//cmd
	U16 ucData_len;		//数据长度
	U8 ucPile_num;		//桩编号
	U8 ucTail_num;		//包尾
	U16 unCrc;			//crc校验
}HMI_Set_Num_t;

typedef struct
{
	U8 ucHead_num;		//包头
	U8 ucCmd;			//cmd
	U16 ucData_len;		//数据长度
	U8 ucPile_num;		//桩编号
	U8 ucVersion[16];	//软件版本
	U8 ucTail_num;		//包尾
	U16 unCrc;			//crc校验
}HMI_Get_Status_t;

#pragma pack()

extern HMI_Charge_Control_t g_HMI_charge_control;
extern HMI_Set_Num_t g_HMI_set_num;
extern HMI_Get_Status_t g_HMI_get_status;

void PileStatusResponse(void);
void HmiStartCharge(U8 ucGun_id);
void HmiCancelCharge(U8 ucGun_id);
void HmiStopCharge(U8 ucGun_id);

#endif /* APP_HMI_HMI_H_ */
