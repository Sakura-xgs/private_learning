/*
 * secc_parse.h
 *
 *  Created on: 2024年11月1日
 *      Author: qjwu
 */
#ifndef APP_AS_CHARGE_PARSE_H_
#define APP_AS_CHARGE_PARSE_H_

#include <charge_parse_IF.h>

#pragma pack(1)

/******SECC通讯 协议数据定义*********/
typedef struct
{
	U16 unCR_Plc_EvPermissibleMaxVolt;//not supported
	U8 ucCR_Plc_Aagvalue;
	U8 CR_Plc_CpStatus:2;	//充电枪状态
	U8 reserve1:2;
	U8 CR_Plc_CommStart:2;	//桩端启动充电标志 1：start 0：not start
	U8 reserve2:2;
	U8 ucVersion_year;
	U8 ucSecc_type;
	U8 ucVersion_month;
	U8 ucVersion_day;
}AS_BHM_Data_t;

typedef struct
{
	U8 ucCR_Plc_VersionMinor;
	U8 ucCR_Plc_VersionMajorL;
	U8 CR_Plc_EvCCID[6];
}AS_BRM_Data_t;

typedef struct
{
	U16 invaild1;
	U16 unCR_Plc_EvmaxCurrent;	//车端最大允许电流
	U16 invaild2;
	U16 unCR_Plc_EvmaxVolt;		//绝缘检测电压
	U8 	invaild3;
	U16 unCR_Plc_EvRessSOC;		//车端初始soc 0.1%
	U16 invaild4;
	U8 CR_Plc_EvTransferType:4;	//能量传输类型：单相电、三相电等
	U8 reserve:4;
}AS_BCP_Data_t;

typedef struct
{
	U8 ucBro_ready;
}AS_BRO_Data_t;

typedef struct
{
	U16 unCR_Plc_EvTargetVolt; 		//目标电压 0.1V
	U16 unCR_Plc_EvTargetCurrent; 	//目标电流0.1A
	U8 ucCR_Plc_ChargingMode;		//充电模式
	U16 unCR_Plc_EvMaxVoltLimit;	//充电中最大限制电压0.1V
	U8 ucCR_Plc_PreCharge_Stauts;	//预充状态标志位
}AS_BCL_Data_t;

typedef struct
{
	U16 invaild1;
	U16 invaild2;
	U16 invaild3;
	U8 ucCR_Plc_EvRessSOC;				//当前soc
	U16 unCR_Plc_FullSOCRemainedTime;	//剩余充电完成时间
}AS_BCS_Data_t;

typedef struct
{
	U8 invaild1;
	U8 invaild2;
	U8 invaild3;
	U8 invaild4;
	U8 invaild5;
	U8 invaild6:2;
	U8 invaild7:2;
    U8 invaild8:2;
    U8 invaild9:2;
    U8 invaild10:2;
    U8 invaild11:2;
    U8 CF_Plc_EvReady:2;//车端控制是否暂停充电
    U8 reserve:2;
}AS_BSM_Data_t;

typedef struct
{
	U8 ucCR_Plc_SuspendSoc;			//结束soc
	U16 invaild1;
	U16 invaild2;
	U8  invaild3;
	U8  invaild4;
	U8 CF_Plc_WeldDetectionReq:1;	//焊接检测
	U8 reserve:7;
}AS_BSD_Data_t;

typedef struct
{
	AS_BHM_Data_t sBHM;
    AS_BRM_Data_t sBRM;
    AS_BCP_Data_t sBCP;
    AS_BRO_Data_t sBRO;
    AS_BCL_Data_t sBCL;
    AS_BCS_Data_t sBCS;
    AS_BSM_Data_t sBSM;
    AS_BSD_Data_t sBSD;
}AS_Charging_Bms_Data_t;

#pragma pack()

extern AS_Charging_Bms_Data_t g_sAS_Bms_data[2];

void SECCMacParse(const AS_BRM_Data_t sBRM_data, const U8 ucGun_id);
void SECCVersionParse(const AS_BHM_Data_t sBHM_data, const U8 ucGun_id);
void AS_BroMessageProcess(const U8 ucBro_status, const U8 ucGun_id);
void AS_BclMessageProcess(const AS_BCL_Data_t sBCL_data, const U8 ucGun_id);
void AS_BsmMessageProcess(const U8 ucGun_id);
void AS_BcpPrase(const U8 *ucBuf, const U8 ucLen, const U8 ucGun_id);
void AS_BcsPrase(const U8 *ucBuf, const U8 ucLen, const U8 ucGun_id);
void RecvMultipackTimerTreat_A(U8 ucPack_num, BOOL bTimer_flag);
void RecvMultipackTimerTreat_B(U8 ucPack_num, BOOL bTimer_flag);
void HandleEBMultipack_A(const U8 mul_type, const char *cName, const U8 *ucBuf);
void HandleEBMultipack_B(const U8 mul_type, const char *cName, const U8 *ucBuf);

#endif /* APP_AS_CHARGE_PARSE_H_ */
