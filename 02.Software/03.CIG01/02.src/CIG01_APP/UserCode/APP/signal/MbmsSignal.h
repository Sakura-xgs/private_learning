#ifndef  _MBMSIGNAL_H_
#define  _MBMSIGNAL_H_




//---------------------------------------------------------------------------------------------//
//------------------------------------------寄存器表（参数）-------------------------------------------//
//---------------------------------------------------------------------------------------------//
#define MBMS_SET_SIG_ID_BEGIN_FLAG                       	(1000)	//设置信号开始位置
#define MBMS_SET_SIG_ID_EXT_EEPROM_TEST_HIGH_FREQ_VAL    	(1001)	//eeprom高频测试参数
#define MBMS_SET_SIG_ID_HIGH_FREQ_END_FLAG               	(1002)	//高频设置信号结束位置
#define MBMS_SET_SIG_ID_EXT_EEPROM_TEST_VAL              	(1003)	//eeprom测试变量
#define MBMS_SET_SIG_ID_SN1                              	(1004)	//SN1
#define MBMS_SET_SIG_ID_SN2                              	(1005)	//SN2
#define MBMS_SET_SIG_ID_SN3                              	(1006)	//SN3
#define MBMS_SET_SIG_ID_SN4                              	(1007)	//SN4
#define MBMS_SET_SIG_ID_SN5                              	(1008)	//SN5
#define MBMS_SET_SIG_ID_SN6                              	(1009)	//SN6
#define MBMS_SET_SIG_ID_SN7                              	(1010)	//SN7
#define MBMS_SET_SIG_ID_SN8                              	(1011)	//SN8
#define MBMS_SET_SIG_ID_SN9                              	(1012)	//SN9
#define MBMS_SET_SIG_ID_SN10                             	(1013)	//SN10
#define MBMS_SET_SIG_ID_SN11                             	(1014)	//SN11
#define MBMS_SET_SIG_ID_SN12                             	(1015)	//SN12
#define MBMS_SET_SIG_ID_SN13                             	(1016)	//SN13
#define MBMS_SET_SIG_ID_SN14                             	(1017)	//SN14
#define MBMS_SET_SIG_ID_SN15                             	(1018)	//SN15
#define MBMS_SET_SIG_ID_SN16                             	(1019)	//SN16
#define MBMS_SET_SIG_ID_END_FLAG                         	(1020)	//设置信号结束位置




//---------------------------------------------------------------------------------------------//
//------------------------------------------寄存器表（实时寄存器数据）-------------------------------------------//
//---------------------------------------------------------------------------------------------//
#define MBMS_SAM_SIG_ID_BEGIN_FLAG                       	(2000)	//MBMS采集信号开始位置
#define MBMS_SAM_SIG_ID_UPDATE_STATE                     	(2001)	//系统升级状态
#define MBMS_SAM_SIG_ID_PCB_TEMP1                        	(2002)	//PCB温度1
#define MBMS_SAM_SIG_ID_PCB_TEMP2                        	(2003)	//PCB温度2
#define MBMS_SAM_SIG_ID_CC1_VOLTAGE_A                    	(2004)	//cc1电压A
#define MBMS_SAM_SIG_ID_CC1_VOLTAGE_B                    	(2005)	//cc1电压B
#define MBMS_SAM_SIG_ID_CC1_VOLTAGE_A_AD_RAW             	(2006)	//cc1电压A, ad原始值
#define MBMS_SAM_SIG_ID_CC1_VOLTAGE_B_AD_RAW             	(2007)	//cc1电压B, ad原始值
#define MBMS_SAM_SIG_ID_PCB_TEMP1_AD_RAW                 	(2008)	//PCB温度1, ad原始值
#define MBMS_SAM_SIG_ID_PCB_TEMP2_AD_RAW                 	(2009)	//PCB温度2, ad原始值
#define MBMS_SAM_SIG_ID_SOFTWARE_VERSION_1               	(2010)	//软件版本号
#define MBMS_SAM_SIG_ID_SOFTWARE_VERSION_2               	(2011)	//
#define MBMS_SAM_SIG_ID_SOFTWARE_VERSION_3               	(2012)	//
#define MBMS_SAM_SIG_ID_SOFTWARE_VERSION_4               	(2013)	//
#define MBMS_SAM_SIG_ID_SOFTWARE_VERSION_5               	(2014)	//
#define MBMS_SAM_SIG_ID_SOFTWARE_VERSION_6               	(2015)	//
#define MBMS_SAM_SIG_ID_SOFTWARE_VERSION_7               	(2016)	//
#define MBMS_SAM_SIG_ID_SOFTWARE_VERSION_8               	(2017)	//
#define MBMS_SAM_SIG_ID_HARDWARE_VERSION_1               	(2018)	//硬件版本号
#define MBMS_SAM_SIG_ID_HARDWARE_VERSION_2               	(2019)	//
#define MBMS_SAM_SIG_ID_HARDWARE_VERSION_3               	(2020)	//
#define MBMS_SAM_SIG_ID_HARDWARE_VERSION_4               	(2021)	//
#define MBMS_SAM_SIG_ID_HARDWARE_VERSION_5               	(2022)	//
#define MBMS_SAM_SIG_ID_HARDWARE_VERSION_6               	(2023)	//
#define MBMS_SAM_SIG_ID_HARDWARE_VERSION_7               	(2024)	//
#define MBMS_SAM_SIG_ID_HARDWARE_VERSION_8               	(2025)	//
#define MBMS_SAM_SIG_ID_END_FLAG                         	(2026)	//MBMS采集信号结束位置




