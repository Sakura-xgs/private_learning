#include "hal_uart.h"
#include "gd32f4xx.h"
#include "board.h"


strUSART_MSG g_Usart_0_CommBuf;
strUSART_MSG g_Usart_1_CommBuf;
strUSART_MSG g_Usart_2_CommBuf;

/* Binary semaphore handle definition. */
SemaphoreHandle_t g_USART_0_Recv_Binary_Semaphore = NULL;
SemaphoreHandle_t g_USART_1_Recv_Binary_Semaphore = NULL;
SemaphoreHandle_t g_USART_2_Recv_Binary_Semaphore = NULL;

SemaphoreHandle_t g_USART_0_Send_Binary_Semaphore = NULL;
SemaphoreHandle_t g_USART_1_Send_Binary_Semaphore = NULL;
SemaphoreHandle_t g_USART_2_Send_Binary_Semaphore = NULL;

SemaphoreHandle_t g_USART_0_Comm_MutexSemaphore = NULL;
SemaphoreHandle_t g_USART_1_Comm_MutexSemaphore = NULL;
SemaphoreHandle_t g_USART_2_Comm_MutexSemaphore = NULL;


const strUSART_PARA g_USART_PARA[] =
{
    {0,                       //ConfigNo 
    RS485_USART,              //ComNo
    RS485_USART_CLK,          //ComClk
    RS485_USART_TX_PIN,       //ComTxPin
    RS485_USART_RX_PIN,       //ComRxPin
    RS485_USART_GPIO_PORT,    //ComGpioPort
    RS485_USART_GPIO_CLK,     //GpioPortClk
    RS485_USART_BAUDRATE},    //UsartBaudRate

    {1,                       //ConfigNo 
    ARS232_USART,              //ComNo
    ARS232_USART_CLK,          //ComClk
    ARS232_USART_TX_PIN,       //ComTxPin
    ARS232_USART_RX_PIN,       //ComRxPin
    ARS232_USART_GPIO_PORT,    //ComGpioPort
    ARS232_USART_GPIO_CLK,     //GpioPortClk
    ARS232_USART_BAUDRATE},    //UsartBaudRate    

    {2,                       //ConfigNo 
    BRS232_USART,              //ComNo
    BRS232_USART_CLK,          //ComClk
    BRS232_USART_TX_PIN,       //ComTxPin
    BRS232_USART_RX_PIN,       //ComRxPin
    BRS232_USART_GPIO_PORT,    //ComGpioPort
    BRS232_USART_GPIO_CLK,     //GpioPortClk
    BRS232_USART_BAUDRATE},    //UsartBaudRate        
};
#define BMS_USE_USART_NUM	sizeof(g_USART_PARA)/sizeof(strUSART_PARA)


//在485模式下，切换通信方向
void Usart0_Switch_To_Send_Mode(void)
{
    //拉高PCS CON，PCS485切换为发送模式
    gpio_bit_set(RS485_CON_GPIO_PORT, RS485_CON_PIN);
}

void Usart1_Switch_To_Send_Mode(void)
{
    
}

void Usart2_Switch_To_Send_Mode(void)
{

}

void Usart0_Switch_To_Recv_Mode(void)
{
    //拉低RS485 CON，RS485切换为接收模式
    gpio_bit_reset(RS485_CON_GPIO_PORT, RS485_CON_PIN);
}

void Usart1_Switch_To_Recv_Mode(void)
{
    
}

void Usart2_Switch_To_Recv_Mode(void)
{

}

