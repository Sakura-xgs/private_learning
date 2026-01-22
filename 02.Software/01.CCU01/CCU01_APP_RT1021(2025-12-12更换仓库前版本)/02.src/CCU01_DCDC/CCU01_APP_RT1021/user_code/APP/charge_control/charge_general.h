/*
 * charge_general.h
 *
 *  Created on: 2025年5月5日
 *      Author: qjwu
 */
#ifndef APP_CHARGE_GENERAL_H_
#define APP_CHARGE_GENERAL_H_

#include "charge_comm_IF.h"
#include "semphr.h"
#include "timers.h"

//CIG继电器控制等待周期
#define CIG_RELAY_RES_CYCLE     50//ms
//CIG继电器控制等待超时时间
#define CIG_RELAY_TIMEOUT		2000//ms

//需求电压与当前电压压差触发式上传需求
#define TARGET_VOL_DIFF         100//0.1V
//需求电流与当前电流差触发式上传需求
#define TARGET_CUR_DIFF         50//0.1A

//最大允许电压过压限制电压值
#define OVER_VOL_LIMIT          150U //0.1V
#define OVER_VOL_TIMEOUT        4000U //ms
//过压比例
#define FAULT_OVER_VOL_RATIO    10//0.1
#define FAULT_OVER_VOL_TIMEOUT  2000U //ms
//普通过压故障时间
#define ABNORMAL_OVER_VOL_TIMEOUT 4000U //ms
//欠压比例
#define FAULT_UNDER_VOL_RATIO   20//0.2
#define FAULT_UNDER_VOL_TIMEOUT 8000U //ms

//告警触发滤波时间
#define ALARM_TRIGGER_DELAY		500U//ms
//过流限制电流倍率
#define OVER_CUR_LIMIT          110	//1.1
#define OVER_CUR_TIMEOUT        4000U //ms
#define FAULT_OVER_CUR_LIMIT    130	//1.3
#define FAULT_OVER_CUR_TIMEOUT  2000U //ms
#define OVER_CUR_DIFF           30U  //0.1A
//绝缘检测压差10V内开始检测
#define IMD_VOL_DIFF            100U	//0.1V
#define IMD_FAULT_VOL			(3*IMD_VOL_DIFF)
//预充标准压差
#define AS_STANDARD_PRECHARGE_LIMIT_VOL 200U//0.1V
#define GB_STANDARD_PRECHARGE_LIMIT_VOL 100U//0.1V
//安全电压
#define SAFE_VOL_LIMIT     		600U //0.1V
//启动时电压上限
#define START_CHARGE_VOL_LIMIT  100U //0.1V
//绝缘检测前回路电压最大值
#define IMD_START_VOL_LIMIT		100U //0.1v
//安全电流
#define SAFE_CUR_LIMIT     		50U //0.1A
//绝缘监测超时时间
#define IMD_TIMEOUT        		30000U //ms
//绝缘监测泄放时间
#define IMD_DISCHARGE_TIMEOUT   6000U //规定5S
//欧美预充超时时间
#define PRECHARGE_TIMEOUT  		7000U //规定7S
#define PRECHARGE_EVCC_TIMEOUT 	(PRECHARGE_TIMEOUT+8000U)

//通讯版本 AS:美标 CS：国标
#define AS_VERSION_MINOR   	1
#define CS_VERSION_MINOR   	1
#define AS_VERSION_MINORL  	0
#define CS_VERSION_MINORL  	1
#define AS_VERSION_MINORH  	1
#define CS_VERSION_MINORH  	0

#define AS_BHM_TIMEOUT_TICK    5100U
#define AS_BRM_TIMEOUT_TICK    10100U //GB:5100
#define AS_BCP_TIMEOUT_TICK    60100U //GB:5100
#define AS_BRO_00_TIMEOUT_TICK 10100U //GB:5100
#define AS_BRO_AA_TIMEOUT_TICK 40100U //SAEJ1772
#define AS_BCL_TIMEOUT_TICK    30050U //吉瓦特建议延迟时间  原来为1S，欧标此阶段切换最大允许时间为60S
#define AS_BCS_TIMEOUT_TICK    10100U
#define AS_BST_TIMEOUT_TICK    10100U
#define AS_BSD_TIMEOUT_TICK    10100U

#define AS_CHM_DATA_LEN        8
#define AS_CRM_DATA_LEN        8
#define AS_CTS_DATA_LEN        7
#define AS_CML_DATA_LEN        8
#define AS_CRO_DATA_LEN        2
#define AS_CCS_DATA_LEN        8
#define AS_CST_DATA_LEN        4
#define AS_CSD_DATA_LEN        5
#define AS_CEM_DATA_LEN        4

