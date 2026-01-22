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
#include "can_fifo.h"
#include "hal_can.h"
#include "hal_can_IF.h"
#include "peripherals.h"
#include "pdu_can.h"
#include "SignalManage.h"



/*******************************************************************************
 * Variables
 ******************************************************************************/
SemaphoreHandle_t g_CanUpdata_Binary_Semaphore = NULL;

QueueHandle_t g_Can1_RecData_xQueue = NULL;
QueueHandle_t g_Can2_RecData_xQueue = NULL;

SemaphoreHandle_t g_Can1_SendData_MutexSemaphore = NULL;
SemaphoreHandle_t g_Can2_SendData_MutexSemaphore = NULL;

flexcan_handle_t CAN1_Handle = {0};
flexcan_handle_t CAN2_Handle = {0};

flexcan_mb_transfer_t g_Can1_rxXfer[CAN1_REC_MB_NUM] = {0};
flexcan_mb_transfer_t g_Can2_rxXfer[CAN2_REC_MB_NUM] = {0};

flexcan_frame_t g_Can1_RecFrame[CAN1_REC_MB_NUM] = {0};
flexcan_frame_t g_Can2_RecFrame[CAN2_REC_MB_NUM] = {0};

static U8 u8Can1MailBoxNo = CAN1_SEND_MB_BEG_NO, u8Can2MailBoxNo = CAN2_SEND_MB_BEG_NO;
/*******************************************************************************
 * Code
 ******************************************************************************/
void CAN_8_DATA_COPY(const U8 data[8], flexcan_frame_t *Frame)
{
    Frame->dataByte0 = data[0];
    Frame->dataByte1 = data[1];
    Frame->dataByte2 = data[2];
    Frame->dataByte3 = data[3];
    Frame->dataByte4 = data[4];
    Frame->dataByte5 = data[5];
    Frame->dataByte6 = data[6];
    Frame->dataByte7 = data[7];
}

status_t PduCanSendFrame(flexcan_frame_t *frame)
{
	return SendCANData(CAN1, frame);
}

status_t SendCANData(CAN_Type *base, flexcan_frame_t *frame)
{
	status_t result = false;
	flexcan_mb_transfer_t txXfer = {0};

	txXfer.frame = frame;

	if(base == CAN1)
	{
		xSemaphoreTake(g_Can1_SendData_MutexSemaphore, portMAX_DELAY);

		txXfer.mbIdx = u8Can1MailBoxNo;
		result = FLEXCAN_TransferSendNonBlocking(base, &CAN1_Handle, &txXfer);
		u8Can1MailBoxNo = (u8Can1MailBoxNo < CAN1_SEND_MB_END_NO) ? (u8Can1MailBoxNo+1) : (CAN1_SEND_MB_BEG_NO);

		xSemaphoreGive(g_Can1_SendData_MutexSemaphore);
	}
	else if(base == CAN2)
	{
		xSemaphoreTake(g_Can2_SendData_MutexSemaphore, portMAX_DELAY);

		txXfer.mbIdx = u8Can2MailBoxNo;
		result = FLEXCAN_TransferSendNonBlocking(base, &CAN2_Handle, &txXfer);
		u8Can2MailBoxNo = (u8Can2MailBoxNo < CAN2_SEND_MB_END_NO) ? (u8Can2MailBoxNo+1) : (CAN2_SEND_MB_BEG_NO);

		xSemaphoreGive(g_Can2_SendData_MutexSemaphore);
	}

    return result;
}

/*!
 * @brief FlexCAN Call Back function
 */
static void CAN1_IRQ_Callback(CAN_Type *base, flexcan_handle_t *handle, status_t status, uint32_t result, void *userData)
{
	BaseType_t xHigherPriorityTaskWoken;

	switch (status)
    {
		case kStatus_FLEXCAN_RxOverflow:
		case kStatus_FLEXCAN_RxIdle:
			if((result >= CAN1_REC_MB_BEG_NO) && (result <= CAN1_REC_MB_END_NO))
			{
				if(FLEXCAN_TransferReceiveNonBlocking(base, handle, &g_Can1_rxXfer[result - CAN1_REC_MB_BEG_NO]) == kStatus_Success)
				{
					if(g_Can1_rxXfer[result - CAN1_REC_MB_BEG_NO].frame == null)
					{
						break;
					}

					if(TRUE == AddFrameToCan1UpdataRxBuf(g_Can1_rxXfer[result - CAN1_REC_MB_BEG_NO].frame))
					{
				        if((NULL != g_CanUpdata_Binary_Semaphore)
				        && (TRUE == g_blUpdataRecFinishFg))
				        {
				            /* release binary semaphores. */
				            xSemaphoreGiveFromISR(g_CanUpdata_Binary_Semaphore, &xHigherPriorityTaskWoken);

				            /* make a task switch if necessary. */
				            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
				        }
					}
					else
					{
						(void)xQueueSendFromISR(g_Can1_RecData_xQueue, g_Can1_rxXfer[result - CAN1_REC_MB_BEG_NO].frame, &xHigherPriorityTaskWoken);

						/* make a task switch if necessary. */
						portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
					}
				}
			}
            break;
        case kStatus_FLEXCAN_TxIdle:
        case kStatus_FLEXCAN_TxSwitchToRx:
            break;

        default:
            break;
    }
}

