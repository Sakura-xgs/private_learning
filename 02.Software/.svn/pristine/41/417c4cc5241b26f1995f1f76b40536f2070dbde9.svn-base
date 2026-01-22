/*
 * pos_IF.h
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */

#ifndef APP_POS_POS_IF_H_
#define APP_POS_POS_IF_H_

#include "PublicDefine.h"
#include "eth.h"

#define BF_POS_MODULE_FLAG   	1
#define	ST_POS_MODULE_FLAG   	2
#define NAYAX_POS_MODULE_FALG 	3

#define POS_COM_TIMEOUT      7000

typedef enum
{
	UNKNOW_COMMAND 			= 0,
	TRANSACTIONSTART		= 1,
	TRANSACTIONCOMPLETION	= 2,
	TRANSACTIONREVERSAL		= 3,
	TRANSACTIONINCREMENTAL	= 4,
	GETCARD_INFO			= 5,
	GETTERMINALINFO			= 6,
	GETCARDSTATUS			= 7
} POS_Command_t;

typedef struct
{
	BOOL bHmi_TransactionStart_flag;//开始交易，需要预扣费金额和生成订单流水号
	BOOL bHmi_TransactionCompletion_flag;//完成交易，需要扣费金额，originalHostReferenceNumber和originalHostData
	BOOL bHmi_TransactionReversal_flag;//取消交易，需要originalHostReferenceNumber和originalHostData
	BOOL bHmi_Abort_flag;//中断命令执行，无参数

	BOOL bHmi_GetCardInfo_flag;//查询卡命令，无参数。POS机也有GetCardInfo命令，但是GetCardInfo需要输入预扣费金额，测试后再决定哪个更合适
	BOOL swipeStartFlag;//刷卡中标志,POS启动充电，防止正在刷卡，HMI又发了启动充电
	BOOL netWorkUnavailable;//POS网络不可用
	BOOL serviceBusy;//POS服务繁忙,当前交易还未完成，不进行新的交易，等一段时间重新发送

	BOOL serviceBusyFlag;//服务繁忙标志，开始交易时候判断该标志，服务繁忙时不启动充电
	char serviceBusyCount;//POS服务繁忙请求次数
	char swipeCount;
	char serviceBusyResendTime;//服务繁忙后再次请求的时间间隔


	char frozenAmount[12];// n...9，0到9位预扣费金额
	char transactionAmount[12];//给POS的交易金额，n...9，0到9位，实际金额用这个参数
	char approvedAmount[12];//n...9，给POS的扣费金额，实际通过的金额
	char registerReferenceNumber[36];//ans...32，给POS机发送的交易流水号，由充电桩生成。

	char originalHostReferenceNumber[36];//ans...32，完成交易时发给POS，来自开始交易时POS机响应的后台流水号，完成或取消交易时要使用
	char originalHostData[260];//ans...256，完成交易时发给POS，来自开始交易时POS机响应的后台数据，结束或取消交易时要使用

	char hostReferenceNumber[36];//ans...32，交易时POS机响应的后台流水号，转存到originalHostReferenceNumber，完成或取消交易时要使用
	char hostData[260];//ans...256，交易时POS机响应的后台数据，转存到originalHostData，结束或取消交易时要使用

	char timestamp[16];//n14,POS响应的交易时间戳，开始交易和完成交易都有
	char cardUniqueIdentifier[68];//ans...64,POS机响应的卡片唯一标识，发送开始交易和查询卡时会返回该值
	char account[20];//ans...19,卡号
	char responseCode[8];//n6,POS机响应码，固定6位。
	char responseMessage[68];//ans...64,POS机响应消息
	char appVersion[32];//ans...32,POS机响应的应用版本号
	char appUpdateTimeStamp[16];//n14,POS响应的应用最后更新的时间戳，如果没更新过，该字段没有

	POS_Command_t POS_Command;

} POS_Transaction_t;

typedef struct
{
	S32 OccupationFee;//占位费
	S32 ChargePrice;//充电费
	S32 AmountSum;//占位费，充电费的和
	S32 PaymentFlag;//掉电保存付款标志

	U8 originalHostData[260];//ans...256，完成交易时发给POS，来自开始交易时POS机响应的后台数据，结束或取消交易时要使用
	U8 originalHostReferenceNumber[36];//ans...32，完成交易时发给POS，来自开始交易时POS机响应的后台流水号，完成或取消交易时要使用
	U8 account[20];//ans...19,卡号
	U8 transactionAmount[12];//给POS的交易金额，n...9，0到9位，实际金额用这个参数

	U8 SendCompletionCount;
	U8 SendCompletionTimeOut;

}POS_ShutDownTransaction_t;

typedef struct
{
	BOOL bEnable_heart_flag;//刷卡的时候等待时间比较久，刷卡期间不判断心跳的标志
	POS_Transaction_t POS_Transaction[2];
	U8 GunId;
	void (*SendPosHeartRequest)(void);
	void (*PosDataParse)(const U8 *buf, U16 data_len);
	BOOL (*Reboot)(void);
}POS_data_t;

extern POS_data_t *g_psPOS_data;
extern RecBufferTypeDef POS_UartRecBuffer;
extern size_t cJSON_MemOffset;

void Pos_Init_Task(void * pvParameters);
void GenerateRandomNumber(U8 *RandomString);
void SaveTransactionData(U8 GunId);
void ClearTransactionData(U8 GunId);
BOOL PositiveIntToString(U16 SigId1,U16 SigId2,U8* OutStr,S32* AmountSum);

#endif /* APP_POS_POS_IF_H_ */
