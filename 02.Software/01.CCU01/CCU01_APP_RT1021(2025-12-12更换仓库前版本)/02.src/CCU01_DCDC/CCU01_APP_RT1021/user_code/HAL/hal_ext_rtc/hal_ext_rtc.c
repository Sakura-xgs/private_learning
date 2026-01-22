/*
 * ext_rtc.c
 *
 *  Created on: 2024��9��13��
 *      Author: 
 */

#include "hal_ext_rtc.h"
#include "uart_comm.h"
#include "time.h"
#include <stdio.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

static U8 u8RtcCommFailCnt = 0;
static Ext_Rtc_TimeDataBuffer Ext_Rtc_TimeData = {0};
static SemaphoreHandle_t g_CtlExtRtc_MutexSemaphore = NULL;
static time_t CCU_SysTimestamp = 0x67e49a54;//UTC 2025/03/17/00/24

/*******************************************************************************
 * Code
 ******************************************************************************/
static time_t date_to_timestamp(U32 year, U32 month, U32 day, U32 hour, U32 minute, U32 second)
{
    struct tm timeinfo = {0};

    // 设置时间结构体的各个字段
    timeinfo.tm_year = (S32)year - 1900; // tm_year是从1900年开始计数的
    timeinfo.tm_mon = (S32)month - 1;    // tm_mon是从0（一月）开始计数的
    timeinfo.tm_mday = (S32)day;
    timeinfo.tm_hour = (S32)hour;
    timeinfo.tm_min = (S32)minute;
    timeinfo.tm_sec = (S32)second;

    // 将时间结构体转换为时间戳（秒）
    // 注意：mktime会假设时间是本地时间，并会进行任何必要的DST（夏令时）调整
    // 如果需要UTC时间戳，可以使用timegm函数（但注意timegm不是标准C库的一部分，可能在某些平台上不可用）
    time_t timestamp = mktime(&timeinfo);

    return timestamp;
}

static void timestamp_to_date(const time_t* timestamp, U32 *year, U32 *month, U32 *day, U32 *hour, U32 *minute, U32 *second)
{
    struct tm *timeinfo;

    //使用localtime将时间戳转换为本地时间（如果需要UTC时间，使用gmtime）
    //timeinfo = localtime(timestamp);
    timeinfo = gmtime(timestamp);
    if(timeinfo == NULL)
    {
        return;
    }

    //提取日期和时间信息
    *year = ((U32)timeinfo->tm_year + 1900U); //tm_year是从1900年开始计数的
    *month = ((U32)timeinfo->tm_mon + 1U);    //tm_mon是从0（一月）开始计数的
    *day = (U32)timeinfo->tm_mday;
    *hour = (U32)timeinfo->tm_hour;
    *minute = (U32)timeinfo->tm_min;
    *second = (U32)timeinfo->tm_sec;

    return;
}

static uint8_t RTC_BinToBcd2(uint8_t BINValue)
{
    uint8_t bcdhigh = 0;

    while (BINValue >= 10U)
    {
        bcdhigh++;
        BINValue -= 10U;
    }

    return ((bcdhigh << 4) | BINValue);
}

static uint8_t RTC_Bcd2ToBin(uint8_t BCDValue)
{
    uint8_t tmp = 0;

    tmp = ((uint8_t)(BCDValue & (uint8_t)0xF0) >> (uint8_t)0x04) * 10U;
    return (tmp + (BCDValue & (uint8_t)0x0F));
}

static status_t RTC_IIC_READ_ARRAY(U8* read_Buffer, U16 start_Addr, U16 number_Bytes)
{
	status_t blRet = kStatus_Fail;

	assert(number_Bytes <= EXT_RTC_I2C_BUFFER_SIZE);

	EXT_RTC_I2C_masterTransfer.flags          = (U32)kLPI2C_TransferDefaultFlag;
	EXT_RTC_I2C_masterTransfer.slaveAddress   = (EXT_RTC_IIC_ADDR >> 1);
	EXT_RTC_I2C_masterTransfer.direction      = kLPI2C_Read;
	EXT_RTC_I2C_masterTransfer.subaddress     = start_Addr;
	EXT_RTC_I2C_masterTransfer.subaddressSize = 1;
	EXT_RTC_I2C_masterTransfer.data           = read_Buffer;
	EXT_RTC_I2C_masterTransfer.dataSize       = number_Bytes;

	blRet = LPI2C_RTOS_Transfer(&EXT_RTC_I2C_masterHandle, &EXT_RTC_I2C_masterTransfer);

	if(kStatus_Success != blRet)
	{
		u8RtcCommFailCnt++;
	}

	return blRet;
}

