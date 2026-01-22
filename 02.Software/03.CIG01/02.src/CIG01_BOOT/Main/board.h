#ifndef __BOARD_H
#define __BOARD_H

#include "gd32f4xx.h"
#include "PublicDefine.h"


/* GPIO DEFINE */
#define RUN_LED_GPIO_CLK        RCU_GPIOE
#define RUN_LED_PORT            GPIOE
#define RUN_LED_PIN             GPIO_PIN_4




void rcu_config(void);
void gpio_config(void);
void fwdgt_init(void);

#endif 
