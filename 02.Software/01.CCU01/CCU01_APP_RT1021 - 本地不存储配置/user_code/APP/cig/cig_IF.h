#ifndef CIG_UPGRADE_IF_H_
#define CIG_UPGRADE_IF_H_

#define CIG_ADDR 00

//使能辅源
#define CLOSE_GUNA_12V_ASSIST	0x28U
#define CLOSE_GUNA_24V_ASSIST	0x88U
#define CLOSE_GUNB_12V_ASSIST	0x14U
#define CLOSE_GUNB_24V_ASSIST	0x44U
//闭合电子锁
#define CLOSE_GUNA_LOCK_REALY	0x2U
#define CLOSE_GUNB_LOCK_REALY	0x1U

//关闭辅源 打开电子锁
#define OPEN_GUNA_LOCK_RELAY_AND_12V_ASSIST		0xFFD5U
#define OPEN_GUNB_LOCK_RELAY_AND_12V_ASSIST		0xFFEAU

#define OPEN_GUNA_LOCK_RELAY_AND_24V_ASSIST		0xFF75U
#define OPEN_GUNB_LOCK_RELAY_AND_24V_ASSIST		0xFFBAU

enum
{
	CIG_CLOSE = 1,
	CIG_OPEN  = 2
};

enum
{
	CCS1_6V		= 0,
	CCS1_12V	= 1,
	CCS1_4V		= 2
};

typedef enum
{
	CIG_READ_STATUS		= 0,//获取状态数据
	CIG_READ_VERSION	= 1,//获取CIG版本
	CIG_REALY_CONTROL	= 2,//小板控制状态获取
	CIG_REALY_RESET		= 3
}CIG_Control_t;

typedef struct
{
	BOOL bSend_flag;
	U16 unControl_data;
	BOOL bReset_flag;
	BOOL bRead_version_flag;
}CIG_Write_Control_t;

typedef struct
{
	U8 TimeOut;
	BOOL gblCigUpgradeStatus;
	U8 gCigMessageId[2];

}CIG_Upgrade_t;

extern CIG_Write_Control_t g_sCig_write_control;
extern __BSS(SRAM_OC) CIG_Upgrade_t CIG_Upgrade;

void Cig_Init_Task(void * pvParameters);
void CigContrlDataInput(const U8 ucType, const U16 unInput_data);

#endif /* APP_RGB_PWM_RGB_PWM_IF_H_ */
