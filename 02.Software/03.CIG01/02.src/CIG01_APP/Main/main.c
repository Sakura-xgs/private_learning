#include "gd32f4xx.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board.h"
#include "boot.h"
#include "led.h"
#include "uart_comm.h"
#include "SignalManage.h"
#include "factory.h"
#include "ext_eeprom.h"
#include "data_sample.h"
#include "boot.h"

#include "hal_rtc.h"
#include "hal_uart.h"
#include "hard_i2c.h"
#include "hal_adc.h"


#define INIT_TASK_PRIO                  ( tskIDLE_PRIORITY + 1 )
#define LED_TASK_PRIO                   ( tskIDLE_PRIORITY + 1 )
#define DATA_SAMPLE_TASK_PRIO           ( tskIDLE_PRIORITY + 3 )
#define FACTORY_TASK_PRIO               ( tskIDLE_PRIORITY + 2 )
#define SAVE_EEPROM_TASK_PRIO           ( tskIDLE_PRIORITY + 4 )


/***************** 版本号***************/

//软件版本号
//格式
//项目编号+年份+月份+日
//测试版本.001
/*
LV N G 096 22 3 04
*/
const SOFT_VER cst_sw_no =
{
    _LINK_T(CIG01, A, D, D, 25, 9, 26),  
    _SW_NUM(N02),
};

//硬件版本号
const HARD_VER cst_hw_no =
{
    'C','I','G','0','1','_','A','D','D','_','V','1','.','0',' ',' '
};
/***************** 版本号***************/


/******************堆栈溢出检测***************************** */
#define STACK_OVER_FLOW_BUF_LEN 5

U32 g_u32GetFreeHeapSize = 0;           //堆栈中剩余空间（bytes）的大小
BOOL g_blAppMallocFailedFg = FALSE;     //任务malloc失败标志
BOOL g_blAppStackOverFlowFg = FALSE;    //任务堆溢出异常标志
U8 g_u8AppStackOverFlowNameBuf[STACK_OVER_FLOW_BUF_LEN][configMAX_TASK_NAME_LEN] = {0};
U8 g_u8HardWareVersion = 0;
/******************堆栈溢出检测***************************** */


void InitSwVersion(void)
{
	U8 i = 0;
	S32 s32SetVal = 0;

	for(i = 0; i < 6; i++)
	{
		s32SetVal = (((S32)cst_sw_no.sw_major_version[2*i])<<8) | ((S32)cst_sw_no.sw_major_version[2*i+1]);
		(void)SetSigVal(MBMS_SAM_SIG_ID_SOFTWARE_VERSION_1 + i, s32SetVal);
	}

    s32SetVal = (S32)cst_sw_no.sw_major_version[12]<<8 | (S32)cst_sw_no.sw_minor_version[0];
    SetSigVal(MBMS_SAM_SIG_ID_SOFTWARE_VERSION_7, s32SetVal);

    s32SetVal = (S32)cst_sw_no.sw_minor_version[1]<<8 | (S32)cst_sw_no.sw_minor_version[2];
    SetSigVal(MBMS_SAM_SIG_ID_SOFTWARE_VERSION_8, s32SetVal);
}

void InitHwVersion(void)
{
	U8 i = 0;
	S32 s32SetVal = 0;

	for(i = 0; i < 7; i++)
	{
		s32SetVal = (S32)(cst_hw_no.hw_version[2*i]<<8) | ((S32)cst_hw_no.hw_version[2*i+1]);
		(void)SetSigVal(MBMS_SAM_SIG_ID_HARDWARE_VERSION_1 + i, s32SetVal);
	}
}


void init_task(void *pvParameters)
{
	taskENTER_CRITICAL();           //进入临界区

    InitSwVersion();
    InitHwVersion();

    vTaskDelay(100);  //延时等待系统稳定

    ExtEepromInit();  //初始化rtos互斥量，必须在initsigval之前
    InitSigVal();

    xTaskCreate(LedTask,            "LED_task",             configMINIMAL_STACK_SIZE, 	    NULL,   LED_TASK_PRIO,              NULL);
    xTaskCreate(UartComm_InitTask,  "UartComm_InitTask",    configMINIMAL_STACK_SIZE,       NULL,   INIT_TASK_PRIO,             NULL);
    xTaskCreate(Save_Eeprom_Task,   "SAVE_EEPROM",          configMINIMAL_STACK_SIZE*2,     NULL,   SAVE_EEPROM_TASK_PRIO,      NULL);
    xTaskCreate(DataSample_Task,    "DataSample_Task",      configMINIMAL_STACK_SIZE*2,     NULL,   DATA_SAMPLE_TASK_PRIO,      NULL);
    xTaskCreate(UpdataFuncInitTask, "UpdataFuncInitTask",   configMINIMAL_STACK_SIZE,       NULL,   INIT_TASK_PRIO,             NULL);    

	taskEXIT_CRITICAL();            //退出临界区	

    vTaskDelete(NULL);
}

/*!
    \brief      main function
    \param[in]  none
    \param[out] none
    \retval     none
*/
int main(void)
{
    /* configure 4 bits pre-emption priority */
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);  

    /* system clocks configuration */
    rcu_config();
    gpio_config(); 
    fwdgt_init();     

    RTC_INIT(); // RTC init
    USART_INIT();
    HARD_I2C_INIT();
    HAL_ADC_INIT();

    xTaskCreate(init_task,          "init_task",            configMINIMAL_STACK_SIZE,       NULL,   INIT_TASK_PRIO,             NULL);

    /* start scheduler */
    vTaskStartScheduler();

    while(1) {
        vTaskDelay(1000);
    }
}



void vApplicationMallocFailedHook( void )
{
    g_blAppMallocFailedFg = TRUE;
}

/**
 * @brief 堆栈溢出后的处理函数
 * 注意：无法保证能捕捉到所有的堆栈溢出。
 * @param xTask	:发生溢出的任务的句柄
 * @param pcTaskName:发生溢出的任务的名称
 */
void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName )
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
