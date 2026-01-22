#include "datasample.h"
#include "hal_adc_IF.h"
#include "FreeRTOSConfig.h"
#include "Signal.h"
#include "SignalManage.h"
#include "portmacro.h"
#include "projdefs.h"
#include "poll_adc.h"
#include "tcp_client_IF.h"
#include "uart_comm.h"
#include "hal_sys_IF.h"
#include "boot.h"
#include "pos_IF.h"
#include "charge_process_IF.h"
#include "fan_pwm_IF.h"

tmp_sample_t TmpMap[TEMPER_NUM] =
{
	{0, CCU_SAM_SIG_ID_GNUA_POSITIVE,&HAL_TEMP_AdToTemp_PT1000},
	{0, CCU_SAM_SIG_ID_GNUA_NEGATIVE,&HAL_TEMP_AdToTemp_PT1000},
	{0, CCU_SAM_SIG_ID_GNUB_POSITIVE,&HAL_TEMP_AdToTemp_PT1000},
	{0, CCU_SAM_SIG_ID_GNUB_NEGATIVE,&HAL_TEMP_AdToTemp_PT1000},
	{0, CCU_SAM_SIG_ID_PCB_TEMP_1,	 &HAL_TEMP_AdToTemp_3435},	//PCB_TEMP1
	{0, 0,						  	 &HAL_TEMP_AdToTemp_3435},//ADGUN1_T3
	{0, CCU_SAM_SIG_ID_PCB_TEMP_2,	 &HAL_TEMP_AdToTemp_3435},	//PCB_TEMP2
	{0, 0,						  	 &HAL_TEMP_AdToTemp_UnKnow},//ADGUN1_T4
	{0, 0,						  	 &HAL_TEMP_AdToTemp_UnKnow},//AD_PD1
	{0, 0,						  	 &HAL_TEMP_AdToTemp_3435},//ADGUN2_T3
	{0, 0,						  	 &HAL_TEMP_AdToTemp_UnKnow},//AD_PD2
	{0, 0,						  	 &HAL_TEMP_AdToTemp_3435}	//ADGUN2_T4
};

U8 g_cTemperatureHMI[2] = {0};//风扇进风口和出风口，用于单板测试的变量