static status_t RTC_IIC_WRITE_ARRAY(U8* write_Buffer, U16 start_Addr, U16 number_Bytes)
{
	status_t blRet = kStatus_Fail;

	assert(number_Bytes <= EXT_RTC_I2C_BUFFER_SIZE);

	EXT_RTC_I2C_masterTransfer.flags          = (U32)kLPI2C_TransferDefaultFlag;
	EXT_RTC_I2C_masterTransfer.slaveAddress   = (EXT_RTC_IIC_ADDR >> 1);
	EXT_RTC_I2C_masterTransfer.direction      = kLPI2C_Write;
	EXT_RTC_I2C_masterTransfer.subaddress     = start_Addr;
	EXT_RTC_I2C_masterTransfer.subaddressSize = 1;
	EXT_RTC_I2C_masterTransfer.data           = write_Buffer;
	EXT_RTC_I2C_masterTransfer.dataSize       = number_Bytes;

	blRet = LPI2C_RTOS_Transfer(&EXT_RTC_I2C_masterHandle, &EXT_RTC_I2C_masterTransfer);

	if(kStatus_Success != blRet)
	{
		u8RtcCommFailCnt++;
	}

	return blRet;
}

static void SetRealTime(Ext_Rtc_TimeMsgBuffer *my_time)
{
	U8 wrBuf[EXT_RTC_I2C_BUFFER_SIZE] = {0};

	assert(g_CtlExtRtc_MutexSemaphore != NULL);
	(void)xSemaphoreTake(g_CtlExtRtc_MutexSemaphore, portMAX_DELAY);

    //拷贝数据
	my_time->Month &= ~PCF_Century_SetBitC;

	wrBuf[6] = my_time->ValidSecond;
    wrBuf[5] = my_time->Min;
    wrBuf[4] = my_time->Hour;
    wrBuf[3] = my_time->Day;
    wrBuf[2] = my_time->WeekDay;
    wrBuf[1] = my_time->Month;
    wrBuf[0] = my_time->Year;
    //写入数据到寄存器
    (void)RTC_IIC_WRITE_ARRAY(wrBuf, EXT_RTC_TIME_SECOND_ADDR, EXT_RTC_I2C_BUFFER_SIZE);

    (void)xSemaphoreGive(g_CtlExtRtc_MutexSemaphore);
}

static void GetRtcTime(void)
{
	assert(g_CtlExtRtc_MutexSemaphore != NULL);
	(void)xSemaphoreTake(g_CtlExtRtc_MutexSemaphore, portMAX_DELAY);

    (void)RTC_IIC_READ_ARRAY(Ext_Rtc_TimeData.TimeData, EXT_RTC_TIME_SECOND_ADDR , 7);

    Ext_Rtc_TimeData.TimeData[0] &= PCF8563_Shield_Seconds;         //屏蔽无效位
    Ext_Rtc_TimeData.TimeData[1] &= PCF8563_Shield_Minutes;
    Ext_Rtc_TimeData.TimeData[2] &= PCF8563_Shield_Hours;
    Ext_Rtc_TimeData.TimeData[3] &= PCF8563_Shield_Days;
    Ext_Rtc_TimeData.TimeData[4] &= PCF8563_Shield_WeekDays;
    Ext_Rtc_TimeData.TimeData[5] &= PCF8563_Shield_Months_Century;
    Ext_Rtc_TimeData.TimeData[6] &= PCF8563_Shield_Years;

    Ext_Rtc_TimeData.TimeMsg.Year  = RTC_Bcd2ToBin(Ext_Rtc_TimeData.TimeData[6]);
    Ext_Rtc_TimeData.TimeMsg.Month   = RTC_Bcd2ToBin(Ext_Rtc_TimeData.TimeData[5]);
    Ext_Rtc_TimeData.TimeMsg.WeekDay = RTC_Bcd2ToBin(Ext_Rtc_TimeData.TimeData[4]);
    Ext_Rtc_TimeData.TimeMsg.Day   = RTC_Bcd2ToBin(Ext_Rtc_TimeData.TimeData[3]);
    Ext_Rtc_TimeData.TimeMsg.Hour  = RTC_Bcd2ToBin(Ext_Rtc_TimeData.TimeData[2]);
    Ext_Rtc_TimeData.TimeMsg.Min = RTC_Bcd2ToBin(Ext_Rtc_TimeData.TimeData[1]);
    Ext_Rtc_TimeData.TimeMsg.ValidSecond = RTC_Bcd2ToBin(Ext_Rtc_TimeData.TimeData[0]);

    (void)xSemaphoreGive(g_CtlExtRtc_MutexSemaphore);
}

