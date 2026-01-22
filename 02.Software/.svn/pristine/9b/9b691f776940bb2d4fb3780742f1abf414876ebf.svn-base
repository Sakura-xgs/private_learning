/*!
    \file    gd32f4xx_it.c
    \brief   interrupt service routines

    \version 2024-10-20, V3.3.1, demo for GD32F4xx
*/

/*
    Copyright (c) 2024, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include "gd32f4xx_it.h"
#include "PublicDefine.h"
#include "hal_uart.h"
#include "boot.h"
#include "factory.h"

/*!
    \brief      this function handles NMI exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void NMI_Handler(void)
{
    /* if NMI exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles HardFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void HardFault_Handler(void)
{
    /* if Hard Fault exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles MemManage exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void MemManage_Handler(void)
{
    /* if Memory Manage exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles BusFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void BusFault_Handler(void)
{
    /* if Bus Fault exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles UsageFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void UsageFault_Handler(void)
{
    /* if Usage Fault exception occurs, go to infinite loop */
    while(1) {
    }
}

///*!
//    \brief      this function handles SVC exception
//    \param[in]  none
//    \param[out] none
//    \retval     none
//*/
//void SVC_Handler(void)
//{
//    /* if SVC exception occurs, go to infinite loop */
//    while(1) {
//    }
//}

/*!
    \brief      this function handles DebugMon exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DebugMon_Handler(void)
{
    /* if DebugMon exception occurs, go to infinite loop */
    while(1) {
    }
}

///*!
//    \brief      this function handles PendSV exception
//    \param[in]  none
//    \param[out] none
//    \retval     none
//*/
//void PendSV_Handler(void)
//{
//    /* if PendSV exception occurs, go to infinite loop */
//    while(1) {
//    }
//}

/*!
    \brief      this function handles SysTick exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
//void SysTick_Handler(void)
//{
//    delay_decrement();
//}


void DMA1_Channel7_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken;
	
    if(dma_interrupt_flag_get(DMA1, DMA_CH7, DMA_INT_FLAG_FTF))
    {     
        dma_interrupt_flag_clear(DMA1, DMA_CH7, DMA_INT_FLAG_FTF);

        if(NULL != g_USART_0_Send_Binary_Semaphore)
        {
            /* release binary semaphores. */
            (void)xSemaphoreGiveFromISR(g_USART_0_Send_Binary_Semaphore, &xHigherPriorityTaskWoken);

            /* make a task switch if necessary. */
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

