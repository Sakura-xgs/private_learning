#ifndef FAN_PWM_H_
#define FAN_PWM_H_
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_qtmr.h"
#include "MIMXRT1021.h"
#include "peripherals.h"

#define FAN_PWM_CHANNEL_CLOCK_PERIOD        (TMR2_CHANNEL_2_CLOCK_SOURCE)

#define FAN_PWM_CHANNEL1                    (TMR2_CHANNEL_2_CHANNEL)

#endif
