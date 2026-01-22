/*
 * charge_process.h
 *
 *  Created on: 2024年9月30日
 *      Author: qjwu
 */
#ifndef APP_CHARGE_PROCESS_CHARGE_PROCESS_IF_H_
#define APP_CHARGE_PROCESS_CHARGE_PROCESS_IF_H_

typedef enum
{
	NORMAL_STATUS 	= 0,
	WARNING_STATUS 	= 1,
	ALARM_STATUS 	= 2
}Alarm_Status_t;

enum
{
	LOCAL_START_MODE 	= 1,/*本地启动*/
	REMOTE_START_MODE	= 2,/*远程启动*/
	RFID_START_MODE		= 3,/*RFID启动*/
	POS_START_MODE		= 4,/*POS启动*/
	PNC_START_MODE		= 5,/*即插即充启动*/
	WEB_START_MODE		= 6	/*web启动*/
};

enum
{
	UKNOWN_STOP_MODE 			= 0,
	LOCAL_STOP_MODE 			= 1,/*本地停止*/
	REMOTE_STOP_MODE			= 2,/*远程停止*/
	RFID_STOP_MODE				= 3,/*RFID停止*/
	POS_STOP_MODE				= 4,/*POS停止*/
	WEB_STOP_MODE				= 5,/*web停止*/
	EV_DISCON_MODE				= 6,/*车端主动停止*/
	REACH_TOTAL_SOC_TARGET		= 7,/*达到设定SOC*/
	PREPAID_BALANCE_OVER_MODE	= 8,/*预扣费金额使用完毕*/
	REACH_TOTAL_VOL_TARGET		= 9,/*达到设定总电压*/
	REACH_SINGLE_VOL_TARGET		= 10,/*达到单体电压设定*/
	FAULT_STOP_MODE 			= 0xFF/*故障停止*/
};

typedef enum
{
	STA_UNKNOW          = 0,
    STA_IDLE 			= 1,/*空闲*/
    STA_PLUGING			= 2,/*插枪*/
    STA_START_CHARGE	= 3,/*充电启动中*/
    STA_CHARGING		= 4,/*充电中*/
    STA_CHARG_FINISHED	= 5,/*充电完成*/
	STA_BOOK			= 6,/*预约中*/
	STA_UNAVAILABLE		= 7,/*枪不可用*/
    STA_FAULT 			= 0xFF/*故障*/
}Charge_Sta_t;  		//枪状态
#define STA_MAX			9

typedef enum
{
	STA_NORMAL  	= 0,/*普通状态*/
	STA_UPDATE  	= 1,/*升级状态*/
	STA_UPLOAD_LOG  = 2,/*日志拉取中*/
}Charge_Special_t;  	//枪特殊状态

typedef enum
{
	UNPLUGGED  = 0,			/*未插枪*/
	PLUGGED_IN = 1,			/*插枪*/
	CP_DUTY_ON = 2,			/*cp输出*/
	EV_READYTOCHARGE = 3	/*车端充电准备完成*/
}CP_STATE_t;

typedef struct
{
	void (*GUNAChargeProcess)(void);
	void (*GUNBChargeProcess)(void);
}g_psCharge_Process_t;

extern BOOL g_bStart_charge_flag[2];

BOOL GunIdValidCheck(U8 ucGun_id);
BOOL CheckEVStop(const U8 ucGun_id);
void Charge_Process_Init_Task(void * pvParameters);
void GunStatusChange(U8 ucGun_id, Charge_Sta_t ucGun_status, BOOL bUnavailable_restore_flag, BOOL bBook_restore_flag);

#endif /* APP_CHARGE_PROCESS_CHARGE_PROCESS_IF_H_ */
