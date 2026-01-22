/*
 * pos.c
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "cr_section_macros.h"

#include "BF_pos.h"
#include "pos.h"
#include "hmi_IF.h"
#include "hal_uart_IF.h"
#include "tcp_client_IF.h"
#include "charge_process_IF.h"
#include "emergency_fault_IF.h"
#include "SignalManage.h"
#include "uart_comm.h"
#include "charge_comm_IF.h"

#include "cJSON.h"
#include "eth.h"
#include "hal_ext_rtc.h"
#include "hal_eeprom_IF.h"
#include "Signal.h"

static TickType_t g_iNow_tick = 0;
POS_data_t *g_psPOS_data = NULL;
static SemaphoreHandle_t POS_Rec_Binary_Semaphore = NULL;
__BSS(SRAM_OC) RecBufferTypeDef POS_UartRecBuffer;
__BSS(SRAM_OC) static POS_ShutDownTransaction_t POS_ShutDownTransaction[2];//掉电保存的订单

static U16 originalHostReferenceNumberAddr[GUNNUMBER] = {0x1C00,0x1C20};//单个分配长度32字节，实际最大也是32字节
static U16 originalHostDataAddr[GUNNUMBER] = {0x1C40,0x1D40};//单个分配长度256个字节，实际最大也是256字节
static U16 accountAddr[GUNNUMBER] = {0x1E40,0x1E58};//卡号单个分配长度24个字节，实际最大占19个字节

/**
 * @brief pos机兼容性函数，用于选择不同厂家的控制函数
 * @param
 */
static void PosModuleSelect(void)
{
	//获取POS型号
	S32 uiModule_flag = 0;

	(void)GetSigVal(CCU_SET_SIG_ID_POS_MODEL, &uiModule_flag);

	if (BF_POS_MODULE_FLAG == uiModule_flag)
	{
		g_psPOS_data = GetBfPosModel();
		my_printf(USER_INFO, "get pos config model: BF\n");
	}
	else
	{
		g_psPOS_data = GetBfPosModel();
		my_printf(USER_INFO, "use pos default config model: BF\n");
	}
}

/**
 * @brief POS通讯检测函数
 * @param
 */
static void PosComCheck(void)
{
	S32 iTmp_flag = 0;

	if ((xTaskGetTickCount() - g_iNow_tick) > DEVICE_TIMEOUT_MS)
	{
		if (g_psPOS_data->bEnable_heart_flag != FALSE)
		{
			(void)GetSigVal(ALARM_ID_POS_COMM_LOST_ALARM, &iTmp_flag);
			if (iTmp_flag != COMM_LOST)
			{
				//通讯超时
				PileAlarmSet(ALARM_ID_POS_COMM_LOST_ALARM);
				my_printf(USER_ERROR, "%s:%d trigger POS comm warning\n", __FILE__, __LINE__);
			}
		}
	}
	else
	{
		(void)GetSigVal(ALARM_ID_POS_COMM_LOST_ALARM, &iTmp_flag);
		if (iTmp_flag == COMM_LOST)
		{
			//恢复
			PileAlarmReset(ALARM_ID_POS_COMM_LOST_ALARM);
			my_printf(USER_ERROR, "%s:%d restore POS comm\n", __FILE__, __LINE__);
		}
	}
}

/**
 * 功能:把两个正数相加后转换成字符串
 * SigId1，SigId2 - 信号量的地址
 * OutStr-返回的相加后费用字符串
 * AmountSum - 返回和
 */
BOOL PositiveIntToString(U16 SigId1,U16 SigId2,U8* OutStr,S32* AmountSum)
{
	S32 Value1 = 0,Value2 = 0,ValueSum = 0;
	U32 stringlen = 0;
	U8 Buffer[8] = {0};

	if(SigId1 != 0U)
	{
		(void)GetSigVal(SigId1, &Value1);

	}
	if(SigId2 != 0U)
	{
		(void)GetSigVal(SigId2, &Value2);
	}

	my_printf(USER_INFO,"Value1:%d Value2:%d\n",Value1,Value2);

	ValueSum = Value1 + Value2;

	if(AmountSum != 0)
	{
		*AmountSum = ValueSum;
	}

	if(ValueSum == 0)
	{
		my_printf(USER_ERROR,"ValueSum:%d\n",ValueSum);
		return FALSE;
	}

	(void)sprintf(Buffer,"%d",ValueSum);//(void)snprintf(Buffer,sizeof(Buffer),"%d",ValueSum);

	stringlen = strlen(Buffer);
	my_printf(USER_INFO,"ValueSum:%s\n",Buffer);

	if((stringlen != 0U) && (stringlen <= 6U))
	{
		(void)memcpy(OutStr,Buffer,stringlen);
	}
	else
	{
		my_printf(USER_ERROR,"ValueSum:%d\n",ValueSum);
		return FALSE;
	}
	return TRUE;
}

/**
 * 功能:把交易信息存入eeprom
 * GunId - 枪号
 */
void SaveTransactionData(U8 GunId)
{
	U32 u32stringlen = 0;

	u32stringlen = strlen(g_psPOS_data->POS_Transaction[GunId].originalHostReferenceNumber) + 1;// +1是为了最后以为补0

	(void)EepromBufferWrite(g_psPOS_data->POS_Transaction[GunId].originalHostReferenceNumber,
			originalHostReferenceNumberAddr[GunId],u32stringlen);

	u32stringlen = strlen(g_psPOS_data->POS_Transaction[GunId].originalHostData) + 1;

	(void)EepromBufferWrite(g_psPOS_data->POS_Transaction[GunId].originalHostData,
			originalHostDataAddr[GunId],u32stringlen);

	u32stringlen = strlen(g_psPOS_data->POS_Transaction[GunId].account) + 1;

	(void)EepromBufferWrite(g_psPOS_data->POS_Transaction[GunId].account,
			accountAddr[GunId],u32stringlen);

}

