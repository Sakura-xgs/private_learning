/*
 * ex_eeprom.h
 *
 *  Created on: 2024年8月19日
 *      Author: Bono
 */

#ifndef HAL_HAL_EEPROM_HAL_EEPROM_IF_H_
#define HAL_HAL_EEPROM_HAL_EEPROM_IF_H_

#include "PublicDefine.h"


extern BOOL ExtEepromSaveParameter(U32 u32SigId, S32 s32SigVal);
extern BOOL ExtEepromReadParameter(U32 u32SigId, P_S32 ps32RetVal);


#endif /* HAL_HAL_EEPROM_HAL_EEPROM_IF_H_ */
