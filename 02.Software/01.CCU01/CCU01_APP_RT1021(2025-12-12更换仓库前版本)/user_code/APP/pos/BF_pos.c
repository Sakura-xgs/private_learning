/*
 * BF_pos.c
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

#include "BF_pos.h"
#include "pos.h"
#include "calculate_crc.h"
#include "hal_uart_IF.h"
#include "cJSON.h"
#include "Signal.h"
#include "SignalManage.h"
#include "uart_comm.h"
#include "pos_IF.h"

__BSS(SRAM_OC) static POS_data_t g_sBF_POS = {0};
__BSS(SRAM_OC) POS_JsonSendBuf_t sPos_JsonSendbuf;
POS_HostState_t sPos_HostState = POS_STATE_IDLE;
size_t cJSON_MemOffset = 0;

//用于获取pos设备信息的命令，该命令参数固定
static U8 g_u8GetTerminalInfoJsonBody[80]={0x02,0x50,0x4A,0x30,0x31,0x37,0x30,0x30,0x30,0x30,0x36,0x30,0x30,0x30,0x30,0x30
									 ,0x30,0x31,0x7B,0x22,0x76,0x65,0x72,0x73,0x69,0x6F,0x6E,0x22,0x3A,0x22,0x32,0x2E
									 ,0x30,0x30,0x22,0x2C,0x22,0x72,0x65,0x71,0x75,0x65,0x73,0x74,0x22,0x3A,0x5B,0x7B
									 ,0x22,0x63,0x6F,0x6D,0x6D,0x61,0x6E,0x64,0x22,0x3A,0x22,0x47,0x65,0x74,0x54,0x65
									 ,0x72,0x6D,0x69,0x6E,0x61,0x6C,0x49,0x6E,0x66,0x6F,0x22,0x7D,0x5D,0x7D,0x17, 0};



/**
  * @brief pos校验函数，逐字节亦或
  * @param 数据帧
  * @param 数据帧长度
  */
static U8 Pos_Checksum(const U8 *buf,U16 len)
{
	 U16 i;
	 U8 u8checksum=0;
	 for(i=0;i<len;i++)
	 {
		 u8checksum^=buf[i];
	 }
	 return u8checksum;
}

/**
  * @brief 给json体加上\x02PJ017帧头，长度，校验，并发送
  * @param
  */
static void Pos_JSON_Send(cJSON *root)
{
	U8 u8JsonLenStr[7] = {0};
    U8 u8PosChecksum = 0;
    U16 u16JsonBodyMaxLen = 0;
    U8 Ret=0;

    u16JsonBodyMaxLen = (U16)sizeof(sPos_JsonSendbuf.JsonConvertString);

    if(true == cJSON_PrintPreallocated(root, sPos_JsonSendbuf.JsonConvertString, u16JsonBodyMaxLen, 0))
    {
    	sPos_JsonSendbuf.Jsonlen = strlen(sPos_JsonSendbuf.JsonConvertString);

		if(((sPos_JsonSendbuf.Jsonlen+19U) < u16JsonBodyMaxLen) && (sPos_JsonSendbuf.Jsonlen > 0U))
		{
			sPos_JsonSendbuf.JsonString[0]=0x02;
			(void)memcpy(&sPos_JsonSendbuf.JsonString[1],"PJ017",5);
			(void)sprintf((char*)u8JsonLenStr, "%06d", sPos_JsonSendbuf.Jsonlen);
			(void)memcpy(&sPos_JsonSendbuf.JsonString[6],u8JsonLenStr,6);
			(void)memcpy(&sPos_JsonSendbuf.JsonString[12],"000001",6);
			(void)memcpy(&sPos_JsonSendbuf.JsonString[18],sPos_JsonSendbuf.JsonConvertString,sPos_JsonSendbuf.Jsonlen);

			u8PosChecksum = Pos_Checksum(&sPos_JsonSendbuf.JsonString[1],sPos_JsonSendbuf.Jsonlen + 17U);
			sPos_JsonSendbuf.JsonString[sPos_JsonSendbuf.Jsonlen + 18U] = u8PosChecksum;

			Uart_Dma_Send(POS_UART, sPos_JsonSendbuf.Jsonlen + 19U, sPos_JsonSendbuf.JsonString);

			sPos_HostState = POS_STATE_TX;
		}
    }
    else
    {
    	//序列化失败，比如数据类型不是字符串
    	my_printf(USER_ERROR,"Preallocated err\n");
    }
}

void BfPosTransactionStart(Gun_Id_e GUN_ID)
{
	if(sPos_HostState == POS_STATE_IDLE)//交互空闲的时候
	{
		BOOL blRet = FALSE;
		U8 frozenAmountLen = 0;

		//清一下预扣费缓存
		memset(g_psPOS_data->POS_Transaction[GUN_ID].frozenAmount,0,sizeof(g_psPOS_data->POS_Transaction[GUN_ID].frozenAmount));
		//从eeprom获取金额并转化为字符串格式
		blRet = PositiveIntToString(CCU_SET_SIG_ID_ADVANCE_CHARGE,0,g_psPOS_data->POS_Transaction[GUN_ID].frozenAmount,0);
		if(TRUE == blRet)
		{
			frozenAmountLen = strlen(g_psPOS_data->POS_Transaction[GUN_ID].frozenAmount);

			if((frozenAmountLen > 0) && (frozenAmountLen <= 6))
			{
				blRet = TRUE;
			}
			else
			{
				blRet = FALSE;
			}
		}

		if(TRUE == blRet)
		{
			if(GUN_A == GUN_ID)
			{
				ClearTransactionData(GUN_A);
			}
			else if(GUN_B == GUN_ID)
			{
				ClearTransactionData(GUN_B);
			}

			GenerateRandomNumber(g_psPOS_data->POS_Transaction[GUN_ID].registerReferenceNumber);

			blRet = Pos_cjson_TransactionStart(g_psPOS_data->POS_Transaction[GUN_ID].frozenAmount,
									   g_psPOS_data->POS_Transaction[GUN_ID].registerReferenceNumber);//交易开始

			if(TRUE == blRet)
			{
				my_printf(USER_INFO,"gun%d:TransactionStart TX %s %s\n",GUN_ID,g_psPOS_data->POS_Transaction[GUN_ID].frozenAmount,g_psPOS_data->POS_Transaction[GUN_ID].registerReferenceNumber);
			}
			else
			{
				my_printf(USER_ERROR,"%s:%d gun%d:TransactionStart error\n", __FILE__, __LINE__,GUN_ID);
			}
		}
		else
		{
			//获取到的值不符，结束本次操作
			g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].swipeStartFlag = FALSE;//清刷卡标志
			my_printf(USER_ERROR,"%s:%d gun%d:Amount retrieval error\n", __FILE__, __LINE__,GUN_ID);
		}

		g_psPOS_data->POS_Transaction[GUN_ID].bHmi_TransactionStart_flag = FALSE;
		g_psPOS_data->GunId = GUN_ID;
	}
	else
	{
		my_printf(USER_ERROR,"%s:%d gun%d:PosBusy\n", __FILE__, __LINE__,GUN_ID);
	}
}