//---------------------------------------------------------------------------------------------//
//------------------------------------------开关量表（常规告警、特殊告警）-------------------------------------------//
//---------------------------------------------------------------------------------------------//
#define ALARM_ID_EVENT_BEGIN_FLAG                        	(3000)	//事件信号开始位置
#define ALARM_ID_UART_COMM_ERR                           	(3001)	//串口通信故障
#define ALARM_ID_EEPROM_ERROR                            	(3002)	//eeprom故障
#define ALARM_ID_EVENT_END_FLAG                          	(3003)	//事件信号结束位置




//---------------------------------------------------------------------------------------------//
//------------------------------------------开关量表（信号状态）---------------------------------------//
//---------------------------------------------------------------------------------------------//
#define SIGNAL_STATUS_BEGIN_FLAG                         	(4000)	//信号状态开始标志
#define SIGNAL_STATUS_DI_A_GUN_POWER                     	(4001)	//A枪辅助电源反馈
#define SIGNAL_STATUS_DI_B_GUN_POWER                     	(4002)	//B枪辅助电源反馈
#define SIGNAL_STATUS_DI_A_GUN_LOCK                      	(4003)	//A枪电子锁反馈
#define SIGNAL_STATUS_DI_B_GUN_LOCK                      	(4004)	//B枪电子锁反馈
#define SIGNAL_STATUS_DO_A_GUN_SUPER                     	(4005)	//A枪超级充电控制
#define SIGNAL_STATUS_DO_B_GUN_SUPER                     	(4006)	//B枪超级充电控制
#define SIGNAL_STATUS_DO_A_GUN_POWER_24V                 	(4007)	//A枪24V辅助电源输出控制
#define SIGNAL_STATUS_DO_B_GUN_POWER_24V                 	(4008)	//B枪24V辅助电源输出控制
#define SIGNAL_STATUS_DO_A_GUN_POWER_12V                 	(4009)	//A枪12V辅助电源输出控制
#define SIGNAL_STATUS_DO_B_GUN_POWER_12V                 	(4010)	//B枪12V辅助电源输出控制
#define SIGNAL_STATUS_DO_A_GUN_POWER_GND                 	(4011)	//A枪辅助电源地控制
#define SIGNAL_STATUS_DO_B_GUN_POWER_GND                 	(4012)	//B枪辅助电源地控制
#define SIGNAL_STATUS_DO_A_GUN_LOCK                      	(4013)	//A枪电子锁控制
#define SIGNAL_STATUS_DO_B_GUN_LOCK                      	(4014)	//B枪电子锁控制
#define SIGNAL_STATUS_FACTORY_MODE                       	(4015)	//生产测试模式
#define SIGNAL_STATUS_END_FLAG                           	(4016)	//信号状态结束标志








#endif
