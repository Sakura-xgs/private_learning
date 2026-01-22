#include "board.h"
#include "gd32f4xx.h"
#include "PublicDefine.h"




/*!
    \brief      configure the different system clocks
    \param[in]  none
    \param[out] none
    \retval     none
*/
void rcu_config(void)
{
    /* enable GPIO clock */
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);

    /* enable SPI0 clock */
    rcu_periph_clock_enable(RCU_SPI0);

    /* enable CAN clock */
    rcu_periph_clock_enable(RCU_CAN0);    

    /* enable ADC clock */
    rcu_periph_clock_enable(RCU_ADC0);

    /* enable DMA0 clock */
    rcu_periph_clock_enable(RCU_DMA0);

    /* enable DMA1 clock */
    rcu_periph_clock_enable(RCU_DMA1);

    /* config ADC clock */
  //  rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV6);    
	adc_clock_config(ADC_ADCCK_PCLK2_DIV6);
	
 //   rcu_periph_clock_enable(RCU_AF);
    
    /* enable IRC40K */
  //  rcu_osci_on(RCU_IRC40K);  
    rcu_osci_on(RCU_IRC32K); 	
	

    /* wait till IRC40K is ready */
	//	while(SUCCESS != rcu_osci_stab_wait(RCU_IRC40K)){
    while(SUCCESS != rcu_osci_stab_wait(RCU_IRC32K)){
    }
}

void gpio_config(void)
{
    gpio_mode_set(RS485_CON_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, RS485_CON_PIN);
    gpio_output_options_set(RS485_CON_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, RS485_CON_PIN);    

    gpio_mode_set(A_GUN_DI_12V_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, A_GUN_DI_12V_PIN);
    gpio_mode_set(B_GUN_DI_12V_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, B_GUN_DI_12V_PIN);
    gpio_mode_set(A_GUN_DI_LOCK_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, A_GUN_DI_LOCK_PIN);
    gpio_mode_set(B_GUN_DI_LOCK_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, B_GUN_DI_LOCK_PIN);

    gpio_mode_set(A_GUN_DO_SUPER_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, A_GUN_DO_SUPER_PIN);
    gpio_output_options_set(A_GUN_DO_SUPER_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, A_GUN_DO_SUPER_PIN);        

    gpio_mode_set(B_GUN_DO_SUPER_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, B_GUN_DO_SUPER_PIN);
    gpio_output_options_set(B_GUN_DO_SUPER_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, B_GUN_DO_SUPER_PIN); 
    
    gpio_mode_set(A_GUN_DO_12V_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, A_GUN_DO_12V_PIN);
    gpio_output_options_set(A_GUN_DO_12V_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, A_GUN_DO_12V_PIN);  
    
    gpio_mode_set(B_GUN_DO_12V_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, B_GUN_DO_12V_PIN);
    gpio_output_options_set(B_GUN_DO_12V_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, B_GUN_DO_12V_PIN); 
    
    gpio_mode_set(A_GUN_DO_24V_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, A_GUN_DO_24V_PIN);
    gpio_output_options_set(A_GUN_DO_24V_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, A_GUN_DO_24V_PIN);    
    
    gpio_mode_set(B_GUN_DO_24V_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, B_GUN_DO_24V_PIN);
    gpio_output_options_set(B_GUN_DO_24V_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, B_GUN_DO_24V_PIN);     

    gpio_mode_set(A_GUN_DO_GND_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, A_GUN_DO_GND_PIN);
    gpio_output_options_set(A_GUN_DO_GND_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, A_GUN_DO_GND_PIN);        

    gpio_mode_set(B_GUN_DO_GND_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, B_GUN_DO_GND_PIN);
    gpio_output_options_set(B_GUN_DO_GND_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, B_GUN_DO_GND_PIN);           

    gpio_mode_set(A_GUN_DO_LOCK_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, A_GUN_DO_LOCK_PIN);
    gpio_output_options_set(A_GUN_DO_LOCK_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, A_GUN_DO_LOCK_PIN);  
    
    gpio_mode_set(B_GUN_DO_LOCK_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, B_GUN_DO_LOCK_PIN);
    gpio_output_options_set(B_GUN_DO_LOCK_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, B_GUN_DO_LOCK_PIN);            
}

/*!
    \brief      fwdgt init
    \param[in]  none
    \param[out] none
    \retval     none
*/
void fwdgt_init(void)
{
    /* confiure FWDGT counter clock: 40KHz(IRC40K) / 128 = 0.3125 KHz */
    fwdgt_config(3125, FWDGT_PSC_DIV128);
    
    /* After 10 seconds to generate a reset */
    fwdgt_enable();

    dbg_periph_enable(DBG_FWDGT_HOLD);
}

