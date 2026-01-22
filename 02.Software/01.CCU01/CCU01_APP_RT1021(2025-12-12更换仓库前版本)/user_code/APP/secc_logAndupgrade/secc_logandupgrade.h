#ifndef SECC_LOGANDUPGRADE_H_
#define SECC_LOGANDUPGRADE_H_

#include "semphr.h"
#include "fsl_flexcan.h"

#define CCU_TO_SECC_Cmd_ID    0x1CECF456U
#define SECC_TO_CCU_Cmd_ID    0x1CEC56F4U
#define CCU_TO_SECC_Pack_ID   0x1CEBF456U
#define SECC_TO_CCU_Pack_ID   0x1CEB56F4U

#define WAITING		2U
#define SECC_PACKET_WAITTIME  2300U

#define CAN_DATA_SIZE 8U
#define CAN_BUFFER_DEPTH 8U

typedef void (*SuspendSeccLogAndUpgradeFuncPtr)(void);
typedef void (*ResumeSeccLogAndUpgradeFuncPtr)(void);

typedef struct
{
    U32 id;
    U8 data[CAN_DATA_SIZE];
    U8 length;
} CANMessage_t;

typedef struct
{
    CANMessage_t buffer[CAN_BUFFER_DEPTH];
    U16 writeIndex;
    U16 readIndex;
    U8 count;
} CANBuffer_t;

typedef enum
{
	CCU_UNKNOW_CMD        = 0x00,
	CCU_HANDSHAKE_REQ     = 0x01,
	SECC_HANDSHAKE_ACK    = 0x02,
	CCU_DATA_REQ		  = 0x03,
	SECC_DATA_ACK		  = 0x04,
	SECC_HANDSHAKE_REQ	  = 0x05,
	CCU_HANDSHAKE_ACK	  = 0x06,
	SECC_DATA_REQ         = 0x07,
	CCU_DATA_ACK          = 0x08,

}SECC_PacketStepTypeDef;

typedef enum
{
	Read_Log_Unknow 		= 0x00,
	Read_Log_Start_Ready    = 0x01,
	Read_Log_Start      	= 0x02,
	Read_Log_Start_Ack  	= 0x03,
	Read_Log_Data_Ready 	= 0x04,
	Read_Log_Data       	= 0x05,
	Read_Log_Data_Ack   	= 0x06,
	Read_Log_Stop       	= 0x07,
	Read_Log_Stop_Ack   	= 0x08,
} CCU_READ_LOG_CMD;

typedef enum
{
    SECC_UPGRADE_UNKNOW_CMD     = 0x0000,	//未定义命令
    SECC_UPGRADE_REQUEST_CMD    = 0x0001,	//升级请求
    SECC_UPGRADE_READY_CMD		= 0x0002,	//升级准备
    SECC_UPGRADE_BEGIN_CMD      = 0x0003,	//起始包
    SECC_UPGRADE_MIDDLE_CMD     = 0x0004,	//中间包
    SECC_UPGRADE_END_CMD        = 0x0005,	//结束包
    SECC_UPGRADE_CHECKSUM_CMD   = 0x0006,	//固件文件校验码包

	SECC_UPGRADE_SEQUENCE_ERROR	= 0x1001,	//未按顺序发送报文
	SECC_UPGRADE_UNDEFINED_CMD	= 0x1002,	//未定义的命令
}SECC_UPGRADE_STEP_CMD;

typedef enum
{
	SECC_READLOG_UNKNOW_CMD  = 0x0000,	//未定义命令
	SECC_READLOG_REQUEST_CMD = 0x0030,	//日志请求
	SECC_READLOG_ACK_CMD     = 0x8015,	//日志响应
}SECC_READLOG_STEP_CMD;
#pragma pack(1)
typedef struct
{
	U32 LogLength;
	U32 appCodeStartAddr;
	U32 appFileLength;
	U32 SeccProgramIndex;
	U32 NorFlashProgramIndex;
	U32 appProgramIndex;
	U32 crc32;

	U16 LogSize;
	U16 ReadPageIndex;
	U16 LogPageNum;
	U16 Reserve;

	U8 rec[8];
	U8 SECC_DataRec[300];

	U8 SECC_DataAck_MsgLen_L;
	U8 SECC_DataAck_MsgLen_H;
	U8 SECC_DataAck_PackNum;
	U8 CCU_DataReq_Packnum;

	U8 RecPackIndex;
	U8 Start_Ack_SizeL;
	U8 Start_Ack_SizeH;
	U8 CanRecFlag;

	U8 Start_Ack_LogLength_Byte1;//低字节在前
	U8 Start_Ack_LogLength_Byte2;
	U8 Start_Ack_LogLength_Byte3;
	U8 Start_Ack_LogLength_Byte4;

	U8 appCodeStartAddr_Byte1;
	U8 appCodeStartAddr_Byte2;
	U8 appCodeStartAddr_Byte3;
	U8 appCodeStartAddr_Byte4;

	U8 appCodeSize_Byte1;
	U8 appCodeSize_Byte2;
	U8 appCodeSize_Byte3;
	U8 appCodeSize_Byte4;

	U8 TCP_SeccUpgrade;
	U8 PacketTimeout;
	U8 MessageId[2];

	U8 SECC_Cmd;

} SECC_LogAndUpGrade_t;
#pragma pack()
typedef struct
{
	SemaphoreHandle_t ReqBinarySemaphore;
	SemaphoreHandle_t SeccUpgrade_Binary_Semaphore;
	SemaphoreHandle_t SeccUpgradeSend_Binary_Semaphore;
	SemaphoreHandle_t SeccReadLog_Binary_Semaphore;

} SECC_BinarySemaphore_t;

