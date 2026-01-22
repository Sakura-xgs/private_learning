#ifndef __BOARD_H
#define __BOARD_H

#include "gd32f4xx.h"
#include "PublicDefine.h"


/* GPIO DEFINE */
#define RUN_LED_GPIO_CLK            RCU_GPIOE
#define RUN_LED_PORT                GPIOE
#define RUN_LED_PIN                 GPIO_PIN_4

#define RS485_CON_GPIO_CLK          RCU_GPIOC
#define RS485_CON_GPIO_PORT         GPIOC 
#define RS485_CON_PIN               GPIO_PIN_9


/* ÊäÈë */
#define A_GUN_DI_12V_GPIO_CLK       RCU_GPIOB    //AÇ¹12VÊäÈë·´À¡
#define A_GUN_DI_12V_GPIO_PORT      GPIOB
#define A_GUN_DI_12V_PIN            GPIO_PIN_0

#define B_GUN_DI_12V_GPIO_CLK       RCU_GPIOB   // BÇ¹12VÊäÈë·´À¡
#define B_GUN_DI_12V_GPIO_PORT      GPIOB
#define B_GUN_DI_12V_PIN            GPIO_PIN_1

#define A_GUN_DI_LOCK_GPIO_CLK      RCU_GPIOE   // AÇ¹Ëø·´À¡
#define A_GUN_DI_LOCK_GPIO_PORT     GPIOE
#define A_GUN_DI_LOCK_PIN           GPIO_PIN_12

#define B_GUN_DI_LOCK_GPIO_CLK      RCU_GPIOE   // BÇ¹Ëø·´À¡
#define B_GUN_DI_LOCK_GPIO_PORT     GPIOE
#define B_GUN_DI_LOCK_PIN           GPIO_PIN_14

/* Êä³ö */
#define A_GUN_DO_SUPER_GPIO_CLK     RCU_GPIOD   // AÇ¹³¬¼¶³äµç
#define A_GUN_DO_SUPER_GPIO_PORT    GPIOD
#define A_GUN_DO_SUPER_PIN          GPIO_PIN_11

#define B_GUN_DO_SUPER_GPIO_CLK     RCU_GPIOD   // BÇ¹³¬¼¶³äµç
#define B_GUN_DO_SUPER_GPIO_PORT    GPIOD
#define B_GUN_DO_SUPER_PIN          GPIO_PIN_10

#define A_GUN_DO_12V_GPIO_CLK       RCU_GPIOE   // AÇ¹12VÊä³ö
#define A_GUN_DO_12V_GPIO_PORT      GPIOE
#define A_GUN_DO_12V_PIN            GPIO_PIN_7

#define B_GUN_DO_12V_GPIO_CLK       RCU_GPIOE   // BÇ¹12VÊä³ö
#define B_GUN_DO_12V_GPIO_PORT      GPIOE
#define B_GUN_DO_12V_PIN            GPIO_PIN_9

#define A_GUN_DO_24V_GPIO_CLK       RCU_GPIOC  // AÇ¹24VÊä³ö
#define A_GUN_DO_24V_GPIO_PORT      GPIOC
#define A_GUN_DO_24V_PIN            GPIO_PIN_4

#define B_GUN_DO_24V_GPIO_CLK       RCU_GPIOC   // BÇ¹24VÊä³ö
#define B_GUN_DO_24V_GPIO_PORT      GPIOC
#define B_GUN_DO_24V_PIN            GPIO_PIN_5

#define A_GUN_DO_GND_GPIO_CLK       RCU_GPIOE   // AÇ¹GND
#define A_GUN_DO_GND_GPIO_PORT      GPIOE
#define A_GUN_DO_GND_PIN            GPIO_PIN_8

#define B_GUN_DO_GND_GPIO_CLK       RCU_GPIOE    // BÇ¹GND
#define B_GUN_DO_GND_GPIO_PORT      GPIOE
#define B_GUN_DO_GND_PIN            GPIO_PIN_10

#define A_GUN_DO_LOCK_GPIO_CLK      RCU_GPIOE    // AÇ¹Ëø¿ØÖÆ
#define A_GUN_DO_LOCK_GPIO_PORT     GPIOE
#define A_GUN_DO_LOCK_PIN           GPIO_PIN_11

#define B_GUN_DO_LOCK_GPIO_CLK      RCU_GPIOE   // BÇ¹Ëø¿ØÖÆ
#define B_GUN_DO_LOCK_GPIO_PORT     GPIOE
#define B_GUN_DO_LOCK_PIN           GPIO_PIN_13


/* usart */
#define RS485_USART             USART0
#define RS485_USART_CLK         RCU_USART0
#define RS485_USART_TX_PIN      GPIO_PIN_9
#define RS485_USART_RX_PIN      GPIO_PIN_10
#define RS485_USART_GPIO_PORT   GPIOA
#define RS485_USART_GPIO_CLK    RCU_GPIOA
#define RS485_USART_BAUDRATE    115200U

#define ARS232_USART             USART1
#define ARS232_USART_CLK         RCU_USART1
#define ARS232_USART_TX_PIN      GPIO_PIN_2
#define ARS232_USART_RX_PIN      GPIO_PIN_3
#define ARS232_USART_GPIO_PORT   GPIOA
#define ARS232_USART_GPIO_CLK    RCU_GPIOA
#define ARS232_USART_BAUDRATE    115200U

