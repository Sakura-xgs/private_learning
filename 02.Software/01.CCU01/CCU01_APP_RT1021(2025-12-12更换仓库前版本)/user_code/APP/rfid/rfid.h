/*
 * rfid.h
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */

#ifndef APP_RFID_RFID_H_
#define APP_RFID_RFID_H_

#include "timers.h"
#include "PublicDefine.h"

//寻卡检测超时时间
#define RFID_TIMER_PERIOD_MS       60000

extern TimerHandle_t g_sTimer_rfid;

#endif /* APP_RFID_RFID_H_ */