void usart0_dma_send(uint8_t* buffer, uint16_t len)
{
    xSemaphoreTake(g_USART_0_Comm_MutexSemaphore, portMAX_DELAY);

    //切换为发送模式
    Usart0_Switch_To_Send_Mode();

    /* 设置DMA传输 */
	dma_channel_disable(DMA1, DMA_CH7);		/* 关闭DMA传输才可以进行设置 */
	dma_memory_address_config(DMA1, DMA_CH7, DMA_MEMORY_0, (uint32_t)(buffer));
	dma_transfer_number_config(DMA1, DMA_CH7, len);
	dma_channel_enable(DMA1, DMA_CH7);		/* 开启DMA传输 */
  
    /* waiting for the transfer to complete*/
    if(NULL != g_USART_0_Send_Binary_Semaphore)
    {  
        (void)xSemaphoreTake(g_USART_0_Send_Binary_Semaphore, SEND_TIME_OUT_VAL);   
    }
    else
    {
        vTaskDelay(SEND_TIME_OUT_VAL);      
    } 

	xSemaphoreGive(g_USART_0_Comm_MutexSemaphore);
}

void usart1_dma_send(uint8_t* buffer, uint16_t len)
{
    xSemaphoreTake(g_USART_1_Comm_MutexSemaphore, portMAX_DELAY);

    //切换为发送模式
    Usart1_Switch_To_Send_Mode();
    
    /* 设置DMA传输 */
	dma_channel_disable(DMA0, DMA_CH6);		/* 关闭DMA传输才可以进行设置 */
	dma_memory_address_config(DMA0, DMA_CH6, DMA_MEMORY_0, (uint32_t)(buffer));
	dma_transfer_number_config(DMA0, DMA_CH6, len);
	dma_channel_enable(DMA0, DMA_CH6);		/* 开启DMA传输 */
  
    /* waiting for the transfer to complete*/
    if(NULL != g_USART_1_Send_Binary_Semaphore)
    {  
        (void)xSemaphoreTake(g_USART_1_Send_Binary_Semaphore, SEND_TIME_OUT_VAL);   
    }
    else
    {
        vTaskDelay(SEND_TIME_OUT_VAL);      
    } 

	xSemaphoreGive(g_USART_1_Comm_MutexSemaphore);
}

void usart2_dma_send(uint8_t* buffer, uint16_t len)
{
    xSemaphoreTake(g_USART_2_Comm_MutexSemaphore, portMAX_DELAY);

    //切换为发送模式
    Usart2_Switch_To_Send_Mode();
    
    /* 设置DMA传输 */
	dma_channel_disable(DMA0, DMA_CH3);		/* 关闭DMA传输才可以进行设置 */
	dma_memory_address_config(DMA0, DMA_CH3, DMA_MEMORY_0, (uint32_t)(buffer));
	dma_transfer_number_config(DMA0, DMA_CH3, len);
	dma_channel_enable(DMA0, DMA_CH3);		/* 开启DMA传输 */
  
    /* waiting for the transfer to complete*/
    if(NULL != g_USART_2_Send_Binary_Semaphore)
    {  
        (void)xSemaphoreTake(g_USART_2_Send_Binary_Semaphore, SEND_TIME_OUT_VAL);   
    }
    else
    {
        vTaskDelay(SEND_TIME_OUT_VAL);      
    } 

	xSemaphoreGive(g_USART_2_Comm_MutexSemaphore);
}

static void usart0_dma_txinit(void)
{	
    /* 定义一个DMA配置结构体 */
	dma_single_data_parameter_struct dma_init_struct;
	
	/* 初始化 DMA1 通道7， usart0 tx */
    dma_deinit(DMA1, DMA_CH7);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;   // 存储器到外设方向
    dma_init_struct.memory0_addr = NULL;    // 存储器基地址
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;    // 存储器地址自增
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;    // 外设数据位宽为8位
    dma_init_struct.number = 0;                                     // 传输数据个数
    dma_init_struct.periph_addr = ((uint32_t)&USART_DATA(USART0));     // 外设基地址，即USART数据寄存器地址
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;    // 外设地址固定不变
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;    // 软件优先级为极高

    dma_single_data_mode_init(DMA1, DMA_CH7, &dma_init_struct);

    dma_circulation_disable(DMA1, DMA_CH7);
    dma_channel_subperipheral_select(DMA1, DMA_CH7, DMA_SUBPERI4);


    /* USART DMA 发送使能 */
    usart_dma_transmit_config(USART0, USART_TRANSMIT_DMA_ENABLE);

	/* DMA1 通道7 中断优先级设置并使能 */
	nvic_irq_enable(DMA1_Channel7_IRQn, 3, 0);

    /* enable DMA transfer complete interrupt */
    dma_interrupt_enable(DMA1, DMA_CH7, DMA_CHXCTL_FTFIE);
    
    /* 使能 DMA1 通道7 */
    dma_channel_enable(DMA1, DMA_CH7);
}

