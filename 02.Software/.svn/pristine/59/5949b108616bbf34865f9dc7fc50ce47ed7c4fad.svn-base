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

SemaphoreHandle_t g_LedTaskSync_Binary_Semaphore = NULL;
SYS_WORKING_STATYS g_enumSysRunMode = SYS_STATUS_IDLE;

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
	NVIC_SetPriority(LPI2C3_IRQn, 2);
}

void ctrl_relay_cmd(U8 relay_pos, BOOL cmd)
{
    switch(relay_pos)
    {
        case K1:
        {
			if(cmd) {
	        	K1_ENABLE();
				(void)SetSigVal(SIGNAL_STATUS_K1, TRUE);
			} else {
				K1_DISABLE();
				(void)SetSigVal(SIGNAL_STATUS_K1, FALSE);
			}
			break;
        }
        case K2:
        {
			if(cmd) {
	        	K2_ENABLE();
				(void)SetSigVal(SIGNAL_STATUS_K2, TRUE);
			} else {
				K2_DISABLE();
				(void)SetSigVal(SIGNAL_STATUS_K2, FALSE);
			}
			break;
        }
        case K3:
        {
			if(cmd) {
	        	K3_ENABLE();
				(void)SetSigVal(SIGNAL_STATUS_K3, TRUE);
			} else {
				K3_DISABLE();
				(void)SetSigVal(SIGNAL_STATUS_K3, FALSE);
			}
			break;
        }
        case K4:
        {
			if(cmd) {
	        	K4_ENABLE();
				(void)SetSigVal(SIGNAL_STATUS_K4, TRUE);
			} else {
				K4_DISABLE();
				(void)SetSigVal(SIGNAL_STATUS_K4, FALSE);
			}
			break;
        }
        case K5:
        {
			if(cmd) {
	        	K5_ENABLE();
				(void)SetSigVal(SIGNAL_STATUS_K5, TRUE);
			} else {
				K5_DISABLE();
				(void)SetSigVal(SIGNAL_STATUS_K5, FALSE);
			}
			break;
        }
        case K6:
        {
			if(cmd) {
	        	K6_ENABLE();
				(void)SetSigVal(SIGNAL_STATUS_K6, TRUE);
			} else {
				K6_DISABLE();
				(void)SetSigVal(SIGNAL_STATUS_K6, FALSE);
			}
			break;
        }
        case K7:
        {
			if(cmd) {
	        	K7_ENABLE();
				(void)SetSigVal(SIGNAL_STATUS_K7, TRUE);
			} else {
				K7_DISABLE();
				(void)SetSigVal(SIGNAL_STATUS_K7, FALSE);
			}
			break;
        }
        case K8:
        {
			if(cmd) {
	        	K8_ENABLE();
				(void)SetSigVal(SIGNAL_STATUS_K8, TRUE);
			} else {
				K8_DISABLE();
				(void)SetSigVal(SIGNAL_STATUS_K8, FALSE);
			}
			break;
        }
        case K9:
        {
			if(cmd) {
	        	K9_ENABLE();
				(void)SetSigVal(SIGNAL_STATUS_K9, TRUE);
			} else {
				K9_DISABLE();
				(void)SetSigVal(SIGNAL_STATUS_K9, FALSE);
			}
			break;
        }
        case K10:
        {
			if(cmd) {
	        	K10_ENABLE();
				(void)SetSigVal(SIGNAL_STATUS_K10, TRUE);
			} else {
				K10_DISABLE();
				(void)SetSigVal(SIGNAL_STATUS_K10, FALSE);
			}
			break;
        }
        case K11:
        {
			if(cmd) {
	        	K11_ENABLE();
				(void)SetSigVal(SIGNAL_STATUS_K11, TRUE);
			} else {
				K11_DISABLE();
				(void)SetSigVal(SIGNAL_STATUS_K11, FALSE);
			}
			break;
        }
        case K12:
        {
			if(cmd) {
	        	K12_ENABLE();
				(void)SetSigVal(SIGNAL_STATUS_K12, TRUE);
			} else {
				K12_DISABLE();
				(void)SetSigVal(SIGNAL_STATUS_K12, FALSE);
			}
			break;
        }
        case K13:
        {
			if(cmd) {
	        	K13_ENABLE();
				(void)SetSigVal(SIGNAL_STATUS_K13, TRUE);
			} else {
				K13_DISABLE();
				(void)SetSigVal(SIGNAL_STATUS_K13, FALSE);
			}
			break;
        }
        case K14:
        {
			if(cmd) {
	        	K14_ENABLE();
				(void)SetSigVal(SIGNAL_STATUS_K14, TRUE);
			} else {
				K14_DISABLE();
				(void)SetSigVal(SIGNAL_STATUS_K14, FALSE);
			}
			break;
        }
        case K15:
        {
			if(cmd) {
	        	K15_ENABLE();
				(void)SetSigVal(SIGNAL_STATUS_K15, TRUE);
			} else {
				K15_DISABLE();
				(void)SetSigVal(SIGNAL_STATUS_K15, FALSE);
			}
			break;
        }
        case K16:
        {
			if(cmd) {
	        	K16_ENABLE();
				(void)SetSigVal(SIGNAL_STATUS_K16, TRUE);
			} else {
				K16_DISABLE();
				(void)SetSigVal(SIGNAL_STATUS_K16, FALSE);
			}
			break;
        }

		default:
			break;
	}
}