void DMA0_Channel6_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken;
	
    if(dma_interrupt_flag_get(DMA0, DMA_CH6, DMA_INT_FLAG_FTF))
    {     
        dma_interrupt_flag_clear(DMA0, DMA_CH6, DMA_INT_FLAG_FTF);

        if(NULL != g_USART_1_Send_Binary_Semaphore)
        {
            /* release binary semaphores. */
            (void)xSemaphoreGiveFromISR(g_USART_1_Send_Binary_Semaphore, &xHigherPriorityTaskWoken);

            /* make a task switch if necessary. */
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

void DMA0_Channel3_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken;
	
    if(dma_interrupt_flag_get(DMA0, DMA_CH3, DMA_INT_FLAG_FTF))
    {     
        dma_interrupt_flag_clear(DMA0, DMA_CH3, DMA_INT_FLAG_FTF);

        if(NULL != g_USART_2_Send_Binary_Semaphore)
        {
            /* release binary semaphores. */
            (void)xSemaphoreGiveFromISR(g_USART_2_Send_Binary_Semaphore, &xHigherPriorityTaskWoken);

            /* make a task switch if necessary. */
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

void USART0_IRQHandler(void)	
{
    BaseType_t xHigherPriorityTaskWoken;

    if(RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_TC)) 
    {
        usart_interrupt_flag_clear(USART0, USART_INT_FLAG_TC);
        
        dma_channel_disable(DMA1, DMA_CH2);						    /* 关闭串口的DMA接收 */	

        //切换为接收模式
        Usart0_Switch_To_Recv_Mode();

		/* 重新设置DMA传输 */
		dma_memory_address_config(DMA1, DMA_CH2, DMA_MEMORY_0, (uint32_t)(g_Usart_0_CommBuf.u8Recvbuf));

		dma_transfer_number_config(DMA1, DMA_CH2, ARRAYNUM(g_Usart_0_CommBuf.u8Recvbuf));

        dma_interrupt_flag_clear(DMA1, DMA_CH2, DMA_INT_FLAG_FTF);  //重新使能前，需要这一步
		dma_channel_enable(DMA1, DMA_CH2);		                    /* 开启串口的DMA接收 */
    }

    if(RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_IDLE))
	{
		usart_interrupt_flag_clear(USART0, USART_INT_FLAG_IDLE);	/* 清除空闲中断标志位 */
        (void)usart_data_receive(USART0);					        /* 清除接收完成标志位 */
        dma_channel_disable(DMA1, DMA_CH2);						    /* 关闭串口的DMA接收 */

        g_Usart_0_CommBuf.u16RecvDataCnt = ARRAYNUM(g_Usart_0_CommBuf.u8Recvbuf) - dma_transfer_number_get(DMA1, DMA_CH2);

        //切换为接收模式
        Usart0_Switch_To_Recv_Mode();

        /* 重新设置DMA传输 */
        dma_memory_address_config(DMA1, DMA_CH2, DMA_MEMORY_0, (uint32_t)g_Usart_0_CommBuf.u8Recvbuf);

        dma_transfer_number_config(DMA1, DMA_CH2, ARRAYNUM(g_Usart_0_CommBuf.u8Recvbuf));

        dma_interrupt_flag_clear(DMA1, DMA_CH2, DMA_INT_FLAG_FTF);  //重新使能前，需要这一步
        dma_channel_enable(DMA1, DMA_CH2);		                    /* 开启串口的DMA接收 */


        if(NULL != g_USART_0_Recv_Binary_Semaphore)
        {
            /* release binary semaphores. */
            (void)xSemaphoreGiveFromISR(g_USART_0_Recv_Binary_Semaphore, &xHigherPriorityTaskWoken);

            /* make a task switch if necessary. */
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }	
    }	
}

void USART1_IRQHandler(void)	
{
    BaseType_t xHigherPriorityTaskWoken;

    if(RESET != usart_interrupt_flag_get(USART1, USART_INT_FLAG_TC)) 
    {
        usart_interrupt_flag_clear(USART1, USART_INT_FLAG_TC);
        
        dma_channel_disable(DMA0, DMA_CH5);						    /* 关闭串口的DMA接收 */	
       
        //切换为接收模式
        Usart1_Switch_To_Recv_Mode();

		/* 重新设置DMA传输 */
		dma_memory_address_config(DMA0, DMA_CH5, DMA_MEMORY_0, (uint32_t)(g_Usart_1_CommBuf.u8Recvbuf));

		dma_transfer_number_config(DMA0, DMA_CH5, ARRAYNUM(g_Usart_1_CommBuf.u8Recvbuf));

        dma_interrupt_flag_clear(DMA0, DMA_CH5, DMA_INT_FLAG_FTF);  //重新使能前，需要这一步
		dma_channel_enable(DMA0, DMA_CH5);		                    /* 开启串口的DMA接收 */
    }

    if(RESET != usart_interrupt_flag_get(USART1, USART_INT_FLAG_IDLE))
	{
		usart_interrupt_flag_clear(USART1, USART_INT_FLAG_IDLE);	/* 清除空闲中断标志位 */
        (void)usart_data_receive(USART1);					        /* 清除接收完成标志位 */
        dma_channel_disable(DMA0, DMA_CH5);						    /* 关闭串口的DMA接收 */

        g_Usart_1_CommBuf.u16RecvDataCnt = ARRAYNUM(g_Usart_1_CommBuf.u8Recvbuf) - dma_transfer_number_get(DMA0, DMA_CH5);
       
        //切换为接收模式
        Usart1_Switch_To_Recv_Mode();

        /* 重新设置DMA传输 */
        dma_memory_address_config(DMA0, DMA_CH5, DMA_MEMORY_0, (uint32_t)g_Usart_1_CommBuf.u8Recvbuf);

        dma_transfer_number_config(DMA0, DMA_CH5, ARRAYNUM(g_Usart_1_CommBuf.u8Recvbuf));

        dma_interrupt_flag_clear(DMA0, DMA_CH5, DMA_INT_FLAG_FTF);  //重新使能前，需要这一步
        dma_channel_enable(DMA0, DMA_CH5);		                    /* 开启串口的DMA接收 */

        if(TRUE == blCheck_Usart1_UpdateMsg())   //检查是否app升级包数据
        {
            //无需额外处理
        }
        else if(NULL != g_USART_1_Recv_Binary_Semaphore)
        {
            /* release binary semaphores. */
            xSemaphoreGiveFromISR(g_USART_1_Recv_Binary_Semaphore, &xHigherPriorityTaskWoken);

            /* make a task switch if necessary. */
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }	
    }	
}

void USART2_IRQHandler(void)	
{
    BaseType_t xHigherPriorityTaskWoken;

    if(RESET != usart_interrupt_flag_get(USART2, USART_INT_FLAG_TC)) 
    {
        usart_interrupt_flag_clear(USART2, USART_INT_FLAG_TC);
        
        dma_channel_disable(DMA0, DMA_CH1);						    /* 关闭串口的DMA接收 */	

        //切换为接收模式
        Usart2_Switch_To_Recv_Mode();

		/* 重新设置DMA传输 */
		dma_memory_address_config(DMA0, DMA_CH1, DMA_MEMORY_0, (uint32_t)(g_Usart_2_CommBuf.u8Recvbuf));

		dma_transfer_number_config(DMA0, DMA_CH1, ARRAYNUM(g_Usart_2_CommBuf.u8Recvbuf));

        dma_interrupt_flag_clear(DMA0, DMA_CH1, DMA_INT_FLAG_FTF);  //重新使能前，需要这一步
		dma_channel_enable(DMA0, DMA_CH1);		                    /* 开启串口的DMA接收 */
    }

    if(RESET != usart_interrupt_flag_get(USART2, USART_INT_FLAG_IDLE))
	{
		usart_interrupt_flag_clear(USART2, USART_INT_FLAG_IDLE);	/* 清除空闲中断标志位 */
        (void)usart_data_receive(USART2);					        /* 清除接收完成标志位 */
        dma_channel_disable(DMA0, DMA_CH1);						    /* 关闭串口的DMA接收 */

        g_Usart_2_CommBuf.u16RecvDataCnt = ARRAYNUM(g_Usart_2_CommBuf.u8Recvbuf) - dma_transfer_number_get(DMA0, DMA_CH1);
       
        //切换为接收模式
        Usart2_Switch_To_Recv_Mode();
       
        /* 重新设置DMA传输 */
        dma_memory_address_config(DMA0, DMA_CH1, DMA_MEMORY_0, (uint32_t)g_Usart_2_CommBuf.u8Recvbuf);

        dma_transfer_number_config(DMA0, DMA_CH1, ARRAYNUM(g_Usart_2_CommBuf.u8Recvbuf));

        dma_interrupt_flag_clear(DMA0, DMA_CH1, DMA_INT_FLAG_FTF);  //重新使能前，需要这一步
        dma_channel_enable(DMA0, DMA_CH1);		                    /* 开启串口的DMA接收 */

        if(TRUE == blCheck_Usart2_UpdateMsg())  //检查是否app升级包数据
        {
            //无需额外处理
        }        
		else if(NULL != g_USART_2_Recv_Binary_Semaphore)
        {
            /* release binary semaphores. */
            xSemaphoreGiveFromISR(g_USART_2_Recv_Binary_Semaphore, &xHigherPriorityTaskWoken);

            /* make a task switch if necessary. */
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }	
    }	
}


