/*
 * rfid_IF.h
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */

#ifndef APP_RFID_RFID_IF_H_
#define APP_RFID_RFID_IF_H_

#include "PublicDefine.h"
#include "semphr.h"

#define HW_RFID_MODULE_FLAG       1
#define M1_KEY_MODE_A             0
#define M1_KEY_MODE_B             4

typedef enum
{
	TIMER_IDLE_STATUS = 0,
	TIMER_OPEN_STATUS = 1,
	TIMER_CLOSE_STATUS,
}Timer_control_e;

typedef struct
{
	Timer_control_e eRfid_start_timing_flag; //记时开始标志  60s超时
	BOOL bRfid_search_flag;      //查询标志位
	BOOL bRfid_rec_flag;         //对应查询，接受到响应之后才再次发送
	U8 ucId_tag[20];
	U8 ucVersion[16];
	U8 ucPassword_A[6];
	U8 ucM1_key_mode_A;
	U8 ucM1_sector_A;
	U8 ucM1_block_A;
	U8 ucPassword_B[6];
	U8 ucM1_key_mode_B;
	U8 ucM1_sector_B;
	U8 ucM1_block_B;
	BOOL bGet_data_flag;
	void (*RfidDataParse)(const U8 *buf, U16 unData_len);
	void (*SendRfidSearchRequest)(void);
	void (*SendRfidHeartRequest)(void);
}RFID_data_t;

extern RFID_data_t *g_psRFID_data;

void Rfid_Init_Task(void * pvParameters);

#endif /* APP_RFID_RFID_IF_H_ */