/**
 * 功能:清除交易信息
 * GunId - 枪号
 */
void ClearTransactionData(U8 GunId)
{
	if(GUN_A == GunId)
	{
		(void)SetSigVal(CCU_SAM_SIG_ID_CHARGE_PRICE_A, 0);//充电费用
		(void)SetSigVal(CCU_SAM_SIG_ID_OCCUPATION_FEE_A, 0);//占位费用

		//还得清充电未结束标志
	}
	else if(GUN_B == GunId)
	{
		(void)SetSigVal(CCU_SAM_SIG_ID_CHARGE_PRICE_B, 0);
		(void)SetSigVal(CCU_SAM_SIG_ID_OCCUPATION_FEE_B, 0);

		//还得清充电未结束标志
	}
	else
	{
		my_printf(USER_INFO,"GunIdErr\n");
		return;
	}

	//清除账单数据
	(void)memset(&g_psPOS_data->POS_Transaction[GunId].originalHostReferenceNumber,0,sizeof(g_psPOS_data->POS_Transaction[GunId].originalHostReferenceNumber));
	(void)memset(&g_psPOS_data->POS_Transaction[GunId].originalHostData,0,sizeof(g_psPOS_data->POS_Transaction[GunId].originalHostData));
	(void)memset(&g_psPOS_data->POS_Transaction[GunId].account,0,sizeof(g_psPOS_data->POS_Transaction[GunId].account));
	(void)memset(&g_psPOS_data->POS_Transaction[GunId].transactionAmount,0,sizeof(g_psPOS_data->POS_Transaction[GunId].transactionAmount));
	(void)memset(&g_psPOS_data->POS_Transaction[GunId].approvedAmount,0,sizeof(g_psPOS_data->POS_Transaction[GunId].approvedAmount));

//	if(kStatus_Success != EepromBufferWrite(g_psPOS_data->POS_Transaction[GunId].originalHostReferenceNumber,originalHostReferenceNumberAddr[GunId],32)) // @suppress("No return")
//	{
//		my_printf(USER_INFO,"gun:%d HostReferenceNumber eeprom write fail\n",GunId);
//	}
//	if(kStatus_Fail == EepromBufferWrite(g_psPOS_data->POS_Transaction[GunId].originalHostData,originalHostDataAddr[GunId],256))
//	{
//		my_printf(USER_INFO,"gun:%d HostData eeprom write fail\n",GunId);
//	}
//	if(kStatus_Fail == EepromBufferWrite(g_psPOS_data->POS_Transaction[GunId].account, accountAddr[GunId],19))
//	{
//		my_printf(USER_INFO,"gun:%d account eeprom write fail\n",GunId);
//	}

	return;
}

/**
 * 功能:获取交易标志，根据交易标志读取响应枪号的交易信息，用于异常掉电账单处理
 *
 */
