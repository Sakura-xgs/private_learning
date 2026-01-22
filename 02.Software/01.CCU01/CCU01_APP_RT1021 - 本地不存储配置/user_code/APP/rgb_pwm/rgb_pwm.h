#ifndef RGB_PWM_H_
#define RGB_PWM_H_
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_qtmr.h"
#include "MIMXRT1021.h"
#include "peripherals.h"

#define SET_PERIODANDDUTY_TASK_PRIO         (5U)

#define RGB_PWM_CHANNEL_SUM_NUM             (6U)

#define RGB_PWM_CHANNEL_CLOCK_PERIOD        (TMR1_CHANNEL_0_CLOCK_SOURCE)

#define RGB_PWM_CHANNEL1                    (TMR1_CHANNEL_0_CHANNEL)
#define RGB_PWM_CHANNEL2                    (TMR1_CHANNEL_1_CHANNEL)
#define RGB_PWM_CHANNEL3                    (TMR1_CHANNEL_2_CHANNEL)
#define RGB_PWM_CHANNEL4                    (TMR1_CHANNEL_3_CHANNEL)
#define RGB_PWM_CHANNEL5                    (TMR2_CHANNEL_0_CHANNEL)
#define RGB_PWM_CHANNEL6                    (TMR2_CHANNEL_1_CHANNEL)

extern void Rgb_Pwm_Init_Task(void * pvParameters);
#endif
