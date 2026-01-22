/*
 * ext_rtc.h
 *
 *  Created on: 2024��9��13��
 *      Author: 
 */

#ifndef EXT_RTC_H_
#define EXT_RTC_H_
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_lpi2c.h"
#include "fsl_lpi2c_edma.h"
#include "fsl_edma.h"
#include "MIMXRT1021.h"
#include "peripherals.h"
#include "PublicDefine.h"

#define EXT_RTC_TASK_PRIO           			(4)

#define EXT_RTC_IIC_ADDR						(0xA2U)

#define EXT_RTC_TIME_SECOND_ADDR           		(0x02U)
#define EXT_RTC_TIME_MINUTE_ADDR           		(0x03U)
#define EXT_RTC_TIME_HOUR_ADDR             		(0x04U)
#define EXT_RTC_TIME_DAY_ADDR              		(0x05U)
#define EXT_RTC_TIME_WEEKDAY_ADDR          		(0x06U)
#define EXT_RTC_TIME_MONTH_ADDR            		(0x07U)
#define EXT_RTC_TIME_YEAR_ADDR             		(0x08U)

#define PCF8563_CTRL_ST1_ADDR					(uint8_t)0x00
#define PCF8563_CTRL_ST2_ADDR					(uint8_t)0x01

#define PCF_Century_SetBitC                     (uint8_t)0x80

#define EXT_RTC_I2C_BUFFER_SIZE             	(7U)

/******************************************************************************
                               参数屏蔽宏定义
******************************************************************************/

#define PCF8563_Shield_Control_Status_1          (uint8_t)0xa8
#define PCF8563_Shield_Control_Status_2          (uint8_t)0x1f

#define PCF8563_Shield_Seconds                   (uint8_t)0x7f
#define PCF8563_Shield_Minutes                   (uint8_t)0x7f
#define PCF8563_Shield_Hours                     (uint8_t)0x3f

#define PCF8563_Shield_Days                      (uint8_t)0x3f
#define PCF8563_Shield_WeekDays                  (uint8_t)0x07
#define PCF8563_Shield_Months_Century            (uint8_t)0x1f
#define PCF8563_Shield_Years                     (uint8_t)0xff

typedef struct
{
    U8 ValidSecond;
    U8 Min;
    U8 Hour;
    U8 Day;
    U8 WeekDay;
    U8 Month;
    U8 Year;
} Ext_Rtc_TimeMsgBuffer;

typedef union 
{
    U8 TimeData[EXT_RTC_I2C_BUFFER_SIZE];
    Ext_Rtc_TimeMsgBuffer TimeMsg;
} Ext_Rtc_TimeDataBuffer;

void SYS_SetTimestamp(U32 *timestamp);
extern void Ext_Rtc_Init_Task(void * pvParameters);
void Rtc_SetTimestamp(const U32* timestamp);
void SYS_GetDate(U32 *year, U32 *month, U32 *day, U32 *hour, U32 *minute, U32 *second);

#endif