static void usart0_dma_rxinit(void)
{
    /* 定义一个DMA配置结构体 */
	dma_single_data_parameter_struct dma_init_struct;
	
	/* 初始化 DMA1 通道2， usart0 rx */
    dma_deinit(DMA1, DMA_CH2);
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;   // 外设到存储器方向
    dma_init_struct.memory0_addr = (uint32_t)g_Usart_0_CommBuf.u8Recvbuf;    // 存储器基地址
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;    // 存储器地址自增
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;    // 外设数据位宽为8位
    dma_init_struct.number = USART_COMM_BUF_LEN;                                     // 传输数据个数
    dma_init_struct.periph_addr = ((uint32_t)&USART_DATA(USART0));     // 外设基地址，即USART数据寄存器地址
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;    // 外设地址固定不变
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;    // 软件优先级为极高

    dma_single_data_mode_init(DMA1, DMA_CH2, &dma_init_struct);

    dma_circulation_disable(DMA1, DMA_CH2);
    dma_channel_subperipheral_select(DMA1, DMA_CH2, DMA_SUBPERI4);

    /* USART DMA 发送和接收使能 */
    usart_dma_receive_config(USART0, USART_RECEIVE_DMA_ENABLE);

	/* DMA1 通道2 中断优先级设置并使能 */
	nvic_irq_enable(DMA1_Channel2_IRQn, 3, 0);

	/* 使能 DMA1 通道2 传输错误中断 */
    dma_interrupt_flag_clear(DMA1, DMA_CH2, DMA_INT_FLAG_TAE);
    // dma_interrupt_enable(DMA0, DMA_CH4, DMA_INT_FTF|DMA_INT_ERR);
    dma_interrupt_enable(DMA1, DMA_CH2, DMA_CHXCTL_TAEIE);

    /* 使能 DMA1 通道2 */
    dma_channel_enable(DMA1, DMA_CH2);
	
	/* USART中断设置，抢占优先级0，子优先级0 */
	nvic_irq_enable(USART0_IRQn, 3, 0); 

	/* 使能USART0空闲中断 */
    usart_interrupt_flag_clear(USART0, USART_INT_FLAG_IDLE);	/* 清除空闲中断标志位 */
    usart_interrupt_enable(USART0, USART_INT_IDLE);

    usart_interrupt_flag_clear(USART0, USART_INT_FLAG_TC);	    /* 清除发送完成中断标志位 */
    usart_interrupt_enable(USART0, USART_INT_TC);
}

