/*
 * JWT_can_comm.h
 *
 *  Created on: 2025年4月15日
 *      Author: qjwu
 */

#ifndef APP_SECC_JWT_CAN_COMM_H_
#define APP_SECC_JWT_CAN_COMM_H_

#define CCUStop_Normal						100
#define Vehicle_RemainTimeis_0_Normal		102
#define Vehicle_CharingDisabled_Normal		103
#define Vehicle_ReqChangeto_0_Normal		104
#define Vehicle_BeforeCharge_Normal 		105
#define CCS2_Trip_EVComplete_StopReq		107
#define CCS2_Trip_SOCFull_StopReq			108

#pragma pack(1)

typedef struct
{
	U8 reserve1;
	U8 reserve2;
	U8 reserve3;
	U8 reserve4;
	U16 unZclErrorCode;
	U8 ucErrorCode;
	U8 ErrorCode_By_CP:3;
	U8 EVErrorCode_By_EV:4;
	U8 reserve5:1;
}Stop_Data_t;

typedef struct
{
	Stop_Data_t sBST;
	Stop_Data_t sBEM;
}JWT_Stop_Data_t;

#pragma pack()

extern JWT_Stop_Data_t g_sJwt_stop_data[2];

void JWTSeccInit(void);
g_psCharge_Control_t* GetJwtSeccModel(void);
void ZCLErrorCodeParseFunc(const U16 unError_code, const U8 ucGun_id);

#endif /* APP_SECC_JWT_CAN_COMM_H_ */