static void TemperatureSample(void)
{
	U16 gsTemperature_GUN_T[TEMPER_NUM]={0};
	U8 i = 0;
	U16 u16temp = 0;
	static U16 Value[4]={0};
	static U8 FilterCount = 0;
	static U8 FunDelayStopCount = 0U;
	static S16 Temperature_FAN = 0;
	static S16 TemperatureFanDiff = 0;

	for(i = 0; i < 12U; i++)
	{
		u16temp = TmpMap[i].func(TmpMap[i].u32AdValue);
		//(void)SetSigVal(TmpMap[i].u32SingalId, s32temp);
		gsTemperature_GUN_T[i] = u16temp;
	}

	if(FilterCount < 6U)
	{
		FilterCount++;
	}
	for(i=0;i<4U;i++)
	{
		if(gsTemperature_GUN_T[i] < 0xFF)
		{
			Value[i] = ((40U*Value[i]) + (60U*gsTemperature_GUN_T[i])) / 100U;
			if(FilterCount > 5U)
			{
				gsTemperature_GUN_T[i] = Value[i];
			}
		}
		else if(gsTemperature_GUN_T[i] >= 0xFF)
		{
			gsTemperature_GUN_T[i] = 0xFF + 59;
		}
	}

	g_cTemperatureHMI[0] = gsTemperature_GUN_T[9];
	g_cTemperatureHMI[1] = gsTemperature_GUN_T[11];

	TemperatureFanDiff = gsTemperature_GUN_T[11] - gsTemperature_GUN_T[9];
	if((gsTemperature_GUN_T[11] - 60) > 25)//出风口温度大于常温
	{
		if(TemperatureFanDiff >= 5)
		{
			K8_ENABLE();//接通风扇电源
		}

		//风扇占空比改变
		if((TemperatureFanDiff >= 5) && (TemperatureFanDiff < 8))
		{
			Fan_Pwm.dutycycle = 80U;//20%转速
			FunDelayStopCount = 0U;
		}
		else if((TemperatureFanDiff >= 8) && (TemperatureFanDiff < 15))
		{
			Fan_Pwm.dutycycle = 50U;//50%转速
			FunDelayStopCount = 0U;
		}
		else if((TemperatureFanDiff >= 15) && (TemperatureFanDiff < 25))
		{
			Fan_Pwm.dutycycle = 20U;//80%转速
			FunDelayStopCount = 0U;
		}
		else if(TemperatureFanDiff >= 25)
		{
			Fan_Pwm.dutycycle = 10U;//90%转速
			FunDelayStopCount = 0U;
		}
		else if(TemperatureFanDiff < 5)//延时停转
		{
			FunDelayStopCount++;
			if(FunDelayStopCount > 5U)
			{
				Fan_Pwm.dutycycle = 100U;//停转
				FunDelayStopCount = 0U;
				K8_DISABLE();//风扇断电
			}
		}
		else
		{

		}
	}//温度小于25度的时候风扇停掉
	else
	{
		K8_DISABLE();//风扇断电
		Fan_Pwm.dutycycle = 100U;//停转
	}

	if (DEBUG_MODE != g_sPile_data.ucPile_config_mode)
	{
		g_sGun_data[GUN_A].nDC_positive_temp = ((S16)gsTemperature_GUN_T[0]-59);
		g_sGun_data[GUN_A].nDC_negative_temp = ((S16)gsTemperature_GUN_T[1]-59);
		g_sGun_data[GUN_B].nDC_positive_temp = ((S16)gsTemperature_GUN_T[2]-59);
		//欧标
		//g_sGun_data[GUN_B].nDC_negative_temp = ((S16)gsTemperature_GUN_T[3]-59);
		//美标NACS当前选型只有一路ADC检测，为方便测试，临时更改
		g_sGun_data[GUN_B].nDC_negative_temp = ((S16)gsTemperature_GUN_T[2]-59);
	}
	else
	{
		g_sGun_data[GUN_A].nDC_positive_temp = (S16)((S16)gsTemperature_GUN_T[0]-59+(S16)g_Test.PosTempGunA);
		g_sGun_data[GUN_A].nDC_negative_temp = (S16)((S16)gsTemperature_GUN_T[1]-59+(S16)g_Test.NegTempGunA);
		g_sGun_data[GUN_B].nDC_positive_temp = (S16)((S16)gsTemperature_GUN_T[2]-59+(S16)g_Test.PosTempGunB);
		g_sGun_data[GUN_B].nDC_negative_temp = (S16)((S16)gsTemperature_GUN_T[3]-59+(S16)g_Test.NegTempGunB);
		my_printf(USER_DEBUG, "%d %d %d %d\n", g_sGun_data[GUN_A].nDC_positive_temp, g_sGun_data[GUN_A].nDC_negative_temp,
				g_sGun_data[GUN_B].nDC_positive_temp, g_sGun_data[GUN_B].nDC_negative_temp);
	}

}

