/*
 * emergency_fault.h
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */

#ifndef APP_FAULT_EMERGENCY_FAULT_H_
#define APP_FAULT_EMERGENCY_FAULT_H_

#include "emergency_fault_IF.h"

//回枪检测告警时间
#define GUN_RETURN_TIMER_PERIOD_MS  30000
//故障触发检测到的次数
//特殊故障
#define EMERGENCY_FAULT_TIGGER_CNT  1U
//普通故障触发
#define COMM_FAULT_TRIGGER_CNT     	10U
//故障恢复
#define FAULT_RESTORE_CNT           10U
//特殊故障触发
#define SPECIAL_FAULT_TRIGGER_CNT	5U

#define TASK_DELAY_PERIOD           100U//ms
//枪温告警/故障恢复回差值
#define TEMP_FAULT_HYSTERESIS       10U//℃
#define TEMP_DIFF_HYSTERESIS		5U

#pragma pack(1)

typedef struct
{
	U8 ucPositive_cnt[2];
	U8 ucNegative_cnt[2];
	U8 ucAssist_power_cnt[2];
	U8 ucGun_relay_cnt[2];
}Gun_item_t;

typedef struct
{
	U8 ucScram_cnt;
	U8 ucDoorGuard_cnt;
	U8 ucWaterImmersion_cnt;
	U8 ucSPD_cnt;
	U8 ucHit_cnt;
	U8 ucMCB_cnt;
	U8 ucGUNAFuse_cnt;
	U8 ucGUNBFuse_cnt;
	U8 ucGunAReturn_cnt;
	U8 ucGunBReturn_cnt;
	U8 ucTemp_diff_cnt[2];
	Gun_item_t sTemp_warn;
	Gun_item_t sTemp_fault;
	Gun_item_t sRelay_stick;
	Gun_item_t sRelay_close;
}Fault_Tirgger_Cnt_t;

typedef struct
{
	U8 ucScram_cnt;
	U8 ucDoorGuard_cnt;
	U8 ucWaterImmersion_cnt;
	U8 ucSPD_cnt;
	U8 ucHit_cnt;
	U8 ucMCB_cnt;
	U8 ucGUNAFuse_cnt;
	U8 ucGUNBFuse_cnt;
	U8 ucGunAReturn_cnt;
	U8 ucGunBReturn_cnt;
	U8 ucTemp_diff_cnt[2];
	Gun_item_t sTemp_warn;
	Gun_item_t sTemp_fault;
	Gun_item_t sRelay_stick;
	Gun_item_t sRelay_close;
}Fault_restore_Cnt_t;

#pragma pack()

#endif /* APP_FAULT_EMERGENCY_FAULT_H_ */