static BOOL GetPaymentFlagFromEeprom(U8 GunId)
{
	S32 AdvanceValue = 0;

	(void)EepromBufferRead(POS_ShutDownTransaction[GunId].originalHostReferenceNumber,
					originalHostReferenceNumberAddr[GunId],32);

	(void)EepromBufferRead(POS_ShutDownTransaction[GunId].originalHostData,
					originalHostDataAddr[GunId],256);

	(void)EepromBufferRead(POS_ShutDownTransaction[GunId].account,
					accountAddr[GunId],19);

	(void)GetSigVal(CCU_SET_SIG_ID_ADVANCE_CHARGE, &AdvanceValue);//预扣费金额

	if (GUN_A == GunId)
	{
		//获取存储在eeprom的占位费和充电费
		(void)GetSigVal(CCU_SET_SIG_ID_OCCUPATION_FEE_A, &POS_ShutDownTransaction[GUN_A].OccupationFee);
		(void)GetSigVal(CCU_SET_SIG_ID_CHARGE_PRICE_A, &POS_ShutDownTransaction[GUN_A].ChargePrice);

		my_printf(USER_INFO,"gunA:OccupationFee:%d,ChargePrice:%d",POS_ShutDownTransaction[GUN_A].OccupationFee,POS_ShutDownTransaction[GUN_A].ChargePrice);

		//金额没超出设置的预扣费金额
		if((POS_ShutDownTransaction[GUN_A].OccupationFee + POS_ShutDownTransaction[GUN_A].ChargePrice) <= AdvanceValue)
		{
			//把金额相加并转化为字符串
			(void)PositiveIntToString(CCU_SET_SIG_ID_OCCUPATION_FEE_A,
									  CCU_SET_SIG_ID_CHARGE_PRICE_A,
									  POS_ShutDownTransaction[GUN_A].transactionAmount,
									  &POS_ShutDownTransaction[GUN_A].AmountSum);
		}
		else
		{
			S32 PaymentFlag = 0;
			//掉电扣款标志
			(void)SetSigVal(CCU_SET_SIG_ID_PAYMENT_FLAG_A, PaymentFlag);
			my_printf(USER_ERROR, "%s:%d OccupationFee = %d + ChargePrice = %d > AdvanceValue = %d\n", __FILE__, __LINE__,
					POS_ShutDownTransaction[GUN_A].OccupationFee, POS_ShutDownTransaction[GUN_A].ChargePrice, AdvanceValue);

			return FALSE;
		}
	}
	else if (GUN_B == GunId)
	{
		//获取存储在eeprom的占位费和充电费
		(void)GetSigVal(CCU_SET_SIG_ID_OCCUPATION_FEE_B, &POS_ShutDownTransaction[GUN_B].OccupationFee);
		(void)GetSigVal(CCU_SET_SIG_ID_CHARGE_PRICE_B, &POS_ShutDownTransaction[GUN_B].ChargePrice);

		my_printf(USER_INFO,"gunB:OccupationFee:%d,ChargePrice:%d",POS_ShutDownTransaction[GUN_B].OccupationFee,POS_ShutDownTransaction[GUN_B].ChargePrice);

		if((POS_ShutDownTransaction[GUN_B].OccupationFee + POS_ShutDownTransaction[GUN_B].ChargePrice) <= AdvanceValue)
		{
			//把金额相加并转化为字符串
			(void)PositiveIntToString(CCU_SET_SIG_ID_OCCUPATION_FEE_B,
									  CCU_SET_SIG_ID_CHARGE_PRICE_B,
									  POS_ShutDownTransaction[GUN_B].transactionAmount,
									  &POS_ShutDownTransaction[GUN_B].AmountSum);
		}
		else
		{
			S32 PaymentFlag = 0;
			//掉电扣款标志
			(void)SetSigVal(CCU_SET_SIG_ID_PAYMENT_FLAG_B, PaymentFlag);
			my_printf(USER_ERROR, "%s:%d OccupationFee = %d + ChargePrice = %d > AdvanceValue = %d\n", __FILE__, __LINE__,
					POS_ShutDownTransaction[GUN_B].OccupationFee, POS_ShutDownTransaction[GUN_B].ChargePrice, AdvanceValue);

			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}

	if((0U == strlen(POS_ShutDownTransaction[GunId].originalHostReferenceNumber)) &&
	   (0U == strlen(POS_ShutDownTransaction[GunId].originalHostData)) &&
	   (0U == strlen(POS_ShutDownTransaction[GunId].account)))
	{
		return FALSE;
	}

	return TRUE;
}

/**
 * 功能:用pos启动充电的时候产生一个0-9字符型的账单流水号
 * RandomString - 用于接收账单流水号
 */
void GenerateRandomNumber(U8 *RandomString)
{
	U8 u8RandomNumStr[11] = {0};
	U32 year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;

	//用日期时间当账单流水号
	SYS_GetDate(&year, &month, &day, &hour, &minute, &second);

	(void)sprintf(&u8RandomNumStr[0],"%02d",month);
	(void)sprintf(&u8RandomNumStr[2],"%02d",day);
	(void)sprintf(&u8RandomNumStr[4],"%02d",hour);
	(void)sprintf(&u8RandomNumStr[6],"%02d",minute);
	(void)sprintf(&u8RandomNumStr[8],"%02d",second);

	(void)memcpy(RandomString,u8RandomNumStr,11);

//下面是用随机数当账单流水号
//	srand(CCU_SysTimestamp);//时间戳当随机数种子
//
//	if(NULL != RandomString)
//	{
//		for (U32 i = 0; i < 10U; i++)
//		{
//			U32 RandomDigit = rand() % 10;//生成一个0到9之间的随机数
//
//			u8RandomNumStr[i] = '0' + RandomDigit;//将随机数转换为字符
//		}
//		u8RandomNumStr[10] = '\0';//添加字符串结束符
//
//		(void)memcpy(RandomString,u8RandomNumStr,11);
//	}
}

/**
 * @brief POS控制命令
 * @param
 */
static BOOL PosCmdControl(U8 ucGun_id)
{
	if (FALSE == GunIdValidCheck(ucGun_id))
	{
		return FALSE;
	}

	if(g_psPOS_data->POS_Transaction[ucGun_id].bHmi_TransactionStart_flag == TRUE)
	{
		g_psPOS_data->bEnable_heart_flag = FALSE;
		BfPosTransactionStart(ucGun_id);
		return FALSE;
	}
	else if(g_psPOS_data->POS_Transaction[ucGun_id].bHmi_TransactionCompletion_flag == TRUE)
	{
		g_psPOS_data->bEnable_heart_flag = FALSE;
		BfPosTransactionCompletion(ucGun_id);
		return FALSE;
	}
	else if(g_psPOS_data->POS_Transaction[ucGun_id].bHmi_TransactionReversal_flag == TRUE)
	{
		g_psPOS_data->bEnable_heart_flag = FALSE;
		BfPosTransactionReversal(ucGun_id);
		return FALSE;
	}
	else if(g_psPOS_data->POS_Transaction[ucGun_id].bHmi_GetCardInfo_flag == TRUE)
	{
		g_psPOS_data->bEnable_heart_flag = FALSE;
		BfPosGetCardInfo(ucGun_id);
		return FALSE;
	}
	else if(g_psPOS_data->POS_Transaction[ucGun_id].bHmi_Abort_flag == TRUE)
	{

		g_psPOS_data->bEnable_heart_flag = FALSE;
		BfPosTransactionAbort(ucGun_id);
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

void SwipeStartFlagTimeout(void)
{
	g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].swipeCount++;

	if(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].swipeCount > 60)
	{
		g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].swipeStartFlag = FALSE;//清刷卡标志
	}

}

