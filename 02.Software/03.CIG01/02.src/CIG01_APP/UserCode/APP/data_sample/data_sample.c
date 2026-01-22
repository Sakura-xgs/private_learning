#include "data_sample.h"
#include "hal_adc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "SignalManage.h"
#include "MbmsSignal.h"
#include "board.h"

#define TEMP_TABLE_SIZE_3435   (sizeof(TempCoffAdTable_3435)/sizeof(U16))

#define SEQ_RISING        0        //升序
#define SEQ_LOWER         1        //降序


/*----------------------------------/NTC温度系数表/----------------------------------------*/
//(B值3435)
const U16 TempCoffAdTable_3435[] =
{
	3904, 3894, 3882, 3871, 3858, 3846, 3832, 3819, 3804, 3789,
	3774, 3758, 3741, 3724, 3706, 3687, 3668, 3648, 3627, 3606,
	3584, 3561, 3538, 3514, 3489, 3464, 3438, 3411, 3384, 3356,
	3327, 3297, 3267, 3237, 3205, 3173, 3141, 3108, 3074, 3040,
	3008, 2970, 2934, 2898, 2862, 2825, 2787, 2750, 2712, 2674,
	2635, 2596, 2557, 2518, 2479, 2440, 2400, 2361, 2322, 2282,
	2243, 2204, 2164, 2125, 2087, 2048, 2010, 1971, 1933, 1896,
	1859, 1822, 1785, 1749, 1713, 1677, 1642, 1608, 1574, 1540,
	1507, 1474, 1442, 1410, 1379, 1348, 1318, 1288, 1259, 1230,
	1202, 1174, 1147, 1121, 1095, 1069, 1044, 1020, 996,  972,
	949,  927,  905,  883,  862,  842,  821,  802,  783,  764,
	746,  728,  710,  693,  677,  660,  645,  629,  614,  599,
	585,  571,  557,  544,  531,  519,  506,  494,  483,  471,
	460,  449,  439,  428,  418,  408,  399,  390,  381,  372,
	363,  355,  347,  339,  331,  323,  316,  309,  302,  295,
	288,  282,  275,  269,  263,  257,  252,  246,  241,  235,
	230,  225,  220,  216,  211,  206,
};


static strFilter g_filters[4] = {0};  //temp1/2, cc1_voltage1/2


static U32 AverageFilter(strFilter *filter, U32 data)
{
	if(filter->u16Cnt < FILTER_BUF_SIZE)   //buffer未满
	{
		filter->u32Sum += data;
		filter->u32Buffer[filter->u16Index++] = data;
		filter->u16Cnt++;
		return filter->u32Sum / filter->u16Cnt;
	}
	else
	{
		if(filter->u16Index >= FILTER_BUF_SIZE) {  //循环填充buffer
			filter->u16Index = 0;
		}

		filter->u32Sum -= filter->u32Buffer[filter->u16Index];   //减去最旧的数据
		filter->u32Buffer[filter->u16Index++] = data;  //填充新数据
		filter->u32Sum += data; 
		return filter->u32Sum / FILTER_BUF_SIZE;
	}
}

/// @brief 查表将AD值转换为温度值
/// @param TempData 
/// @return 温度，单位 1，偏移量40
static U16 AdToTemp_3435(U16 TempData)
{

    U16 High = (TEMP_TABLE_SIZE_3435 - 1);
    U16 Low = 0;
    U16 uc16Temp = 0;
    U16 Mid = (U16)(High + Low) / 2;
    U16 Sequence = SEQ_RISING;

	//判断表格升降序
	if( TempCoffAdTable_3435[0] < TempCoffAdTable_3435[TEMP_TABLE_SIZE_3435 - 1] )
	{
		Sequence = SEQ_RISING;
		//超出量程
		if( TempData < TempCoffAdTable_3435[0] || TempData > TempCoffAdTable_3435[TEMP_TABLE_SIZE_3435-1] )
		{
			return 0;
		}
	}
	else
	{
		Sequence = SEQ_LOWER;
		//超出量程
		if( TempData > TempCoffAdTable_3435[0] || TempData < TempCoffAdTable_3435[TEMP_TABLE_SIZE_3435-1] )
		{
			return 0;
		}
	}

	//二分查找
	for(;;)
	{

		if( TempData < TempCoffAdTable_3435[Mid] )
        {
            (Sequence==SEQ_RISING)?(High = Mid):(Low = Mid);
        }

        if( TempData > TempCoffAdTable_3435[Mid] )
        {
            (Sequence==SEQ_RISING)?(Low = Mid):(High = Mid);
        }

        if( (TempData==TempCoffAdTable_3435[Mid]) || (1==High-Low) || (1==Low-High) )
        {
        	uc16Temp = Mid;
        	break;
        }

        Mid = (U16)(((U16)High + (U16)Low) / 2);
	}

    return  (uc16Temp);	/* 偏移量为40 */
}


