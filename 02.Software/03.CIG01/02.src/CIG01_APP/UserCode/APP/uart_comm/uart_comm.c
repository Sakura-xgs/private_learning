#include "uart_comm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hal_uart.h"
#include "modbus.h"
#include "MbmsSignal.h"
#include "SignalManage.h"
#include "factory.h"
#include "io_ctrl.h"



#define UART_COMM_LOST_TIME    10  //串口通信超时时间, 10S

extern void SystemResetFunc(void);

static uint16_t modbusRegs[MODBUS_REG_NUM] = {0}; //模拟寄存器地址

static U8 g_CommLostCnt = 0; //通信丢失计数

MODBUS_EXPORT(ModbusUart0, MODBUS_DEFAULT_ADDR, usart0_dma_send, modbusRegs, MODBUS_REG_NUM)
MODBUS_EXPORT(ModbusUart1, MODBUS_DEFAULT_ADDR, usart1_dma_send, modbusRegs, MODBUS_REG_NUM)
MODBUS_EXPORT(ModbusUart2, MODBUS_DEFAULT_ADDR, usart2_dma_send, modbusRegs, MODBUS_REG_NUM)


/// @brief 开机后一次性更新 软件版本号，硬件版本号以及SN号
/// @param  
static void ModbusReg_Updata_BoardInfo(void)
{
    modbusRegs[MODBUS_REG20_SW_VS1] = GetMsgVal(MBMS_SAM_SIG_ID_SOFTWARE_VERSION_1);
    modbusRegs[MODBUS_REG21_SW_VS2] = GetMsgVal(MBMS_SAM_SIG_ID_SOFTWARE_VERSION_2);
    modbusRegs[MODBUS_REG22_SW_VS3] = GetMsgVal(MBMS_SAM_SIG_ID_SOFTWARE_VERSION_3);
    modbusRegs[MODBUS_REG23_SW_VS4] = GetMsgVal(MBMS_SAM_SIG_ID_SOFTWARE_VERSION_4);
    modbusRegs[MODBUS_REG24_SW_VS5] = GetMsgVal(MBMS_SAM_SIG_ID_SOFTWARE_VERSION_5);
    modbusRegs[MODBUS_REG25_SW_VS6] = GetMsgVal(MBMS_SAM_SIG_ID_SOFTWARE_VERSION_6);
    modbusRegs[MODBUS_REG26_SW_VS7] = GetMsgVal(MBMS_SAM_SIG_ID_SOFTWARE_VERSION_7);
    modbusRegs[MODBUS_REG27_SW_VS8] = GetMsgVal(MBMS_SAM_SIG_ID_SOFTWARE_VERSION_8);

    modbusRegs[MODBUS_REG28_HW_VS1] = GetMsgVal(MBMS_SAM_SIG_ID_HARDWARE_VERSION_1);
    modbusRegs[MODBUS_REG29_HW_VS2] = GetMsgVal(MBMS_SAM_SIG_ID_HARDWARE_VERSION_2);
    modbusRegs[MODBUS_REG30_HW_VS3] = GetMsgVal(MBMS_SAM_SIG_ID_HARDWARE_VERSION_3);
    modbusRegs[MODBUS_REG31_HW_VS4] = GetMsgVal(MBMS_SAM_SIG_ID_HARDWARE_VERSION_4);
    modbusRegs[MODBUS_REG32_HW_VS5] = GetMsgVal(MBMS_SAM_SIG_ID_HARDWARE_VERSION_5);
    modbusRegs[MODBUS_REG33_HW_VS6] = GetMsgVal(MBMS_SAM_SIG_ID_HARDWARE_VERSION_6);
    modbusRegs[MODBUS_REG34_HW_VS7] = GetMsgVal(MBMS_SAM_SIG_ID_HARDWARE_VERSION_7);
    modbusRegs[MODBUS_REG35_HW_VS8] = GetMsgVal(MBMS_SAM_SIG_ID_HARDWARE_VERSION_8);

    modbusRegs[MODBUS_REG36_SN1] = GetMsgVal(MBMS_SET_SIG_ID_SN1);
    modbusRegs[MODBUS_REG37_SN2] = GetMsgVal(MBMS_SET_SIG_ID_SN2);
    modbusRegs[MODBUS_REG38_SN3] = GetMsgVal(MBMS_SET_SIG_ID_SN3);
    modbusRegs[MODBUS_REG39_SN4] = GetMsgVal(MBMS_SET_SIG_ID_SN4);
    modbusRegs[MODBUS_REG40_SN5] = GetMsgVal(MBMS_SET_SIG_ID_SN5);
    modbusRegs[MODBUS_REG41_SN6] = GetMsgVal(MBMS_SET_SIG_ID_SN6);
    modbusRegs[MODBUS_REG42_SN7] = GetMsgVal(MBMS_SET_SIG_ID_SN7);
    modbusRegs[MODBUS_REG43_SN8] = GetMsgVal(MBMS_SET_SIG_ID_SN8);
    modbusRegs[MODBUS_REG44_SN9] = GetMsgVal(MBMS_SET_SIG_ID_SN9);
    modbusRegs[MODBUS_REG45_SN10] = GetMsgVal(MBMS_SET_SIG_ID_SN10);
    modbusRegs[MODBUS_REG46_SN11] = GetMsgVal(MBMS_SET_SIG_ID_SN11);
    modbusRegs[MODBUS_REG47_SN12] = GetMsgVal(MBMS_SET_SIG_ID_SN12);
    modbusRegs[MODBUS_REG48_SN13] = GetMsgVal(MBMS_SET_SIG_ID_SN13);
    modbusRegs[MODBUS_REG49_SN14] = GetMsgVal(MBMS_SET_SIG_ID_SN14);
    modbusRegs[MODBUS_REG50_SN15] = GetMsgVal(MBMS_SET_SIG_ID_SN15);
    modbusRegs[MODBUS_REG51_SN16] = GetMsgVal(MBMS_SET_SIG_ID_SN16);
}