static void Pos_Request_Task(void *parameter)
{
	#ifdef FREE_STACK_SPACE_CHECK_ENABLE
	volatile UBaseType_t uxHighWaterMark;
	#endif
	BOOL blSendHeartFlag[2] = {FALSE, FALSE};

	vTaskDelay(3000);

	(void)GetSigVal(CCU_SET_SIG_ID_PAYMENT_FLAG_A, &POS_ShutDownTransaction[GUN_A].PaymentFlag);
	(void)GetSigVal(CCU_SET_SIG_ID_PAYMENT_FLAG_B, &POS_ShutDownTransaction[GUN_B].PaymentFlag);

	if(POS_ShutDownTransaction[GUN_A].PaymentFlag == 1)
	{
		my_printf(USER_INFO,"gunA:PaymentFlag=1\n");

		if(FALSE == GetPaymentFlagFromEeprom(GUN_A))
		{
			POS_ShutDownTransaction[GUN_A].PaymentFlag = 0;
			(void)SetSigVal(CCU_SET_SIG_ID_PAYMENT_FLAG_A, POS_ShutDownTransaction[GUN_A].PaymentFlag);
		}
	}
	if(POS_ShutDownTransaction[GUN_B].PaymentFlag == 1)
	{
		my_printf(USER_INFO,"gunB:PaymentFlag=1\n");

		if(FALSE == GetPaymentFlagFromEeprom(GUN_B))
		{
			POS_ShutDownTransaction[GUN_B].PaymentFlag = 0;
			(void)SetSigVal(CCU_SET_SIG_ID_PAYMENT_FLAG_B, POS_ShutDownTransaction[GUN_B].PaymentFlag);
		}
	}

	g_psPOS_data->bEnable_heart_flag = TRUE;

	//延时用于等待pos开机
	vTaskDelay(10000);
	vTaskDelay(10000);
	vTaskDelay(7000);

    // 获取当前的tick数
	g_iNow_tick = xTaskGetTickCount();
	//POS_ShutDownTransaction[GUN_B].PaymentFlag = 0;

	while(1)
	{
		PosComCheck();

		if (NULL != g_psPOS_data)
		{
			if((POS_ShutDownTransaction[GUN_A].PaymentFlag == 0) && (POS_ShutDownTransaction[GUN_B].PaymentFlag == 0))
			{
				blSendHeartFlag[GUN_A] = PosCmdControl(GUN_A);
				blSendHeartFlag[GUN_B] = PosCmdControl(GUN_B);
			}
			else//处理掉电前的订单
			{
				if(POS_ShutDownTransaction[GUN_A].PaymentFlag == 1)//一个一个处理
				{
					POS_ShutDownTransaction[GUN_A].SendCompletionCount++;

					if((POS_ShutDownTransaction[GUN_A].SendCompletionCount > 15U) && (sPos_HostState == POS_STATE_IDLE))
					{
						POS_ShutDownTransaction[GUN_A].SendCompletionCount = 0U;
						POS_ShutDownTransaction[GUN_A].SendCompletionTimeOut++;

						if(POS_ShutDownTransaction[GUN_A].SendCompletionTimeOut < 5U)
						{
							g_psPOS_data->GunId = GUN_A;//当前操作的枪号
							(void)Pos_cjson_TransactionCompletion(POS_ShutDownTransaction[GUN_A].transactionAmount,
															POS_ShutDownTransaction[GUN_A].originalHostReferenceNumber,
															POS_ShutDownTransaction[GUN_A].originalHostData);
						}
						else
						{
							POS_ShutDownTransaction[GUN_A].SendCompletionTimeOut = 0U;
							POS_ShutDownTransaction[GUN_A].PaymentFlag = 0;
							(void)SetSigVal(CCU_SET_SIG_ID_PAYMENT_FLAG_A, POS_ShutDownTransaction[GUN_A].PaymentFlag);
							my_printf(USER_ERROR, "gunA shut down transaction timeout:%s %s %s\n",
										POS_ShutDownTransaction[GUN_A].transactionAmount,
										POS_ShutDownTransaction[GUN_A].originalHostReferenceNumber,
										POS_ShutDownTransaction[GUN_A].originalHostData);
						}
					}
				}
				else
				{
					POS_ShutDownTransaction[GUN_B].SendCompletionCount++;

					if((POS_ShutDownTransaction[GUN_B].SendCompletionCount > 15U) && (sPos_HostState == POS_STATE_IDLE))
					{
						POS_ShutDownTransaction[GUN_B].SendCompletionCount = 0U;
						POS_ShutDownTransaction[GUN_B].SendCompletionTimeOut++;

						if(POS_ShutDownTransaction[GUN_B].SendCompletionTimeOut < 5U)
						{
							g_psPOS_data->GunId = GUN_B;//当前操作的枪号
							(void)Pos_cjson_TransactionCompletion(POS_ShutDownTransaction[GUN_B].transactionAmount,
															POS_ShutDownTransaction[GUN_B].originalHostReferenceNumber,
															POS_ShutDownTransaction[GUN_B].originalHostData);
						}
						else
						{
							POS_ShutDownTransaction[GUN_B].SendCompletionTimeOut = 0U;
							POS_ShutDownTransaction[GUN_B].PaymentFlag = 0;
							(void)SetSigVal(CCU_SET_SIG_ID_PAYMENT_FLAG_B, POS_ShutDownTransaction[GUN_B].PaymentFlag);
							my_printf(USER_ERROR, "gunB shut down transaction timeout:%s %s %s\n",
										POS_ShutDownTransaction[GUN_B].transactionAmount,
										POS_ShutDownTransaction[GUN_B].originalHostReferenceNumber,
										POS_ShutDownTransaction[GUN_B].originalHostData);
						}
					}
				}
			}

			if (blSendHeartFlag[GUN_A] && blSendHeartFlag[GUN_B])
			{
				g_psPOS_data->SendPosHeartRequest();
			}

			if(sPos_HostState == POS_STATE_TX)//用于发送，3秒收不到POS ACK回复，重发，重发3次
			{
				sPos_JsonSendbuf.RxTimeOut++;
				if(sPos_JsonSendbuf.RxTimeOut > 3U)
				{
					sPos_JsonSendbuf.RxTimeOut=0;

					sPos_JsonSendbuf.ErrTimes++;
					if(sPos_JsonSendbuf.ErrTimes < 3U)
					{
						POS_UartRecBuffer.u16DealPos=0;
						POS_UartRecBuffer.u16ReachPos=0;
						if (sizeof(sPos_JsonSendbuf.JsonString) >= (sPos_JsonSendbuf.Jsonlen + 19U))
						{
							Uart_Dma_Send(POS_UART, sPos_JsonSendbuf.Jsonlen + 19U , sPos_JsonSendbuf.JsonString);
						}
					}
					else
					{
						(void)memset(&sPos_JsonSendbuf,0,sizeof(sPos_JsonSendbuf));
						sPos_JsonSendbuf.ErrTimes=0;
						POS_UartRecBuffer.u16DealPos=0;
						POS_UartRecBuffer.u16ReachPos=0;
						sPos_JsonSendbuf.FixedTimeGetTerminalInfo=0;
						sPos_HostState=POS_STATE_IDLE;

						if(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].swipeStartFlag == TRUE)
						{
							g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].swipeStartFlag = FALSE;
						}
					}
				}
			}
			if(sPos_HostState == POS_STATE_TX_END)//等待响应报文,这个根据网络速度好坏，速度不同
			{
				sPos_JsonSendbuf.ResponseTimeOut++;
				if(sPos_JsonSendbuf.ResponseTimeOut > 30U)//30S，
				{
					POS_UartRecBuffer.u16DealPos=0;
					POS_UartRecBuffer.u16ReachPos=0;
					sPos_JsonSendbuf.ResponseTimeOut=0;
					sPos_JsonSendbuf.RequestCount = 0;
					sPos_HostState=POS_STATE_IDLE;

					if(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].swipeStartFlag == TRUE)
					{
						g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].swipeStartFlag = FALSE;
					}
				}
			}
			if(sPos_HostState == POS_STATE_RX)//发送ACK响应后等待EOT
			{
				sPos_JsonSendbuf.EotTimeOut++;
				if(sPos_JsonSendbuf.EotTimeOut > 2U)
				{
					POS_UartRecBuffer.u16DealPos=0;
					POS_UartRecBuffer.u16ReachPos=0;
					sPos_JsonSendbuf.EotTimeOut=0;
					sPos_HostState=POS_STATE_IDLE;

					if(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].swipeStartFlag == TRUE)
					{
						g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].swipeStartFlag = FALSE;
					}
				}
			}

			SwipeStartFlagTimeout();
		}
		else
		{
			vTaskDelay(2000);
			//printf("error: %s g_psPOS_data = NULL!\n", __FILE__);
		}

		vTaskDelay(1000);
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
		uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
