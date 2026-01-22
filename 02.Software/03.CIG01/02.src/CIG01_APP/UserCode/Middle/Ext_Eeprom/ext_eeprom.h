/*******************************************************************************
Copyright (C), 2009, PowerWise Tech. Co., Ltd.
-------------------------------------------------------------------------------|
ProjectName:
-------------------------------------------------------------------------------|
Description:    Application layer
-------------------------------------------------------------------------------|
Version:        0.1
-------------------------------------------------------------------------------|
Function List:
    1.
-------------------------------------------------------------------------------|
History:
    <author>        <time>      <version >      <desc>
             	 	 2010-11     0.1             build this moudle
*******************************************************************************/
#ifndef _EXT_EEPROM_H_
#define _EXT_EEPROM_H_

#include "PublicDefine.h"
/**** 定义常量区域 ****/
#define EXT_EEPROM_HQ_PAR_BACKUP_NUM 				(8)		//高频参数备份个数
#define EXT_EEPROM_NL_PAR_BACKUP_NUM 				(2)		//普通参数备份个数
#define EXT_EEPROM_PAR_DATA_LEN						(8)	    //每个参数的数据长度      

#define EXT_EEPROM_BEGIN_ADDR                       (0) 
#define EXT_EEPROM_END_ADDR                         (8*1024)  //8KB

#define EXT_EEPROM_HQ_PAR_SAVE_BEGIN_ADDR           (EXT_EEPROM_BEGIN_ADDR) 
#define EXT_EEPROM_HQ_PAR_SAVE_END_ADDR             (2*1024)  //2KB
#define EXT_EEPROM_NL_PAR_SAVE_BEGIN_ADDR           (EXT_EEPROM_HQ_PAR_SAVE_END_ADDR) 
#define EXT_EEPROM_NL_PAR_SAVE_END_ADDR             (EXT_EEPROM_END_ADDR) 

/**** 定义类型区域 ****/

typedef enum
{
	NEWEST     = 0xFF,   //最新的参数值
	ORIGINAL   = 0xF5,   //原本的参数值
    ABANDONED  = 0xA5    //废弃的参数值
}PARA_STAUTS;
/**** 声明变量区域 ****/

/**** 声明函数区域 ****/
extern void ExtEepromInit(void);
extern BOOL ExtEepromSaveParameter(U32 u32SigId, S32 s32SigVal);
// extern void ExtEepromParameterInit(void);
extern BOOL ExtEepromReadParameter(U32 u32SigId, P_S32 ps32RetVal);
#endif //_EXT_EEPROM_H_