void BfPosTransactionCompletion(Gun_Id_e GUN_ID)
{
	if (GUN_ID >= 2U)
	{
		return;
	}

	if(sPos_HostState == POS_STATE_IDLE)//交互空闲的时候
	{
		BOOL blRet = FALSE;
		S32 AdvanceAmount = 0;
		S32 transactionAmountSum = 0;

		my_printf(USER_INFO,"gun%d:TransactionAmount 1 %s\n",GUN_ID,g_psPOS_data->POS_Transaction[GUN_ID].transactionAmount);

		if(GUN_A == GUN_ID)
		{
			blRet = PositiveIntToString(CCU_SAM_SIG_ID_CHARGE_PRICE_A,CCU_SAM_SIG_ID_OCCUPATION_FEE_A,g_psPOS_data->POS_Transaction[GUN_A].transactionAmount,&transactionAmountSum);//从eeprom获取金额并转化为字符串格式
		}
		if(GUN_B == GUN_ID)
		{
			blRet = PositiveIntToString(CCU_SAM_SIG_ID_CHARGE_PRICE_B,CCU_SAM_SIG_ID_OCCUPATION_FEE_B,g_psPOS_data->POS_Transaction[GUN_B].transactionAmount,&transactionAmountSum);//从eeprom获取金额并转化为字符串格式
		}

		my_printf(USER_INFO,"gun%d:TransactionAmount 2 %s\n",GUN_ID,g_psPOS_data->POS_Transaction[GUN_ID].transactionAmount);

		if(TRUE == blRet)
		{
			(void)GetSigVal(CCU_SET_SIG_ID_ADVANCE_CHARGE, &AdvanceAmount);

			if(transactionAmountSum <= AdvanceAmount)//交易金额小于预扣费金额，执行交易完成指令
			{
				blRet = Pos_cjson_TransactionCompletion(g_psPOS_data->POS_Transaction[GUN_ID].transactionAmount,
														g_psPOS_data->POS_Transaction[GUN_ID].originalHostReferenceNumber,
														g_psPOS_data->POS_Transaction[GUN_ID].originalHostData);
				if(TRUE == blRet)
				{
					my_printf(USER_INFO,"gun%d:TransactionCompletion TX %s, %s, %s\n",
							GUN_ID,
							g_psPOS_data->POS_Transaction[GUN_ID].transactionAmount,
							g_psPOS_data->POS_Transaction[GUN_ID].originalHostReferenceNumber,
							g_psPOS_data->POS_Transaction[GUN_ID].originalHostData);
				}
				else
				{
					my_printf(USER_ERROR,"%s:%d gun%d:TransactionCompletion error\n", __FILE__, __LINE__,GUN_ID);
				}
			}
			else//交易金额大于预扣费金额，执行扣费到实际金额指令
			{
				my_printf(USER_ERROR,"%s:%d transactionAmountSum > advance\n", __FILE__, __LINE__);
//				blRet = Pos_cjson_TransactionIncremental(g_psPOS_data->POS_Transaction[GUN_ID].transactionAmount,
//														 g_psPOS_data->POS_Transaction[GUN_ID].originalHostReferenceNumber,
//														 g_psPOS_data->POS_Transaction[GUN_ID].originalHostData);
//				if(TRUE == blRet)
//				{
//					my_printf(USER_INFO,"TransactionIncremental TX %s , %s , %s\n",
//							g_psPOS_data->POS_Transaction[GUN_ID].transactionAmount,
//							g_psPOS_data->POS_Transaction[GUN_ID].originalHostReferenceNumber,
//							g_psPOS_data->POS_Transaction[GUN_ID].originalHostData);
//				}
//				else
//				{
//					my_printf(USER_ERROR,"%s:%d TransactionIncremental error\n", __FILE__, __LINE__);
//				}
			}
		}
		else
		{
			//获取到的值不符，金额为0，或金额超过6位(包含小数点后两位)，结束本次操作，可能要做取消上笔账单
			g_psPOS_data->POS_Transaction[GUN_ID].bHmi_TransactionStart_flag = FALSE;
			my_printf(USER_INFO,"gun%d:Amount retrieval error\n",GUN_ID);
		}
		g_psPOS_data->POS_Transaction[GUN_ID].bHmi_TransactionCompletion_flag = FALSE;
		g_psPOS_data->GunId = GUN_ID;
	}
	else
	{
		my_printf(USER_ERROR,"%s:%d gun%d:PosBusy\n", __FILE__, __LINE__,GUN_ID);
	}
}

void BfPosTransactionReversal(Gun_Id_e GUN_ID)
{
	if(sPos_HostState == POS_STATE_IDLE)//交互空闲的时候
	{
		(void)Pos_cjson_TransactionReversal(g_psPOS_data->POS_Transaction[GUN_ID].originalHostReferenceNumber,
									  	  	  g_psPOS_data->POS_Transaction[GUN_ID].originalHostData);

		g_psPOS_data->POS_Transaction[GUN_ID].bHmi_TransactionReversal_flag = FALSE;
		g_psPOS_data->GunId = GUN_ID;

		my_printf(USER_INFO,"gun%d:TransactionReversal TX %s,%s\n",
				GUN_ID,
				g_psPOS_data->POS_Transaction[GUN_ID].originalHostReferenceNumber,
				g_psPOS_data->POS_Transaction[GUN_ID].originalHostData);
	}
	else
	{
		my_printf(USER_ERROR,"%s:%d gun%d:PosBusy\n", __FILE__, __LINE__,GUN_ID);
	}
}

void BfPosGetCardInfo(Gun_Id_e GUN_ID)
{
	if(sPos_HostState == POS_STATE_IDLE)//交互空闲的时候
	{
		(void)GetCardInfo();

		g_psPOS_data->POS_Transaction[GUN_ID].bHmi_GetCardInfo_flag = FALSE;
		g_psPOS_data->GunId = GUN_ID;

		my_printf(USER_INFO,"gun%d:GetCardInfo TX\n",GUN_ID);
	}
	else
	{
		my_printf(USER_ERROR,"%s:%d gun%d:PosBusy\n", __FILE__, __LINE__,GUN_ID);
	}

}

void BfPosTransactionAbort(U8 GUN_ID)
{
	if(sPos_HostState != POS_STATE_IDLE)//交互空闲的时候
	{
		(void)Pos_cjson_Abort();

		g_psPOS_data->GunId = GUN_ID;
	}
	else
	{
		my_printf(USER_ERROR,"%s:%d gun%d:PosIdle\n", __FILE__, __LINE__,GUN_ID);
	}

	g_psPOS_data->POS_Transaction[GUN_ID].bHmi_Abort_flag = FALSE;
}

BOOL Pos_cjson_GetTerminalInfo(void)
{
	cJSON *root = cJSON_CreateObject();

	if(root != NULL)
	{
		if (NULL != cJSON_AddStringToObject(root, "version", "2.00"))
		{
			cJSON *requestArray = cJSON_AddArrayToObject(root, "request");

			if(requestArray != NULL)
			{
				cJSON *request = cJSON_CreateObject();

				if(request != NULL)
				{
					if (NULL != cJSON_AddStringToObject(request, "command", "GetTerminalInfo"))
					{
						cJSON_AddItemToArray(requestArray, request);

						 // 将 JSON 对象转换为字符串并发送
						Pos_JSON_Send(root);

						cJSON_Delete(root);
						cJSON_MemOffset = 0;
						return TRUE;
					}
					else
					{
						my_printf(USER_ERROR, "Error:%s:%d GetTerminalInfo add command to object\n", __FILE__, __LINE__);
					}
				}
				else
				{
					my_printf(USER_ERROR, "Error:%s:%d GetTerminalInfo creat request object\n", __FILE__, __LINE__);
				}
			}
			else
			{
				my_printf(USER_ERROR, "Error:%s:%d GetTerminalInfo creat requestArray object\n", __FILE__, __LINE__);
			}
		}
		else
		{
			my_printf(USER_ERROR, "Error:%s:%d TransactionStart add version to object\n", __FILE__, __LINE__);
		}

		cJSON_Delete(root);
		cJSON_MemOffset = 0;
		return FALSE;
	}
	return FALSE;
}

