#include "factory.h"
#include "SignalManage.h"
#include "hal_uart.h"
#include "boot.h"
#include "board.h"
#include "string.h"

// extern const SOFT_VER cst_sw_no;

// static U8 g_ack_buf[40] = {0};  //生产测试应答数据缓存区

// static enumFactoryPort g_factory_port = FACTORY_PORT_UART0;

// SemaphoreHandle_t g_Factory_Binary_Semaphore = NULL;

// static void Factory_ModeCtrl(U8* pDatabuf, U16 len)
// {
//     if(pDatabuf[3] == 0x01)
//     {
//         SetSigVal(SIGNAL_STATUS_FACTORY_MODE, TRUE);
//     }
//     else if(pDatabuf[3] == 0x00)
//     {
//         // SetSigVal(SIGNAL_STATUS_FACTORY_MODE, FALSE);
//         SystemResetFunc();  //复位重启
//     }

//     U8 cnt = 0;
//     g_ack_buf[cnt++] = FACTORY_FRAME_HEAD1;  //帧头
//     g_ack_buf[cnt++] = FACTORY_FRAME_HEAD2;
//     g_ack_buf[cnt++] = pDatabuf[2];  //命令码
//     g_ack_buf[cnt++] = pDatabuf[3];

//     if(g_factory_port == FACTORY_PORT_UART0){
//         usart0_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//     }
//     else if(g_factory_port == FACTORY_PORT_UART1){
//         usart1_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//     }
//     else if (g_factory_port == FACTORY_PORT_UART2){
//         usart2_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//     }
// }

// static void Factory_IO(U8* pDatabuf, U16 len)
// {
//     if(pDatabuf[3] == 0x01)
//     {
//         pDatabuf[4] == 1?   ENABLE_A_GUN_DO_SUPER() : DISABLE_A_GUN_DO_SUPER();
//         pDatabuf[5] == 1?   ENABLE_B_GUN_DO_SUPER() : DISABLE_B_GUN_DO_SUPER();
//         pDatabuf[6] == 1?   ENABLE_A_GUN_DO_24V()   : DISABLE_A_GUN_DO_24V();
//         pDatabuf[7] == 1?   ENABLE_B_GUN_DO_24V()   : DISABLE_B_GUN_DO_24V();
//         pDatabuf[8] == 1?   ENABLE_A_GUN_DO_12V()   : DISABLE_A_GUN_DO_12V();
//         pDatabuf[9] == 1?   ENABLE_B_GUN_DO_12V()   : DISABLE_B_GUN_DO_12V();
//         pDatabuf[10] == 1?  ENABLE_A_GUN_DO_GND()   : DISABLE_A_GUN_DO_GND();
//         pDatabuf[11] == 1?  ENABLE_B_GUN_DO_GND()   : DISABLE_B_GUN_DO_GND();
//         pDatabuf[12] == 1?  ENABLE_A_GUN_DO_LOCK()  : DISABLE_A_GUN_DO_LOCK();
//         pDatabuf[13] == 1?  ENABLE_B_GUN_DO_LOCK()  : DISABLE_B_GUN_DO_LOCK();

//         U8 cnt = 0;
//         g_ack_buf[cnt++] = FACTORY_FRAME_HEAD1;  //帧头
//         g_ack_buf[cnt++] = FACTORY_FRAME_HEAD2;
//         g_ack_buf[cnt++] = pDatabuf[2];  //命令码
//         g_ack_buf[cnt++] = pDatabuf[3];

