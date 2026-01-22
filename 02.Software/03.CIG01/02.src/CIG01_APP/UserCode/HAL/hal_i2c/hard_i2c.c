#include "hard_i2c.h"
#include "gd32f4xx.h"
#include "gd32f4xx_libopt.h"
#include "board.h"

SemaphoreHandle_t g_HARD_I2C1_MutexSemaphore = NULL;

static void Delay_I2C(uint32_t i)
{
    while(i--);
}

static void Stop_IIC(uint32_t I2Cx)
{
	i2c_stop_on_bus(I2Cx);
}

static void GPIO_Configuration_I2C(uint32_t I2Cx)
{
    uint32_t GPIO_SDA;
    uint32_t GPIO_SCL;
    uint32_t GPIO_Pin_SDA,GPIO_Pin_SCL;

    if(I2Cx == I2C1)
    {
        rcu_periph_reset_enable(RCU_I2C1RST);
        rcu_periph_reset_disable(RCU_I2C1RST);

        /* enable GPIOB clock */
        rcu_periph_clock_enable(HARD_I2C1_GPIO_CLK);
        /* enable BOARD_I2C APB1 clock */
        rcu_periph_clock_enable(RCU_I2C1);

        GPIO_SCL = HARD_I2C1_SCL_GPIO_PORT;
        GPIO_Pin_SCL = HARD_I2C1_SCL_PIN;
        GPIO_SDA = HARD_I2C1_SDA_GPIO_PORT;
        GPIO_Pin_SDA = HARD_I2C1_SDA_PIN;        
    }

    /* configure  I2C2 GPIO */
    /* connect I2C_SCL_PIN to I2C_SCL */
    gpio_af_set(GPIO_SCL, GPIO_AF_4, GPIO_Pin_SCL);
    /* connect I2C_SDA_PIN to I2C_SDA */
    gpio_af_set(GPIO_SDA, GPIO_AF_4, GPIO_Pin_SDA);
    /* configure GPIO pins of I2C */
    gpio_mode_set(GPIO_SCL, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_Pin_SCL);
    gpio_output_options_set(GPIO_SCL, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO_Pin_SCL);
    gpio_mode_set(GPIO_SDA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_Pin_SDA);
    gpio_output_options_set(GPIO_SDA, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO_Pin_SDA);
}

static void Hard_I2C_Init(uint32_t I2Cx)
{
    GPIO_Configuration_I2C(I2Cx);

    i2c_clock_config(I2Cx, 40000, I2C_DTCY_2);
    // i2c_clock_config(I2Cx, 30000, I2C_DTCY_2);
    /* I2C address configure */
    i2c_mode_addr_config(I2Cx, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, 0);
    /* enable acknowledge */
    i2c_ack_config(I2Cx, I2C_ACK_ENABLE);
    /* enable I2Cx */
    i2c_enable(I2Cx);
}

static void Resume_IIC(uint32_t Timeout,uint32_t I2Cx )
{
    uint32_t GPIO_SDA =0;
    uint32_t GPIO_SCL =0;
    uint32_t GPIO_Pin_SDA =0,GPIO_Pin_SCL =0;

    if(I2Cx==I2C1)
    {
        /* enable GPIOB clock */
        rcu_periph_clock_enable(HARD_I2C1_GPIO_CLK);
        /* enable BOARD_I2C APB1 clock */
        rcu_periph_clock_disable(RCU_I2C1);

        GPIO_SCL = HARD_I2C1_SCL_GPIO_PORT;
        GPIO_Pin_SCL = HARD_I2C1_SCL_PIN;
        GPIO_SDA = HARD_I2C1_SDA_GPIO_PORT;
        GPIO_Pin_SDA = HARD_I2C1_SDA_PIN;
    }

    do
    {
        gpio_mode_set(GPIO_SCL, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_Pin_SCL);
        gpio_output_options_set(GPIO_SCL, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_Pin_SCL);
        gpio_mode_set(GPIO_SDA, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_Pin_SDA);
        gpio_output_options_set(GPIO_SDA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_Pin_SDA);
        gpio_bit_reset(GPIO_SCL, GPIO_Pin_SCL);
        Delay_I2C(20);
        gpio_bit_reset(GPIO_SDA, GPIO_Pin_SDA);
        Delay_I2C(20);
        gpio_bit_set(GPIO_SCL, GPIO_Pin_SCL);
        Delay_I2C(20);
        gpio_bit_set(GPIO_SDA, GPIO_Pin_SDA);
        Delay_I2C(20);

        if(Timeout-- == 0) return;

    }while((!gpio_input_bit_get(GPIO_SDA, GPIO_Pin_SDA))||(!gpio_input_bit_get(GPIO_SCL, GPIO_Pin_SCL)));

    Hard_I2C_Init(I2Cx);
}

