#ifndef __IO_CTRL_H
#define __IO_CTRL_H

#include "PublicDefine.h"


typedef enum
{
    IO_A_GUN_DO_SUPER,
    IO_B_GUN_DO_SUPER,
    IO_A_GUN_DO_24V,
    IO_B_GUN_DO_24V,    
    IO_A_GUN_DO_12V,
    IO_B_GUN_DO_12V,
    IO_A_GUN_DO_GND,
    IO_B_GUN_DO_GND,
    IO_A_GUN_DO_LOCK,
    IO_B_GUN_DO_LOCK,
}enumIO_CTRL_NO;


void DisableAllGun(void);
void IO_Ctrl(enumIO_CTRL_NO ioNo, BOOL outEnable);

#endif


