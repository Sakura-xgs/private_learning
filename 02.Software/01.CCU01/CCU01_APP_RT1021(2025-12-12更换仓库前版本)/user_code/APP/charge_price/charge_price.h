/*
 * charge_price.h
 *
 *  Created on: 2025年4月24日
 *      Author: qjwu
 */

#ifndef APP_CHARGE_PRICE_CHARGE_PRICE_H_
#define APP_CHARGE_PRICE_CHARGE_PRICE_H_

#include "charge_price_IF.h"

//占位费更新时间
#define FEE_TIMER_PERIOD_MS        		10000//10s
// 电表与计费配置
#define METER_PRECISION 				10000// 电量精度：0.0001度（1/10000）

#define COST_PRECISION_MILLI 			10  // 1分=10厘（用于厘转分的四舍五入）

#endif /* APP_CHARGE_PRICE_CHARGE_PRICE_H_ */
