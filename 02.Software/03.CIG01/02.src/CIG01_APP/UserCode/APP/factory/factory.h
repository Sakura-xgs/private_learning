#ifndef __FACTORY_H__
#define __FACTORY_H__

#include "PublicDefine.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"




// #define FACTORY_FRAME_HEAD1 0XA5
// #define FACTORY_FRAME_HEAD2 0X5A

// #define FACTORY_ACK_OK  0x66
// #define FACTORY_ACK_ERR 0x77

// typedef enum
// {
//     FACTORY_PORT_UART0,
//     FACTORY_PORT_UART1,
//     FACTORY_PORT_UART2,
// }enumFactoryPort;

// typedef enum
// {
//     FACTORY_MODE_START_EXIT     = 0x00,
//     FACTORY_IO                  = 0x01,
//     FACTORY_PCB_TEMP            = 0x02,
//     FACTORY_CC1_VOLTAGE         = 0x03,
//     FACTORY_SN                  = 0x04,
//     FACTORY_SOFTWARE_VERSION    = 0x05,
//     FACTORY_UART                = 0x06,  
// }enumFactoryFun;


// void Factory_Task(void * pvParameters);
// BOOL blCheckFactoryFrame(enumFactoryPort port);

void EnterFactoryMode(void);
void ExitFactoryMode(void);

#endif