static void usart1_dma_txinit(void)
{	
    /* 定义一个DMA配置结构体 */
	dma_single_data_parameter_struct dma_init_struct;
	
	/* 初始化 DMA0 通道6， usart1 tx */
    dma_deinit(DMA0, DMA_CH6);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;   // 存储器到外设方向
    dma_init_struct.memory0_addr = NULL;    // 存储器基地址
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;    // 存储器地址自增
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;    // 外设数据位宽为8位
    dma_init_struct.number = 0;                                     // 传输数据个数
    dma_init_struct.periph_addr = ((uint32_t)&USART_DATA(USART1));     // 外设基地址，即USART数据寄存器地址
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;    // 外设地址固定不变
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;    // 软件优先级为极高

    dma_single_data_mode_init(DMA0, DMA_CH6, &dma_init_struct);

    dma_circulation_disable(DMA0, DMA_CH6);
    dma_channel_subperipheral_select(DMA0, DMA_CH6, DMA_SUBPERI4);

    /* USART DMA 发送使能 */
    usart_dma_transmit_config(USART1, USART_TRANSMIT_DMA_ENABLE);

	/* DMA0 通道6 中断优先级设置并使能 */
	nvic_irq_enable(DMA0_Channel6_IRQn, 3, 0);

    /* enable DMA transfer complete interrupt */
    dma_interrupt_enable(DMA0, DMA_CH6, DMA_CHXCTL_FTFIE);

    /* 使能 DMA0 通道6 */
    dma_channel_enable(DMA0, DMA_CH6);
}

static void usart1_dma_rxinit(void)
{
	/* 定义一个DMA配置结构体 */
	dma_single_data_parameter_struct dma_init_struct;
	
	/* 初始化 DMA0 通道5 */
    dma_deinit(DMA0, DMA_CH5);
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;   //  外设到存储器方向
    dma_init_struct.memory0_addr = (uint32_t)g_Usart_1_CommBuf.u8Recvbuf;    // 存储器基地址
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;    // 存储器地址自增
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;    // 外设数据位宽为8位
    dma_init_struct.number = USART_COMM_BUF_LEN;                                     // 传输数据个数
    dma_init_struct.periph_addr = ((uint32_t)&USART_DATA(USART1));     // 外设基地址，即USART数据寄存器地址
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;    // 外设地址固定不变
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;    // 软件优先级为极高

    dma_single_data_mode_init(DMA0, DMA_CH5, &dma_init_struct);

    dma_circulation_disable(DMA0, DMA_CH5);
    dma_channel_subperipheral_select(DMA0, DMA_CH5, DMA_SUBPERI4);

    /* USART DMA 发送和接收使能 */
    usart_dma_receive_config(USART1, USART_RECEIVE_DMA_ENABLE);

	/* DMA0 通道4 中断优先级设置并使能 */
	nvic_irq_enable(DMA0_Channel5_IRQn, 3, 0);

	/* 使能 DMA0 通道4 传输错误中断 */
    dma_interrupt_flag_clear(DMA0, DMA_CH5, DMA_INT_FLAG_TAE);
    // dma_interrupt_enable(DMA0, DMA_CH5, DMA_INT_FTF|DMA_INT_ERR);
    dma_interrupt_enable(DMA0, DMA_CH5, DMA_CHXCTL_TAEIE);

    /* 使能 DMA0 通道4 */
    dma_channel_enable(DMA0, DMA_CH5);
	
	/* USART中断设置，抢占优先级0，子优先级0 */
	nvic_irq_enable(USART1_IRQn, 3, 0); 

	/* 使能USART0空闲中断 */
    usart_interrupt_flag_clear(USART1, USART_INT_FLAG_IDLE);	/* 清除空闲中断标志位 */
    usart_interrupt_enable(USART1, USART_INT_IDLE);

    usart_interrupt_flag_clear(USART1, USART_INT_FLAG_TC);	    /* 清除发送完成中断标志位 */
    usart_interrupt_enable(USART1, USART_INT_TC);
}