static BOOL Pos_cjson_TransactionStart(const char *transactionAmount_a,const char *registerReferenceNumber_a)
{
	if((transactionAmount_a == NULL) || (registerReferenceNumber_a == NULL))
	{
		return FALSE;
	}

	cJSON *root = cJSON_CreateObject();

	if(root != NULL)
	{
		if (NULL != cJSON_AddStringToObject(root, "version", "2.00"))
		{
			cJSON *requestArray = cJSON_AddArrayToObject(root, "request");

			if(requestArray != NULL)
			{
				cJSON *request = cJSON_CreateObject();

				if(request != NULL)
				{
					if (NULL != cJSON_AddStringToObject(request, "command", "TransactionStart"))
					{
						cJSON *properties = cJSON_AddObjectToObject(request, "properties");

						if(properties != NULL)
						{
							if(0 != (cJSON_AddStringToObject(properties, "transactionAmount", transactionAmount_a) &&
							   cJSON_AddStringToObject(properties, "registerReferenceNumber", registerReferenceNumber_a) &&
							   cJSON_AddStringToObject(properties, "enableCardUniqueIdentifier", "Off")))
							{
								cJSON_AddItemToArray(requestArray, request);

								 // 将 JSON 对象转换为字符串并发送
								Pos_JSON_Send(root);

								cJSON_Delete(root);
								cJSON_MemOffset = 0;
								return TRUE;
							}
							else
							{
								my_printf(USER_ERROR, "Error:%s:%d TransactionStart add to object\n", __FILE__, __LINE__);
							}

						}
						else
						{
							my_printf(USER_ERROR, "Error:%s:%d TransactionStart creat properties object\n", __FILE__, __LINE__);
						}

					}
					else
					{
						my_printf(USER_ERROR, "Error:%s:%d TransactionStart add command to object\n", __FILE__, __LINE__);
					}
				}
				else
				{
					my_printf(USER_ERROR, "Error:%s:%d TransactionStart creat request object\n", __FILE__, __LINE__);
				}
			}
			else
			{
				my_printf(USER_ERROR, "Error:%s:%d TransactionStart creat requestArray object\n", __FILE__, __LINE__);
			}
		}
		else
		{
			my_printf(USER_ERROR, "Error:%s:%d TransactionStart add version to object\n", __FILE__, __LINE__);
		}

		cJSON_Delete(root);
		cJSON_MemOffset = 0;
		return FALSE;
	}
	return FALSE;
}

BOOL Pos_cjson_TransactionCompletion(const char *transactionAmount_a,const char *originalHostReferenceNumber_a,const char *originalHostData_a)
{
	if((transactionAmount_a == NULL) || (originalHostReferenceNumber_a == NULL) || (originalHostData_a == NULL))
	{
		return FALSE;
	}

	cJSON *root = cJSON_CreateObject();

	if(root != NULL)
	{
		if (NULL != cJSON_AddStringToObject(root, "version", "2.00"))
		{
			cJSON *requestArray = cJSON_AddArrayToObject(root, "request");

			if(requestArray != NULL)
			{
				cJSON *request = cJSON_CreateObject();

				if(request != NULL)
				{
					if(NULL != cJSON_AddStringToObject(request, "command", "TransactionCompletion"))
					{
						cJSON *properties = cJSON_AddObjectToObject(request, "properties");

						if(properties != NULL)
						{
							if(0 != (cJSON_AddStringToObject(properties, "transactionAmount", transactionAmount_a) &&
							   cJSON_AddStringToObject(properties, "originalHostReferenceNumber", originalHostReferenceNumber_a) &&
							   cJSON_AddStringToObject(properties, "originalHostData", originalHostData_a)))
							{
								cJSON_AddItemToArray(requestArray, request);

								 // 将 JSON 对象转换为字符串并发送
								Pos_JSON_Send(root);

								cJSON_Delete(root);
								cJSON_MemOffset = 0;
								return TRUE;
							}
							else
							{
								my_printf(USER_ERROR, "Error:%s:%d TransactionCompletion add to object\n", __FILE__, __LINE__);
							}
						}
						else
						{
							my_printf(USER_ERROR, "Error:%s:%d TransactionCompletion creat properties object\n", __FILE__, __LINE__);
						}

					}
					else
					{
						my_printf(USER_ERROR, "Error:%s:%d TransactionCompletion add command to object\n", __FILE__, __LINE__);
					}
				}
				else
				{
					my_printf(USER_ERROR, "Error:%s:%d TransactionCompletion creat request object\n", __FILE__, __LINE__);
				}
			}
			else
			{
				my_printf(USER_ERROR, "Error:%s:%d TransactionCompletion creat requestArray object\n", __FILE__, __LINE__);
			}
		}
		else
		{
			my_printf(USER_ERROR, "Error:%s:%d TransactionCompletion add version to object\n", __FILE__, __LINE__);
		}

		cJSON_Delete(root);
		cJSON_MemOffset = 0;
		return FALSE;
	}
	return FALSE;
}

static BOOL Pos_cjson_TransactionReversal(const char *originalHostReferenceNumber_a,const char *originalHostData_a)
{
	if((originalHostReferenceNumber_a == NULL) || (originalHostData_a == NULL))
	{
		return FALSE;
	}

	cJSON *root = cJSON_CreateObject();

	if(root != NULL)
	{
		if(NULL != cJSON_AddStringToObject(root, "version", "2.00"))
		{
			cJSON *requestArray = cJSON_AddArrayToObject(root, "request");

			if(requestArray != NULL)
			{
				cJSON *request = cJSON_CreateObject();

				if(request != NULL)
				{
					if(NULL != cJSON_AddStringToObject(request, "command", "TransactionReversal"))
					{
						cJSON *properties = cJSON_AddObjectToObject(request, "properties");

						if(properties != NULL)
						{
							if(0 != (cJSON_AddStringToObject(properties, "originalHostReferenceNumber", originalHostReferenceNumber_a) &&
							   cJSON_AddStringToObject(properties, "originalHostData", originalHostData_a)))
							{
								cJSON_AddItemToArray(requestArray, request);

								 // 将 JSON 对象转换为字符串并发送
								Pos_JSON_Send(root);

								cJSON_Delete(root);
								cJSON_MemOffset = 0;
								return TRUE;
							}
							else
							{
								my_printf(USER_ERROR, "Error:%s:%d TransactionReversal add to object\n", __FILE__, __LINE__);
							}
						}
						else
						{
							my_printf(USER_ERROR, "Error:%s:%d TransactionReversal creat properties object\n", __FILE__, __LINE__);
						}
					}
					else
					{
						my_printf(USER_ERROR, "Error:%s:%d TransactionReversal add command to object\n", __FILE__, __LINE__);
					}
				}
				else
				{
					my_printf(USER_ERROR, "Error:%s:%d TransactionReversal creat request object\n", __FILE__, __LINE__);
				}
			}
			else
			{
				my_printf(USER_ERROR, "Error:%s:%d TransactionReversal creat requestArray object\n", __FILE__, __LINE__);
			}
		}
		else
		{
			my_printf(USER_ERROR, "Error:%s:%d TransactionReversal add version to object\n", __FILE__, __LINE__);
		}

		cJSON_Delete(root);
		cJSON_MemOffset = 0;
		return FALSE;
	}
	return FALSE;
}

