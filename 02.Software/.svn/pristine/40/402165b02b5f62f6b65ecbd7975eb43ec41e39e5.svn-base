/*
 * hal_eeprom.h
 *
 *  Created on: 2024年8月30日
 *      Author: Bono
 */

#ifndef HAL_HAL_EEPROM_HAL_EEPROM_H_
#define HAL_HAL_EEPROM_HAL_EEPROM_H_

/**** 定义常量区域 ****/
#define EXT_EEPROM_ADDR								(0xA0U)

#define EXT_EEPROM_HQ_PAR_BACKUP_NUM 				(8U)		//高频参数备份个数
#define EXT_EEPROM_NL_PAR_BACKUP_NUM 				(2U)		//普通参数备份个数
#define EXT_EEPROM_PAR_DATA_LEN						(8U)	    //每个参数的数据长度

#define EXT_EEPROM_BEGIN_ADDR                       (0U)
#define EXT_EEPROM_END_ADDR                         (7U*1024U)  //共8KB，最后1k用于存储交易信息

#define EXT_EEPROM_HQ_PAR_SAVE_BEGIN_ADDR           (EXT_EEPROM_BEGIN_ADDR)
#define EXT_EEPROM_HQ_PAR_SAVE_END_ADDR             (2U*1024U)  //2KB
#define EXT_EEPROM_NL_PAR_SAVE_BEGIN_ADDR           (EXT_EEPROM_HQ_PAR_SAVE_END_ADDR)
#define EXT_EEPROM_NL_PAR_SAVE_END_ADDR             (EXT_EEPROM_END_ADDR)

/**** 定义类型区域 ****/

typedef enum
{
	NEWEST     = 0xFF,   //最新的参数值
	ORIGINAL   = 0xF5,   //原本的参数值
    ABANDONED  = 0xA5    //废弃的参数值
}PARA_STAUTS;

#endif /* HAL_HAL_EEPROM_HAL_EEPROM_H_ */
