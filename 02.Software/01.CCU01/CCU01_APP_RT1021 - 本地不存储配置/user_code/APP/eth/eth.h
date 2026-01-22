/*
 * eth.h
 *
 *  Created on: 2024年11月2日
 *      Author: Bono
 */

#ifndef APP_ETH_ETH_H_
#define APP_ETH_ETH_H_

#include "PublicDefine.h"
#include "lwip/opt.h"

#define MAX_NAME_SIZE			512

#define ETH_REC_RINGBUF_SIZE   	1024U

#define GREETING           "this is test msg\n"

typedef struct
{
    int length;
    char bytes[MAX_NAME_SIZE];
    int done;
} recev_packet;

typedef struct
{
	U16 u16ReachPos;
	U16 u16DealPos;
	U16 u16DealCount;
	U16 u16ReadMsgTimeOutCount;
	U16 u16ReadMsgDataNum;
	U16 u16Datacount;

	U8 u8DealData[ETH_REC_RINGBUF_SIZE];
	U8 u8AnalysisData[MAX_NAME_SIZE];

	U8 u8ReadMsgBeginFlag;
	U8 reserve[3];

}RecBufferTypeDef;

extern TaskHandle_t tcp_client_Task_Handler;

extern void tcp_client_task(void *arg);
extern void user_eth_init(void);

#endif /* APP_ETH_ETH_H_ */
