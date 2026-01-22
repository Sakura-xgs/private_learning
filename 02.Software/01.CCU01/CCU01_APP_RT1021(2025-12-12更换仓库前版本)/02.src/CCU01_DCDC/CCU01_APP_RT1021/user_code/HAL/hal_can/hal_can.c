/*
 * hal_can.c
 *
 *  Created on: 2024年8月23日
 *      Author: Bono
 */
#include "fsl_debug_console.h"
#include "fsl_flexcan.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_flexcan.h"
#include "hal_can.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "hal_can.h"
#include "hal_can_IF.h"
#include "PublicDefine.h"
#include "peripherals.h"



/*******************************************************************************
 * Variables
 ******************************************************************************/
QueueHandle_t g_Can1_RecData_xQueue = NULL;
QueueHandle_t g_Can2_RecData_xQueue = NULL;

static SemaphoreHandle_t g_Can1_SendData_MutexSemaphore = NULL;
static SemaphoreHandle_t g_Can2_SendData_MutexSemaphore = NULL;

static flexcan_handle_t CAN1_Handle = {0};
static flexcan_handle_t CAN2_Handle = {0};

static flexcan_mb_transfer_t g_Can1_rxXfer[CAN1_REC_MB_NUM] = {0};
static flexcan_mb_transfer_t g_Can2_rxXfer[CAN2_REC_MB_NUM] = {0};

/*******************************************************************************
 * Code
 ******************************************************************************/
status_t ChgACanSendFrame(flexcan_frame_t *frame)
{
	return SendCANData(CAN1, frame);
}

status_t ChgBCanSendFrame(flexcan_frame_t *frame)
{
	return SendCANData(CAN2, frame);
}

status_t SendCANData(CAN_Type *base, flexcan_frame_t *frame)
{
	status_t result = false;
	flexcan_mb_transfer_t txXfer = {0};
	static U8 u8Can1MailBoxNo = CAN1_SEND_MB_BEG_NO, u8Can2MailBoxNo = CAN2_SEND_MB_BEG_NO;

	txXfer.frame = frame;

	if(base == CAN1)
	{
		(void)xSemaphoreTake(g_Can1_SendData_MutexSemaphore, portMAX_DELAY);

		txXfer.mbIdx = u8Can1MailBoxNo;
		result = FLEXCAN_TransferSendNonBlocking(base, &CAN1_Handle, &txXfer);
		u8Can1MailBoxNo = (u8Can1MailBoxNo < CAN1_SEND_MB_END_NO) ? (u8Can1MailBoxNo+1U) : (CAN1_SEND_MB_BEG_NO);

		(void)xSemaphoreGive(g_Can1_SendData_MutexSemaphore);
	}
	else if(base == CAN2)
	{
		(void)xSemaphoreTake(g_Can2_SendData_MutexSemaphore, portMAX_DELAY);

		txXfer.mbIdx = u8Can2MailBoxNo;
		result = FLEXCAN_TransferSendNonBlocking(base, &CAN2_Handle, &txXfer);
		u8Can2MailBoxNo = (u8Can2MailBoxNo < CAN2_SEND_MB_END_NO) ? (u8Can2MailBoxNo+1U) : (CAN2_SEND_MB_BEG_NO);

		(void)xSemaphoreGive(g_Can2_SendData_MutexSemaphore);
	}
	else
	{
		//不响应
	}

    return result;
}

/*!
 * @brief FlexCAN Call Back function
 */
static void Chg_A_CAN_Callback(CAN_Type *base, flexcan_handle_t *handle, status_t status, uint32_t result, void *userData)
{
	BaseType_t xHigherPriorityTaskWoken;

	switch (status)
    {
		case kStatus_FLEXCAN_RxIdle:
			if((result >= CAN1_REC_MB_BEG_NO) && (result <= CAN1_REC_MB_END_NO))
			{
				if(FLEXCAN_TransferReceiveNonBlocking(base, handle, &g_Can1_rxXfer[result - CAN1_REC_MB_BEG_NO]) == kStatus_Success)
				{
					(void)xQueueSendFromISR(g_Can1_RecData_xQueue, g_Can1_rxXfer[result - CAN1_REC_MB_BEG_NO].frame, &xHigherPriorityTaskWoken);

					/* make a task switch if necessary. */
					portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
				}
			}
            break;
        case kStatus_FLEXCAN_RxOverflow:
			if((result >= CAN1_REC_MB_BEG_NO) && (result <= CAN1_REC_MB_END_NO))
			{
				if(FLEXCAN_TransferReceiveNonBlocking(base, handle, &g_Can1_rxXfer[result - CAN1_REC_MB_BEG_NO]) == kStatus_Success)
				{
					(void)xQueueSendFromISR(g_Can1_RecData_xQueue, g_Can1_rxXfer[result - CAN1_REC_MB_BEG_NO].frame, &xHigherPriorityTaskWoken);

					/* make a task switch if necessary. */
					portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
				}
			}
        	break;
        case kStatus_FLEXCAN_TxIdle:
        case kStatus_FLEXCAN_TxSwitchToRx:
            break;

        default:
        	//
            break;
    }
}

