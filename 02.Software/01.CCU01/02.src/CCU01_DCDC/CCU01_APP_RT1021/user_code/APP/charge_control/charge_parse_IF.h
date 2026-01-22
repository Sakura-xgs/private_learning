/*
 * charge_parse_IF.h
 *
 *  Created on: 2024年11月1日
 *      Author: qjwu
 */

#ifndef APP_CHARGE_PARSE_IF_H_
#define APP_CHARGE_PARSE_IF_H_

#include "PublicDefine.h"

//使用外设选择
#define JWT_SECC_MODULE_FLAG    1
#define RKN_SECC_MODULE_FLAG    2

typedef struct
{
	void (*ChargeControl)(Gun_Id_e eGun_id);
	void (*CAN1DataParse)(U32 uiCan_id, const U8* pucBuf);
	void (*CAN2DataParse)(U32 uiCan_id, const U8* pucBuf);
	void (*CANMessageSend)(Gun_Id_e eGun_id);
}g_psCharge_Control_t;

extern g_psCharge_Control_t *g_psCharge_control;
extern g_psCharge_Control_t *g_psCharge_control_A;
extern g_psCharge_Control_t *g_psCharge_control_B;

BOOL ChargeStartVoltageCheck(const Gun_Id_e eGun_id);
void RKNCAN1Parse(const U32 uiCan_id, const U8* pucBuf);
void RKNCAN2Parse(const U32 uiCan_id, const U8* pucBuf);
void JWTCAN1Parse(const U32 uiCan_id, const U8* pucBuf);
void JWTCAN2Parse(const U32 uiCan_id, const U8* pucBuf);
void GBCAN1Parse(const U32 uiCan_id, const U8* pucBuf);
void GBCAN2Parse(const U32 uiCan_id, const U8* pucBuf);

#endif /* APP_CHARGE_PARSE_IF_H_ */
