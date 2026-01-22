/*
 * rgb_pwm_IF.h
 *
 *  Created on: 2025年2月10日
 *      Author: qjwu
 */

#ifndef APP_RGB_PWM_RGB_PWM_IF_H_
#define APP_RGB_PWM_RGB_PWM_IF_H_

typedef struct
{
    uint16_t period;
    uint8_t  dutycycle;
    uint8_t current_led;
}Rgb_Pwm_DataSetType;

extern Rgb_Pwm_DataSetType Rgb_Pwm_LED_A[3];
extern Rgb_Pwm_DataSetType Rgb_Pwm_LED_B[3];
extern SemaphoreHandle_t g_sLed_change_sem;

#endif /* APP_RGB_PWM_RGB_PWM_IF_H_ */