static void Chg_B_CAN_Callback(CAN_Type *base, flexcan_handle_t *handle, status_t status, uint32_t result, void *userData)
{
	BaseType_t xHigherPriorityTaskWoken;

    switch (status)
    {
        case kStatus_FLEXCAN_RxIdle:

        	if((result >= CAN2_REC_MB_BEG_NO) && (result <= CAN2_REC_MB_END_NO))
			{
            	if(FLEXCAN_TransferReceiveNonBlocking(base, handle, &g_Can2_rxXfer[result - CAN2_REC_MB_BEG_NO]) == kStatus_Success)
    			{
    				(void)xQueueSendFromISR(g_Can2_RecData_xQueue, g_Can2_rxXfer[result - CAN2_REC_MB_BEG_NO].frame, &xHigherPriorityTaskWoken);

    				/* make a task switch if necessary. */
    				portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    			}
			}
            break;
        case kStatus_FLEXCAN_RxOverflow:
        	if((result >= CAN2_REC_MB_BEG_NO) && (result <= CAN2_REC_MB_END_NO))
			{
            	if(FLEXCAN_TransferReceiveNonBlocking(base, handle, &g_Can2_rxXfer[result - CAN2_REC_MB_BEG_NO]) == kStatus_Success)
    			{
    				(void)xQueueSendFromISR(g_Can2_RecData_xQueue, g_Can2_rxXfer[result - CAN2_REC_MB_BEG_NO].frame, &xHigherPriorityTaskWoken);

    				/* make a task switch if necessary. */
    				portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    			}
			}
        	break;
        case kStatus_FLEXCAN_TxIdle:
        case kStatus_FLEXCAN_TxSwitchToRx:
            break;

        default:
        	//
            break;
    }
}

void Hal_Can_Init(void)
{
	static flexcan_frame_t g_Can1_RecFrame[CAN1_REC_MB_NUM] = {0};
	static flexcan_frame_t g_Can2_RecFrame[CAN2_REC_MB_NUM] = {0};

	U8 u8RecMbId = 0;

	g_Can1_RecData_xQueue = xQueueCreate(300, sizeof(flexcan_frame_t));
	g_Can2_RecData_xQueue = xQueueCreate(300, sizeof(flexcan_frame_t));

	g_Can1_SendData_MutexSemaphore = xSemaphoreCreateMutex();
	g_Can2_SendData_MutexSemaphore = xSemaphoreCreateMutex();

	/* Create FlexCAN handle structure and set call back function. */
	FLEXCAN_TransferCreateHandle(CHARGE_A_CAN, &CAN1_Handle, &Chg_A_CAN_Callback, NULL);
	FLEXCAN_TransferCreateHandle(CHARGE_B_CAN, &CAN2_Handle, &Chg_B_CAN_Callback, NULL);

	FLEXCAN_SetRxMbGlobalMask(CHARGE_A_CAN, FLEXCAN_RX_MB_EXT_MASK(0,0,0));
	FLEXCAN_SetRxMbGlobalMask(CHARGE_B_CAN, FLEXCAN_RX_MB_EXT_MASK(0,0,0));

	(void)EnableIRQ(CHARGE_CAN_A_FLEXCAN_IRQN);
	(void)EnableIRQ(CHARGE_CAN_B_FLEXCAN_IRQN);

	for(u8RecMbId = CAN1_REC_MB_BEG_NO; u8RecMbId < CAN1_REC_MB_END_NO; u8RecMbId++)
	{
		g_Can1_rxXfer[u8RecMbId - CAN1_REC_MB_BEG_NO].mbIdx = u8RecMbId;
		g_Can1_rxXfer[u8RecMbId - CAN1_REC_MB_BEG_NO].frame = &g_Can1_RecFrame[u8RecMbId - CAN1_REC_MB_BEG_NO];
		(void)FLEXCAN_TransferReceiveNonBlocking(CHARGE_A_CAN, &CAN1_Handle, &g_Can1_rxXfer[u8RecMbId - CAN1_REC_MB_BEG_NO]);
	}

	for(u8RecMbId = CAN2_REC_MB_BEG_NO; u8RecMbId < CAN2_REC_MB_END_NO; u8RecMbId++)
	{
		g_Can2_rxXfer[u8RecMbId - CAN2_REC_MB_BEG_NO].mbIdx = u8RecMbId;
		g_Can2_rxXfer[u8RecMbId - CAN2_REC_MB_BEG_NO].frame = &g_Can2_RecFrame[u8RecMbId - CAN2_REC_MB_BEG_NO];
		(void)FLEXCAN_TransferReceiveNonBlocking(CHARGE_B_CAN, &CAN2_Handle, &g_Can2_rxXfer[u8RecMbId - CAN2_REC_MB_BEG_NO]);
	}
}