#endif
	}
}

/**
 * @brief POS启动充电控制
 * @param
 * @return
 */
static void PosStartCharge(void)
{
	CHECK_PTR_NULL_NO_RETURN(g_psHMI_data);

	if ((g_psHMI_data->ucGun_id == g_sStorage_data.sPublic_data[GUN_A].ucGun_id)
			|| (g_psHMI_data->ucGun_id == g_sStorage_data.sPublic_data[GUN_B].ucGun_id))
	{
		U8 GunId = g_psHMI_data->ucGun_id - ((2U * g_sStorage_data.ucPile_num) - 1U);

		if (GunId < GUN_MAX_NUM)
		{
			(void)memcpy(g_sGun_data[GunId].sTemp.ucIdtag, g_psPOS_data->POS_Transaction[GunId].account, sizeof(g_psPOS_data->POS_Transaction[GunId].account));
			g_sGun_data[GunId].sTemp.ucStart_Charge_type = POS_START_MODE;
			g_sGun_id_status.ucStart_control_id = g_psHMI_data->ucGun_id;
			g_sGun_data[GunId].sTemp.ucAuth_type = START_CHARGE_AUTH;
			TcpSendControl(&g_sGun_id_status.bSend_auth_flag);

			my_printf(USER_INFO, "%s POS request start charge auth id = %s\n", (GunId==GUN_A)?"GUN_A":"GUN_B", g_sGun_data[GunId].sTemp.ucIdtag);
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d START: gun id error, ucPile_num = %d\n", __FILE__, __LINE__, g_sStorage_data.ucPile_num);
		}
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d START: HMI set gun id error = %d\n", __FILE__, __LINE__, g_psHMI_data->ucGun_id);
	}
}

/**
 * @brief POS结束充电控制
 * @param
 * @return
 */
static void PosStopCharge(void)
{
	CHECK_PTR_NULL_NO_RETURN(g_psHMI_data);

	if ((g_psHMI_data->ucGun_id == g_sStorage_data.sPublic_data[GUN_A].ucGun_id)
			|| (g_psHMI_data->ucGun_id == g_sStorage_data.sPublic_data[GUN_B].ucGun_id))
	{
		U8 GunId = g_psHMI_data->ucGun_id - ((2U * g_sStorage_data.ucPile_num) - 1U);

		if (GunId < GUN_MAX_NUM)
		{
			g_sGun_data[GunId].sTemp.ucStop_Charge_type = POS_STOP_MODE;
			g_sGun_id_status.ucFinish_control_id = g_psHMI_data->ucGun_id;
			g_sGun_data[GunId].sTemp.ucAuth_type = STOP_CHARGE_AUTH;

			//POS结束本地鉴权，不发送鉴权请求，鉴权通过则直接停止充电
			StopChargeControl(GunId);

			my_printf(USER_INFO, "%s POS request stop charge auth id = %s\n", (GunId==GUN_A)?"GUN_A":"GUN_B", g_sGun_data[GunId].sTemp.ucIdtag);
		}
		else
		{
			my_printf(USER_ERROR, "%s:%d STOP: gun id error, ucPile_num = %d\n", __FILE__, __LINE__, g_sStorage_data.ucPile_num);
		}
	}
	else
	{
		my_printf(USER_ERROR, "%s:%d STOP: HMI set gun id error = %d\n", __FILE__, __LINE__, g_psHMI_data->ucGun_id);
	}
}