static BOOL Pos_cjson_TransactionIncremental(const char *transactionAmount_a,const char *originalHostReferenceNumber_a,const char *originalHostData_a)
{
	if((transactionAmount_a == NULL) || (originalHostReferenceNumber_a == NULL) || (originalHostData_a == NULL))
	{
		return FALSE;
	}

	cJSON *root = cJSON_CreateObject();

	if(root != NULL)
	{
		if(NULL != cJSON_AddStringToObject(root, "version", "2.00"))
		{
			cJSON *requestArray = cJSON_AddArrayToObject(root, "request");

			if(requestArray != NULL)
			{
				cJSON *request = cJSON_CreateObject();

				if(request != NULL)
				{
					if(NULL != cJSON_AddStringToObject(request, "command", "TransactionIncremental"))
					{
						cJSON *properties = cJSON_AddObjectToObject(request, "properties");

						if(properties != NULL)
						{
							if(0 != (cJSON_AddStringToObject(properties, "transactionAmount", transactionAmount_a) &&
							   cJSON_AddStringToObject(properties, "originalHostReferenceNumber", originalHostReferenceNumber_a) &&
							   cJSON_AddStringToObject(properties, "originalHostData", originalHostData_a)))
							{
								cJSON_AddItemToArray(requestArray, request);

								 // 将 JSON 对象转换为字符串并发送
								Pos_JSON_Send(root);

								cJSON_Delete(root);
								cJSON_MemOffset = 0;
								return TRUE;
							}
							else
							{
								my_printf(USER_ERROR, "Error:%s:%d TransactionIncremental add to object\n", __FILE__, __LINE__);
							}
						}
						else
						{
							my_printf(USER_ERROR, "Error:%s:%d TransactionIncremental creat properties object\n", __FILE__, __LINE__);
						}
					}
					else
					{
						my_printf(USER_ERROR, "Error:%s:%d TransactionIncremental add command to object\n", __FILE__, __LINE__);
					}
				}
				else
				{
					my_printf(USER_ERROR, "Error:%s:%d TransactionIncremental creat request object\n", __FILE__, __LINE__);
				}
			}
			else
			{
				my_printf(USER_ERROR, "Error:%s:%d TransactionIncremental creat requestArray object\n", __FILE__, __LINE__);
			}
		}
		else
		{
			my_printf(USER_ERROR, "Error:%s:%d TransactionIncremental add version to object\n", __FILE__, __LINE__);
		}

		cJSON_Delete(root);
		cJSON_MemOffset = 0;
		return FALSE;
	}
	return FALSE;
}

/* 当已经发送了命令A，并且终端正在等待用户操作,
 * 自助服务设备可以发送Abort命令来中断命令的执行，并收到命令A的执行结果.
 */
static BOOL Pos_cjson_Abort(void)
{
	cJSON *root = cJSON_CreateObject();

	if(root != NULL)
	{
		if(NULL != cJSON_AddStringToObject(root, "version", "2.00"))
		{
			cJSON *requestArray = cJSON_AddArrayToObject(root, "request");

			if(requestArray != NULL)
			{
				cJSON *request = cJSON_CreateObject();

				if(request != NULL)
				{
					if(NULL != cJSON_AddStringToObject(request, "command", "Abort"))
					{
						cJSON_AddItemToArray(requestArray, request);

						 // 将 JSON 对象转换为字符串并发送
						Pos_JSON_Send(root);

						cJSON_Delete(root);
						cJSON_MemOffset = 0;
						return TRUE;
					}
					else
					{
						my_printf(USER_ERROR, "Error:%s:%d Abort add command to object\n", __FILE__, __LINE__);
					}
				}
				else
				{
					my_printf(USER_ERROR, "Error:%s:%d Abort creat request object\n", __FILE__, __LINE__);
				}
			}
			else
			{
				my_printf(USER_ERROR, "Error:%s:%d Abort creat requestArray object\n", __FILE__, __LINE__);
			}
		}
		else
		{
			my_printf(USER_ERROR, "Error:%s:%d Abort add version to object\n", __FILE__, __LINE__);
		}

		cJSON_Delete(root);
		cJSON_MemOffset = 0;
		return FALSE;
	}
	return FALSE;
}

/* 重启设备
 */
static BOOL BfPosReboot(void)
{
	cJSON *root = cJSON_CreateObject();

	if(root != NULL)
	{
		if(NULL != cJSON_AddStringToObject(root, "version", "2.00"))
		{
			cJSON *requestArray = cJSON_AddArrayToObject(root, "request");

			if(requestArray != NULL)
			{
				cJSON *request = cJSON_CreateObject();

				if(request != NULL)
				{
					if(NULL != cJSON_AddStringToObject(request, "command", "Reboot"))
					{
						cJSON_AddItemToArray(requestArray, request);

						 // 将 JSON 对象转换为字符串并发送
						Pos_JSON_Send(root);

						cJSON_Delete(root);
						cJSON_MemOffset = 0;
						return TRUE;
					}
					else
					{
						my_printf(USER_ERROR, "Error:%s:%d Reboot add command to object\n", __FILE__, __LINE__);
					}
				}
				else
				{
					my_printf(USER_ERROR, "Error:%s:%d Reboot creat request object\n", __FILE__, __LINE__);
				}
			}
			else
			{
				my_printf(USER_ERROR, "Error:%s:%d Reboot creat requestArray object\n", __FILE__, __LINE__);
			}
		}
		else
		{
			my_printf(USER_ERROR, "Error:%s:%d Reboot add version to object\n", __FILE__, __LINE__);
		}

		cJSON_Delete(root);
		cJSON_MemOffset = 0;
		return FALSE;
	}
	return FALSE;
}

void BfPosServiceBusyProcess(U8 GunId)
{
	if(g_psPOS_data->POS_Transaction[GunId].serviceBusy == TRUE)
	{
		g_psPOS_data->POS_Transaction[GunId].serviceBusyResendTime++;
		if(g_psPOS_data->POS_Transaction[GunId].serviceBusyResendTime > 10U)
		{
			g_psPOS_data->POS_Transaction[GunId].serviceBusyResendTime = 0U;

			if(g_psPOS_data->POS_Transaction[GunId].serviceBusyCount < 2U)
			{
				g_psPOS_data->POS_Transaction[GunId].serviceBusyCount++;
				Uart_Dma_Send(POS_UART, sPos_JsonSendbuf.Jsonlen+19U, sPos_JsonSendbuf.JsonString);
			}
			else
			{
				g_psPOS_data->POS_Transaction[GunId].serviceBusy = FALSE;
				//把POS置于可用状态
				g_psPOS_data->POS_Transaction[GunId].serviceBusyFlag = FALSE;
				g_psPOS_data->POS_Transaction[GunId].serviceBusyResendTime = 0U;
			}
		}
	}
}