/// @brief 刷新IO状态，cc1电压，PCB温度
/// @param  
static void ModbusReg_UpdateNormal(void)
{
    //DI反馈
    modbusRegs[MODBUS_REG0_DI] = (GetMsgVal(SIGNAL_STATUS_DI_A_GUN_POWER)&0x01)<<3
                                | (GetMsgVal(SIGNAL_STATUS_DI_B_GUN_POWER)&0x01)<<2
                                | (GetMsgVal(SIGNAL_STATUS_DI_A_GUN_LOCK)&0x01)<<1
                                | (GetMsgVal(SIGNAL_STATUS_DI_B_GUN_LOCK)&0x01);

    //DO输出
    modbusRegs[MODBUS_REG1_DO] = (GetMsgVal(SIGNAL_STATUS_DO_A_GUN_SUPER)&0x01)<<9
                                | (GetMsgVal(SIGNAL_STATUS_DO_B_GUN_SUPER)&0x01)<<8
                                | (GetMsgVal(SIGNAL_STATUS_DO_A_GUN_POWER_24V)&0x01)<<7
                                | (GetMsgVal(SIGNAL_STATUS_DO_B_GUN_POWER_24V)&0x01)<<6
                                | (GetMsgVal(SIGNAL_STATUS_DO_A_GUN_POWER_12V)&0x01)<<5
                                | (GetMsgVal(SIGNAL_STATUS_DO_B_GUN_POWER_12V)&0x01)<<4
                                | (GetMsgVal(SIGNAL_STATUS_DO_A_GUN_POWER_GND)&0x01)<<3
                                | (GetMsgVal(SIGNAL_STATUS_DO_B_GUN_POWER_GND)&0x01)<<2
                                | (GetMsgVal(SIGNAL_STATUS_DO_A_GUN_LOCK)&0x01)<<1
                                | (GetMsgVal(SIGNAL_STATUS_DO_B_GUN_LOCK)&0x01);

    //CC1电压
    modbusRegs[MODBUS_REG2_CC1A] = GetMsgVal(MBMS_SAM_SIG_ID_CC1_VOLTAGE_A);
    modbusRegs[MODBUS_REG3_CC1B] = GetMsgVal(MBMS_SAM_SIG_ID_CC1_VOLTAGE_B);

    //PCB温度
    modbusRegs[MODBUS_REG4_PCBTemp1] = (U16)(((S16)GetMsgVal(MBMS_SAM_SIG_ID_PCB_TEMP1)+20)*10);  //有符号数, 额外加上20度偏移量（充电桩统一60度偏移量），扩大10倍，精度0.1
    modbusRegs[MODBUS_REG5_PCBTemp2] = (U16)(((S16)GetMsgVal(MBMS_SAM_SIG_ID_PCB_TEMP2)+20)*10);  //有符号数

    //AD原始采样值
    modbusRegs[MODBUS_REG54_CC1_A_AD_RAW] = GetMsgVal(MBMS_SAM_SIG_ID_CC1_VOLTAGE_A_AD_RAW);
    modbusRegs[MODBUS_REG55_CC1_B_AD_RAW] = GetMsgVal(MBMS_SAM_SIG_ID_CC1_VOLTAGE_B_AD_RAW);
    modbusRegs[MODBUS_REG56_TEMP1_AD_RAW] = GetMsgVal(MBMS_SAM_SIG_ID_PCB_TEMP1_AD_RAW);
    modbusRegs[MODBUS_REG57_TEMP2_AD_RAW] = GetMsgVal(MBMS_SAM_SIG_ID_PCB_TEMP2_AD_RAW);
}