void ctrl_led_cmd(U8 led_pos, BOOL cmd)
{
    switch(led_pos)
    {
        case PWM_LED1:
        {
			if(cmd) {
	        	LED1_ON();
			} else {
				LED1_OFF();
			}
			break;
        }
        case PWM_LED2:
        {
			if(cmd) {
	        	LED2_ON();
			} else {
				LED2_OFF();
			}
			break;
        }
        case PWM_LED3:
        {
			if(cmd) {
	        	LED3_ON();
			} else {
				LED3_OFF();
			}
			break;
        }
		case USER_LED:
        {
			if(cmd) {
	        	USER_LED_ON();
			} else {
				USER_LED_OFF();
			}
			break;
        }

		default:
			break;
	}
}

void AllLedToggle(void)
{
	USER_LED_TOGGLE();
	LED1_TOGGLE();
	LED2_TOGGLE();
	LED3_TOGGLE();
}

void AllLedOn(void)
{
	USER_LED_ON();
	LED1_ON();
	LED2_ON();
	LED3_ON();
}

void AllLedOff(void)
{
	USER_LED_OFF();
	LED1_OFF();
	LED2_OFF();
	LED3_OFF();
}

void FeedWatchDog(void)
{
	WDOG_WDI_TOGGLE();
}

SYS_WORKING_STATYS GetSysRunModeFg(void)
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

void InitRelayCtrlTimes(void)
{
	U8 i = 0;
	for(i = 0; i < MAX_RELAY_NUM; i++)
	{
		s32RelayCtrlTimes[i] = GetMsgVal(PDU_SET_RELAY_1_OPERATE_TIMES+i);
	}
}

void SaveRealyCtrlTimes(void)
{
	U8 i = 0;
	for(i = 0; i < MAX_RELAY_NUM; i++)
	{
		(void)SetSigVal(PDU_SET_RELAY_1_OPERATE_TIMES+i, s32RelayCtrlTimes[i]);
		vTaskDelay(2);
	}
}

void WatchDogFeedFunc(void)
{
	if(FALSE == GetMsgVal(SIGNAL_STATUS_WDG_TEST))
	{
		FeedWatchDog();
	}
}

void LedBlinkFunc(void)
{
	if(SYS_STATUS_POWER_OFF != GetSysRunModeFg())
	{
		AllLedToggle();
	}
}

void vLedTaskDelay(U32 delay_ms)
{
	U32 TaskPeriod = 0;
	static U8 flashCount = 0;
	BaseType_t err = pdFALSE;

	if(NO_UPGRADE != GetMsgVal(PDU_SAM_SIG_ID_UPDATE_STATE)) {
		TaskPeriod = delay_ms/4;			/* 升级中4倍频率闪烁 */
	} else if(SYS_STATUS_SHUTTING_DOWN == GetSysRunModeFg()){
		TaskPeriod = delay_ms/10;			/* 关机后10倍频率闪烁 */
	} else if(TRUE == GetMsgVal(SIGNAL_STATUS_FACTORY_TEST)){
		if(flashCount % 7 == 0u)
		{
			TaskPeriod = delay_ms * 2;
			flashCount = 0;
		}
		else
		{
			TaskPeriod = delay_ms / 2;		/* 生产测试三短一长(2倍+2倍+2倍+0.5倍)闪烁 */
		}
		flashCount++;
	} else if(EVENT_HAPPEN == GetMsgVal(ALARM_ID_PCU_OFFLINE)){
		TaskPeriod = delay_ms * 2;			/* 通信丢失0.5倍频率闪烁 */
	} else {
		TaskPeriod = delay_ms;
	}

	if(NULL != g_LedTaskSync_Binary_Semaphore)
	{
		for(U8 i = 0; i < TaskPeriod / WATCHDOG_INTERVAL_MS; i++)						//将等待时间划分为更小的间隔，防止看门狗超时
		{
			WatchDogFeedFunc();
			if(xSemaphoreTake(g_LedTaskSync_Binary_Semaphore, WATCHDOG_INTERVAL_MS))	//led同频
			{
				break;
			}
		}
	}
	else
    {
		for(U8 i = 0; i < TaskPeriod / WATCHDOG_INTERVAL_MS; i++)
		{
			WatchDogFeedFunc();
			vTaskDelay(WATCHDOG_INTERVAL_MS);
		}
    }
}