#define MAXPOWERLIMIT          180U //KW最大功率限制

#define USE_MAX_CURRENT     	1
#define NO_USE_MAX_CURRENT  	0

#define PERMISSIBLE_CHARGE  	1 //允许充电
#define SUSPENDED_CHARGE    	0 //暂停充电

#define SEND_BMS_BYTE_NUM 		0x08U
#define REC_MUL_TIMEOUT_TICK    1000

//电池反接电表电压判定
#define BMS_BAT_REVERSE_METER_VOL	2000//0.1V
//电池反接绝缘板电压判定
#define BMS_BAT_REVERSE_IMD_VOL		100//0.1V

//27930交互数据最大长度
#define CAN_MAX_MSG_LEN_27930		80

//充电机->BMS
#define SEND_BMS_EC_PACK  0x1cecf456
#define SEND_BMS_CHM_PACK 0x1826f456
#define SEND_BMS_CRM_PACK 0x1801f456
#define SEND_BMS_CTS_PACK 0x1807f456
#define SEND_BMS_CML_PACK 0x1808f456
#define SEND_BMS_CRO_PACK 0x100af456
#define SEND_BMS_CCS_PACK 0x1812f456
#define SEND_BMS_CST_PACK 0x101af456
#define SEND_BMS_CSD_PACK 0x181df456
#define SEND_BMS_CEM_PACK 0x081ff456

//BMS->充电机
#define READ_BMS_EC_PACK  0x1cec56f4
#define READ_BMS_EB_PACK  0x1ceb56f4
#define READ_BMS_BHM_PACK 0x182756f4
#define READ_BMS_BRM_PACK 0x1c0256f4
#define READ_BMS_BRO_PACK 0x100956f4
#define READ_BMS_BCL_PACK 0x181056f4
#define READ_BMS_BSM_PACK 0x181356f4
#define READ_BMS_BST_PACK 0x101956f4
#define READ_BMS_BSD_PACK 0x181c56f4
#define READ_BMS_BEM_PACK 0x081e56f4

// 充电停止等待总超时时间（ms)
#define CHARGE_STOP_WAIT_TIMEOUT_MS    	5000U
#define CHARGE_STOP_WAIT_INTERVAL_MS   	25U
#define CHARGE_STOP_WAIT_MAX_CNT       	(CHARGE_STOP_WAIT_TIMEOUT_MS / CHARGE_STOP_WAIT_INTERVAL_MS)

#define BCL_RECV_FLAG    				0x01U
#define BCS_RECV_FLAG    				0x02U
#define BCL_BCS_RECV_FLAG_ALL_DONE     	(BCL_RECV_FLAG | BCS_RECV_FLAG)

#pragma pack(1)

//CIG辅源反馈
enum
{
	CIG_ASSIST_CLOSE = 0,
	CIG_ASSIST_OPEN = 1
};
//CIG枪锁反馈
enum
{
	CIG_LOCK_RELAY_OPEN = 0,
	CIG_LOCK_RELAY_CLOSE = 1
};

enum
{
	REC_MUL_BRM	= 0,
	REC_MUL_BCP	= 1,
	REC_MUL_BCS	= 2,
	REC_MUL_BSP	= 3,
	REC_MUL_BMV	= 4,
	REC_MUL_BMT	= 5,
	REC_MUL_MAX	= 6
};

enum
{
	PLC_WAIT_TO_START		= 0,//等待启动
	PLC_START_CHARGING_REQ	= 1	//发送启动请求
};

enum
{
	DEFAULT			 = 0,
	STEPDOWN_VOLTAGE = 1,	//泄压
	STEPUP_VOLTAGE	 = 2	//升压
};

typedef enum
{
    INITDATA	= 0,	//初始状态
    HANDSHAKE	= 1,	//握手阶段
	CONFIGURE	= 2,	//配置阶段
    CHARGING	= 3,	//充电阶段
    STOPCHARGE	= 4,	//停止充电阶段
    TIMEOUT		= 5,	//超时处理阶段
	STAGEMAX	= 6
}Com_Stage_Def_e;

//记录当前阶段和上一个阶段
typedef struct
{
	Com_Stage_Def_e eLast_status;
	Com_Stage_Def_e eNow_status;
}Com_Stage_t;

