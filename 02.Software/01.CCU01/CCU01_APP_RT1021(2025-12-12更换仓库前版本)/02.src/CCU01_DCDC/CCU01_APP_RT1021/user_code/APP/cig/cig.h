#ifndef CIG_UPGRADE_H_
#define CIG_UPGRADE_H_

#include "cig_IF.h"

#define CIG_SEND_TIMEOUT   	 	500U
#define CIG_WAIT_TIMEOUT    	20U

#define CRC_LEN					2U
#define CRC_OFFSET           	6U
//CIG版本/sn信息
#define CIG_VERSION_DATA		69U
//CIG状态信息
#define CIG_STATUS_DATA			17U
//CIG控制响应信息
#define CIG_RESPONSE_DATA		8U

#define READ_CMD				0x03U
#define WRITE_CMD				0x06U
#define SLAVE_ADDR				0x01U

//控制状态获取
#define CIG_STATUS_START_REG	0U
#define	CIG_STATUS_READ_REG_NUM	6U
//继电器控制
#define CIG_CONTROL_START_REG	1U
//CIG板信息获取
#define CIG_MSG_START_REG		20U
#define CIG_MSG_READ_REG_NUM	32U
//CIG复位
#define CIG_RESET_CONTROL		52U
#define CIG_RESET_CMD			0x01U

#define CC1_CHECK_TIMEOUT_CNT 	3 //5

enum
{
	IDLE_STATUS 		= 0,
	GUN_PRESS_DOWN 		= 1,
	GUN_LOCK_RELEASE 	= 2
};

typedef enum
{
    CC1_DISCONNECT,     	//6V
    CC1_NEW_CONNECT,    	//6V-->12V-->6V-->4V
    CC1_ALREADY_CONNECT,	//4V
    CC1_NEW_DISCONNECT,   	//4V-->6V
    CC1_WAIT_CONNECT,    	//12V
}CC1_State_e;

#pragma pack(1)
typedef struct
{
	U8 ucSlave_addr;	//从机地址
	U8 ucCmd;			//cmd
	U8 ucData_len;		//数据长度
	U16 unDI_status;
	U16 unDO_status;
	U16 unGunA_cc1;
	U16 unGunB_cc1;
	U16 unPcb_temp1;
	U16 unPcb_temp2;
	U16 unCrc;			//crc校验
}Cig_Status_t;

typedef struct
{
	U8 ucSlave_addr;	//从机地址
	U8 ucCmd;			//cmd
	U8 ucData_len;		//数据长度
	U16 unSoft_version[8];
	U16 unHardware[8];
	U16 unSN[16];
	U16 unCrc;			//crc校验
}Cig_Msg_t;

typedef struct
{
	U8 ucSlave_addr;	//从机地址
	U8 ucCmd;			//cmd
	U8 ucAddr_H;		//寄存器地址高字节
	U8 ucAddr_L;		//寄存器地址低字节
	U8 ucData_H;		//数据高字节
	U8 ucData_L;		//数据低字节
	U16 unCrc;			//crc校验
}Cig_Control_Res_t;

typedef struct
{
    BOOL  bEnableSecondUart;
    U8    ucUart;
    U32   uiNowTickCig;
}CigUartCtx_t;

#pragma pack()

#endif