static void POS_EventAnalysis(void)
{
	U16 u16RecByteNum=0;
	U8 u8DecimalStr[7] = "000000";//最后一位放了'\0'

	(void)memcpy(u8DecimalStr,&POS_UartRecBuffer.u8AnalysisData[6],6);//取出json体长度位
	u16RecByteNum = DecimalFromString(u8DecimalStr);//转换成10进制

	CHECK_PTR_NULL_NO_RETURN(g_psPOS_data);
	CHECK_PTR_NULL_NO_RETURN(g_psHMI_data);

	sPos_JsonSendbuf.FixedTimeGetTerminalInfo=0;//收到POS机数据就清零
	//解析
	g_psPOS_data->PosDataParse(POS_UartRecBuffer.u8AnalysisData, u16RecByteNum);

	if(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode != NULL)
	{
		g_iNow_tick = xTaskGetTickCount();
		switch(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].POS_Command)
		{
			case UNKNOW_COMMAND:

				break;

			case TRANSACTIONSTART://交易开始
				if(strncmp(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode, "000000", 6) == 0)//响应码OK
				{
					(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].originalHostReferenceNumber,
						   g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostReferenceNumber,
						   sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].originalHostReferenceNumber));

					(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].originalHostData,
						   g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostData,
						   sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].originalHostData));

					//预扣费完成，启动充电
					PosStartCharge();

					//打印通过的预扣费金额和卡号
					my_printf(USER_INFO, "gun%d:TransactionStart RX %s %s\n",
							g_psPOS_data->GunId,
							g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].frozenAmount,
							g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].account);
				}
				else
				{
					//给HMI鉴权失败信号
					g_psHMI_data->ucAuthorize_flag[g_psPOS_data->GunId] = AUTHORIZE_FAILED;
					//清刷卡标志
					g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].swipeStartFlag = FALSE;

					//响应码不为ok
					my_printf(USER_ERROR, "%s:%d gun%d:TransactionStart RX %s\n", __FILE__, __LINE__, g_psPOS_data->GunId,g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode);
				}

				break;

			case TRANSACTIONCOMPLETION://交易完成
				if(strncmp(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode, "000000", 6) == 0)//响应码OK
				{
					if((POS_ShutDownTransaction[GUN_A].PaymentFlag == 0) && (POS_ShutDownTransaction[GUN_B].PaymentFlag == 0))
					{
						//清除交易存储信息,在开始交易的时候也清了
						ClearTransactionData(g_psPOS_data->GunId);

						//打印通过的交易金额
						my_printf(USER_INFO, "gun%d:TransactionCompletion RX %s\n",
								g_psPOS_data->GunId,g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].approvedAmount);
					}
					else
					{
						//和掉电存储订单流水号比对，判断是否为掉电保存订单
						//交易流水号是根据10位日期生成的，POS会在10位流水号后面添加一些数据，但只比较10位就够了
						//if(strncmp(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostReferenceNumber,
						//		   POS_ShutDownTransaction[g_psPOS_data->GunId].originalHostReferenceNumber,10) == 0)
						if(g_psPOS_data->GunId == GUN_A)
						{
							POS_ShutDownTransaction[GUN_A].PaymentFlag = 0;
							(void)SetSigVal(CCU_SET_SIG_ID_PAYMENT_FLAG_A, POS_ShutDownTransaction[GUN_A].PaymentFlag);
						}
						if(g_psPOS_data->GunId == GUN_B)
						{
							POS_ShutDownTransaction[GUN_B].PaymentFlag = 0;
							(void)SetSigVal(CCU_SET_SIG_ID_PAYMENT_FLAG_B, POS_ShutDownTransaction[GUN_B].PaymentFlag);
						}

						//打印通过的交易金额
						my_printf(USER_INFO, "Gun%d:TransactionCompletion RX %s\n",
								g_psPOS_data->GunId,g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].approvedAmount);
					}
				}
				else
				{
					//考虑一种情况，如果对掉电保存的订单扣费失败了，要怎么处理
					if((POS_ShutDownTransaction[GUN_A].PaymentFlag == 1) || (POS_ShutDownTransaction[GUN_B].PaymentFlag == 1))
					{
						if(g_psPOS_data->GunId == GUN_A)
						{
							POS_ShutDownTransaction[GUN_A].PaymentFlag = 0;
							(void)SetSigVal(CCU_SET_SIG_ID_PAYMENT_FLAG_A, POS_ShutDownTransaction[GUN_A].PaymentFlag);
						}
						if(g_psPOS_data->GunId == GUN_B)
						{
							POS_ShutDownTransaction[GUN_B].PaymentFlag = 0;
							(void)SetSigVal(CCU_SET_SIG_ID_PAYMENT_FLAG_B, POS_ShutDownTransaction[GUN_B].PaymentFlag);
						}
					}

					if(strncmp(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode, "110112", 6) == 0)
					{
						//网络不可用
					}
					else if(strncmp(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode, "110223", 6) == 0)
					{
						//服务繁忙，过一段时间重发，重发3次
						g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].serviceBusy = TRUE;
						g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].serviceBusyFlag = TRUE;
						g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].serviceBusyCount = 0U;
					}

					//响应码不为OK
					my_printf(USER_ERROR, "%s:%d gun%d:TransactionCompletion RX %s %s\n", __FILE__, __LINE__,
												g_psPOS_data->GunId,
												g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode,
												g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseMessage);
				}

				g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].swipeStartFlag = FALSE;//刷卡结束清标志

				break;

			case TRANSACTIONREVERSAL://交易取消
				if(strncmp(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode, "000000", 6) == 0)//响应码OK
				{
					//清除交易存储信息,在开始交易的时候清了
					//ClearTransactionData(g_psPOS_data->GunId);

					my_printf(USER_INFO, "Gun%d:TransactionReversal RX\n",g_psPOS_data->GunId);
				}
				else
				{
					//响应码不为ok
					my_printf(USER_ERROR, "%s:%d gun%d:TransactionReversal RX %s %s\n", __FILE__, __LINE__,
							g_psPOS_data->GunId,
							g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode,
							g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseMessage);
				}

				break;

			case TRANSACTIONINCREMENTAL://扣费到实际金额,可能不用
				if(strncmp(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode, "000000", 6) == 0)//响应码OK
				{
					//清除交易存储信息,在开始交易的时候清了
					//ClearTransactionData(g_psPOS_data->GunId);

					//打印通过的交易金额
					my_printf(USER_INFO, "TransactionIncremental RX %s\n",
							g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].approvedAmount);
				}
				else
				{
					//响应码不为ok
					my_printf(USER_ERROR, "%s:%d ERR TransactionIncremental RX %s %s\n", __FILE__, __LINE__,
								g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode,
								g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseMessage);
				}
				break;

			case GETCARD_INFO://获取卡状态，获取卡唯一标识，用于用户提前停充
				if(strncmp(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode, "000000", 6) == 0)//响应码OK
				{
					//打印卡唯一标识
					my_printf(USER_INFO, "GetCardInfo RX %s\n",g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].account);
					//判定结束卡号和启动卡号是否相同
					U8 GunId = g_psHMI_data->ucGun_id - ((2U * g_sStorage_data.ucPile_num) - 1U);
					switch (GunId)
					{
					case GUN_A:
						if (0 == strncmp(g_sGun_data[GUN_A].sTemp.ucIdtag, g_psPOS_data->POS_Transaction[GUN_A].account, sizeof(g_psPOS_data->POS_Transaction[GUN_A].account)))
						{
							//告知HMI成功
							if (NULL != g_psHMI_data)
							{
								//停止充电
								PosStopCharge();
								g_psHMI_data->ucAuthorize_flag[GUN_A] = AUTHORIZE_SUCCESS;
							}
							//发送实际扣费
							//g_psPOS_data->POS_Transaction[GUN_A].bHmi_TransactionCompletion_flag = TRUE;
						}
						else
						{
							//告知HMI失败
							if (NULL != g_psHMI_data)
							{
								g_psHMI_data->ucAuthorize_flag[GUN_A] = AUTHORIZE_FAILED;
							}
							my_printf(USER_ERROR, "%s:%d GUN_A: POS stop gun id error\n", __FILE__, __LINE__);
						}
						break;
					case GUN_B:
						if (0 == strncmp(g_sGun_data[GUN_B].sTemp.ucIdtag, g_psPOS_data->POS_Transaction[GUN_B].account, sizeof(g_psPOS_data->POS_Transaction[GUN_B].account)))
						{
							//告知HMI成功
							if (NULL != g_psHMI_data)
							{
								g_psHMI_data->ucAuthorize_flag[GUN_B] = AUTHORIZE_SUCCESS;
								//停止充电
								PosStopCharge();
							}
							//发送实际扣费
							//g_psPOS_data->POS_Transaction[GUN_B].bHmi_TransactionCompletion_flag = TRUE;
						}
						else
						{
							//告知HMI失败
							if (NULL != g_psHMI_data)
							{
								g_psHMI_data->ucAuthorize_flag[GUN_B] = AUTHORIZE_FAILED;
							}
							my_printf(USER_ERROR, "%s:%d GUN_B: POS stop Idtag error\n", __FILE__, __LINE__);
						}
						break;
					default:
						my_printf(USER_ERROR, "%s:%d POS stop card gun id error = %d\n", __FILE__, __LINE__, GunId);
						break;
					}
				}
				else
				{
					//响应码不为ok
					my_printf(USER_ERROR, "%s:%d ERR GetCardInfo RX %s %s", __FILE__, __LINE__,
							g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode,
							g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseMessage);
				}

				break;

			case GETTERMINALINFO://心跳
				if(strncmp(g_psPOS_data->POS_Transaction[GUN_A].responseCode, "000000", 6) == 0)//响应码OK
				{
					//打印软件版本号
					my_printf(USER_DEBUG, "TransactionGetterminalInfo RX %s\n", g_psPOS_data->POS_Transaction[GUN_A].appVersion);
					if (0U == strlen(g_sPile_data.ucPos_version))
					{
						(void)memcpy(g_sPile_data.ucPos_version, g_psPOS_data->POS_Transaction[GUN_A].appVersion, sizeof(g_sPile_data.ucPos_version));
					}
				}
				else
				{
					//响应码不为ok
					my_printf(USER_DEBUG, "%s:%d ERR TransactionGetterminalInfo RX %s %s", __FILE__, __LINE__,
									g_psPOS_data->POS_Transaction[GUN_A].responseCode,
									g_psPOS_data->POS_Transaction[GUN_A].responseMessage);
				}

				break;

			default:
				//
				break;
		}

		g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].POS_Command = UNKNOW_COMMAND;
	}
}

