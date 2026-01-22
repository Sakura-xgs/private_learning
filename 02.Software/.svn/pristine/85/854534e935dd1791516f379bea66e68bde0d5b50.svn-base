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

extern Charge_Sta_e g_eCharge_status[2];

void AlarmStatusProcess(const Gun_Id_e eGun_id);
void IdleStatusProcess(const Gun_Id_e eGun_id);
void PlugingStatusProcess(const Gun_Id_e eGun_id);
void ChargingStatusProcess(const Gun_Id_e eGun_id);
void FaultStatusProcess(const Gun_Id_e eGun_id);

#endif /* APP_CHARGE_PROCESS_CHARGE_PROCESS_H_ */
