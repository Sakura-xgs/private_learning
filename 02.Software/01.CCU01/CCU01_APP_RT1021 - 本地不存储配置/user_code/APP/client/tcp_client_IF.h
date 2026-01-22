/*
 * tcp_client_IF.h
 *
 *  Created on: 2024年10月7日
 *      Author: qjwu
 */
#ifndef APP_CLIENT_TCP_CLIENT_IF_H_
#define APP_CLIENT_TCP_CLIENT_IF_H_

#include "timers.h"
#include "semphr.h"
#include "PublicDefine.h"

#pragma pack(1)

//桩禁用状态
#define GUN_DISABLE_STATUS          	0x02U
#define GUN_ENABLE_STATUS           	0x01U

//桩告警&故障预留字段长度
#define PILE_ALARM_ERROR_RESERVE_LEN    5
//POS告警&故障预留字段长度
#define POS_ALARM_ERROR_RESERVE_LEN     1
//RFID告警&故障预留字段长度
#define RFID_ALARM_ERROR_RESERVE_LEN    1
//HMI告警&故障预留字段长度
#define HMI_ALARM_ERROR_RESERVE_LEN     1
//桩所属告警故障总长度
#define PILE_DEVICE_LEN                 8U

//枪告警字段长度
#define GUN_WARN_ERROR_RESERVE_LEN      5
//枪故障字段长度
#define GUN_FAULT_ERROR_RESERVE_LEN     8
//METER告警&故障字段长度
#define METER_ALARM_ERROR_RESERVE_LEN   1
//IMD告警&故障字段长度
#define IMD_ALARM_ERROR_RESERVE_LEN     1
//SECC告警字段长度
#define SECC_WARN_ERROR_RESERVE_LEN     1
//SECC故障字段长度
#define SECC_ALARM_ERROR_RESERVE_LEN    30U
//枪告警长度
#define GUN_WARN_DEVICE_LEN             8U
//枪告警预留
#define GUN_WARN_RESERVE_LEN			4U
//枪故障长度
#define GUN_ERROR_DEVICE_LEN            40U
//枪故障预留
#define GUN_ERROR_RESERVE_LEN			10U

#define C_SECC_UP_LOG_CMD				0x0015//SECC日志上传指令
#define C_SECC_UP_LOG_ANS_CMD			0x8015
#define S_CCU_UPDATE_CMD				0x0028//CCU升级指令
#define C_CCU_UPDATE_ANS_CMD			0x8028
#define S_SECC_UPDATE_CMD				0x0029//SECC升级指令
#define C_SECC_UPDATE_ANS_CMD			0x8029
#define S_SECC_LOG_REQUEST_CMD			0x0030//SECC日志请求指令
#define C_SECC_LOG_REQUEST_ANS			0x8030

//枪类型
enum
{
	CCS1 = 0x01,
	NACS = 0x02,
	CCS2 = 0x03,
	GB 	 = 0x04
};

typedef struct
{
	TimerHandle_t sTimer_login;
	BOOL bLogin_timeup_flag;
}Login_Control_t;

typedef struct
{
	TimerHandle_t sTimer_heart;
	BOOL bHeart_timeup_flag;
}Herat_Control_t;

typedef struct
{
	TimerHandle_t sTimer_auth;
	BOOL bAuth_timeup_flag;
}Auth_Control_t;

typedef struct
{
	TimerHandle_t sTimer_charging;
	BOOL bCharging_timeup_flag;
	BOOL bSend_charging_data_flag;
}Charging_Control_t;

typedef struct
{
	TimerHandle_t sTimer_stop_charge;
	BOOL bStop_charge_timeup_flag;
}Stop_Charge_Control_t;

typedef struct
{
	BOOL bSend_log_flag;
	U32 uiLog_len;
	U8 ucLog_data[UART_COMM_BUF_LEN];
}CCU_Log_Control_t;

typedef struct
{
	Login_Control_t sLogin_control;
	Herat_Control_t sHeart_control;
	Auth_Control_t sAuth_control;
	Charging_Control_t sCharging_control;
	Stop_Charge_Control_t sStop_charge_control;
	CCU_Log_Control_t sCCU_log_control;
}CMD_Msg_Control_t;