static void CAN2_IRQ_Callback(CAN_Type *base, flexcan_handle_t *handle, status_t status, uint32_t result, void *userData)
{
	BaseType_t xHigherPriorityTaskWoken;

    switch (status)
    {
        case kStatus_FLEXCAN_RxIdle:

        	if((result >= CAN2_REC_MB_BEG_NO) && (result <= CAN2_REC_MB_END_NO))
			{
            	if(FLEXCAN_TransferReceiveNonBlocking(base, handle, &g_Can2_rxXfer[result - CAN2_REC_MB_BEG_NO]) == kStatus_Success)
    			{
            		if(g_Can2_rxXfer[result - CAN2_REC_MB_BEG_NO].frame == null)
					{
						break;
					}

    				(void)xQueueSendFromISR(g_Can2_RecData_xQueue, g_Can2_rxXfer[result - CAN2_REC_MB_BEG_NO].frame, &xHigherPriorityTaskWoken);

    				/* make a task switch if necessary. */
    				portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    			}
			}
            break;

        case kStatus_FLEXCAN_TxIdle:
        case kStatus_FLEXCAN_TxSwitchToRx:
            break;

        case kStatus_FLEXCAN_ErrorStatus:
			break;

        default:
            break;
    }
}

void Hal_Can_Init(void)
{
	U8 u8RecMbId = 0;
	U32 u32RxFilterId = 0;
	S32 s32PduId = 0;
	flexcan_rx_mb_config_t mbConfig;

	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32PduId);

	g_Can1_RecData_xQueue = xQueueCreate(300, sizeof(flexcan_frame_t));
	g_Can2_RecData_xQueue = xQueueCreate(300, sizeof(flexcan_frame_t));
	g_Can1_SendData_MutexSemaphore = xSemaphoreCreateMutex();
	g_Can2_SendData_MutexSemaphore = xSemaphoreCreateMutex();
	g_CanUpdata_Binary_Semaphore = xSemaphoreCreateBinary();

	/* Create FlexCAN handle structure and set call back function. */
	FLEXCAN_TransferCreateHandle(CHARGE_A_CAN, &CAN1_Handle, CAN1_IRQ_Callback, NULL);

	mbConfig.type = kFLEXCAN_FrameTypeData;
	mbConfig.format = kFLEXCAN_FrameFormatExtend;
	for(u8RecMbId = CAN1_REC_MB_BEG_NO; u8RecMbId < CAN1_REC_MB_END_NO; u8RecMbId++)
	{
		/* 前一半用于接收目标地址为自己 */
		if(u8RecMbId <= CAN1_REC_MB_BEG_NO + ((CAN1_REC_MB_END_NO - CAN1_REC_MB_BEG_NO)/2))
		{
			u32RxFilterId = s32PduId<<8;
			mbConfig.id = FLEXCAN_ID_EXT(u32RxFilterId);
			FLEXCAN_SetRxIndividualMask(CHARGE_A_CAN, u8RecMbId, FLEXCAN_ID_EXT(0x0000FF00));
			FLEXCAN_SetRxMbConfig(CHARGE_A_CAN, u8RecMbId, &mbConfig, true);
		}
		else/* 后一半用于接收目标地址为广播 */
		{
			u32RxFilterId = BOARDCAST_ADDR<<8;
		    mbConfig.id = FLEXCAN_ID_EXT(u32RxFilterId);
		    FLEXCAN_SetRxIndividualMask(CHARGE_A_CAN, u8RecMbId, FLEXCAN_ID_EXT(0x0000FF00));
		    FLEXCAN_SetRxMbConfig(CHARGE_A_CAN, u8RecMbId, &mbConfig, true);
		}
	}

	EnableIRQ(PDU_CAN_FLEXCAN_IRQN);

	for(u8RecMbId = CAN1_REC_MB_BEG_NO; u8RecMbId < CAN1_REC_MB_END_NO; u8RecMbId++)
	{
		g_Can1_rxXfer[u8RecMbId - CAN1_REC_MB_BEG_NO].mbIdx = u8RecMbId;
		g_Can1_rxXfer[u8RecMbId - CAN1_REC_MB_BEG_NO].frame = &g_Can1_RecFrame[u8RecMbId - CAN1_REC_MB_BEG_NO];
		FLEXCAN_TransferReceiveNonBlocking(CHARGE_A_CAN, &CAN1_Handle, &g_Can1_rxXfer[u8RecMbId - CAN1_REC_MB_BEG_NO]);
	}
}