/// @brief 检查是否有Modbus寄存器更新，若有则更新对应的信号状态并执行对应的动作
/// @param  
static void CheckModbusRegUpdate(void)
{
    BOOL temp;

    //生产测试功能判断
    if(modbusRegs[MODBUS_REG53_FACTORY_MODE] == 0x55){
        if(GetMsgVal(SIGNAL_STATUS_FACTORY_MODE) != TRUE){
            SetSigVal(SIGNAL_STATUS_FACTORY_MODE, TRUE);
            EnterFactoryMode();
        }
    }
    else if(modbusRegs[MODBUS_REG53_FACTORY_MODE] == 0xAA){
        ExitFactoryMode();
    }

    //检测是否写入SN
    for(int i=0; i<16; i++)
    {
        if(modbusRegs[MODBUS_REG36_SN1+i] != GetMsgVal(MBMS_SET_SIG_ID_SN1+i)){
            SetSigVal(MBMS_SET_SIG_ID_SN1+i, modbusRegs[MODBUS_REG36_SN1+0]);
        }
    }
    

    //DO输出控制
    temp = READ_BIT(modbusRegs[MODBUS_REG1_DO], 9);
    if(temp != GetMsgVal(SIGNAL_STATUS_DO_A_GUN_SUPER)){
        IO_Ctrl(IO_A_GUN_DO_SUPER, temp);
    }

    temp = READ_BIT(modbusRegs[MODBUS_REG1_DO], 8);
    if(temp != GetMsgVal(SIGNAL_STATUS_DO_B_GUN_SUPER)){
        IO_Ctrl(IO_B_GUN_DO_SUPER, temp);
    }

    temp = READ_BIT(modbusRegs[MODBUS_REG1_DO], 7);
    if(temp != GetMsgVal(SIGNAL_STATUS_DO_A_GUN_POWER_24V)){
        IO_Ctrl(IO_A_GUN_DO_24V, temp);
    }

    temp = READ_BIT(modbusRegs[MODBUS_REG1_DO], 6);
    if(temp != GetMsgVal(SIGNAL_STATUS_DO_B_GUN_POWER_24V)){
        IO_Ctrl(IO_B_GUN_DO_24V, temp);
    }

    temp = READ_BIT(modbusRegs[MODBUS_REG1_DO], 5);
    if(temp != GetMsgVal(SIGNAL_STATUS_DO_A_GUN_POWER_12V)){
        IO_Ctrl(IO_A_GUN_DO_12V, temp);
    }

    temp = READ_BIT(modbusRegs[MODBUS_REG1_DO], 4);
    if(temp != GetMsgVal(SIGNAL_STATUS_DO_B_GUN_POWER_12V)){
        IO_Ctrl(IO_B_GUN_DO_12V, temp);
    }

    temp = READ_BIT(modbusRegs[MODBUS_REG1_DO], 3);
    if(temp != GetMsgVal(SIGNAL_STATUS_DO_A_GUN_POWER_GND)){
        IO_Ctrl(IO_A_GUN_DO_GND, temp);
    }

    temp = READ_BIT(modbusRegs[MODBUS_REG1_DO], 2);
    if(temp != GetMsgVal(SIGNAL_STATUS_DO_B_GUN_POWER_GND)){
        IO_Ctrl(IO_B_GUN_DO_GND, temp);
    }

    temp = READ_BIT(modbusRegs[MODBUS_REG1_DO], 1);
    if(temp != GetMsgVal(SIGNAL_STATUS_DO_A_GUN_LOCK)){
        IO_Ctrl(IO_A_GUN_DO_LOCK, temp);
    }

    temp = READ_BIT(modbusRegs[MODBUS_REG1_DO], 0);
    if(temp != GetMsgVal(SIGNAL_STATUS_DO_B_GUN_LOCK)){
        IO_Ctrl(IO_B_GUN_DO_LOCK, temp);
    }

    //软件复位
    if(modbusRegs[MODBUS_REG52_RST] == TRUE){
        SystemResetFunc();
    } 

}

