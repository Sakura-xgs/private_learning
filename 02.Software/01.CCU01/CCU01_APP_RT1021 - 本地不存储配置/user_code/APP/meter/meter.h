/*
 * meter.h
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */

#ifndef APP_METER_METER_H_
#define APP_METER_METER_H_

#define CRC_OFFSET           6U
#define CRC_LEN              2U
//从机地址
#define METER_ADD_A          0x1U
#define METER_ADD_B          0x2U

#define METER_SEND_TIMEOUT   500U
#define METER_WAIT_TIMEOUT   20U

extern U32 g_uiNow_tick_meter[2];

float gHexToFloat(const S8 *cBuf_A, U8 ucArrIndex);

#endif /* APP_METER_METER_H_ */