static BOOL GetCardInfo(void)
{
	cJSON *root = cJSON_CreateObject();

	if(root != NULL)
	{
		if(NULL != cJSON_AddStringToObject(root, "version", "2.00"))
		{
			cJSON *requestArray = cJSON_AddArrayToObject(root, "request");

			if(requestArray != NULL)
			{
				cJSON *request = cJSON_CreateObject();

				if(request != NULL)
				{
					if(NULL != cJSON_AddStringToObject(request, "command", "GetCardInfo"))
					{
						cJSON *properties = cJSON_AddObjectToObject(request, "properties");

						if(properties != NULL)
						{
							if(0 != (cJSON_AddStringToObject(properties, "timeout", "600") &&
							   cJSON_AddStringToObject(properties, "enableCardUniqueIdentifier", "Off")))
							{
								cJSON_AddItemToArray(requestArray, request);

								 // 将 JSON 对象转换为字符串并发送
								Pos_JSON_Send(root);

								cJSON_Delete(root);
								cJSON_MemOffset = 0;
								return TRUE;
							}

						}
						else
						{
							my_printf(USER_ERROR, "Error:%s:%d GetCardInfo creat properties object\n", __FILE__, __LINE__);
						}
					}
					else
					{
						my_printf(USER_ERROR, "Error:%s:%d GetCardInfo add command to object\n", __FILE__, __LINE__);
					}
				}
				else
				{
					my_printf(USER_ERROR, "Error:%s:%d GetCardInfo creat request object\n", __FILE__, __LINE__);
				}
			}
			else
			{
				my_printf(USER_ERROR, "Error:%s:%d GetCardInfo creat requestArray object\n", __FILE__, __LINE__);
			}
		}
		else
		{
			my_printf(USER_ERROR, "Error:%s:%d GetCardInfo add version to object\n", __FILE__, __LINE__);
		}

		cJSON_Delete(root);
		cJSON_MemOffset = 0;
		return FALSE;
	}
	return FALSE;
}

static void GetTerminalInfoParse(const cJSON *properties)
{
	U32 stringlen = 0;

	// 提取 properties 中的各个字段
	cJSON *responseCode = cJSON_GetObjectItem(properties, "responseCode");
	cJSON *responseMessage = cJSON_GetObjectItem(properties, "responseMessage");
	cJSON *sn = cJSON_GetObjectItem(properties, "sn");
	cJSON *modelName = cJSON_GetObjectItem(properties, "modelName");
	cJSON *osVersion = cJSON_GetObjectItem(properties, "osVersion");
	cJSON *appVersion = cJSON_GetObjectItem(properties, "appVersion");
	cJSON *appUpdateTimeStamp = cJSON_GetObjectItem(properties, "appUpdateTimeStamp");
	cJSON *ipAddress = cJSON_GetObjectItem(properties, "ipAddress");

	if ((0 != cJSON_IsString(responseCode)) && (responseCode->valuestring != NULL))
	{
		stringlen = strlen(responseCode->valuestring);

		if(stringlen <= 6U)
		{
			//先清缓存
			(void)memset(g_psPOS_data->POS_Transaction[GUN_A].responseCode,0,sizeof(g_psPOS_data->POS_Transaction[GUN_A].responseCode));

			(void)memcpy(g_psPOS_data->POS_Transaction[GUN_A].responseCode,responseCode->valuestring,stringlen);
		}

		g_psPOS_data->POS_Transaction[GUN_A].POS_Command = GETTERMINALINFO;
	}
	if ((0 != cJSON_IsString(responseMessage)) && (responseMessage->valuestring != NULL))
	{
		//my_printf(USER_INFO,"Response Message: %s\n", responseMessage->valuestring);
	}
	if ((0 != cJSON_IsString(sn)) && (sn->valuestring != NULL))
	{
		//my_printf(USER_INFO,"SN: %s\n", sn->valuestring);
	}
	if ((0 != cJSON_IsString(modelName)) && (modelName->valuestring != NULL))
	{
		//my_printf(USER_INFO,"Model Name: %s\n", modelName->valuestring);
	}
	if ((0 != cJSON_IsString(osVersion)) && (osVersion->valuestring != NULL))
	{
		//my_printf(USER_INFO,"OS Version: %s\n", osVersion->valuestring);
	}
	if ((0 != cJSON_IsString(appVersion)) && (appVersion->valuestring != NULL))
	{
		stringlen = strlen(appVersion->valuestring);
		if(stringlen <= 32U)
		{
			(void)memcpy(g_psPOS_data->POS_Transaction[GUN_A].appVersion,appVersion->valuestring,stringlen);
		}
	}
	if ((0 != cJSON_IsString(appUpdateTimeStamp)) && (appUpdateTimeStamp->valuestring != NULL))
	{
		//my_printf(USER_INFO,"App Update TimeStamp: %s\n", appUpdateTimeStamp->valuestring);
	}
	if ((0 != cJSON_IsString(ipAddress)) && (ipAddress->valuestring != NULL))
	{
		//my_printf(USER_INFO,"IP Address: %s\n", ipAddress->valuestring);
	}
}

