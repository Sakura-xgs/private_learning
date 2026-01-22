/*
 * HW_rfid.h
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */

#ifndef APP_RFID_HW_RFID_H_
#define APP_RFID_HW_RFID_H_

#include "rfid_IF.h"

//协议头尾标识
#define HW_HEAD_SIGN        	0x02U
#define HW_END_SIGN         	0x03U
//响应标志
#define HW_SUCCESS_STATUS   	0x00U
#define HW_FAIL_STATUS      	0x5AU
//控制命令
#define HW_VERSION_CMD      	0x01
#define HW_SEARCH_CMD       	0x10
#define HW_M1_READ_CMD      	0x1B
#define HW_M1_KEY_MODE_A    	0x00
#define HW_M1_KEY_MODE_B    	0x04
#define HW_M1_SECTOR        	0x01
#define HW_M1_ADDRESS       	0x02
//数据长度
#define HW_M1_READ_DATA_LEN 	0x09
//协议数据长度下标
#define DATA_LEN_INDEX_H    	3
#define DATA_LEN_INDEX_L    	4
//响应状态下标
#define SIGN_INDEX          	0
#define RESPONSE_STATUS_INDEX   2
#define CMD_INDEX           	1
#define VALID_DATA_OFFSET   	5U

#define RFID_SEND_TIMEOUT   	500U
#define RFID_WAIT_TIMEOUT   	20U

#define HW_FRAME_FORMAT_LENGTH_BYTES    (7U)

typedef enum
{
	Read_Unknow  = 0x00,
	ReadFirst 	 = 0x01,
	ReadSecond 	 = 0x02,
} HW_ReadStep_st;

RFID_data_t* GetHwRfidModel(void);
void HwRfidInit(void);

#endif /* APP_RFID_HW_RFID_H_ */
