#include "led.h"
#include "gd32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "board.h"

static void LedInit(void)
{
    /* enable the led clock */
    rcu_periph_clock_enable(RUN_LED_GPIO_CLK);
    /* configure led GPIO port */ 
    gpio_mode_set(RUN_LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,RUN_LED_PIN);
	gpio_output_options_set(RUN_LED_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,RUN_LED_PIN);
    gpio_bit_set(RUN_LED_PORT, RUN_LED_PIN); // Set the LED pin to high (LED off)
}

void LedToggle(void)
{
    /* toggle the led */
    gpio_bit_toggle(RUN_LED_PORT, RUN_LED_PIN);
}

void LedTask(void *pvParameters)
{
    LedInit();

    while (1) {
		/* reload FWDGT counter */
        fwdgt_counter_reload();

        LedToggle();

        vTaskDelay(250);
    }
}