//         if(g_factory_port == FACTORY_PORT_UART0){
//             usart0_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//         }
//         else if(g_factory_port == FACTORY_PORT_UART1){
//             usart1_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//         }
//         else if (g_factory_port == FACTORY_PORT_UART2){
//             usart2_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//         }    
//     }
//     else if(pDatabuf[3] == 0x00)
//     {
//         U8 cnt = 0;
//         g_ack_buf[cnt++] = FACTORY_FRAME_HEAD1;  //帧头
//         g_ack_buf[cnt++] = FACTORY_FRAME_HEAD2;
//         g_ack_buf[cnt++] = pDatabuf[2];  //命令码
//         g_ack_buf[cnt++] = pDatabuf[3];
//         g_ack_buf[cnt++] = GetMsgVal(SIGNAL_STATUS_DI_A_GUN_POWER);
//         g_ack_buf[cnt++] = GetMsgVal(SIGNAL_STATUS_DI_B_GUN_POWER);
//         g_ack_buf[cnt++] = GetMsgVal(SIGNAL_STATUS_DI_A_GUN_LOCK);
//         g_ack_buf[cnt++] = GetMsgVal(SIGNAL_STATUS_DI_B_GUN_LOCK);

//         if(g_factory_port == FACTORY_PORT_UART0){
//             usart0_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//         }
//         else if(g_factory_port == FACTORY_PORT_UART1){
//             usart1_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//         }
//         else if (g_factory_port == FACTORY_PORT_UART2){
//             usart2_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//         }
//     }
// }


// static void Factory_PCB_Temp(U8* pDatabuf, U16 len)
// {
//     U8 cnt = 0;
//     g_ack_buf[cnt++] = FACTORY_FRAME_HEAD1;  //帧头
//     g_ack_buf[cnt++] = FACTORY_FRAME_HEAD2;
//     g_ack_buf[cnt++] = pDatabuf[2];  //命令码
//     g_ack_buf[cnt++] = GetMsgVal(MBMS_SAM_SIG_ID_PCB_TEMP1);
//     g_ack_buf[cnt++] = GetMsgVal(MBMS_SAM_SIG_ID_PCB_TEMP2);

//     if(g_factory_port == FACTORY_PORT_UART0){
//         usart0_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//     }
//     else if(g_factory_port == FACTORY_PORT_UART1){
//         usart1_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//     }
//     else if (g_factory_port == FACTORY_PORT_UART2){
//         usart2_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//     }
// }


// static void Factory_CC1_Voltage(U8* pDatabuf, U16 len)
// {
//     U8 cnt = 0;
//     g_ack_buf[cnt++] = FACTORY_FRAME_HEAD1;  //帧头
//     g_ack_buf[cnt++] = FACTORY_FRAME_HEAD2;
//     g_ack_buf[cnt++] = pDatabuf[2];  //命令码
//     g_ack_buf[cnt++] = GetMsgVal(MBMS_SAM_SIG_ID_CC1_VOLTAGE_A)>>8 & 0xff;
//     g_ack_buf[cnt++] = GetMsgVal(MBMS_SAM_SIG_ID_CC1_VOLTAGE_A) & 0xff;
//     g_ack_buf[cnt++] = GetMsgVal(MBMS_SAM_SIG_ID_CC1_VOLTAGE_B)>>8 & 0xff;
//     g_ack_buf[cnt++] = GetMsgVal(MBMS_SAM_SIG_ID_CC1_VOLTAGE_B) & 0xff;

//     if(g_factory_port == FACTORY_PORT_UART0){
//         usart0_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//     }
//     else if(g_factory_port == FACTORY_PORT_UART1){
//         usart1_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//     }
//     else if (g_factory_port == FACTORY_PORT_UART2){
//         usart2_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//     }
// }


// static void Factory_SN(U8* pDatabuf, U16 len)
// {
//     bl ret = TRUE;

//     if(pDatabuf[3] == 0x01)  //写入SN
//     {
//         for(uint16_t i=0; i<16; i++)
//         {
//             g_ack_buf[i] = pDatabuf[i+3];
//             if(SetSigVal(MBMS_SET_SIG_ID_SN1+i, pDatabuf[4+i*2]<<8 | pDatabuf[5+i*2]) != TRUE)
//             {
//                 ret = FALSE;
//                 break;
//             }
//         }