static void PosRecOneData(U8 u8RecDataTmp)
{
	U8 u8DecimalStr[7] = "000000";//最后一位放了'\0'

	if(POS_UartRecBuffer.u8ReadMsgBeginFlag == 0U)
	{
		POS_UartRecBuffer.u16ReadMsgTimeOutCount = 0;
		POS_UartRecBuffer.u16ReadMsgDataNum = 0;
		if(u8RecDataTmp == 0x02U)
		{
			POS_UartRecBuffer.u8AnalysisData[POS_UartRecBuffer.u16ReadMsgDataNum++] = u8RecDataTmp;
			POS_UartRecBuffer.u8ReadMsgBeginFlag = 1;
		}
	}
	else
	{
		if ((POS_UartRecBuffer.u16ReadMsgDataNum == 1U) && (u8RecDataTmp != 0x50U))//P
		{
			POS_UartRecBuffer.u16ReadMsgDataNum = 0;
			POS_UartRecBuffer.u8ReadMsgBeginFlag = 0;
		}

		POS_UartRecBuffer.u8AnalysisData[POS_UartRecBuffer.u16ReadMsgDataNum++] = u8RecDataTmp;

		if(POS_UartRecBuffer.u16ReadMsgDataNum == 13U)//到长度位
		{
			(void)memcpy(u8DecimalStr,&POS_UartRecBuffer.u8AnalysisData[6],6);//取出json体长度位
			POS_UartRecBuffer.u16Datacount = DecimalFromString(u8DecimalStr);//转换成10进制
		}

		if (POS_UartRecBuffer.u16ReadMsgDataNum >= (POS_UartRecBuffer.u16Datacount + 19U))//加19个协议帧
		{
			POS_EventAnalysis();
			POS_UartRecBuffer.u16ReadMsgDataNum=0;
			POS_UartRecBuffer.u8ReadMsgBeginFlag=0;
		}
		if(POS_UartRecBuffer.u16ReadMsgDataNum > 500U)//当索引值比最长帧500还要长的时候，肯定是长度位出问题了，重新开始
		{
			POS_UartRecBuffer.u16ReadMsgDataNum=0;
			POS_UartRecBuffer.u8ReadMsgBeginFlag=0;
		}

	}
}

