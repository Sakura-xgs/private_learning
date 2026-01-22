/**
  ******************************************************************************
  * @file    main.h 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Header for main.c module
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BOOT_H
#define __BOOT_H

/* Includes ------------------------------------------------------------------*/
#include "PublicDefine.h"

/**** 定义常量区域 ****/

/**** 定义宏表达式区域 ****/

/**** 定义类型区域 ****/ 

/**** 声明变量区域 ****/

/**** 声明函数区域 ****/
extern void SystemResetFunc(void);
extern void Boot2AppFunc(void);
extern BOOL TransferFlashData(const U32 u32SourceBeginAddr, const U32 u32TargetBeginAddr, const U32 u32FlashPageNum);
#endif /* __MAIN_H */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