typedef struct
{
	U8 ucAuth_type;				//鉴权类型：start/stop
	U8 ucStart_Charge_type;		//启动充电类型
	U8 ucStop_Charge_type;		//结束充电类型
	U8 ucIdtag[20];				//标识id
	U32 uiStart_meter_dn;		//启动电表电量
	U32 uiStart_time;			//启动时间
	U8 ucEVCCID[6];
	U8 ucCharge_current_step;	//当前充电步骤
	U16 unEV_max_imd_vol;		//绝缘检测电压 0.1V
	U16 unEV_max_allow_cur;		//最大运行电流 0.1A
	U16 unStart_soc;			//启动soc 	  %0.1
	U16 unEV_target_vol;		//车端目标电压 0.1V
	U16 unEV_target_cur;		//车端目标电流 0.1A
	U16 unEV_max_limit_vol;		//车端充电中最大限制电压 0.1V
	U8 ucImd_control_flag;		//绝缘升压/泄压控制位
	U8 ucImd_status;			//绝缘状态
	U8 ucPrecharge_status;		//预充状态
	U8 ucCharging_gun_status;	//充电枪状态
	U32 uiOrder_id;				//账单id
	U8 ucCurrent_soc;			//当前soc 		%1
	U16 unAlready_charge_time;	//已充电时间 	s
	U16 unRemain_time;			//剩余充电时间 	分
	U8 ucBms_version[3]; 		//BMS协议
	U8 ucBattery_type;
	U16 unVehicle_rate_cap;	 	//整车额定容量 0.1Ah
	U16 unVehicle_rate_vol;		//整车额定电压 0.1V
	U8 ucVin[17];				//车辆VIN码
	U16 unSingle_bat_max_vol;	//单体电池最高允许电压
	U16 unBat_total_energy;		//电池标称总能量
	U8 ucBms_max_allow_temp;	//最高允许温度
	//U8 ucSingle_bat_max_allow_temp;	//单体电池最高允许温度
	U16 unBms_current_soc; 		//动力蓄电池荷电状态0.1%
	U16 unBat_current_vol;		//蓄电池当前电压0.1V
	U16 unVehicle_check_vol;	//车辆接口监测电压0.1V
	U16 unVehicle_check_cur;	//车辆接口检测电流0.1A
	U16 unMax_single_bat_vol_and_group;//最高单体电池电压及组号
	U16 unMax_single_bat_vol_num;//最高电梯电池电压编号
	U8 ucSingle_bat_max_temp;	//单体电池最高温度
	U8 ucMax_bat_temp_num;		//最高温度电池编号
	U8 ucSingle_bat_min_temp;	//单体电池最低温度
	U8 ucMin_bat_temp_num;		//最低温度电池编号
	U8 ucSingle_bat_vol_status;	//单体动力电池电压状态
	U8 ucBat_soc_status;		//动力蓄电池SOC状态
	U8 ucBat_cur_status;		//动力蓄电池电流状态
	U8 ucBat_temp_status; 		//动力蓄电池温度状态
	U8 ucBat_imd_status;		//动力蓄电池绝缘状态
	U8 ucBat_connector_status;	//动力蓄电池连接器状态
	BOOL bAllow_charge_flag;	//充电允许状态

}Temp_Data_t;

//枪数据
typedef struct
{
	Temp_Data_t sTemp;
	U32 uiOutput_vol;				//输出电压 0.01
	U32 uiOutput_cur;				//输出电流 0.01
	U32 uiOutput_power;				//输出功率 0.0001
	U32 uiCurrent_meter_dn;			//输出电量 0.0001
	U8 ucImd_version[16];			//绝缘监测版本
	U8 ucSecc_version[16];			//SECC版本
	U8 ucMeter_version[16];			//电表版本
	U8 ucGun_common_status;			//枪普通状态
	U8 ucGun_special_status;		//枪特殊状态
	U8 ucConnect_status;			//cp/cc1状态
	S16 nDC_positive_temp;			//正极枪温
	S16 nDC_negative_temp;			//负极枪温
	BOOL bGun_return_flag;			//回枪标志位
	BOOL bPrecharge_relay_status;	//预充继电器状态
	BOOL bPositive_relay_status;	//正极继电器状态
	BOOL bNegative_relay_status;	//负极继电器状态
	BOOL bPositive_relay_feedback_status;//正极继电器反馈状态
	BOOL bNegative_relay_feedback_status;//负极继电器反馈状态
	BOOL bFuse_io_status;			//熔断器状态
	U8 ucLed_dutycycle[3];          //led pwm占空比
	U16 unAdmin_limit_power;		//平台最大限制功率
	U8 ucGun_disable_status;		//枪禁用状态
	U8 ucGun_warn[8];				//枪告警
	U8 ucGun_fault[23];				//枪故障
	U32 uiBook_start_time;			//预约开始时间
	U32 uiBook_end_time;			//预约结束时间
	U8 ucBms_assist_power_status;	//辅源状态
	U8 ucBms_assist_power_feedback_status;//辅源反馈状态
	U8 ucGun_lock_relay_status;		//枪锁继电器状态
	U8 ucGun_lock_relay_feedback_status;//枪锁继电器反馈状态
	U16 unCC1_vol;					//CCS1电压
}Gun_Data_t;