static void Pos_analysis_task(void *arg)
{
	#ifdef FREE_STACK_SPACE_CHECK_ENABLE
	volatile UBaseType_t uxHighWaterMark;
	#endif
	BaseType_t err = pdFALSE;

	while(1)
	{
		err = xSemaphoreTake(POS_Rec_Binary_Semaphore, portMAX_DELAY);
		if(pdTRUE == err)
		{
			while((POS_UartRecBuffer.u16DealPos != POS_UartRecBuffer.u16ReachPos) && (POS_UartRecBuffer.u16DealCount < ETH_REC_RINGBUF_SIZE))
			{
				POS_UartRecBuffer.u16DealCount+=1U;
				PosRecOneData(POS_UartRecBuffer.u8DealData[POS_UartRecBuffer.u16DealPos]);
				(void)memset(&POS_UartRecBuffer.u8DealData[POS_UartRecBuffer.u16DealPos],0,1);//取出一个数据后把该数据从缓存里清掉
				POS_UartRecBuffer.u16DealPos++;
				if(POS_UartRecBuffer.u16DealPos >= ETH_REC_RINGBUF_SIZE)
				{
					POS_UartRecBuffer.u16DealPos = 0;
				}
			}
			POS_UartRecBuffer.u16DealCount=0;
		}
		else
		{
			vTaskDelay(50);
		}
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
		uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
#endif
	}
}

static void PosDataProcess(void)
{
	__BSS(SRAM_OC) static uint8_t ucPOS_data[UART_COMM_BUF_LEN] = {0};
	U16 u16RecByteNum = 0;

	u16RecByteNum = RecUartData(POS_UART, ucPOS_data, UART_COMM_BUF_LEN);

//	my_printf(USER_INFO, "POS RX Len:%d \n",u16RecByteNum);
//	uPRINTF("RX:");
//	for(U16 i = 0;i< u16RecByteNum;i++)
//	{
//		uPRINTF("%c ",ucPOS_data[i]);
//	}
//	uPRINTF("\n");

	if(UART_COMM_BUF_LEN < u16RecByteNum)
	{
		return ;
	}

	if(u16RecByteNum > 1U)
	{
		U16 FreeSize = ETH_REC_RINGBUF_SIZE - POS_UartRecBuffer.u16ReachPos;//剩余的空间
		U16 CpyLen = (FreeSize >= u16RecByteNum) ? u16RecByteNum : FreeSize;
		(void)memcpy(&POS_UartRecBuffer.u8DealData[POS_UartRecBuffer.u16ReachPos], ucPOS_data, CpyLen);
		POS_UartRecBuffer.u16ReachPos += CpyLen;
		if(POS_UartRecBuffer.u16ReachPos == ETH_REC_RINGBUF_SIZE)
		{
			POS_UartRecBuffer.u16ReachPos=0;
			if(CpyLen != u16RecByteNum)
			{
				(void)memcpy(&POS_UartRecBuffer.u8DealData[POS_UartRecBuffer.u16ReachPos], &ucPOS_data[CpyLen],u16RecByteNum - CpyLen);
				POS_UartRecBuffer.u16ReachPos += (u16RecByteNum - CpyLen);
			}
		 }

		if(NULL != POS_Rec_Binary_Semaphore)
		{
			(void)xSemaphoreGive(POS_Rec_Binary_Semaphore);
		}
	}
	else if(u16RecByteNum == 1U)
	{
		g_psPOS_data->PosDataParse(ucPOS_data, u16RecByteNum);//ACK,EOT,NAK
	}
	else
	{
		//
	}

	return;
}

static void Pos_Parse_Task(void *parameter)
{
	BaseType_t err = pdFALSE;
	#ifdef FREE_STACK_SPACE_CHECK_ENABLE
	volatile UBaseType_t uxHighWaterMark;
	#endif

	while(1)
	{
		if(NULL != Uart_recBinarySemaphore(POS_UART))
		{
			err = xSemaphoreTake(Uart_recBinarySemaphore(POS_UART), portMAX_DELAY);
			if(pdTRUE == err)
			{
				PosDataProcess();
			}
			else
			{
				vTaskDelay(300);
			}
		}
		else
		{
			vTaskDelay(300);
		}
#ifdef FREE_STACK_SPACE_CHECK_ENABLE
		uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
#endif
	}
}

void Pos_Init_Task(void * pvParameters)
{
	//外设初始化
	BfPosInit();
	PosModuleSelect();

	CheckServerConfig();

	taskENTER_CRITICAL();

	if (TRUE == g_sStorage_data.bUse_ad_pos)
	{
		POS_Rec_Binary_Semaphore = xSemaphoreCreateBinary();

		//add other pos_init task here
		(void)xTaskCreate(&Pos_Request_Task,  "POS_REQUEST",	1000U/4U, NULL, GENERAL_TASK_PRIO, NULL);
		(void)xTaskCreate(&Pos_Parse_Task,	  "POS_PARSE",	1600U/4U, NULL, GENERAL_TASK_PRIO, NULL);
		(void)xTaskCreate(&Pos_analysis_task, "POS_ANALYSIS", 1100U/4U, NULL, GENERAL_TASK_PRIO, NULL);
	}
	vTaskDelete(NULL);

	taskEXIT_CRITICAL();
}
