#include "MbmsSignal.h"
#include "PublicDefine.h"
#include "SignalManageDefine.h"






const tag_BMUsignal MBMSetSignalArry[] = 
{
	//SIGNALENNAME,                                     Default Value,Max Value,    Min Value,    Zoom,         Offset,       },//SignalChName
	{MBMS_SET_SIG_ID_BEGIN_FLAG,                       	0,            0,            0,            1,            0,            },//设置信号开始位置
	{MBMS_SET_SIG_ID_EXT_EEPROM_TEST_HIGH_FREQ_VAL,    	0,            65535,        0,            1,            0,            },//eeprom高频测试参数
	{MBMS_SET_SIG_ID_HIGH_FREQ_END_FLAG,               	0,            0,            0,            1,            0,            },//高频设置信号结束位置
	{MBMS_SET_SIG_ID_EXT_EEPROM_TEST_VAL,              	0,            65535,        0,            1,            0,            },//eeprom测试变量
	{MBMS_SET_SIG_ID_SN1,                              	0,            65535,        0,            1,            0,            },//SN1
	{MBMS_SET_SIG_ID_SN2,                              	0,            65535,        0,            1,            0,            },//SN2
	{MBMS_SET_SIG_ID_SN3,                              	0,            65535,        0,            1,            0,            },//SN3
	{MBMS_SET_SIG_ID_SN4,                              	0,            65535,        0,            1,            0,            },//SN4
	{MBMS_SET_SIG_ID_SN5,                              	0,            65535,        0,            1,            0,            },//SN5
	{MBMS_SET_SIG_ID_SN6,                              	0,            65535,        0,            1,            0,            },//SN6
	{MBMS_SET_SIG_ID_SN7,                              	0,            65535,        0,            1,            0,            },//SN7
	{MBMS_SET_SIG_ID_SN8,                              	0,            65535,        0,            1,            0,            },//SN8
	{MBMS_SET_SIG_ID_SN9,                              	0,            65535,        0,            1,            0,            },//SN9
	{MBMS_SET_SIG_ID_SN10,                             	0,            65535,        0,            1,            0,            },//SN10
	{MBMS_SET_SIG_ID_SN11,                             	0,            65535,        0,            1,            0,            },//SN11
	{MBMS_SET_SIG_ID_SN12,                             	0,            65535,        0,            1,            0,            },//SN12
	{MBMS_SET_SIG_ID_SN13,                             	0,            65535,        0,            1,            0,            },//SN13
	{MBMS_SET_SIG_ID_SN14,                             	0,            65535,        0,            1,            0,            },//SN14
	{MBMS_SET_SIG_ID_SN15,                             	0,            65535,        0,            1,            0,            },//SN15
	{MBMS_SET_SIG_ID_SN16,                             	0,            65535,        0,            1,            0,            },//SN16
	{MBMS_SET_SIG_ID_END_FLAG,                         	0,            0,            0,            1,            0,            },//设置信号结束位置
};