typedef struct
{
    U32 uiCmd_id;		//报文can id
    U16 unPeriod_time;	//发送周期
    void (*pSend_fun)(Gun_Id_e eGun_id);
}Msg2bms_Info_t;

typedef struct
{
    BOOL bSend_flag;		//发送标志位
    U32 uiLast_send_tick;	//上一次报文发送时间
    U32 uiSend_cnt;			//发送次数
}Msg2bms_Ctrl_t;

typedef struct
{
    U32 uiStart_tick;	//开始发送时间
    U16 unRecv_cnt;		//接受报文次数
    BOOL bTimeout_flag;	//超时标志
    BOOL bEnable_flag; 	//报文有效使能标志位
}Recv_Bms_Timeout_t;

typedef struct
{
	Msg2bms_Ctrl_t sMsg2bms_ctrl[CMAX];
	Recv_Bms_Timeout_t sRecv_bms_timeout[BMAX];
	Com_Stage_t sStage;
	U8 ucReconnect_cnt;
}Bms_Ctrl_t;

typedef union
{
	U32 uiTimeout_id;
	struct
	{
		U32 brm:2;  //brm 超时
		U32 reserve1:6;
		U32 bcp:2;  //
		U32 bro:2;  //bro 超时
		U32 reserve2:4;
		U32 bcs:2;
		U32 bcl:2;
		U32 bst:2;
		U32 reserve3:2;
		U32 bsd:2;
		U32 bsm:2;
		U32 reserve4:4;
	}sTimeout;
}CEM_Data_t;

typedef struct
{
    U32 uiOver_vol_timeout[2];
    U32 uiUnder_vol_timeout[2];
    U32 uiOver_cur_timeout[2];
} PowerProtectTimeouts_t;

typedef struct
{
    g_psCharge_Control_t *psCharge_control;
    g_psCharge_Control_t *psCharge_control_A;
    g_psCharge_Control_t *psCharge_control_B;
} g_psCharge_Control_Ptrs_t;

#pragma pack()

extern BOOL g_bStart_charge_success_flag[2];
extern U8 g_ucSend_BCS_end_cnt[2];
extern U8 g_ucRec_multipack_len[2][REC_MUL_MAX];
extern U8 g_ucRec_packnum[2][REC_MUL_MAX];
extern TimerHandle_t g_sRec_BCS_timer[2];
extern TimerHandle_t g_sRec_BCP_timer[2];
extern Bms_Ctrl_t g_sBms_ctrl[2];
extern U32 g_uiErr_vol_tick[2];

U8 DECtoBCD(const U32 uiData);
void SendStopChargeMsg(const Gun_Id_e eGun_id);
void StopRelayControl_A(void);
void StopRelayControl_B(void);
void StartPrechargeRelayControl(const Gun_Id_e eGun_id);
void PrechargeFailRelayControl(const Gun_Id_e eGun_id);
void PrechargeFinishRelayControl(const Gun_Id_e eGun_id);
void ChargeFinishProcess(const Gun_Id_e eGun_id, const BOOL bFlag);
void PrechargeFailProcess(const Gun_Id_e eGun_id);
BOOL AS_CheckSysChargingErr(const Gun_Id_e eGun_id);

BOOL BmsInsulationCheck(const Gun_Id_e eGun_id);
BOOL BmsChargingAlarmCheck(const U32 uiNow_tick, const Gun_Id_e eGun_id);
BOOL BmsStageSwitchCheck(Com_Stage_t* psStage);

void SendGunConnectAbort(U32 uiPgn, const Gun_Id_e eGun_id);
void SendGunRecBRMRequestPack(const Gun_Id_e eGun_id);
void SendGunRecBRMEndPack(const Gun_Id_e eGun_id);
void SendGunRecBCPRequestPack(const Gun_Id_e eGun_id);
void SendGunRecBCPEndPack(const Gun_Id_e eGun_id);
void SendGunRecBCSRequestPack(const Gun_Id_e eGun_id);
void SendGunRecBCSEndPack(const Gun_Id_e eGun_id);
void SendGunRecBSPRequestPack(const Gun_Id_e eGun_id);
void SendGunRecBSPEndPack(const Gun_Id_e eGun_id);
void SendGunRecBMVRequestPack(const Gun_Id_e eGun_id);
void SendGunRecBMVEndPack(const Gun_Id_e eGun_id);
void SendGunRecBMTRequestPack(const Gun_Id_e eGun_id);
void SendGunRecBMTEndPack(const Gun_Id_e eGun_id);

#endif /* APP_CHARGE_GENERAL_H_ */
