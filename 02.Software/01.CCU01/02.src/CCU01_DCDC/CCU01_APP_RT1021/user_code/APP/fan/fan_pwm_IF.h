#ifndef APP_FAN_PWM_RGB_PWM_IF_H_
#define APP_FAN_PWM_RGB_PWM_IF_H_

typedef struct
{
    uint16_t period;
    uint8_t  dutycycle;
    uint8_t current_led;
}Fan_Pwm_DataSetType;

extern Fan_Pwm_DataSetType Fan_Pwm;

extern void Fan_Pwm_Init_Task(void * pvParameters);

#endif /* APP_RGB_PWM_RGB_PWM_IF_H_ */
