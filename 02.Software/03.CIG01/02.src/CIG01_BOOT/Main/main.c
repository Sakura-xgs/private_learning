#include "gd32f4xx.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board.h"
#include "boot.h"
#include "led.h"
#include "hal_rtc.h"


#define INIT_TASK_PRIO                  ( tskIDLE_PRIORITY + 1 )
#define LED_TASK_PRIO                   ( tskIDLE_PRIORITY + 2 )



void SystemResetTask(void * pvParameters)
{
    (void)pvParameters;

    vTaskDelay(1000 * 10);  //延时10秒复位

    SystemResetFunc();      //复位

    vTaskDelete(NULL);
}

void init_task(void *pvParameters)
{
	taskENTER_CRITICAL();           //进入临界区

	//创建LED任务
	xTaskCreate(LedTask,     	    "LED_task",   	configMINIMAL_STACK_SIZE,   NULL,	LED_TASK_PRIO,	    NULL); 
    xTaskCreate(SystemResetTask,    "SYS_RESET",    configMINIMAL_STACK_SIZE,   NULL,   LED_TASK_PRIO,      NULL);                

	taskEXIT_CRITICAL();            //退出临界区	
    vTaskDelete(NULL); //删除当前任务
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

    /* fwgt peripheral init */
    fwdgt_init();     

    RTC_INIT(); // RTC init

    Boot2AppFunc(); // Boot to APP

    xTaskCreate(init_task, "init_task", configMINIMAL_STACK_SIZE, NULL, INIT_TASK_PRIO, NULL);

    /* start scheduler */
    vTaskStartScheduler();

    while(1) {
        vTaskDelay(1000);
    }
}

