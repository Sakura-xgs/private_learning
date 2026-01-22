/*
 * charge_comm.h
 *
 *  Created on: 2024年11月1日
 *      Author: qjwu
 */

#ifndef APP_AS_CHARGE_COMM_H_
#define APP_AS_CHARGE_COMM_H_

#include "charge_comm_IF.h"
#include "charge_general.h"

#pragma pack(1)

//CR_Plc_EvTransferType
enum
{
	AC_SINGLE_PHASE_CORE	= 0,
	AC_THREE_PHASE_CORE		= 1,
	DC_CORE					= 2,
	DC_EXTRNDED				= 3,
	DC_COMBO_CORE			= 4,
	DC_DUAL					= 5,
	AC_CORE1P_DC_EXTERDED	= 6,
	AC_SINGLE_DC_CORE		= 7,
	AC_SINGLE_PHASE_THRESS_PHASE_CORE_DC_EXTENDED	= 8,
	AC_CORE3P_DC_EXTENDED	= 9
};

typedef struct
{
    U8 invaild1:2;
    U8 CF_Secc_ArtificialSuspend:2;//人工停止
    U8 CF_Secc_FaultSpended:2;//no supported
    U8 CF_Secc_BmsSuspended:2;//车端停止
    U8 invaild3:2;
    U8 invaild4:2;
    U8 invaild5:2;
    U8 invaild6:2;
    U8 CF_Secc_SuddenStop:2;//急停停止
    U8 invaild7:2;
    U8 invaild8:2;
    U8 invaild9:2;
}CST_Data_t;

typedef struct
{
	BOOL bVersion_flag;
    U8 ucCrm_status;
    U8 ucCro_status;
    U32 stop_start_tick;
    CST_Data_t sCST;
}AS_Charging_Charger_Data_t;

#pragma pack()

extern AS_Charging_Charger_Data_t g_sAS_Charger_data[2];

Com_Stage_Def_e AS_BmsHandshakeTreat(const Gun_Id_e eGun_id);
Com_Stage_Def_e AS_BmsConfigTreat(const Gun_Id_e eGun_id);
BOOL AS_BmsPrechargeTreat(const Gun_Id_e eGun_id);
Com_Stage_Def_e AS_BmsChargingTreat(const Gun_Id_e eGun_id);
BOOL AS_BmsTimeoutTreat(const Gun_Id_e eGun_id);
BOOL AS_BmsStopChargeTreat(const Gun_Id_e eGun_id);
void SeccMessageSend(const Gun_Id_e eGun_id);

#endif /* APP_AS_CHARGE_COMM_H_ */