typedef struct
{
	U8 ucStart_control_id;	//控制启动充电的id
	U8 ucFinish_control_id;	//控制结束充电的id
	BOOL bSend_auth_flag;	//发送鉴权报文标志位
	BOOL bStop_charge_flag;	//发送请求停止充电报文标志位
	BOOL bFinsh_charge_flag;//发送结束充电报文标志位
}Gun_ID_Status_t;

//枪通用数据（桩数据）
typedef struct
{
	U8 ucPile_config_mode;
	BOOL bHit_enable_flag;
	BOOL bHit_io_status;
	BOOL bMCB_enable_flag;
	BOOL bMCB_io_status;
	BOOL bSPD_enable_flag;
	BOOL bSPD_io_status;
	BOOL bDoor_enable_flag;
	BOOL bDoor_io_status;
	BOOL bWater_enable_flag;
	BOOL bWater_io_status;
	BOOL bEstop_io_status;
	U8 ucFan_pwm;
	U8 ucPile_warn[16];
	U8 ucPile_fault[16];
	U8 ucHmi_version[16];
	U8 ucPos_version[32];
	U8 ucRfid_version[16];
	U8 ucSoftware[16];
	U8 ucHardware[16];
 	U8 ucCig_soft_version[16];		//Cig小板软件版本
 	U8 ucCig_hardware[16];			//Cig小板硬件版本
 	U8 ucCig_SN[32];
 	U16 unCig_temp1;	//0.1℃
 	U16 unCig_temp2;	//0.1℃

}Pile_Data_t;

typedef struct
{
	U8 ucGun_type;		//枪类型
	U8 ucGun_id;		//枪id
	U16 unGun_max_vol;	//最大输出电压V
	U16 unGun_min_vol;	//枪最小输出电压V
	U16 unGun_max_cur;	//最大输出电流A
	U16 unGun_min_cur;	//最小输出电流A
	U8 ucMeter_model;	//电表型号
	U8 ucSECC_model;	//SECC型号
	U8 ucIMD_model;		//绝缘监测板型号
	U16 unGun_warn_temp;//告警枪温 偏移量-60
	U16 unGun_fault_temp;//故障枪温 偏移量-60
	U16 unGun_diff_temp;//温差阈值 偏移量-60
	U8 reserve2[42];
}Storage_Public_Data_t;

//充电桩需存储的数据
typedef struct
{
	U8 ucPile_num;				//桩编号
//	U8 ucGun_num;
	U8 ucCharge_id[32];			//OCPP下发id
	BOOL bLocal_bill_flag;		//本地计费使能
	BOOL bUse_ad_screen;		//是否使用显示屏
	U8 ucHMI_model;				//HMI型号
	BOOL bUse_ad_pos;			//是否使用POS
	U8 ucPOS_model;				//POS型号
	BOOL bUse_ad_rfid;			//是否使用RFID
	U8 ucRFID_model;			//RFID型号
	U8 ucM1_key_mode_first;		//M1卡秘钥模式
	U8 ucM1_sector_first;		//M1卡区号
	U8 ucM1_block_first;		//M1卡块号
	U8 ucM1_password_first[12];	//秘钥
	U8 ucM1_key_mode_second;
	U8 ucM1_sector_second;
	U8 ucM1_block_second;
	U8 ucM1_password_second[12];
	U16 unCC1_12V_max;			//12V上限
	U16 unCC1_12V_min;			//12V下限
	U16 unCC1_6V_max;			//6V上限
	U16 unCC1_6V_min;			//6V下限
	U16 unCC1_4V_max;			//4V上限
	U16 unCC1_4V_min;			//4V下限
	U8 reserve1[12];
	Storage_Public_Data_t sPublic_data[2];
}Storage_Gun_Data_t;

