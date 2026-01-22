#include "average_filter.h"

/***********************************温度滤波状态变量定义*******************************/
static AverageFilterState AverageFilters[CHANNEL_COUNT] = { 0 };

/******************************************************************************
* 名  	称:  AverageFilter_Update
* 功  	能:  滤波算法
* 入口参数:  AverageFilterType type - 温度通道类型
*            S32 newValue - 新的温度采样值
* 出口参数:  滤波后的温度值
*******************************************************************************/
S32 AverageFilter_Update(AverageFilterType type, S32 newValue)
{
    if(type >= CHANNEL_COUNT) 
    {
        return newValue; // 通道号错误，直接返回原始值
    }
    
    AverageFilterState *filter = &AverageFilters[type];
    S32 s32sum = 0;
    U8 i;
    
    // 更新历史数据
    filter->s32tempHistory[filter->u8tempIndex] = newValue;
    
    // 更新有效计数
    if(filter->u8validCount < AVERAGE_FILTER_WINDOW_SIZE)
    {
        filter->u8validCount++;
    }
    
    // 更新环形缓冲区索引
    filter->u8tempIndex = (filter->u8tempIndex + 1) % AVERAGE_FILTER_WINDOW_SIZE;
    
    // 计算历史数据总和
    for(i = 0; i < filter->u8validCount; i++)
    {
        s32sum += filter->s32tempHistory[i];
    }
    
    // 计算平均值
    return (filter->u8validCount > 0) ? (s32sum / filter->u8validCount) : newValue;
}
