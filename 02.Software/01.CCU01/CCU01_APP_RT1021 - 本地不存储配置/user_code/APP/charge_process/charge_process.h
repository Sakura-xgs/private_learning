/*
 * charge_precess.h
 *
 *  Created on: 2024年11月1日
 *      Author: qjwu
 */

#ifndef APP_CHARGE_PROCESS_CHARGE_PROCESS_H_
#define APP_CHARGE_PROCESS_CHARGE_PROCESS_H_

#include "charge_process_IF.h"

//#define ENABLE_PNC 1

extern Charge_Sta_t g_sCharge_status[2];

void AlarmStatusProcess(const U8 ucGun_id);
void IdleStatusProcess(const U8 ucGun_id);
void PlugingStatusProcess(const U8 ucGun_id);
void ChargingStatusProcess(const U8 ucGun_id);
void FaultStatusProcess(const U8 ucGun_id);

#endif /* APP_CHARGE_PROCESS_CHARGE_PROCESS_H_ */
