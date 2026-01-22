#ifndef _modbus_crc_h_
#define _modbus_crc_h_

#include "PublicDefine.h"



extern U16  CalCrc16(U8 *puchMsg, U16 usDataLen);

extern void Sol_ark_LenClhkSum(U16 DataLength, char * Data1, char * Data2, char * Data3, char * Data4);

extern void Sol_ark_ClhkSum(U8 *puchMsg,  U16 DataLen, char * Data1,  char * Data2, char * Data3, char * Data4);
#endif