static void CcuShutDown(void)
{
	static U16 u16Cnt = 0;
	S32 iCharge_Price[2] = {0};
	S32 iOccupation_Price[2] = {0};
	S32 PaymentFlag = 1;//掉电扣款标志

	(void)SetSigVal(SIGNAL_STATUS_POWER_OFF, (S32)TRUE);
	SetSysShuttingDownModeFg();

	//判断是否为POS启动，只有POS启动且在充电中，才会保存
	//if(((U8)POS_START_MODE == g_sGun_data[GUN_A].sTemp.ucStart_Charge_type) && ((U8)STA_CHARGING == g_sGun_data[GUN_A].ucGun_common_status))
	if((U8)POS_START_MODE == g_sGun_data[GUN_A].sTemp.ucStart_Charge_type)
	{
		//充电费用
		(void)GetSigVal(CCU_SAM_SIG_ID_CHARGE_PRICE_A, &iCharge_Price[GUN_A]);
		//占位费用
		(void)GetSigVal(CCU_SAM_SIG_ID_OCCUPATION_FEE_A, &iOccupation_Price[GUN_A]);

		if((iCharge_Price[GUN_A] + iOccupation_Price[GUN_A]) > 0)//只有充电费用不为0的时候才需要保存
		{
			SaveTransactionData(GUN_A);//保存A枪的交易信息
			(void)SetSigVal(CCU_SET_SIG_ID_CHARGE_PRICE_A, iCharge_Price[GUN_A]);
			(void)SetSigVal(CCU_SET_SIG_ID_OCCUPATION_FEE_A, iOccupation_Price[GUN_A]);

			//掉电扣款标志
			(void)SetSigVal(CCU_SET_SIG_ID_PAYMENT_FLAG_A, PaymentFlag);
		}
	}
	//if(((U8)POS_START_MODE == g_sGun_data[GUN_B].sTemp.ucStart_Charge_type) && ((U8)STA_CHARGING == g_sGun_data[GUN_B].ucGun_common_status))
	if((U8)POS_START_MODE == g_sGun_data[GUN_B].sTemp.ucStart_Charge_type)
	{
		//充电费用
		(void)GetSigVal(CCU_SAM_SIG_ID_CHARGE_PRICE_B, &iCharge_Price[GUN_B]);
		//占位费用
		(void)GetSigVal(CCU_SAM_SIG_ID_OCCUPATION_FEE_B, &iOccupation_Price[GUN_B]);

		if((iCharge_Price[GUN_B] + iOccupation_Price[GUN_B]) > 0)//只有充电费用不为0的时候才需要保存
		{
			SaveTransactionData(GUN_B);//保存B枪的交易信息
			vTaskDelay(5);
			(void)SetSigVal(CCU_SET_SIG_ID_CHARGE_PRICE_B, iCharge_Price[GUN_B]);
			vTaskDelay(5);
			(void)SetSigVal(CCU_SET_SIG_ID_OCCUPATION_FEE_B, iOccupation_Price[GUN_B]);
			vTaskDelay(5);

			//掉电扣款标志
			(void)SetSigVal(CCU_SET_SIG_ID_PAYMENT_FLAG_B, PaymentFlag);
		}
	}

	my_printf(USER_INFO,"Power off! all massage has been saved.");
	SetSysPowerOffModeFg();

	vTaskDelay(5000);
	my_printf(USER_INFO,"Power on recover, reset!");
	TcpClose();
	while(u16Cnt < 5U)
	{
		vTaskDelay(30);
		if(GET_WDI_PFO_STATUS() == (U32)TRUE)
		{
			u16Cnt++;
		}
		else
		{
			u16Cnt = 0;
		}
	}
	SystemResetFunc();
}

static void PowerOffSample(void)
{
	//static BOOL blPwOnFg = FALSE;
	static U16 u16Cnt = 0;
	static BOOL blPwOffFg = FALSE;

	if ((GET_WDI_PFO_STATUS() == (U32)FALSE) || (blPwOffFg == TRUE))
	{
		u16Cnt++;
		if(u16Cnt >= 3U)
		{
			u16Cnt = 0;
			blPwOffFg = TRUE;
			CcuShutDown();
		}
	}
	else
	{
		u16Cnt = 0;
	}
}

static void DataSample_task(void * pvParameters)
{
	(void)pvParameters;

    for( ;; )
    {
    	if(g_blAdcConversionCompletionFlag == TRUE)
    	{
    		TemperatureSample();
    		g_blAdcConversionCompletionFlag = FALSE;
    	}

    	PowerOffSample();

    	vTaskDelay(100);
    }
}

void DataSample_Init_task(void * pvParameters)
{
	taskENTER_CRITICAL();

	(void)xTaskCreate(&DataSample_task,  "SYS_DATA_SAMPLE",  400U/4U, NULL,   GENERAL_TASK_PRIO,   NULL);

	taskEXIT_CRITICAL();

	vTaskDelete(NULL);
}
