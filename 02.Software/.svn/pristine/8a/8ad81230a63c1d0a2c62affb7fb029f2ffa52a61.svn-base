/*
 * BF_pos.h
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */

#ifndef APP_POS_BF_POS_H_
#define APP_POS_BF_POS_H_

#include "pos_IF.h"

#define ACK 0x06U
#define NAK 0x15U
#define EOT 0x04U
#define POS_STRING_MEXLEN 400U

typedef enum
{
	POS_STATE_IDLE		= 0,
	POS_STATE_TX		= 1,
	POS_STATE_TX_END	= 2,
	POS_STATE_RX		= 3,
	POS_STATE_BUSY		= 4,
	POS_CREATE_ERR		= 5
}POS_HostState_t;

typedef struct
{
	U32 Jsonlen;
	U8 JsonString[POS_STRING_MEXLEN];
	U8 JsonConvertString[POS_STRING_MEXLEN - 17];

	U8 RxTimeOut;//接收时的超时计数
	U8 ErrTimes;//接收失败次数计数
	U8 FixedTimeGetTerminalInfo;//接收失败次数计数
	U8 RequestCount;//POS机恢复NAK，CCU重发请求次数计数
	U8 ResponseTimeOut;//响应超时时间。推荐120s
	U8 EotTimeOut;//EOT超时时间，1s

}POS_JsonSendBuf_t;

typedef struct
{
	U16 u16ReachPos;
	U16 u16DealPos;
	U16 u16DealCount;
	U16 u16ReadMsgTimeOutCount;
	U16 u16ReadMsgDataNum;
	U16 u16Datacount;

	U8 u8DealData[345];
	U8 u8AnalysisData[345];

	U8 u8ReadMsgBeginFlag;

}PosRecBufferTypeDef;


POS_data_t* GetBfPosModel(void);
void BfPosInit(void);
BOOL Pos_cjson_GetTerminalInfo(void);
static BOOL Pos_cjson_TransactionStart(const char *transactionAmount_a,const char *registerReferenceNumber_a);
BOOL Pos_cjson_TransactionCompletion(const char *transactionAmount_a,const char *originalHostReferenceNumber_a,const char *originalHostData_a);
static BOOL Pos_cjson_TransactionReversal(const char *originalHostReferenceNumber_a,const char *originalHostData_a);
static BOOL Pos_cjson_TransactionIncremental(const char *transactionAmount_a,const char *originalHostReferenceNumber_a,const char *originalHostData_a);
static BOOL Pos_cjson_Abort(void);
static BOOL GetCardInfo(void);
U16 DecimalFromString(const U8 *str);

void BfPosTransactionStart(U8 GUN_ID);
void BfPosTransactionCompletion(U8 GUN_ID);
void BfPosTransactionReversal(U8 GUN_ID);
void BfPosTransactionAbort(U8 GUN_ID);
void BfPosGetCardInfo(U8 GUN_ID);

extern POS_JsonSendBuf_t sPos_JsonSendbuf;
extern POS_HostState_t sPos_HostState;


#endif /* APP_POS_BF_POS_H_ */
