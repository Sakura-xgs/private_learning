/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* FreeRTOS kernel includes. */
#include <APP/pdu_can/pdu_can.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

/* Freescale includes. */
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_xbara.h"
#include "board.h"
#include "peripherals.h"

/*app includes*/
#include "hal_can_IF.h"
#include "hal_uart_IF.h"
#include "hal_eeprom_IF.h"
#include "hal_sys_IF.h"
#include "boot.h"
#include "uart_comm.h"
#include "relay_ctrl.h"
#include "poll_adc.h"
#include "data_sample.h"
#include "cr_section_macros.h"
#include "SignalManage.h"
#include "PublicDefine.h"
#include "hal/factory_test/factory_test.h"


/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void Led_Task(void *pvParameters);
static void Init_Task(void * pvParameters);

/*******************************************************************************
 * Version Info
 ******************************************************************************/
__RODATA(APP_VERSION) volatile const SOFT_VER cst_sw_no =
{
	_LINK_T(PDU01, A, D, D, 25, B, 24),
    _SW_NUM(N01),
};

const HARD_VER cst_hw_no =
{
    'P','D','U','0','1','_','A','D','D','_','V','1','.','0',' ',' '
};

/*******************************************************************************
 * Variables
 ******************************************************************************/
BOOL g_blAppMallocFailedFg = FALSE;
BOOL g_blAppStackOverFlowFg = FALSE;
U8 g_u8AppStackOverFlowNameBuf[STACK_OVER_FLOW_BUF_LEN][configMAX_TASK_NAME_LEN] = {0};

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Application entry point.
 */
int main(void)
{
    /* Init MCU hardware. */
	BOARD_ConfigMPU();
	BOARD_InitBootPins();
    BOARD_InitBootClocks();
    Hal_InitEnetModuleClock();
    XBARA_Init(XBARA);

    /* Peripheral INIT */
	BOARD_InitBootPeripherals();

	/* 识别硬件板号用于初始化外设驱动 */
	Board_Addr_Init();

    /* HAL Init */
    Hal_Can_Init();
    Hal_Uart_Init();
    Hal_I2c_Init();
    Hal_Norflash_Init();

    /* App Init */
    Boot_Init();

    /* FreeRtos Init */
	xTaskCreate(Save_Eeprom_Task,           "SAVE_EEPROM",          configMINIMAL_STACK_SIZE*2,   	NULL,   SAVE_EEPROM_TASK_PRIO,      NULL);
    xTaskCreate(Init_Task,					"INIT",					configMINIMAL_STACK_SIZE*2,		NULL,	INIT_TASK_PRIO,				NULL);
	xTaskCreate(Led_Task,                   "LED",                  configMINIMAL_STACK_SIZE/2, 	NULL,   LED_TASK_PRIO,            	NULL);

    vTaskStartScheduler();
    for (;;){};
}

static void Init_Task(void * pvParameters)
{
	volatile UBaseType_t uxHighWaterMark = 0;
    g_Factory_Test_Msg_xQueue = xQueueCreate(300, sizeof(flexcan_frame_t));

	vTaskDelay(20);	/* 等待EEPROM线程创建完成 */
    InitSigVal();
    InitRelayCtrlTimes();

	taskENTER_CRITICAL();

    xTaskCreate(UpdataFuncInitTask,      	"UPDATE_INIT",          configMINIMAL_STACK_SIZE,     	NULL,   INIT_TASK_PRIO,            	NULL);
    xTaskCreate(SetSignalValInit_Task,      "SIGNAL_INIT",          configMINIMAL_STACK_SIZE,     	NULL,   INIT_TASK_PRIO,            	NULL);
    xTaskCreate(DataSample_Init_task,       "SAMPLE_INIT",          configMINIMAL_STACK_SIZE,   	NULL,   INIT_TASK_PRIO,           	NULL);
    xTaskCreate(Pdu_Can_Comm_Init_task,  	"PDU_CAN_COMM_INIT",    configMINIMAL_STACK_SIZE,   	NULL,   INIT_TASK_PRIO,         	NULL);
    xTaskCreate(Relay_Ctrl_Init_task,       "RELAY_CATL",          	configMINIMAL_STACK_SIZE,   	NULL,   INIT_TASK_PRIO,    			NULL);
    xTaskCreate(Uart_Init_Task,           	"UART_INIT",          	configMINIMAL_STACK_SIZE,   	NULL,   INIT_TASK_PRIO,    			NULL);
    xTaskCreate(Poll_Adc_Init_Task,         "POLL_ADC_Init",        configMINIMAL_STACK_SIZE,   	NULL,   INIT_TASK_PRIO,             NULL);
    xTaskCreate(Factory_Test_Init_task,     "FACTORY_TEST_INIT",    configMINIMAL_STACK_SIZE,   	NULL,   INIT_TASK_PRIO,    			NULL);

    vTaskDelete(NULL);

    taskEXIT_CRITICAL();
}

/*!
 * @brief Task responsible for printing of "Hello world." message.
 */
static void Led_Task(void *pvParameters)
{
	U16 u16Life = 0u;
	g_LedTaskSync_Binary_Semaphore = xSemaphoreCreateBinary();

    for (;;)
    {
    	LedBlinkFunc();
    	vLedTaskDelay(1000);
        u16Life++;
    }
}

void vApplicationStackOverflowHook( TaskHandle_t xTask, char * pcTaskName )
{
    static U8 u8BufLoc = 0;
    U8 u8AppNameCnt = 0;

    g_blAppStackOverFlowFg = TRUE;

    for (u8AppNameCnt = 0; u8AppNameCnt < configMAX_TASK_NAME_LEN; u8AppNameCnt++)
    {
        g_u8AppStackOverFlowNameBuf[u8BufLoc][u8AppNameCnt] = pcTaskName[u8AppNameCnt];
    }

    u8BufLoc = (u8BufLoc+1)%STACK_OVER_FLOW_BUF_LEN;
}

void vApplicationMallocFailedHook( void )
{
    g_blAppMallocFailedFg = TRUE;
}