static void usart2_dma_txinit(void)
{	
    /* 定义一个DMA配置结构体 */
	dma_single_data_parameter_struct dma_init_struct;
	
	/* 初始化 DMA0 通道3， usart2 tx */
    dma_deinit(DMA0, DMA_CH3);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;   // 存储器到外设方向
    dma_init_struct.memory0_addr = NULL;    // 存储器基地址
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;    // 存储器地址自增
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;    // 外设数据位宽为8位
    dma_init_struct.number = 0;                                     // 传输数据个数
    dma_init_struct.periph_addr = ((uint32_t)&USART_DATA(USART2));     // 外设基地址，即USART数据寄存器地址
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;    // 外设地址固定不变
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;    // 软件优先级为极高

    dma_single_data_mode_init(DMA0, DMA_CH3, &dma_init_struct);

    dma_circulation_disable(DMA0, DMA_CH3);
    dma_channel_subperipheral_select(DMA0, DMA_CH3, DMA_SUBPERI4);

    /* USART DMA 发送使能 */
    usart_dma_transmit_config(USART2, USART_TRANSMIT_DMA_ENABLE);

	/* DMA0 通道6 中断优先级设置并使能 */
	nvic_irq_enable(DMA0_Channel3_IRQn, 3, 0);

    /* enable DMA transfer complete interrupt */
    dma_interrupt_enable(DMA0, DMA_CH3, DMA_CHXCTL_FTFIE);

    /* 使能 DMA0 通道1 */
    dma_channel_enable(DMA0, DMA_CH3);
}

static void usart2_dma_rxinit(void)
{
	/* 定义一个DMA配置结构体 */
	dma_single_data_parameter_struct dma_init_struct;
	
	/* 初始化 DMA0 通道1 */
    dma_deinit(DMA0, DMA_CH1);
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;   //  外设到存储器方向
    dma_init_struct.memory0_addr = (uint32_t)g_Usart_2_CommBuf.u8Recvbuf;    // 存储器基地址
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;    // 存储器地址自增
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;    // 外设数据位宽为8位
    dma_init_struct.number = USART_COMM_BUF_LEN;                                     // 传输数据个数
    dma_init_struct.periph_addr = ((uint32_t)&USART_DATA(USART2));     // 外设基地址，即USART数据寄存器地址
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;    // 外设地址固定不变
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;    // 软件优先级为极高

    dma_single_data_mode_init(DMA0, DMA_CH1, &dma_init_struct);

    dma_circulation_disable(DMA0, DMA_CH1);
    dma_channel_subperipheral_select(DMA0, DMA_CH1, DMA_SUBPERI4);

    /* USART DMA 发送和接收使能 */
    usart_dma_receive_config(USART2, USART_RECEIVE_DMA_ENABLE);

	/* DMA0 通道2 中断优先级设置并使能 */
	nvic_irq_enable(DMA0_Channel1_IRQn, 3, 0);

	/* 使能 DMA0 通道2 传输错误中断 */
    dma_interrupt_flag_clear(DMA0, DMA_CH1, DMA_INT_FLAG_TAE);
    dma_interrupt_enable(DMA0, DMA_CH1, DMA_CHXCTL_TAEIE);

    /* 使能 DMA0 通道2 */
    dma_channel_enable(DMA0, DMA_CH1);
	
	/* USART中断设置，抢占优先级0，子优先级0 */
	nvic_irq_enable(USART2_IRQn, 3, 0); 

	/* 使能USART0空闲中断 */
    usart_interrupt_flag_clear(USART2, USART_INT_FLAG_IDLE);	/* 清除空闲中断标志位 */
    usart_interrupt_enable(USART2, USART_INT_IDLE);

    usart_interrupt_flag_clear(USART2, USART_INT_FLAG_TC);	    /* 清除发送完成中断标志位 */
    usart_interrupt_enable(USART2, USART_INT_TC);
}