typedef enum
{
	Upgrade_Unknow 	              = 0x00,
	Upgrade_Req_Ready             = 0x01,
	Upgrade_Req                   = 0x02,
	Upgrade_Req_Ack               = 0x03,
	Upgrade_Config_Req_Ready       = 0x04,
	Upgrade_Config_Req             = 0x05,
	Upgrade_Config_Ack             = 0x06,
	Upgrade_FileInfo_Req_Ready    = 0x07,
	Upgrade_FileInfo_Req          = 0x08,
	Upgrade_FileInfo_Ack          = 0x09,
	Upgrade_Program_Req_Ready     = 0x0A,
	Upgrade_Program_Req           = 0x0B,
	Upgrade_Program_Ack           = 0x0C,
	Upgrade_Verify_Req_Ready      = 0x0D,
	Upgrade_Verify_Req            = 0x0E,
	Upgrade_Verify_Ack            = 0x0F,
	Upgrade_Reboot_Req_Ready      = 0x10,
	Upgrade_Reboot_Req            = 0x11,
	Upgrade_Reboot_Ack            = 0x12,

} SECC_UpgradeCmd_t;

typedef enum
{
	NO_READLOG = 0,
	READLOG,
}SECC_ReagLog_t;

//typedef struct
//{
//	U8 version[6];//low-high
//
//} SECC_Version_t;

typedef struct
{
	U8 GunId;
	CAN_Type *CanBase;
	U8 (*UpgradeReq)(U8 GUN_ID,CAN_Type *base);
	U8 (*UpgradeConfig)(U8 GUN_ID,CAN_Type *base);
	U8 (*UpgradeFileInfo)(U8 GUN_ID,CAN_Type *base);
	U8 (*UpgradeProgram)(U8 GUN_ID,CAN_Type *base);
	U8 (*UpgradeVerify)(U8 GUN_ID,CAN_Type *base);
	U8 (*UpgradeReboot)(U8 GUN_ID,CAN_Type *base);
}SECC_UpgradeCmdFun_t;

typedef struct
{
	U32 productld;
	U32 productSerial;
	U32 hardwareVersion;
	U32 upgradeTime;
	U32 appCodeStartAddr;//app start addr and program start addr
	U32 appCodeSize;     //app length
	U32 crc32;
	U8 reserved[32];
	U32 firmwareHeaderChecksum;

}FirmwareHeader_t;

extern SECC_LogAndUpGrade_t g_Secc_LogAndUpgrade[GUN_MAX_NUM];
extern CCU_READ_LOG_CMD    SECC_ReadLog_Cmd[GUN_MAX_NUM];
extern SECC_BinarySemaphore_t SECC_BinarySemaphore[GUN_MAX_NUM];
extern UpdateTypeDef g_SeccUpgrade_CommBuf[GUN_MAX_NUM];
extern UPGRADE_TYPE g_Secc_sam_upgrade_state;
extern TaskHandle_t SeccA_LogAndUpgradeTask_Handler;
extern TaskHandle_t SeccB_LogAndUpgradeTask_Handler;
extern U8 SendMessageIdAndGunId[2][3];
extern CANBuffer_t CanBufferGun[2];

static U8 ReadSeccChecksum(const U8 *buf,U32 len);
static U8 SeccPacketAlternatelyProcess(const U8 * ReadLogReq,U8 GUN_ID,CAN_Type *base);
static void TCP_LogAndUpgradeReply(const U8 *SeccData,U16 index,U8 cmd,U16 messageid,U8 GUN_ID,U16 len);
static U8 SeccUpgradeReq(U8 GUN_ID,CAN_Type *base);
static U8 SeccUpgradeConfig(U8 GUN_ID,CAN_Type *base);
static U8 SeccUpgradeFileInfo(U8 GUN_ID,CAN_Type *base);
static U8 SeccUpgradeProgram(U8 GUN_ID,CAN_Type *base);
static U8 SeccUpgradeVerify(U8 GUN_ID,CAN_Type *base);
static U8 SeccUpgradeReboot(U8 GUN_ID,CAN_Type *base);
static void SuspendSeccALogAndUpgradeTaskFunc(void);
static void SuspendSeccBLogAndUpgradeTaskFunc(void);
static void ResumeSeccBLogAndUpgradeTaskFunc(void);
static void ResumeSeccALogAndUpgradeTaskFunc(void);

void CANBuffer_Init(CANBuffer_t *buffer);
BaseType_t CANCache_Write(CANBuffer_t *buffer, const U8 *data, U32 id, U8 length);

BOOL blSeccCheckUpgradeMsg(U8 GUN_ID);
void SeccUpgrade_Task(void * pvParameters);
BOOL blSeccCheckReadLogMsg(U8 GUN_ID);
void SeccA_LogAndUpgrade_Task(void * pvParameters);
void SeccB_LogAndUpgrade_Task(void * pvParameters);

#endif
