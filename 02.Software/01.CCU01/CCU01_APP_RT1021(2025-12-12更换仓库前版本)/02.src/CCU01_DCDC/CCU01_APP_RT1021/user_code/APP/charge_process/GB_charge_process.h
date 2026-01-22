/*
 * GB_charge_process.h
 *
 *  Created on: 2025年5月5日
 *      Author: qjwu
 */

#ifndef APP_CHARGE_PROCESS_GB_CHARGE_PROCESS_H_
#define APP_CHARGE_PROCESS_GB_CHARGE_PROCESS_H_

//非充电和插枪状态持续关闭辅源和断开枪锁指令
//#define ENABLE_CIG_RELAY_CONTROL          1

g_psCharge_Process_t* GB_ChargeProcess(void);
void GBChargeProcessInit(void);

#endif /* APP_CHARGE_PROCESS_GB_CHARGE_PROCESS_H_ */
