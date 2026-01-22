/*
 * hal_sys.c
 *
 *  Created on: 2024年9月5日
 *      Author: Bono
 */
#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "fsl_gpio.h"
#include "pin_mux.h"
#include "hal_sys_IF.h"
#include "boot.h"

static SYS_WORKING_STATYS g_enumSysRunMode = SYS_STATUS_IDLE;

static void WatchDogFeedFunc(void);
static void LedBlinkFunc(void);
static void vLedTaskDelay(U32 delay_ms);

void Hal_InitEnetModuleClock(void)
{
	/* set 50Mhz output to PHY */
    const clock_enet_pll_config_t config = {
        .enableClkOutput = true, .enableClkOutput500M = false, .enableClkOutput25M = false, .loopDivider = 1};
    CLOCK_InitEnetPll(&config);

    /* enable 50Mhz output to PHY */
    IOMUXC_EnableMode(IOMUXC_GPR, kIOMUXC_GPR_ENET1TxClkOutputDir, true);
}

void Hal_I2c_Init(void)
{
	NVIC_SetPriority(LPI2C1_IRQn, 2);
	NVIC_SetPriority(LPI2C3_IRQn, 2);
}

//void ctrl_relay_cmd(U8 cmd)
//{
//    switch(cmd)
//    {
//        case K1_ON:
//        {
//        	K1_ENABLE();
//			(void)SetSigVal(SIGNAL_STATUS_K1, TRUE);
//			break;
//        }
//        case K1_OFF:
//        {
//        	K1_DISABLE();
//			(void)SetSigVal(SIGNAL_STATUS_K1, FALSE);
//			break;
//        }
//		case K2_ON:
//        {
//        	K2_ENABLE();
//			(void)SetSigVal(SIGNAL_STATUS_K2, TRUE);
//			break;
//        }
//		case K2_OFF:
//        {
//        	K2_DISABLE();
//			(void)SetSigVal(SIGNAL_STATUS_K2, FALSE);
//			break;
//        }
//		case K3_ON:
//        {
//        	K3_ENABLE();
//			(void)SetSigVal(SIGNAL_STATUS_K3, TRUE);
//			break;
//        }
//		case K3_OFF:
//        {
//        	K3_DISABLE();
//			(void)SetSigVal(SIGNAL_STATUS_K3, FALSE);
//			break;
//        }
//		case K4_ON:
//        {
//        	K4_ENABLE();
//			(void)SetSigVal(SIGNAL_STATUS_K4, TRUE);
//			break;
//        }
//		case K4_OFF:
//        {
//        	K4_DISABLE();
//			(void)SetSigVal(SIGNAL_STATUS_K4, FALSE);
//			break;
//        }
//		case K5_ON:
//        {
//        	K5_ENABLE();
//			(void)SetSigVal(SIGNAL_STATUS_K5, TRUE);
//			break;
//        }
//		case K5_OFF:
//        {
//        	K5_DISABLE();
//			(void)SetSigVal(SIGNAL_STATUS_K5, FALSE);
//			break;
//        }
//		case K6_ON:
//        {
//        	K6_ENABLE();
//			(void)SetSigVal(SIGNAL_STATUS_K6, TRUE);
//			break;
//        }
//		case K6_OFF:
//        {
//        	K6_DISABLE();
//			(void)SetSigVal(SIGNAL_STATUS_K6, FALSE);
//			break;
//        }
//		case K7_ON:
//        {
//        	K7_ENABLE();
//			(void)SetSigVal(SIGNAL_STATUS_K7, TRUE);
//			break;
//        }
//		case K7_OFF:
//        {
//        	K7_DISABLE();
//			(void)SetSigVal(SIGNAL_STATUS_K7, FALSE);
//			break;
//        }
//		case K8_ON:
//        {
//        	K8_ENABLE();
//			(void)SetSigVal(SIGNAL_STATUS_K8, TRUE);
//			break;
//        }
//		case K8_OFF:
//        {
//        	K8_DISABLE();
//			(void)SetSigVal(SIGNAL_STATUS_K8, FALSE);
//			break;
//        }
//		default:
//			break;
//	}
//}


void AllLedToggle(void)
{
	USER_LED_TOGGLE();
}
void FeedWatchDog(void)
{
	WDOG_WDI_TOGGLE();
}

static SYS_WORKING_STATYS GetSysRunModeFg(void)
{
	return g_enumSysRunMode;
}

void SetSysNormalModeFg(void)
{
	g_enumSysRunMode = SYS_STATUS_NORMAL;
}

void SetSysShuttingDownModeFg(void)
{
	g_enumSysRunMode = SYS_STATUS_SHUTTING_DOWN;
}

void SetSysPowerOffModeFg(void)
{
	g_enumSysRunMode = SYS_STATUS_POWER_OFF;
}


static void WatchDogFeedFunc(void)
{
	//if(FALSE == GetMsgVal(SIGNAL_STATUS_WDG_TEST))
	//{
		FeedWatchDog();
	//}
}

static void LedBlinkFunc(void)
{
	AllLedToggle();
}

static void vLedTaskDelay(U32 delay_ms)
{
	static SemaphoreHandle_t g_LedTaskSync_Binary_Semaphore = NULL;

	U32 TaskPeriod = 0;
	BaseType_t err = pdFALSE;

	if((S32)NO_UPGRADE != GetMsgVal(CCU_SAM_SIG_ID_UPDATE_STATE)) {
		TaskPeriod = delay_ms/4U;			/* 升级中4倍频率闪烁 */
	} else if(SYS_STATUS_SHUTTING_DOWN == GetSysRunModeFg()){
		TaskPeriod = delay_ms/10U;			/* 关机后10倍频率闪烁 */
	} else {
		TaskPeriod = delay_ms;
	}

	if(NULL != g_LedTaskSync_Binary_Semaphore)
    {
        err = xSemaphoreTake(g_LedTaskSync_Binary_Semaphore, TaskPeriod);
        if(pdTRUE == err)
        {
        	// LED同频
        	vTaskDelay(TaskPeriod);
        }
        else if(pdFALSE == err)
        {
    		vTaskDelay(TaskPeriod);
        }
        else
        {
        	//
        }
    }
}