//桩告警缓存
typedef struct
{
	U8 ucPile_warn_buf[PILE_ALARM_ERROR_RESERVE_LEN];	//桩告警缓存
	U8 ucPos_warn_buf[POS_ALARM_ERROR_RESERVE_LEN];		//POS机告警缓存
	U8 ucRfid_warn_buf[RFID_ALARM_ERROR_RESERVE_LEN];	//RFID告警缓存
	U8 ucHmi_warn_buf[HMI_ALARM_ERROR_RESERVE_LEN];		//HMI告警缓存
	U8 ucSecc_warn_buf[SECC_WARN_ERROR_RESERVE_LEN];	//SECC告警缓存
}Pile_Warn_Buf_t;

//桩故障缓存
typedef struct
{
	U8 ucPile_fault_buf[PILE_ALARM_ERROR_RESERVE_LEN];	//桩故障
	U8 ucPos_fault_buf[POS_ALARM_ERROR_RESERVE_LEN];	//POS机故障
	U8 ucRfid_fault_buf[RFID_ALARM_ERROR_RESERVE_LEN];	//RFID故障
	U8 ucHmi_fault_buf[HMI_ALARM_ERROR_RESERVE_LEN];	//HMI故障
}Pile_Fault_Buf_t;

//枪告警缓存
typedef struct
{
	U8 ucGun_warn_buf[GUN_WARN_ERROR_RESERVE_LEN];		//枪告警缓存
	U8 ucImd_warn_buf[IMD_ALARM_ERROR_RESERVE_LEN];		//绝缘监测板桩告警缓存
	U8 ucMeter_warn_buf[METER_ALARM_ERROR_RESERVE_LEN];	//电表枪桩告警缓存
	U8 ucSecc_warn_buf[SECC_WARN_ERROR_RESERVE_LEN];	//SECC枪桩告警缓存
}Gun_Warn_Buf_t;

//枪故障缓存
typedef struct
{
	U8 ucGun_fault_buf[GUN_FAULT_ERROR_RESERVE_LEN];	//枪故障缓存
	U8 ucImd_fault_buf[IMD_ALARM_ERROR_RESERVE_LEN];	//绝缘监测板桩故障缓存
	U8 ucMeter_fault_buf[METER_ALARM_ERROR_RESERVE_LEN];//电表枪桩故障缓存
	U8 ucSecc_fault_buf[SECC_ALARM_ERROR_RESERVE_LEN];	//SECC桩故障缓存
}Gun_Fault_Buf_t;

typedef struct
{
	Gun_Warn_Buf_t sGun_warn;
	Gun_Fault_Buf_t sGun_fault;
}Gun_Alarm_Upload_Buff_t;

typedef struct
{
	Pile_Warn_Buf_t sPile_warn;
	Pile_Fault_Buf_t sPile_fault;
}Pile_Alarm_Upload_Buff_t;

typedef struct
{
	BOOL bLoginResult;		//登录是否成功
	U8 ucHeart_unansNum;	//心跳包未应答次数
	U8 ucLogin_unansNum;	//登录包未应答次数
	U8 ucCharging_unansNum;	//充电实时数据未应答次数
	U8 ucAuth_unansNum;     //鉴权数据未应答次数
}Connect_Control_t;

#pragma pack()

extern Pile_Data_t g_sPile_data;
extern Gun_Data_t g_sGun_data[2];
extern Gun_ID_Status_t g_sGun_id_status;
extern CMD_Msg_Control_t g_sCMD_msg_control;
extern Storage_Gun_Data_t g_sStorage_data;
extern S32 sockfd;
extern SemaphoreHandle_t g_sAlarm_tigger_sem;
extern Connect_Control_t g_sConnect_control;

void TCP_Init_Task(void * pvParameters);
void TcpSendControl(U8* ucFlag);
void GetPileConfig(void);
void Int2Bigendian(U8 *pucBuf, U32 uiValue);
void CommunicationParse(U8 *pucBuf, U16 unBuf_len);
U8 SetAlarmFlagFunc(void);
void SpecialStatusSetProcess(const U8 ucGun_id, const U8 ucStatus);
BOOL TcpSend(const U8 *ucSendBuff, U32 uiSendLength);
BOOL CheckChargingStatus(const U8 ucGun_id);
void TcpClose(void);
void CheckServerConfig(void);

#endif /* APP_CLIENT_TCP_CLIENT_IF_H_ */
