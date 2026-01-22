/*
 * led.h
 *
 *  Created on: 2024年11月1日
 *      Author: qjwu
 */

#ifndef APP_LED_LED_H_
#define APP_LED_LED_H_

#include "led_IF.h"

enum
{
	RED 	= 0,
	GREEN 	= 1,
	BLUE	= 2,
	YELLOW	= 3,
	WHITE	= 4,
	UKNOWN 	= 5
};

#define LED_BREATH_TIMER_PERIOD_MS  50	//ms
#define FLASH_PERIOD_MS		 		1500//ms

#endif /* APP_LED_LED_H_ */