/// @brief 获取cc1输入电压，adc采集电压为1/8.27
/// @param adValue 
/// @return cc1电压，单位mV
static U32 AdToVoltage_CC1(uint16_t adValue)
{
	U16 CC1_Voltage = 0;
	CC1_Voltage = (U32)adValue * 33 * 827 / 4096;
	return CC1_Voltage;	//单位mV
}


static void Update_PCBTemp(void)
{
	//测量板上温度
	S32 temp1 = 0, temp2 = 0;
	temp1 = HAL_ADC_GET_CHAN_AD(ADC0_IN10_PCB_TEM1);  //获取原始ad值
	temp1 = AdToTemp_3435(temp1);  //温度转换
	temp1 = AverageFilter(&g_filters[0], temp1);  //滤波

	temp2 = HAL_ADC_GET_CHAN_AD(ADC0_IN13_PCB_TEM2);
	temp2 = AdToTemp_3435(temp2);
	temp2 = AverageFilter(&g_filters[1], temp2);

	SetSigVal(MBMS_SAM_SIG_ID_PCB_TEMP1, temp1);
	SetSigVal(MBMS_SAM_SIG_ID_PCB_TEMP2, temp2); //偏移量40

	SetSigVal(MBMS_SAM_SIG_ID_PCB_TEMP1_AD_RAW, HAL_ADC_GET_CHAN_AD(ADC0_IN10_PCB_TEM1));  //原始ad值
	SetSigVal(MBMS_SAM_SIG_ID_PCB_TEMP2_AD_RAW, HAL_ADC_GET_CHAN_AD(ADC0_IN13_PCB_TEM2)); 
}

static void Update_CC1Voltage(void)
{
	//测量CC1输入电压
	S32 voltageA, voltageB;

	voltageA = HAL_ADC_GET_CHAN_AD(ADC0_IN11_A_CC1);    //获取原始ad值
	voltageA = AdToVoltage_CC1(voltageA);  //电压换算
	voltageA = AverageFilter(&g_filters[2], voltageA);  //滤波

	voltageB = HAL_ADC_GET_CHAN_AD(ADC0_IN12_B_CC1);
	voltageB = AdToVoltage_CC1(voltageB);
	voltageB = AverageFilter(&g_filters[3], voltageB);

	SetSigVal(MBMS_SAM_SIG_ID_CC1_VOLTAGE_A, voltageA/100); //mv*100, 精度0.1v
	SetSigVal(MBMS_SAM_SIG_ID_CC1_VOLTAGE_B, voltageB/100); 

	SetSigVal(MBMS_SAM_SIG_ID_CC1_VOLTAGE_A_AD_RAW, HAL_ADC_GET_CHAN_AD(ADC0_IN11_A_CC1));
	SetSigVal(MBMS_SAM_SIG_ID_CC1_VOLTAGE_B_AD_RAW, HAL_ADC_GET_CHAN_AD(ADC0_IN12_B_CC1));
}

static void Update_GunStatus(void)
{
	SetSigVal(SIGNAL_STATUS_DI_A_GUN_POWER, GET_A_GUN_DI_12V()); //A枪电源状态
	SetSigVal(SIGNAL_STATUS_DI_B_GUN_POWER, GET_B_GUN_DI_12V()); //B枪电源状态
	SetSigVal(SIGNAL_STATUS_DI_A_GUN_LOCK, GET_A_GUN_DI_LOCK()); //A枪锁状态
	SetSigVal(SIGNAL_STATUS_DI_B_GUN_LOCK, GET_B_GUN_DI_LOCK()); //B枪锁状态
}


/// @brief 测量板上温度，CC1输入电压
/// @param pvParameters 
void DataSample_Task(void *pvParameters)
{
	volatile UBaseType_t uxHighWaterMark;

	while(1)
	{
		Update_PCBTemp();
		Update_CC1Voltage();
		Update_GunStatus();

		vTaskDelay(5);

		uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
	}
}