static void GetTransactionStartParse(const cJSON *properties)
{
	U32 stringlen = 0;

	// 提取 properties 中的一些字段
	cJSON *responseCode = cJSON_GetObjectItem(properties, "responseCode");
	cJSON *responseMessage = cJSON_GetObjectItem(properties, "responseMessage");
	cJSON *hostReferenceNumber = cJSON_GetObjectItem(properties, "hostReferenceNumber");
	cJSON *hostData = cJSON_GetObjectItem(properties, "hostData");
	cJSON *account = cJSON_GetObjectItem(properties, "account");
	cJSON *timestamp = cJSON_GetObjectItem(properties, "timestamp");

	if (0 != cJSON_IsString(responseCode))
	{
		stringlen = strlen(responseCode->valuestring);

		if(stringlen <= 6U)
		{
			//先清缓存
			(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode));

			(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode,responseCode->valuestring,stringlen);
		}

		g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].POS_Command = TRANSACTIONSTART;
	}
	if (0 != cJSON_IsString(responseMessage))
	{
		stringlen = strlen(responseMessage->valuestring);
		if(stringlen <= 64U)
		{
			//先清缓存
			(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseMessage,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseMessage));

			(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseMessage,responseMessage->valuestring,stringlen);
		}
	}
	if (0 != cJSON_IsString(hostReferenceNumber))
	{
		stringlen = strlen(hostReferenceNumber->valuestring);

		if(stringlen <= 32U)
		{
			//先清缓存
			(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostReferenceNumber,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostReferenceNumber));

			(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostReferenceNumber,hostReferenceNumber->valuestring,stringlen);
		}
	}
	if (0 != cJSON_IsString(hostData))
	{
		stringlen = strlen(hostData->valuestring);

		if(stringlen <= 256U)
		{
			//先清缓存
			(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostData,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostData));

			(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostData,hostData->valuestring,stringlen);
		}
	}
	if (0 != cJSON_IsString(account))
	{
		stringlen = strlen(account->valuestring);

		if(stringlen <= 19U)
		{
			//先清缓存
			(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].account,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].account));

			(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].account,account->valuestring,stringlen);
		}
	}
	if (0 != cJSON_IsString(timestamp))
	{
		//my_printf(USER_INFO,"Timestamp: %s\n", timestamp->valuestring);
	}

	// 提取 accountInformation 对象
	cJSON *accountInformation = cJSON_GetObjectItem(properties, "accountInformation");
	if (0 != cJSON_IsObject(accountInformation))
	{
		cJSON *expireDate = cJSON_GetObjectItem(accountInformation, "expireDate");
		cJSON *cardType = cJSON_GetObjectItem(accountInformation, "cardType");
		cJSON *cardTypeName = cJSON_GetObjectItem(accountInformation, "cardTypeName");
		cJSON *cardUniqueIdentifier = cJSON_GetObjectItem(accountInformation, "cardUniqueIdentifier");

		if (0 != cJSON_IsString(expireDate))
		{
			//my_printf(USER_INFO,"Expire Date: %s\n", expireDate->valuestring);
		}
		if (0 != cJSON_IsString(cardType))
		{
			//my_printf(USER_INFO,"Card Type: %s\n", cardType->valuestring);
		}
		if (0 != cJSON_IsString(cardTypeName))
		{
			//my_printf(USER_INFO,"Card Type Name: %s\n", cardTypeName->valuestring);
		}
		if (0 != cJSON_IsString(cardUniqueIdentifier))
		{
			stringlen = strlen(cardUniqueIdentifier->valuestring);

			if(stringlen <= 64U)
			{
				(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].cardUniqueIdentifier,cardUniqueIdentifier->valuestring,stringlen);
			}
		}
	}

	// 提取 hostInformation 对象
	cJSON *hostInformation = cJSON_GetObjectItem(properties, "hostInformation");
	if (0 != cJSON_IsObject(hostInformation))
	{
		cJSON *token = cJSON_GetObjectItem(hostInformation, "token");
		if (0 != cJSON_IsString(token))
		{
			//my_printf(USER_INFO,"Token: %s\n", token->valuestring);
		}
	}

	// 提取 amountInformation 对象
	cJSON *amountInformation = cJSON_GetObjectItem(properties, "amountInformation");
	if (0 != cJSON_IsObject(amountInformation))
	{
		cJSON *approvedAmount = cJSON_GetObjectItem(amountInformation, "approvedAmount");
		if (0 != cJSON_IsString(approvedAmount))
		{
			stringlen = strlen(approvedAmount->valuestring);

			if(stringlen <= 9U)
			{
				//先清缓存
				(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].frozenAmount,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].frozenAmount));

				(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].frozenAmount,approvedAmount->valuestring,stringlen);
			}
		}
	}
}

static void GetTransactionCompletionParse(const cJSON *properties)
{
	U32 stringlen = 0;

	// 获取 properties 中的各个字段
	cJSON *responseCode = cJSON_GetObjectItem(properties, "responseCode");
	cJSON *responseMessage = cJSON_GetObjectItem(properties, "responseMessage");
	cJSON *hostReferenceNumber = cJSON_GetObjectItem(properties, "hostReferenceNumber");
	cJSON *hostData = cJSON_GetObjectItem(properties, "hostData");
	cJSON *timestamp = cJSON_GetObjectItem(properties, "timestamp");

	if ((0 != cJSON_IsString(responseCode)) && (responseCode->valuestring != NULL))
	{
		stringlen = strlen(responseCode->valuestring);

		if(stringlen <= 6U)
		{
			//先清缓存
			(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode));

			(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode,responseCode->valuestring,stringlen);
		}

		g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].POS_Command = TRANSACTIONCOMPLETION;
	}
	if ((0 != cJSON_IsString(responseMessage)) && (responseMessage->valuestring != NULL))
	{
		stringlen = strlen(responseMessage->valuestring);
		if(stringlen <= 64U)
		{
			//先清缓存
			(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseMessage,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseMessage));

			(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseMessage,responseMessage->valuestring,stringlen);
		}
	}
	if ((0 != cJSON_IsString(hostReferenceNumber)) && (hostReferenceNumber->valuestring != NULL))
	{
		stringlen = strlen(hostReferenceNumber->valuestring);

		if(stringlen <= 32U)
		{
			//先清缓存
			(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostReferenceNumber,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostReferenceNumber));

			(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostReferenceNumber,hostReferenceNumber->valuestring,stringlen);
		}
	}
	if ((0 != cJSON_IsString(hostData)) && (hostData->valuestring != NULL))
	{
		stringlen = strlen(hostData->valuestring);

		if(stringlen <= 256U)
		{
			//先清缓存
			(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostData,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostData));

			(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostData,hostData->valuestring,stringlen);
		}
	}
	if ((0 != cJSON_IsString(timestamp)) && (timestamp->valuestring != NULL))
	{
		//my_printf(USER_INFO,"timestamp: %s\n", timestamp->valuestring);
	}

	// 获取 amountInformation 对象
	cJSON *amountInformation = cJSON_GetObjectItem(properties, "amountInformation");
	if (0 != cJSON_IsObject(amountInformation))
	{
		cJSON *approvedAmount = cJSON_GetObjectItem(amountInformation, "approvedAmount");
		if ((0 != cJSON_IsString(approvedAmount)) && (approvedAmount->valuestring != NULL))
		{
			stringlen = strlen(approvedAmount->valuestring);

			if(stringlen <= 9U)
			{
				//先清缓存
				(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].approvedAmount,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].approvedAmount));

				(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].approvedAmount,approvedAmount->valuestring,stringlen);
			}
		}
	}
}

static void GetTransactionReversalParse(const cJSON *properties)
{
	U32 stringlen = 0;

	// 提取responseCode字段
	cJSON *responseCode = cJSON_GetObjectItem(properties, "responseCode");
	if ((0 != cJSON_IsString(responseCode)) && (responseCode->valuestring != NULL))
	{
		stringlen = strlen(responseCode->valuestring);

		if(stringlen <= 6U)
		{
			//先清缓存
			(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode));

			(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode,responseCode->valuestring,stringlen);
		}

		g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].POS_Command = TRANSACTIONREVERSAL;
	}

	// 提取responseMessage字段
	cJSON *responseMessage = cJSON_GetObjectItem(properties, "responseMessage");
	if ((0 != cJSON_IsString(responseMessage)) && (responseMessage->valuestring != NULL))
	{
		stringlen = strlen(responseMessage->valuestring);
		if(stringlen <= 64U)
		{
			//先清缓存
			(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseMessage,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseMessage));

			(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseMessage,responseMessage->valuestring,stringlen);
		}
	}
}