static U32 Rtc_GetTimestamp(void)
{
	time_t timestamp = date_to_timestamp(2000U + Ext_Rtc_TimeData.TimeMsg.Year,
										 Ext_Rtc_TimeData.TimeMsg.Month,
										 Ext_Rtc_TimeData.TimeMsg.Day,
										 Ext_Rtc_TimeData.TimeMsg.Hour,
										 Ext_Rtc_TimeData.TimeMsg.Min,
										 Ext_Rtc_TimeData.TimeMsg.ValidSecond);
	return timestamp;
}

void Rtc_SetTimestamp(const U32 *timestamp)
{
	U32 year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
	Ext_Rtc_TimeMsgBuffer Time;

	timestamp_to_date((time_t*)timestamp, &year, &month, &day, &hour, &minute, &second);

	if (TRUE == my_unsigned_abs(Rtc_GetTimestamp(), *timestamp, 2))
	{
		if(year > 2000U)
		{
			Time.Year = (U8)(year - 2000U);
			Time.Month = (U8)month;
			Time.Day = (U8)day;
			Time.Hour = (U8)hour;
			Time.Min = (U8)minute;
			Time.ValidSecond = (U8)second;

			SetRealTime(&Time);
		}
	}
}

void SYS_GetDate(U32 *year, U32 *month, U32 *day, U32 *hour, U32 *minute, U32 *second)
{
	timestamp_to_date(&CCU_SysTimestamp,year,month,day,hour,minute,second);
}

void SYS_SetTimestamp(U32 *timestamp)
{
	CCU_SysTimestamp = *timestamp;
}

static void PCF8563Init(void)
{
	U8 u8TempBuf[EXT_RTC_I2C_BUFFER_SIZE] = {0};
	Ext_Rtc_TimeMsgBuffer TestTime = {0x24, 0x11, 0x1, 0x14, 0x23, 0x49,0x06};

	SetRealTime(&TestTime);

	(void)RTC_IIC_WRITE_ARRAY(u8TempBuf, PCF8563_CTRL_ST1_ADDR , 1);
	(void)RTC_IIC_WRITE_ARRAY(u8TempBuf, PCF8563_CTRL_ST2_ADDR , 1);
}

/*!
 * @brief Main function
 */
static void Ext_Rtc_Task (void * pvParameters)
{
	//volatile UBaseType_t uxHighWaterMark;

    for( ;; )
    {
    	CCU_SysTimestamp++;//系统时间戳计时

        vTaskDelay(1000);
        //uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    }
}

static void Ext_Rtc_Init(void)
{
	PCF8563Init();
}

void Ext_Rtc_Init_Task(void * pvParameters)
{
	(void)pvParameters;

	g_CtlExtRtc_MutexSemaphore = xSemaphoreCreateMutex();

	//Ext_Rtc_Init();//后续板子取消了RTC芯片

	taskENTER_CRITICAL();

	(void)xTaskCreate(&Ext_Rtc_Task,     "EXT_RTC_TASK",	200U/4U,   NULL,   EXT_RTC_TASK_PRIO,		NULL);

	vTaskDelete(NULL);

	taskEXIT_CRITICAL();
}
