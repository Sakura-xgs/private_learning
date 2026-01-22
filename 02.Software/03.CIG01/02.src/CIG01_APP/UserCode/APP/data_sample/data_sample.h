#ifndef  __DATA_SAMPLE_H
#define  __DATA_SAMPLE_H

#include "PublicDefine.h"

#define FILTER_BUF_SIZE  10

typedef struct 
{
    U32 u32Buffer[FILTER_BUF_SIZE];
    U16 u16Index;
    U16 u16Cnt;
    U32 u32Sum;
    U32 u32FilteredData;
}strFilter;



void DataSample_Task(void *pvParameters);


#endif