/*!
    \brief      I2Cx Read NBytes
    \param[in]  i2c_periph  : I2Cx(x=0,1)
    \param[in]  driver Addr : slave address
    \param[in]  start_Addr  : reg
    \param[in]  number_Bytes: number to read
    \param[out] read_Buffer
    \param[in]  ADDR_Length : number of the addr
*/
I2C_Status I2Cx_Read_NBytes(uint32_t I2Cx,uint8_t driver_Addr, uint16_t start_Addr, uint8_t number_Bytes, uint8_t *read_Buffer,uint8_t ADDR_Length)
{
    uint32_t I2C_Timeout = I2C_SHORT_TIMEOUT;
	i2c_ack_config(I2Cx,I2C_ACK_ENABLE);

    /* wait until I2C bus is idle */
    while(i2c_flag_get(I2Cx, I2C_FLAG_I2CBSY))
    {
        if((I2C_Timeout--) == 0)
        {
		    Stop_IIC(I2Cx);
		    return I2C_FAIL;
        }
    }

    if(number_Bytes==2)
    {
        i2c_ackpos_config(I2Cx,I2C_ACKPOS_NEXT);
    }
    
    /* send a start condition to I2C bus */
    i2c_start_on_bus(I2Cx);

     /* wait until SBSEND bit is set */
	I2C_Timeout = I2C_SHORT_TIMEOUT;
    while(!i2c_flag_get(I2Cx, I2C_FLAG_SBSEND))
    {
        if((I2C_Timeout--) == 0)
        {
            Stop_IIC(I2Cx);
        return I2C_FAIL;
        }
    }

    /* send slave address */
	i2c_master_addressing(I2Cx, driver_Addr, I2C_TRANSMITTER);

    /* wait until ADDSEND bit is set */
	I2C_Timeout = I2C_SHORT_TIMEOUT;
    while(!i2c_flag_get(I2Cx, I2C_FLAG_ADDSEND))
    {
        if((I2C_Timeout--) == 0)
        {
            Stop_IIC(I2Cx);
        return I2C_FAIL;
        }
    }

    /* clear the ADDSEND bit */
    i2c_flag_clear(I2Cx,I2C_FLAG_ADDSEND);

    /* wait until the transmit data buffer is empty */
    I2C_Timeout = I2C_SHORT_TIMEOUT;
    while(SET != i2c_flag_get( I2Cx , I2C_FLAG_TBE ))
    {
        if((I2C_Timeout--) == 0)
        {
        Stop_IIC(I2Cx);
        return I2C_FAIL;
        }
    }

    i2c_enable(I2Cx);

     /* send the EEPROM's internal address to write to */
	if(ADDR_Length)//两字节地址
	{
        i2c_data_transmit(I2Cx, (uint8_t)((start_Addr & 0xFF00) >> 8));
        I2C_Timeout = I2C_SHORT_TIMEOUT;
        //while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTING))
        while(!i2c_flag_get(I2Cx, I2C_FLAG_BTC))
        {
            if((I2C_Timeout--) == 0)
            {
                Stop_IIC(I2Cx);
                return I2C_FAIL;
            }
        }
        i2c_data_transmit(I2Cx, (uint8_t)(start_Addr & 0x00FF));
    }
	else
	{
		i2c_data_transmit(I2Cx, start_Addr);
	}

    /* wait until BTC bit is set */
	I2C_Timeout = I2C_SHORT_TIMEOUT;
    while(!i2c_flag_get(I2Cx, I2C_FLAG_BTC))
    {
        if((I2C_Timeout--) == 0)
        {
                Stop_IIC(I2Cx);
        return I2C_FAIL;
        }
    }

    i2c_start_on_bus(I2Cx);
    I2C_Timeout = I2C_SHORT_TIMEOUT;
    while(!i2c_flag_get(I2Cx, I2C_FLAG_SBSEND))
    {
        if((I2C_Timeout--) == 0)
        {
            Stop_IIC(I2Cx);
        return I2C_FAIL;
        }
    }


	i2c_master_addressing(I2Cx, driver_Addr,  I2C_RECEIVER);
	I2C_Timeout = I2C_SHORT_TIMEOUT;
	if(number_Bytes<3)
	{
        i2c_ack_config(I2Cx,I2C_ACK_DISABLE);
    }

    while(!i2c_flag_get(I2Cx, I2C_FLAG_ADDSEND))
    {
        if((I2C_Timeout--) == 0)
        {
                Stop_IIC(I2Cx);
        return I2C_FAIL;
        }
    }

    /* clear the ADDSEND bit */
    i2c_flag_clear(I2Cx,I2C_FLAG_ADDSEND);
    if(number_Bytes==1)
    {
        i2c_stop_on_bus(I2Cx);
    }

    while(number_Bytes)
    {
        if(3 == number_Bytes){
        /* wait until BTC bit is set */

        I2C_Timeout = I2C_LONG_TIMEOUT;
        while(!i2c_flag_get(I2Cx, I2C_FLAG_BTC))
        {
            if((I2C_Timeout--) == 0)
            {
            Stop_IIC(I2Cx);
            return I2C_FAIL;
            }
        }

    /* disable acknowledge */
    /* disable acknowledge */
    i2c_ack_config(I2Cx,I2C_ACK_DISABLE);
    }
    if(2 == number_Bytes){
      /* wait until BTC bit is set */
        I2C_Timeout = I2C_LONG_TIMEOUT;
        while(!i2c_flag_get(I2Cx, I2C_FLAG_BTC))
        {
            if((I2C_Timeout--) == 0)
            {
            Stop_IIC(I2Cx);
            return I2C_FAIL;
            }
        }

        /* send a stop condition to I2C bus */
        i2c_stop_on_bus(I2Cx);
        I2C_Timeout = I2C_SHORT_TIMEOUT;
        while (I2C_CTL0(I2Cx) & 0x0200)
        {
            if((I2C_Timeout--) == 0)
            {
            Stop_IIC(I2Cx);
            return I2C_FAIL;
            }
        }
        }

        /* wait until the RBNE bit is set and clear it */
        if(i2c_flag_get(I2Cx, I2C_FLAG_RBNE)){
        /* read a byte from the EEPROM */
        *read_Buffer = i2c_data_receive(I2Cx);

        /* point to the next location where the byte read will be saved */
        read_Buffer++;

        /* decrement the read bytes counter */
        number_Bytes--;
        }
    }
    while(I2C_CTL0(I2Cx)&I2C_CTL0_STOP)
    {
        if((I2C_Timeout--) == 0)
        {
            Stop_IIC(I2Cx);
            return I2C_FAIL;
            }
    }

    /* enable acknowledge */
    i2c_ack_config(I2Cx,I2C_ACK_ENABLE);

    i2c_ackpos_config(I2Cx,I2C_ACKPOS_CURRENT);
    return I2C_OK;
}