static void GetTransactionIncrementalParse(const cJSON *properties)
{
	U32 stringlen = 0;

	// 获取properties中的各个字段
	cJSON *responseCode = cJSON_GetObjectItem(properties, "responseCode");
	cJSON *responseMessage = cJSON_GetObjectItem(properties, "responseMessage");
	cJSON *hostReferenceNumber = cJSON_GetObjectItem(properties, "hostReferenceNumber");
	cJSON *hostData = cJSON_GetObjectItem(properties, "hostData");
	cJSON *timestamp = cJSON_GetObjectItem(properties, "timestamp");

	if ((0 != cJSON_IsString(responseCode)) && (responseCode->valuestring != NULL))
	{
		stringlen = strlen(responseCode->valuestring);

		if(stringlen <= 6U)
		{
			//先清缓存
			(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode));

			(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode,responseCode->valuestring,stringlen);
		}

		g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].POS_Command = TRANSACTIONINCREMENTAL;
	}
	if ((0 != cJSON_IsString(responseMessage)) && (responseMessage->valuestring != NULL))
	{
		stringlen = strlen(responseMessage->valuestring);
		if(stringlen <= 64U)
		{
			//先清缓存
			(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseMessage,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseMessage));

			(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseMessage,responseMessage->valuestring,stringlen);
		}
	}
	if ((0 != cJSON_IsString(hostReferenceNumber)) && (hostReferenceNumber->valuestring != NULL))
	{
		stringlen = strlen(hostReferenceNumber->valuestring);

		if(stringlen <= 32U)
		{
			//先清缓存
			(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostReferenceNumber,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostReferenceNumber));

			(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostReferenceNumber,hostReferenceNumber->valuestring,stringlen);
		}
	}
	if ((0 != cJSON_IsString(hostData)) && (hostData->valuestring != NULL))
	{
		stringlen = strlen(hostData->valuestring);

		if(stringlen <= 256U)
		{
			//先清缓存
			(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostData,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostData));

			(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].hostData,hostData->valuestring,stringlen);
		}
	}
	if ((0 != cJSON_IsString(timestamp)) && (timestamp->valuestring != NULL))
	{
		//my_printf(USER_INFO,"timestamp: %s\n", timestamp->valuestring);
	}

	cJSON *amountInformation = cJSON_GetObjectItem(properties, "amountInformation");
	if (0 != cJSON_IsObject(amountInformation))
	{
		cJSON *approvedAmount = cJSON_GetObjectItem(amountInformation, "approvedAmount");
		if ((0 != cJSON_IsString(approvedAmount)) && (approvedAmount->valuestring != NULL))
		{
			//my_printf(USER_INFO,"approvedAmount: %s\n", approvedAmount->valuestring);
		}
	}
}

static void GetCardInfoParse(const cJSON *properties)
{
	U32 stringlen = 0;

	// 获取响应码字段
	cJSON *responseCode = cJSON_GetObjectItem(properties, "responseCode");
	if (0 != cJSON_IsString(responseCode))
	{
		stringlen = strlen(responseCode->valuestring);

		if(stringlen <= 6U)
		{
			//先清缓存
			(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode));

			(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseCode,responseCode->valuestring,stringlen);
		}

		g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].POS_Command = GETCARD_INFO;
	}

	// 获取响应消息字段
	cJSON *responseMessage = cJSON_GetObjectItem(properties, "responseMessage");
	if (0 != cJSON_IsString(responseMessage))
	{
		stringlen = strlen(responseMessage->valuestring);
		if(stringlen <= 64U)
		{
			//先清缓存
			(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseMessage,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseMessage));

			(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].responseMessage,responseMessage->valuestring,stringlen);
		}
	}

	// 获取账户卡号字段
	cJSON *account = cJSON_GetObjectItem(properties, "account");
	if (0 != cJSON_IsString(account))
	{
		stringlen = strlen(account->valuestring);

		if(stringlen <= 19U)
		{
			//先清缓存
			(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].account,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].account));

			(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].account,account->valuestring,stringlen);
		}
	}

	// 获取账户信息对象
	cJSON *account_information = cJSON_GetObjectItem(properties, "accountInformation");
	if (0 != cJSON_IsObject(account_information))
	{
		// 获取过期日期字段
		cJSON *expire_date = cJSON_GetObjectItem(account_information, "expireDate");
		if (0 != cJSON_IsString(expire_date))
		{
			//my_printf(USER_INFO,"Expire Date: %s\n", expire_date->valuestring);
		}

		// 获取卡类型字段
		cJSON *card_type = cJSON_GetObjectItem(account_information, "cardType");
		if (0 != cJSON_IsString(card_type))
		{
			//my_printf(USER_INFO,"Card Type: %s\n", card_type->valuestring);
		}

		// 获取卡类型名称字段
		cJSON *card_type_name = cJSON_GetObjectItem(account_information, "cardTypeName");
		if (0 != cJSON_IsString(card_type_name))
		{
			//my_printf(USER_INFO,"Card Type Name: %s\n", card_type_name->valuestring);
		}

		// 获取卡唯一标识符字段
		cJSON *cardUniqueIdentifier = cJSON_GetObjectItem(account_information, "cardUniqueIdentifier");
		if (0 != cJSON_IsString(cardUniqueIdentifier))
		{
			stringlen = strlen(cardUniqueIdentifier->valuestring);

			if(stringlen <= 64U)
			{
				//先清缓存
				(void)memset(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].cardUniqueIdentifier,0,sizeof(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].cardUniqueIdentifier));

				(void)memcpy(g_psPOS_data->POS_Transaction[g_psPOS_data->GunId].cardUniqueIdentifier,cardUniqueIdentifier->valuestring,stringlen);
			}
		}
	}
}

/**
  * @brief 解析 JSON 字符串
  * @param json_string json字符串，unData_len 字符串长度
  */