/*!
    \brief      configure COM port
    \param[in]  com: COM NO
    \param[out] none
    \retval     none
*/
static void usart_config(uint32_t com, uint32_t baudrate)
{
    uint32_t u32UsartComNo = 0;
	uint32_t u32UsartComClk = 0;
	uint32_t u32UsartComTxPin = 0;
	uint32_t u32UsartComRxPin = 0;
	uint32_t u32UsartComGpioPort = 0;
	uint32_t u32UsartComGpioPortClk = 0;

    u32UsartComNo = g_USART_PARA[com].UsartComNo;
    u32UsartComClk = g_USART_PARA[com].UsartComClk;
    u32UsartComTxPin = g_USART_PARA[com].UsartComTxPin;
    u32UsartComRxPin = g_USART_PARA[com].UsartComRxPin;
    u32UsartComGpioPort = g_USART_PARA[com].UsartComGpioPort;
    u32UsartComGpioPortClk = g_USART_PARA[com].UsartComGpioPortClk;
    
    /* enable GPIO clock */
    rcu_periph_clock_enable((rcu_periph_enum)u32UsartComGpioPortClk);

    /* enable USART clock */
    rcu_periph_clock_enable((rcu_periph_enum)u32UsartComClk);

    /* connect port to USARTx_Tx */
    gpio_af_set(u32UsartComGpioPort, GPIO_AF_7, u32UsartComTxPin);
    gpio_mode_set(u32UsartComGpioPort, GPIO_MODE_AF, GPIO_PUPD_PULLUP,u32UsartComTxPin);
    gpio_output_options_set(u32UsartComGpioPort, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,u32UsartComTxPin);    

    /* connect port to USARTx_Rx */
    gpio_af_set(u32UsartComGpioPort, GPIO_AF_7, u32UsartComRxPin);
    gpio_mode_set(u32UsartComGpioPort, GPIO_MODE_AF, GPIO_PUPD_PULLUP,u32UsartComRxPin);
    gpio_output_options_set(u32UsartComGpioPort, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,u32UsartComRxPin);    

    /* USART configure */
    usart_deinit(u32UsartComNo);
    usart_baudrate_set(u32UsartComNo, baudrate);
    usart_word_length_set(u32UsartComNo, USART_WL_8BIT);
    usart_stop_bit_set(u32UsartComNo, USART_STB_1BIT);
    usart_parity_config(u32UsartComNo, USART_PM_NONE);
    usart_hardware_flow_rts_config(u32UsartComNo, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(u32UsartComNo, USART_CTS_DISABLE);
    usart_receive_config(u32UsartComNo, USART_RECEIVE_ENABLE);
    usart_transmit_config(u32UsartComNo, USART_TRANSMIT_ENABLE);
    usart_enable(u32UsartComNo);

    switch (u32UsartComNo)
    {
        case USART0:
            usart0_dma_txinit();
            usart0_dma_rxinit();
            break;

        case USART1:
            usart1_dma_txinit();
            usart1_dma_rxinit();
            break;

        case USART2:
            usart2_dma_txinit();
            usart2_dma_rxinit();
            break;
    
        default:
            break;
    }
}


void USART_INIT(void)
{
    U8 u8UsartCnt = 0;

    /* create a binary semaphore. */
    g_USART_0_Recv_Binary_Semaphore = xSemaphoreCreateBinary();  
    g_USART_1_Recv_Binary_Semaphore = xSemaphoreCreateBinary();  
    g_USART_2_Recv_Binary_Semaphore = xSemaphoreCreateBinary();  

    g_USART_0_Send_Binary_Semaphore = xSemaphoreCreateBinary();  
    g_USART_1_Send_Binary_Semaphore = xSemaphoreCreateBinary();  
    g_USART_2_Send_Binary_Semaphore = xSemaphoreCreateBinary(); 
    
    g_USART_0_Comm_MutexSemaphore = xSemaphoreCreateMutex(); 
    g_USART_1_Comm_MutexSemaphore = xSemaphoreCreateMutex();
    g_USART_2_Comm_MutexSemaphore = xSemaphoreCreateMutex();


    for(u8UsartCnt = 0; u8UsartCnt < BMS_USE_USART_NUM; u8UsartCnt++)
    {
        usart_config(g_USART_PARA[u8UsartCnt].ConfigNo, g_USART_PARA[u8UsartCnt].UsartBaudRate);
    }
}