uint8_t g_u8ErrStep = 0;
/*!
    \brief      I2Cx Write NBytes
    \param[in]  i2c_periph  : I2Cx(x=0,1)
    \param[in]  addr        : slave address
    \param[in]  start_Addr  : reg
    \param[in]  number_Bytes: number to Write
    \param[in]  ADDR_Length : number of the addr
*/
I2C_Status I2Cx_Write_NBytes(uint32_t I2Cx,uint8_t driver_Addr, uint16_t start_Addr, uint8_t number_Bytes, uint8_t *write_Buffer,uint8_t ADDR_Length)
{
    uint32_t I2C_Timeout = I2C_SHORT_TIMEOUT;
    i2c_ack_config(I2Cx,I2C_ACK_ENABLE);

    while(i2c_flag_get(I2Cx, I2C_FLAG_I2CBSY))
    {
        if((I2C_Timeout--) == 0)
        {
                Stop_IIC(I2Cx);
        g_u8ErrStep = 0x01;
        return I2C_FAIL;
        }
    }

    i2c_start_on_bus(I2Cx);
    I2C_Timeout = I2C_SHORT_TIMEOUT;
    while(!i2c_flag_get(I2Cx, I2C_FLAG_SBSEND))
    {
        if((I2C_Timeout--) == 0)
        {
            Stop_IIC(I2Cx);
        g_u8ErrStep = 0x02;
        return I2C_FAIL;
        }
    }

    i2c_master_addressing(I2Cx, driver_Addr, I2C_TRANSMITTER);
    I2C_Timeout = I2C_SHORT_TIMEOUT;
    while(!i2c_flag_get(I2Cx, I2C_FLAG_ADDSEND))
    {
        if((I2C_Timeout--) == 0)
        {
                Stop_IIC(I2Cx);
        g_u8ErrStep = 0x03;
        return I2C_FAIL;
        }
    }

    i2c_flag_clear(I2Cx,I2C_FLAG_ADDSEND);
    I2C_Timeout = I2C_SHORT_TIMEOUT;
    while(SET != i2c_flag_get( I2Cx , I2C_FLAG_TBE ))
    {
        if((I2C_Timeout--) == 0)
        {
            Stop_IIC(I2Cx);
        g_u8ErrStep = 0x04;
        return I2C_FAIL;
        }
    }

    i2c_enable(I2Cx);
    if(ADDR_Length)//两字节地址
    {
        i2c_data_transmit(I2Cx, (uint8_t)((start_Addr & 0xFF00) >> 8));
        I2C_Timeout = I2C_SHORT_TIMEOUT;
        while(!i2c_flag_get(I2Cx, I2C_FLAG_BTC))
        {
            if((I2C_Timeout--) == 0)
            {
                Stop_IIC(I2Cx);
                g_u8ErrStep = 0x05;
                return I2C_FAIL;
            }
        }
        i2c_data_transmit(I2Cx, (uint8_t)(start_Addr & 0x00FF));
    }
    else
    {
        i2c_data_transmit(I2Cx, start_Addr);
    }

    I2C_Timeout = I2C_SHORT_TIMEOUT;
    while(!i2c_flag_get(I2Cx, I2C_FLAG_BTC))
    {
        if((I2C_Timeout--) == 0)
        {

                Stop_IIC(I2Cx);
        g_u8ErrStep = 0x06;
        return I2C_FAIL;
        }
    }

    while(number_Bytes)
    {
        i2c_data_transmit(I2Cx, *write_Buffer);
        I2C_Timeout = I2C_SHORT_TIMEOUT;
        while(!i2c_flag_get(I2Cx, I2C_FLAG_BTC))
        {
            if((I2C_Timeout--) == 0)
            {
                Stop_IIC(I2Cx);
                g_u8ErrStep = 0x07;
                return I2C_FAIL;
            }
        }


        /* point to the next location where the byte read will be saved */
        write_Buffer++;

        /* decrement the read bytes counter */
        number_Bytes--;
    }

    i2c_stop_on_bus(I2Cx);
    I2C_Timeout = I2C_SHORT_TIMEOUT;
    while (I2C_CTL0(I2Cx) & I2C_CTL0_STOP)
    {
        if((I2C_Timeout--) == 0)
        {
        Stop_IIC(I2Cx);
        g_u8ErrStep = 0x08;
        return I2C_FAIL;
        }
    }

    i2c_ack_config(I2Cx,I2C_ACK_ENABLE);
    g_u8ErrStep = 0x00;
    return I2C_OK;
}

void I2C_DEINIT(void)
{
    i2c_deinit(I2C1);
    /* enable I2Cx */
    i2c_disable(I2C1);
}


void HARD_I2C_INIT(void)
{
    g_HARD_I2C1_MutexSemaphore = xSemaphoreCreateMutex();

    //先重置引脚再进行初始化
    Resume_IIC(I2C_LONG_TIMEOUT, I2C1);
}