//         U8 cnt = 0;
//         g_ack_buf[cnt++] = FACTORY_FRAME_HEAD1;  //帧头
//         g_ack_buf[cnt++] = FACTORY_FRAME_HEAD2;
//         g_ack_buf[cnt++] = pDatabuf[2];  //命令码
//         g_ack_buf[cnt++] = pDatabuf[3];
//         g_ack_buf[cnt++] = ret;

//         if(g_factory_port == FACTORY_PORT_UART0){
//             usart0_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//         }
//         else if(g_factory_port == FACTORY_PORT_UART1){
//             usart1_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//         }
//         else if (g_factory_port == FACTORY_PORT_UART2){
//             usart2_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//         }
//     }
//     else if(pDatabuf[3] == 0x00)  //读取SN
//     {
//         U8 cnt = 0;
//         g_ack_buf[cnt++] = FACTORY_FRAME_HEAD1;  //帧头
//         g_ack_buf[cnt++] = FACTORY_FRAME_HEAD2;
//         g_ack_buf[cnt++] = pDatabuf[2];  //命令码

//         for(uint16_t i=0; i<16; i++)
//         {
//             g_ack_buf[cnt++] = GetMsgVal(MBMS_SET_SIG_ID_SN1+i)>>8 & 0xff;
//             g_ack_buf[cnt++] = GetMsgVal(MBMS_SET_SIG_ID_SN1+i) & 0xff;
//         }

//         if(g_factory_port == FACTORY_PORT_UART0){
//             usart0_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//         }
//         else if(g_factory_port == FACTORY_PORT_UART1){
//             usart1_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//         }
//         else if (g_factory_port == FACTORY_PORT_UART2){
//             usart2_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//         }
//     }
// }

// static void Factory_SoftwareVersion(U8* pDatabuf, U16 len)
// {
//     U8 cnt = 0;

//     g_ack_buf[cnt++] = FACTORY_FRAME_HEAD1;  //帧头
//     g_ack_buf[cnt++] = FACTORY_FRAME_HEAD2;
//     g_ack_buf[cnt++] = pDatabuf[2];  //命令码
//     memcpy(g_ack_buf + cnt, cst_sw_no.sw_major_version, 13);  //拷贝软件版本号到应答数据中
//     memcpy(g_ack_buf + cnt + 13, cst_sw_no.sw_minor_version, 3);  //拷贝软件版本号到应答数据中

//     if(g_factory_port == FACTORY_PORT_UART0){
//         usart0_dma_send(g_ack_buf, cnt+16);  //响应工厂测试开始命令
//     }
//     else if(g_factory_port == FACTORY_PORT_UART1){
//         usart1_dma_send(g_ack_buf, cnt+16);  //响应工厂测试开始命令
//     }
//     else if (g_factory_port == FACTORY_PORT_UART2){
//         usart2_dma_send(g_ack_buf, cnt+16);  //响应工厂测试开始命令
//     }
// }

// static void Factory_Uart(U8* pDatabuf, U16 len)
// {
//     U8 cnt = 0;

//     g_ack_buf[cnt++] = FACTORY_FRAME_HEAD1;  //帧头
//     g_ack_buf[cnt++] = FACTORY_FRAME_HEAD2;
//     g_ack_buf[cnt++] = FACTORY_ACK_OK;  //命令码    

//     if(g_factory_port == FACTORY_PORT_UART0){
//         usart0_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//     }
//     else if(g_factory_port == FACTORY_PORT_UART1){
//         usart1_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//     }
//     else if (g_factory_port == FACTORY_PORT_UART2){
//         usart2_dma_send(g_ack_buf, cnt);  //响应工厂测试开始命令
//     }    
// }



// static void Factory_Process(U8* pDatabuf, U16 len)
// {
//     enumFactoryFun funType = (enumFactoryFun)pDatabuf[2];
//     switch (funType)
//     {
//     case FACTORY_MODE_START_EXIT:
//         Factory_ModeCtrl(pDatabuf, len);
//         break;
    
