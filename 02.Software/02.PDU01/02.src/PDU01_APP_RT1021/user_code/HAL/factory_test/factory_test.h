/*
 * factory_test.h
 *
 *  Created on: 2024年12月3日
 *      Author: Bono
 */

#ifndef HAL_FACTORY_TEST_FACTORY_TEST_H_
#define HAL_FACTORY_TEST_FACTORY_TEST_H_

#include "PublicDefine.h"

#define FACTORY_REPLY_ID	(0x10018000)

typedef struct
{
    U32 cmd;
    void (*Func)(U8*);
}TEST_PARAM_T;


void FactoryModeCtlFunc(U8 *data);
void RelayCtlTestFunc(U8 *data);
void LedTestFunc(U8 *data);
void SwVersionTestFunc(U8 *data);
void EepromTestFunc(U8 *data);
void WdgTestFunc(U8 *data);
void GpioDITestFunc(U8 *data);

extern QueueHandle_t g_Factory_Test_Msg_xQueue;
extern void FactoryTestParse(flexcan_frame_t *Frame);
extern void Factory_Test_Init_task(void * pvParameters);

#endif /* HAL_FACTORY_TEST_FACTORY_TEST_H_ */
