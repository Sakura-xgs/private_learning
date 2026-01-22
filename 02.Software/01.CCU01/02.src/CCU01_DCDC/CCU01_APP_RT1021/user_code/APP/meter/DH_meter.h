/*
 * DH_meter.h
 *
 *  Created on: 2024年10月21日
 *      Author: qjwu
 */

#ifndef APP_METER_DH_METER_H_
#define APP_METER_DH_METER_H_

#include <APP/meter/meter_IF.h>

#define DH_START_REG_ADD_L              0x00
#define DH_START_REG_ADD_H              0x40
#define DH_READ_REG_NUM               	19
#define DH_READ_CMD            			0x04U

#define DH_VOL_INDEX                    3
#define DH_CUR_INDEX					7
#define DH_POWER_INDEX					11
#define DH_ENERGY_INDEX					31
#define DH_CURRENT_ALARM_FLAG_INDEX     39

#define ADDR_INDEX                 	 	0
#define CMD_INDEX                   	1
#define DATA_LEN_INDEX              	2

typedef struct
{
	BOOL bCurrent_alarm_flag_A;
	BOOL bCurrent_alarm_flag_B;;
}DH_private_alarm_t;

Meter_data_t* GetDhMeterModel(void);
void DhMeterInit(void);

#endif /* APP_METER_DH_METER_H_ */
