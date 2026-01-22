#ifndef AVERAGE_FILTER_H
#define AVERAGE_FILTER_H

#include "PublicDefine.h"


#define AVERAGE_FILTER_WINDOW_SIZE (5)  				// 温度滤波窗口大小

// 滤波状态类型定义
typedef struct {
    S32 s32tempHistory[AVERAGE_FILTER_WINDOW_SIZE];  	// 历史温度值
    U8  u8tempIndex;                              		// 当前历史温度索引
    U8  u8validCount;                             		// 当前有效数据个数
} AverageFilterState;

// 滤波通道类型
typedef enum {
    TEMP_DC_NEG_TYPE,
    TEMP_PCB_1_TYPE,
    TEMP_DC_POS_TYPE,
    TEMP_PCB_2_TYPE,
    CHANNEL_COUNT
} AverageFilterType;

S32 AverageFilter_Update(AverageFilterType type, S32 newValue);

#endif // AVERAGE_FILTER_H