#define BRS232_USART             USART2
#define BRS232_USART_CLK         RCU_USART2
#define BRS232_USART_TX_PIN      GPIO_PIN_8
#define BRS232_USART_RX_PIN      GPIO_PIN_9
#define BRS232_USART_GPIO_PORT   GPIOD
#define BRS232_USART_GPIO_CLK    RCU_GPIOD
#define BRS232_USART_BAUDRATE    115200U

/* HARD I2C */
#define HARD_I2C1_SCL_PIN               GPIO_PIN_10
#define HARD_I2C1_SCL_GPIO_PORT         GPIOB
#define HARD_I2C1_SDA_PIN               GPIO_PIN_11
#define HARD_I2C1_SDA_GPIO_PORT         GPIOB
#define HARD_I2C1_GPIO_CLK              RCU_GPIOB



//Òý½Å×´Ì¬¶ÁÈ¡
#define GET_A_GUN_DI_12V()              gpio_input_bit_get(A_GUN_DI_12V_GPIO_PORT, A_GUN_DI_12V_PIN)
#define GET_B_GUN_DI_12V()              gpio_input_bit_get(B_GUN_DI_12V_GPIO_PORT, B_GUN_DI_12V_PIN)
#define GET_A_GUN_DI_LOCK()             gpio_input_bit_get(A_GUN_DI_LOCK_GPIO_PORT, A_GUN_DI_LOCK_PIN)
#define GET_B_GUN_DI_LOCK()             gpio_input_bit_get(B_GUN_DI_LOCK_GPIO_PORT, B_GUN_DI_LOCK_PIN)
//Òý½Å¿ØÖÆ
#define ENABLE_A_GUN_DO_SUPER()          gpio_bit_set(A_GUN_DO_SUPER_GPIO_PORT, A_GUN_DO_SUPER_PIN)
#define DISABLE_A_GUN_DO_SUPER()         gpio_bit_reset(A_GUN_DO_SUPER_GPIO_PORT, A_GUN_DO_SUPER_PIN)

#define ENABLE_B_GUN_DO_SUPER()          gpio_bit_set(B_GUN_DO_SUPER_GPIO_PORT, B_GUN_DO_SUPER_PIN)
#define DISABLE_B_GUN_DO_SUPER()         gpio_bit_reset(B_GUN_DO_SUPER_GPIO_PORT, B_GUN_DO_SUPER_PIN)

#define ENABLE_A_GUN_DO_12V()            gpio_bit_set(A_GUN_DO_12V_GPIO_PORT, A_GUN_DO_12V_PIN)
#define DISABLE_A_GUN_DO_12V()           gpio_bit_reset(A_GUN_DO_12V_GPIO_PORT, A_GUN_DO_12V_PIN)

#define ENABLE_B_GUN_DO_12V()            gpio_bit_set(B_GUN_DO_12V_GPIO_PORT, B_GUN_DO_12V_PIN)
#define DISABLE_B_GUN_DO_12V()           gpio_bit_reset(B_GUN_DO_12V_GPIO_PORT, B_GUN_DO_12V_PIN)

#define ENABLE_A_GUN_DO_24V()            gpio_bit_set(A_GUN_DO_24V_GPIO_PORT, A_GUN_DO_24V_PIN)
#define DISABLE_A_GUN_DO_24V()           gpio_bit_reset(A_GUN_DO_24V_GPIO_PORT, A_GUN_DO_24V_PIN)

#define ENABLE_B_GUN_DO_24V()            gpio_bit_set(B_GUN_DO_24V_GPIO_PORT, B_GUN_DO_24V_PIN)
#define DISABLE_B_GUN_DO_24V()           gpio_bit_reset(B_GUN_DO_24V_GPIO_PORT, B_GUN_DO_24V_PIN)

#define ENABLE_A_GUN_DO_GND()            gpio_bit_set(A_GUN_DO_GND_GPIO_PORT, A_GUN_DO_GND_PIN)
#define DISABLE_A_GUN_DO_GND()           gpio_bit_reset(A_GUN_DO_GND_GPIO_PORT, A_GUN_DO_GND_PIN)

#define ENABLE_B_GUN_DO_GND()            gpio_bit_set(B_GUN_DO_GND_GPIO_PORT, B_GUN_DO_GND_PIN)
#define DISABLE_B_GUN_DO_GND()           gpio_bit_reset(B_GUN_DO_GND_GPIO_PORT, B_GUN_DO_GND_PIN)

#define ENABLE_A_GUN_DO_LOCK()           gpio_bit_set(A_GUN_DO_LOCK_GPIO_PORT, A_GUN_DO_LOCK_PIN)
#define DISABLE_A_GUN_DO_LOCK()          gpio_bit_reset(A_GUN_DO_LOCK_GPIO_PORT, A_GUN_DO_LOCK_PIN)

#define ENABLE_B_GUN_DO_LOCK()           gpio_bit_set(B_GUN_DO_LOCK_GPIO_PORT, B_GUN_DO_LOCK_PIN)
#define DISABLE_B_GUN_DO_LOCK()          gpio_bit_reset(B_GUN_DO_LOCK_GPIO_PORT, B_GUN_DO_LOCK_PIN)




void rcu_config(void);
void gpio_config(void);
void fwdgt_init(void);

#endif 