static BOOL Pos_cjson_Parse(const U8 *json_string,U16 unData_len)
{
	BOOL blRet = TRUE;
	cJSON *root = cJSON_Parse(json_string);

	if (root != NULL)
	{
		// 检查解析是否成功，并提取数据
		cJSON *version = cJSON_GetObjectItem(root, "version");
		if ((0 != cJSON_IsString(version)) && (version->valuestring != NULL))
		{
			//my_printf(USER_INFO,"Version: %s\n", version->valuestring);
			// 获取 response 数组
			cJSON *response_array = cJSON_GetObjectItem(root, "response");
			if (0 != cJSON_IsArray(response_array))
			{
				// 数组里只有一个元素
				cJSON *response_object = cJSON_GetArrayItem(response_array, 0);
				if (response_object != NULL)
				{
					// 提取 command
					cJSON *command = cJSON_GetObjectItem(response_object, "command");
					if ((0 != cJSON_IsString(command)) && (command->valuestring != NULL))
					{
						if(strcmp(command->valuestring,"GetTerminalInfo") == 0)
						{
							//如果已经获取到版本号了，就不解析json了，只作为心跳
							if(strlen(g_psPOS_data->POS_Transaction[GUN_A].appVersion) == 0U)
							{
								// 获取 properties 对象
								cJSON *properties = cJSON_GetObjectItem(response_object, "properties");
								if (0 != cJSON_IsObject(properties))
								{
									GetTerminalInfoParse(properties);
								}
							}
							else
							{
								g_psPOS_data->POS_Transaction[GUN_A].POS_Command = GETTERMINALINFO;
							}
						}
						else if(strcmp(command->valuestring, "TransactionStart") == 0)
						{
							// 提取 properties 对象
							cJSON *properties = cJSON_GetObjectItem(response_object, "properties");
							if (0 != cJSON_IsObject(properties))
							{
								GetTransactionStartParse(properties);
							}
						}
						else if(strcmp(command->valuestring, "TransactionCompletion") == 0)
						{
							// 获取 properties 对象
							cJSON *properties = cJSON_GetObjectItem(response_object, "properties");
							if (0 != cJSON_IsObject(properties))
							{
								GetTransactionCompletionParse(properties);
							}
						}
						else if(strcmp(command->valuestring, "TransactionReversal") == 0)//取消交易
						{
							// 提取properties对象
							cJSON *properties = cJSON_GetObjectItem(response_object, "properties");
							if (0 != cJSON_IsObject(properties))
							{
								GetTransactionReversalParse(properties);
							}
						}
						else if(strcmp(command->valuestring, "TransactionIncremental") == 0)//增加金额到实际消费金额
						{
							// 获取properties对象
							cJSON *properties = cJSON_GetObjectItem(response_object, "properties");
							if (0 != cJSON_IsObject(properties))
							{
								GetTransactionIncrementalParse(properties);
							}
						}
						else if ((0 != cJSON_IsString(command)) && (strcmp(command->valuestring, "GetCardInfo") == 0))
						{
							// 获取属性对象
							cJSON *properties = cJSON_GetObjectItem(response_object, "properties");
							if (0 != cJSON_IsObject(properties))
							{
								GetCardInfoParse(properties);
							}
						}
						else
						{
							my_printf(USER_ERROR,"Error%s:%d unknow command\n", __FILE__, __LINE__);
							blRet = FALSE;
						}
					}
					else
					{
						my_printf(USER_ERROR,"Error%s:%d parsing command\n", __FILE__, __LINE__);
						blRet = FALSE;
					}
				}
				else
				{
					my_printf(USER_ERROR,"Error%s:%d parsing response object\n", __FILE__, __LINE__);
					blRet = FALSE;
				}
			}
			else
			{
				my_printf(USER_ERROR,"Error%s:%d parsing response array\n", __FILE__, __LINE__);
				blRet = FALSE;
			}
		}
		else
		{
			my_printf(USER_ERROR,"Error%s:%d parsing version\n", __FILE__, __LINE__);
			blRet = FALSE;
		}

		cJSON_Delete(root);
		cJSON_MemOffset = 0;
	}
	else
	{
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL)
		{
			my_printf(USER_ERROR, "Error:%s:%d Pare creat root object\n", __FILE__, __LINE__);
			blRet = FALSE;
		}
	}

	return blRet;
}

U16 DecimalFromString(const U8 *str)
{
    U16 u16decimal = 0;
    while ((*str) != 0)
    {
    	u16decimal = (u16decimal * 10U) + (*str - (U8)'0');
        str++;
    }
    return u16decimal;
}

static void BfPosHeartRequest(void)
{
	//心跳
	sPos_JsonSendbuf.FixedTimeGetTerminalInfo++;

	if(sPos_JsonSendbuf.FixedTimeGetTerminalInfo > 3U)
	{
		if(sPos_HostState == POS_STATE_IDLE)
		{
			sPos_JsonSendbuf.FixedTimeGetTerminalInfo=0;
			Uart_Dma_Send(POS_UART, 79 , g_u8GetTerminalInfoJsonBody);//命令固定，字符串直接写在数组里
			sPos_HostState=POS_STATE_TX;

			g_psPOS_data->bEnable_heart_flag = TRUE;

			//Pos_cjson_GetTerminalInfo();//用cJSON的方式生成字符串
		//	my_printf(USER_INFO," PosHeartRequest\n");
		}
		else
		{
			//my_printf(USER_INFO," Pos Busy Heart Request Fail\n");
		}
	}
}

static void BfPosParse(const U8 *buf, U16 unData_len)
{
	U8 u8Checksum=0;
	U8 u8PosACK[1] = {0x06},u8PosNAK[1] = {0x15};

	CHECK_MSG_LEN_NO_RETURN(UART_COMM_BUF_LEN, unData_len + 18U);
	CHECK_PTR_NULL_NO_RETURN(buf);
	CHECK_MSG_LEN_NO_RETURN(UART_COMM_BUF_LEN, unData_len);

	//解析
	sPos_JsonSendbuf.RxTimeOut=0;
	sPos_JsonSendbuf.ErrTimes=0;

//	uPRINTF("JX:");
//	for(U8 i = 0;i< unData_len;i++)
//	{
//		uPRINTF("%c ",buf[i]);
//	}
//	uPRINTF("\n");

	if((buf[0] == 0x02U) && ((S8)buf[1] == 'P') && ((S8)buf[2] == 'J'))//POS机响应报文
	{
		u8Checksum=Pos_Checksum(&buf[1],unData_len + 17U);
		if(u8Checksum == buf[unData_len + 18U])
		{
			//如果校验正确在这里需要给POS机回复ACK
			Uart_Dma_Send(POS_UART, 1 , u8PosACK);
			sPos_HostState=POS_STATE_RX;//接收到响应后回复ACK，然后等待EOT
			sPos_JsonSendbuf.EotTimeOut=0;

			(void)Pos_cjson_Parse(&buf[18],unData_len);//解析JSON
		}
		else
		{
			//如果校验失败，在这里需要给POS机回复NAK,POS机会重发
			POS_UartRecBuffer.u16ReadMsgDataNum=0;
			POS_UartRecBuffer.u8ReadMsgBeginFlag=0;
			POS_UartRecBuffer.u16DealPos=0;
			POS_UartRecBuffer.u16ReachPos=0;
			Uart_Dma_Send(POS_UART, 1 , u8PosNAK);
			//my_printf(USER_INFO, "ToPOS NAK \n");
		}
	}
	else if(buf[0] == ACK)//发送请求后，收到pos机的ACK，表示POS收到的数据正确，等待响应报文
	{
		sPos_JsonSendbuf.RequestCount=0;
		sPos_JsonSendbuf.ResponseTimeOut=0;
		sPos_HostState=POS_STATE_TX_END;
	}
	else if(buf[0] == EOT)//EOT
	{
		//如果收到了EOT,代表POS收到了CCU发送的ACK,本包交互完成
		(void)memset(&sPos_JsonSendbuf,0,sizeof(sPos_JsonSendbuf));

		sPos_HostState=POS_STATE_IDLE;
	}
	else if(buf[0] == NAK)//NAK
	{
		sPos_JsonSendbuf.RequestCount++;
		if(sPos_JsonSendbuf.RequestCount < 3U)
		{
			POS_UartRecBuffer.u16DealPos=0;
			POS_UartRecBuffer.u16ReachPos=0;
			//发送请求后，收到pos机的NAK，表示POS收到的数据有错误，给pos机重发请求，最多重发3次
			if (sizeof(sPos_JsonSendbuf.JsonString) >= (sPos_JsonSendbuf.Jsonlen+19U))
			{
				Uart_Dma_Send(POS_UART, sPos_JsonSendbuf.Jsonlen+19U, sPos_JsonSendbuf.JsonString);
			}
		}
		else
		{
			sPos_JsonSendbuf.RequestCount=0;
		}
	}
	else
	{
		//
	}
}

POS_data_t* GetBfPosModel(void)
{
	return &g_sBF_POS;
}

void BfPosInit(void)
{
	g_sBF_POS.PosDataParse = &BfPosParse;
	g_sBF_POS.SendPosHeartRequest = &BfPosHeartRequest;
	g_sBF_POS.Reboot = &BfPosReboot;
}