static void Uart0_Comm_Task(void * pvParameters)
{
    volatile UBaseType_t uxHighWaterMark;

    BaseType_t err = pdFALSE;

    (void)pvParameters;

    while(1)
    {
        // Wait for the semaphore to be given by the ISR
        // This will block until the semaphore is available
        if(g_USART_0_Recv_Binary_Semaphore != NULL)
        {
            err = xSemaphoreTake(g_USART_0_Recv_Binary_Semaphore, portMAX_DELAY);

            if(pdTRUE == err)
            {
                ModbusReg_UpdateNormal();

                if(TRUE == Modbus_ParaseFrame(g_Usart_0_CommBuf.u8Recvbuf, g_Usart_0_CommBuf.u16RecvDataCnt, &ModbusUart0))
                {
                    g_CommLostCnt = 0; // Reset communication lost count if frame is parsed successfully
                    CheckModbusRegUpdate();
                }
            }
        }
        else
        {
            vTaskDelay(300);
        }

        uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    }

}

static void Uart1_Comm_Task(void * pvParameters)
{
    volatile UBaseType_t uxHighWaterMark;

    BaseType_t err = pdFALSE;

    (void)pvParameters;

    while(1)
    {
        // Wait for the semaphore to be given by the ISR
        // This will block until the semaphore is available
        if(g_USART_1_Recv_Binary_Semaphore != NULL)
        {
            err = xSemaphoreTake(g_USART_1_Recv_Binary_Semaphore, portMAX_DELAY);

            if(pdTRUE == err)
            {
                ModbusReg_UpdateNormal();

                if(TRUE == Modbus_ParaseFrame(g_Usart_1_CommBuf.u8Recvbuf, g_Usart_1_CommBuf.u16RecvDataCnt, &ModbusUart1))
                {
                    g_CommLostCnt = 0; // Reset communication lost count if frame is parsed successfully
                    CheckModbusRegUpdate();
                }
            }
        }
        else
        {
            vTaskDelay(300);
        }

        uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    }

}

static void Uart2_Comm_Task(void * pvParameters)
{
    volatile UBaseType_t uxHighWaterMark;

    BaseType_t err = pdFALSE;

    (void)pvParameters;

    while(1)
    {
        // Wait for the semaphore to be given by the ISR
        // This will block until the semaphore is available 
        if(g_USART_2_Recv_Binary_Semaphore != NULL)
        {
            err = xSemaphoreTake(g_USART_2_Recv_Binary_Semaphore, portMAX_DELAY);

            if(pdTRUE == err)
            {
                ModbusReg_UpdateNormal();

                if(TRUE == Modbus_ParaseFrame(g_Usart_2_CommBuf.u8Recvbuf, g_Usart_2_CommBuf.u16RecvDataCnt, &ModbusUart2))
                {
                    g_CommLostCnt = 0; // Reset communication lost count if frame is parsed successfully
                    CheckModbusRegUpdate();
                }        
            }
        }
        else
        {
            vTaskDelay(300);
        }
        
        uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    }
}


/// @brief 检测通信是否丢失
/// @param pvParameters 
static void CommFaultMonitor_Task(void * pvParameters)
{
    volatile UBaseType_t uxHighWaterMark;

    (void)pvParameters;

    while(1)
    {
        if(GetMsgVal(SIGNAL_STATUS_FACTORY_MODE) == TRUE)
        {
            vTaskDelete(NULL);
        }
        
        if(++g_CommLostCnt >= UART_COMM_LOST_TIME)
        {
            g_CommLostCnt = UART_COMM_LOST_TIME; // Limit the count to UART_COMM_LOST_TIME

            SetSigVal(ALARM_ID_UART_COMM_ERR, EVENT_HAPPEN); 
            DisableAllGun(); 
        }
        else
        {
            SetSigVal(ALARM_ID_UART_COMM_ERR, EVENT_CANCEL); 
        }

        vTaskDelay(pdMS_TO_TICKS(1000));

        uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    }
}


void UartComm_InitTask(void * pvParameters)
{
    (void)pvParameters;

    taskENTER_CRITICAL();

    ModbusReg_Updata_BoardInfo();

    // Create tasks for UART communication
    xTaskCreate(Uart0_Comm_Task,        "Uart0_Comm_Task",       configMINIMAL_STACK_SIZE*4,    NULL,   tskIDLE_PRIORITY+4,     NULL);
    xTaskCreate(Uart1_Comm_Task,        "Uart1_Comm_Task",       configMINIMAL_STACK_SIZE*4,    NULL,   tskIDLE_PRIORITY+4,     NULL);
    xTaskCreate(Uart2_Comm_Task,        "Uart2_Comm_Task",       configMINIMAL_STACK_SIZE*4,    NULL,   tskIDLE_PRIORITY+4,     NULL);
    xTaskCreate(CommFaultMonitor_Task,  "CommFaultMonitor_Task", configMINIMAL_STACK_SIZE*2,    NULL,   tskIDLE_PRIORITY+4,     NULL);

	taskEXIT_CRITICAL();

	vTaskDelete(NULL);    
}




