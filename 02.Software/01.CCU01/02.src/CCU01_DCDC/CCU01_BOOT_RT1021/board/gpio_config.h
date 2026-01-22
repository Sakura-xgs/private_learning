/*
 * gpio_config.h
 *
 *  Created on: 2024年8月16日
 *      Author: Bono
 */

#ifndef GPIO_CONFIG_H_
#define GPIO_CONFIG_H_

#include "pin_mux.h"
#include "fsl_gpio.h"
#include "PublicDefine.h"


#define RUN_LED_GPIO_PORT           BOARD_INITPINS_RUN_LED_PORT
#define RUN_LED_GPIO_PIN            BOARD_INITPINS_RUN_LED_GPIO_PIN

#define RUN_LED_ON()                GPIO_PortClear(RUN_LED_GPIO_PORT, 1U << RUN_LED_GPIO_PIN)
#define RUN_LED_OFF()               GPIO_PortSet(RUN_LED_GPIO_PORT, 1U << RUN_LED_GPIO_PIN)
#define GET_RUN_LED_STATUS()        GPIO_PinRead(RUN_LED_GPIO_PORT, RUN_LED_GPIO_PIN)
#define RUN_LED_TOGGLE()            GPIO_PinWrite(RUN_LED_GPIO_PORT, RUN_LED_GPIO_PIN, \
                                                0x1 ^ GPIO_PinRead(RUN_LED_GPIO_PORT, RUN_LED_GPIO_PIN))

#define WDOG_WDI_TOGGLE()			GPIO_PinWrite(BOARD_INITPINS_WDOG_B_GPIO, BOARD_INITPINS_WDOG_B_GPIO_PIN,\
												0x1 ^ GPIO_PinRead(BOARD_INITPINS_WDOG_B_GPIO, BOARD_INITPINS_WDOG_B_GPIO_PIN))


#endif /* GPIO_CONFIG_H_ */