//     case FACTORY_IO:
//         Factory_IO(pDatabuf, len);
//         break;
    
//     case FACTORY_PCB_TEMP:
//         Factory_PCB_Temp(pDatabuf, len);
//         break;

//     case FACTORY_CC1_VOLTAGE:
//         Factory_CC1_Voltage(pDatabuf, len);
//         break;

//     case FACTORY_SN:
//         Factory_SN(pDatabuf, len);
//         break;

//     case FACTORY_SOFTWARE_VERSION:
//         Factory_SoftwareVersion(pDatabuf, len);
//         break;

//     case FACTORY_UART:
//         Factory_Uart(pDatabuf, len);
//         break;
    
//     default:
//         break;
//     }
// }


// /// @brief 检查是否是工厂测试帧
// /// @param port 
// /// @return 
// BOOL blCheckFactoryFrame(enumFactoryPort port)
// {
//     BaseType_t xHigherPriorityTaskWoken;
//     BOOL ret = FALSE;

//     if(port == FACTORY_PORT_UART0)
//     {
//         if(g_Usart_0_CommBuf.u8Recvbuf[0] == FACTORY_FRAME_HEAD1
//         && g_Usart_0_CommBuf.u8Recvbuf[1] == FACTORY_FRAME_HEAD2)
//         {
//             g_factory_port = FACTORY_PORT_UART0;
//             ret = TRUE;
//         }        
//     }
//     else if(port == FACTORY_PORT_UART1)
//     {
//         if(g_Usart_1_CommBuf.u8Recvbuf[0] == FACTORY_FRAME_HEAD1
//         && g_Usart_1_CommBuf.u8Recvbuf[1] == FACTORY_FRAME_HEAD2)
//         {
//             g_factory_port = FACTORY_PORT_UART1;
//             ret = TRUE;
//         }                
//     }
//     else if(port == FACTORY_PORT_UART2)
//     {
//         if(g_Usart_2_CommBuf.u8Recvbuf[0] == FACTORY_FRAME_HEAD1
//         && g_Usart_2_CommBuf.u8Recvbuf[1] == FACTORY_FRAME_HEAD2)
//         {
//             g_factory_port = FACTORY_PORT_UART2;
//             ret = TRUE;
//         }                
//     }

//     if(ret == TRUE)
//     {
//         (void)xSemaphoreGiveFromISR(g_Factory_Binary_Semaphore, &xHigherPriorityTaskWoken);  //触发
//         portYIELD_FROM_ISR(xHigherPriorityTaskWoken);      
//     }

//     return ret;
// }

// void Factory_Task(void * pvParameters)
// {
//     volatile UBaseType_t uxHighWaterMark;

//     (void)pvParameters;

//     g_Factory_Binary_Semaphore = xSemaphoreCreateBinary(); 

//     while (1)
//     {
//         xSemaphoreTake(g_Factory_Binary_Semaphore, portMAX_DELAY);  //阻塞等待信号量

//         if(FACTORY_PORT_UART0 == g_factory_port)
//         {
//             Factory_Process(g_Usart_0_CommBuf.u8Recvbuf, g_Usart_0_CommBuf.u16RecvDataCnt);
//         }
//         else if(FACTORY_PORT_UART1 == g_factory_port)
//         {
//             Factory_Process(g_Usart_1_CommBuf.u8Recvbuf, g_Usart_1_CommBuf.u16RecvDataCnt);
//         }
//         else if(FACTORY_PORT_UART2 == g_factory_port)
//         {
//             Factory_Process(g_Usart_2_CommBuf.u8Recvbuf, g_Usart_2_CommBuf.u16RecvDataCnt);
//         }
        
//         uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
//     }
    
    
// }


void EnterFactoryMode(void)
{
    SetSigVal(SIGNAL_STATUS_FACTORY_MODE, TRUE);
}

void ExitFactoryMode(void)
{
    SystemResetFunc();  //复位重启
}


