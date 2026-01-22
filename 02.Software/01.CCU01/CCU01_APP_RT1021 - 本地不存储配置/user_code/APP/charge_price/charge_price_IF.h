/*
 * charge_price_IF.h
 *
 *  Created on: 2025年4月24日
 *      Author: qjwu
 */

#ifndef APP_CHARGE_PRICE_CHARGE_PRICE_IF_H_
#define APP_CHARGE_PRICE_CHARGE_PRICE_IF_H_

extern SemaphoreHandle_t g_sStart_timing_sem[2];

void Charge_Price_Init_Task(void * pvParameters);

#endif /* APP_CHARGE_PRICE_CHARGE_PRICE_IF_H_ */
