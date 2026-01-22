/*
 * charge_comm_IF.h
 *
 *  Created on: 2024年11月1日
 *      Author: qjwu
 */
#ifndef APP_CHARGE_COMM_IF_H_
#define APP_CHARGE_COMM_IF_H_

#include <charge_parse_IF.h>

//27930桩端发送协议阶段
enum
{
	CHM = 0,
	CRM	= 1,
	CTS = 2,
	CML = 3,
	CRO = 4,
	CCS = 5,
	CST = 6,
	CSD = 7,
	CEM = 8,
	CMAX = 9
};
//27930车端响应协议阶段
enum
{
	BHM		= 0,
	BRM		= 1,
	BCP		= 2,
	BRO_00	= 3,
    BRO_AA	= 4,
	BCL		= 5,
	BCS		= 6,
	BSP		= 7,
	BST		= 8,
	BSD		= 9,
    BEM		= 10,
	BSM		= 11,
	BMAX	= 12
};

enum
{
	//握手
	HANDSHAKE_BHM 		= 1,
	HANDSHAKE_BRM 		= 2,

	//配置
	CONFIG_BCP			= 3,
	CONFIG_BRO			= 4,
	CONFIG_ISO_CHECK	= 5,
	CONFIG_PRECHARGE	= 6,

	//充电中
	CHARGING_BCL_BCS	= 7,

	//结束充电
	END_OF_CHARGE_CST	= 8,
	END_OF_CHARGE_CSD	= 9,
	END_OF_CHARGE_CEM	= 10
};

typedef enum
{
    PRECHARGE_RELAY = 0,//预充继电器
	POSITIVE_RELAY	= 1,//高压正极继电器
	NEGATIVE_RELAY	= 2,//高压负极继电器
}RELAY_CONTROL_t;

extern U8 g_ucCF_Secc_StartPlc[2];

U8 AS_GetGunPrechargeStatus(const U8 ucGun_id);
void RelayControl_A(const RELAY_CONTROL_t sMode, BOOL ucFlag);
void RelayControl_B(const RELAY_CONTROL_t sMode, BOOL ucFlag);
void StopChargeControl(const U8 ucGun_id);
void StartChargeControl(const U8 ucGun_id);

U8 GetGunStatus(const U8 ucGun_id);
void ClearChargingData(const U8 ucGun_id);
void CheckAnotherGunStatus(const U8 ucGun_id);
void Charge_Init_Task(void * pvParameters);
void GB2015Init(void);

#endif /* APP_CHARGE_COMM_IF_H_ */
