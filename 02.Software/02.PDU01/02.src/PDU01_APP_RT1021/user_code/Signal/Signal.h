#ifndef  _SIGNAL_H_
#define  _SIGNAL_H_


//---------------------------------------------------------------------------------------------//
//------------------------------------------寄存器表（参数）-------------------------------------------//
//---------------------------------------------------------------------------------------------//
#define PDU_SET_SIG_ID_BEGIN_FLAG                        	(1000)	//设置信号开始位置
#define PDU_SET_SIG_ID_RESET_EVENT_FLASH_FG              	(1001)	//初始化FLASH记录数据参数标志
#define PDU_SET_SIG_ID_FLASH_HIS_WR_ADDR                 	(1002)	//FLASH历史数据当前存储地址
#define PDU_SET_SIG_ID_FLASH_ERR_WR_ADDR                 	(1003)	//FLASH告警数据当前存储地址
#define PDU_SET_SIG_ID_FLASH_HIS_RD_ADDR                 	(1004)	//FLASH历史数据读取开始地址
#define PDU_SET_SIG_ID_FLASH_ERR_RD_ADDR                 	(1005)	//FLASH告警数据读取开始地址
#define PDU_SET_SIG_ID_FLASH_HIS_SAVE_FULL_FLAG          	(1006)	//FLASH历史数据存满标识
#define PDU_SET_SIG_ID_FLASH_ERR_SAVE_FULL_FLAG          	(1007)	//FLASH告警数据存满标识
#define PDU_SET_SIG_ID_FLASH_HIS_DATA_RECORD_NUM         	(1008)	//FLASH历史数据记录条数
#define PDU_SET_SIG_ID_FLASH_ERR_DATA_RECORD_NUM         	(1009)	//FLASH告警数据记录条数
#define PDU_SET_SIG_ID_RESET_TIMES                       	(1010)	//复位次数
#define PDU_SET_SIG_ID_STARTUP_DAY                       	(1011)	//上次开机日期(空、年、月、日)
#define PDU_SET_SIG_ID_STARTUP_SEC                       	(1012)	//上次开机时刻(空、时、分、秒)
#define PDU_SET_SIG_ID_HIGH_FREQ_END_FLAG                	(1013)	//高频设置信号结束位置
#define PDU_SET_SIG_ID_SN_NUMBER_1_2_3_4                 	(1014)	//产品SN(1、2、3、4位)
#define PDU_SET_SIG_ID_SN_NUMBER_5_6_7_8                 	(1015)	//产品SN(5、6、7、8位)
#define PDU_SET_SIG_ID_SN_NUMBER_9_10_11_12              	(1016)	//产品SN(9、10、11、12位)
#define PDU_SET_SIG_ID_SN_NUMBER_13_14_15_16             	(1017)	//产品SN(13、14、15、16位)
#define PDU_SET_SIG_ID_SN_NUMBER_17_18_19_20             	(1018)	//产品SN(17、18、19、20位)
#define PDU_SET_SIG_ID_SN_NUMBER_21_22_23_24             	(1019)	//产品SN(21、22、23、24位)
#define PDU_SET_SIG_ID_SN_NUMBER_25_26_27_28             	(1020)	//产品SN(25、26、27、28位)
#define PDU_SET_SIG_ID_SN_NUMBER_29_30_31_32             	(1021)	//产品SN(29、30、31、32位)
#define PDU_SET_SIG_ID_EXT_EEPROM_TEST_VAL               	(1022)	//外部eeprom功能测试信号
#define PDU_SET_SIG_ID_TEST_STATION_RECORD               	(1023)	//测试工站信息记录
#define PDU_SET_RELAY_1_OPERATE_TIMES                    	(1024)	//继电器1动作次数
#define PDU_SET_RELAY_2_OPERATE_TIMES                    	(1025)	//继电器2动作次数
#define PDU_SET_RELAY_3_OPERATE_TIMES                    	(1026)	//继电器3动作次数
#define PDU_SET_RELAY_4_OPERATE_TIMES                    	(1027)	//继电器4动作次数
#define PDU_SET_RELAY_5_OPERATE_TIMES                    	(1028)	//继电器5动作次数
#define PDU_SET_RELAY_6_OPERATE_TIMES                    	(1029)	//继电器6动作次数
#define PDU_SET_RELAY_7_OPERATE_TIMES                    	(1030)	//继电器7动作次数
#define PDU_SET_RELAY_8_OPERATE_TIMES                    	(1031)	//继电器8动作次数
#define PDU_SET_RELAY_9_OPERATE_TIMES                    	(1032)	//继电器9动作次数
#define PDU_SET_RELAY_10_OPERATE_TIMES                   	(1033)	//继电器10动作次数
#define PDU_SET_RELAY_11_OPERATE_TIMES                   	(1034)	//继电器11动作次数
#define PDU_SET_RELAY_12_OPERATE_TIMES                   	(1035)	//继电器12动作次数
#define PDU_SET_RELAY_13_OPERATE_TIMES                   	(1036)	//继电器13动作次数
#define PDU_SET_RELAY_14_OPERATE_TIMES                   	(1037)	//继电器14动作次数
#define PDU_SET_RELAY_15_OPERATE_TIMES                   	(1038)	//继电器15动作次数
#define PDU_SET_RELAY_16_OPERATE_TIMES                   	(1039)	//继电器16动作次数
#define PDU_SET_SIG_ID_END_FLAG                          	(1040)	//设置信号结束位置


//---------------------------------------------------------------------------------------------//
//------------------------------------------寄存器表（实时寄存器数据）-------------------------------------------//
//---------------------------------------------------------------------------------------------//
#define PDU_SAM_SIG_ID_BEGIN_FLAG                        	(2000)	//采集信号开始位置
#define PDU_SAM_SIG_ID_UPDATE_STATE                      	(2001)	//升级状态
#define PDU_SAM_SIG_ID_SW_VERSION_1_2_3_4                	(2002)	//软件版本(1、2、3、4位)
#define PDU_SAM_SIG_ID_SW_VERSION_5_6_7_8                	(2003)	//软件版本(5、6、7、8位)
#define PDU_SAM_SIG_ID_SW_VERSION_9_10_11_12             	(2004)	//软件版本(9、10、11、12位)
#define PDU_SAM_SIG_ID_SW_VERSION_13_14_15_16            	(2005)	//软件版本(13、14、15、16位)
#define PDU_SAM_SIG_ID_HW_VERSION_1_2_3_4                	(2006)	//硬件版本(1、2、3、4位)
#define PDU_SAM_SIG_ID_HW_VERSION_5_6_7_8                	(2007)	//硬件版本(5、6、7、8位)
#define PDU_SAM_SIG_ID_HW_VERSION_9_10_11_12             	(2008)	//硬件版本(9、10、11、12位)
#define PDU_SAM_SIG_ID_HW_VERSION_13_14_15_16            	(2009)	//硬件版本(13、14、15、16位)
#define PDU_SAM_SIG_ID_HW_VERSION_PIN                    	(2010)	//硬件版本读取结果
#define PDU_SAM_SIG_ID_BOARD_ID                          	(2011)	//PDU板号
#define PDU_SAM_SIG_ID_BAT_RATE_VOLT                     	(2012)	//电池额定总电压
#define PDU_SAM_SIG_ID_CHARGE_CUTOFF_VOLT                	(2013)	//充电截止总电压
#define PDU_SAM_SIG_ID_CHARGE_MAX_VOLT                   	(2014)	//充电桩电压上限
#define PDU_SAM_SIG_ID_PCB_TEMP_1                        	(2015)	//PDU板温1
#define PDU_SAM_SIG_ID_PCB_TEMP_2                        	(2016)	//PDU板温2
#define PDU_SAM_SIG_ID_DC_POS_TEMP                       	(2017)	//DC+温度
#define PDU_SAM_SIG_ID_DC_NEG_TEMP                       	(2018)	//DC-温度
#define PDU_SAM_SIG_ID_END_FLAG                          	(2019)	//采集信号结束位置


//---------------------------------------------------------------------------------------------//
//------------------------------------------开关量表（常规告警、特殊告警）-------------------------------------------//
//---------------------------------------------------------------------------------------------//
#define ALARM_ID_EVENT_BEGIN_FLAG                        	(3000)	//事件信号开始位置
#define ALARM_ID_SYS_ERROR_LEV_1                         	(3001)	//系统故障等级一级
#define ALARM_ID_SYS_ERROR_LEV_2                         	(3002)	//系统故障等级二级
#define ALARM_ID_PDU_ID_REPEAT                           	(3003)	//PDU地址重复故障
#define ALARM_ID_PCU_OFFLINE                             	(3004)	//PCU通讯丢失故障
#define ALARM_ID_EEPROM_ERROR                            	(3005)	//EEPROM故障
#define ALARM_ID_SYS_ERROR_DIVIDE_SIGNAL                 	(3006)	//系统故障等级分隔位置
#define ALARM_ID_K1_ADHESION                             	(3007)	//K1继电器粘连
#define ALARM_ID_K2_ADHESION                             	(3008)	//K2继电器粘连
#define ALARM_ID_K3_ADHESION                             	(3009)	//K3继电器粘连
#define ALARM_ID_K4_ADHESION                             	(3010)	//K4继电器粘连
#define ALARM_ID_K5_ADHESION                             	(3011)	//K5继电器粘连
#define ALARM_ID_K6_ADHESION                             	(3012)	//K6继电器粘连
#define ALARM_ID_K7_ADHESION                             	(3013)	//K7继电器粘连
#define ALARM_ID_K8_ADHESION                             	(3014)	//K8继电器粘连
#define ALARM_ID_K9_ADHESION                             	(3015)	//K9继电器粘连
#define ALARM_ID_K10_ADHESION                            	(3016)	//K10继电器粘连
#define ALARM_ID_K11_ADHESION                            	(3017)	//K11继电器粘连
#define ALARM_ID_K12_ADHESION                            	(3018)	//K12继电器粘连
#define ALARM_ID_K13_ADHESION                            	(3019)	//K13继电器粘连
#define ALARM_ID_K14_ADHESION                            	(3020)	//K14继电器粘连
#define ALARM_ID_K15_ADHESION                            	(3021)	//K15继电器粘连
#define ALARM_ID_K16_ADHESION                            	(3022)	//K16继电器粘连
#define ALARM_ID_K1_DRIVE_FAILED                         	(3023)	//K1继电器驱动失败
#define ALARM_ID_K2_DRIVE_FAILED                         	(3024)	//K2继电器驱动失败
#define ALARM_ID_K3_DRIVE_FAILED                         	(3025)	//K3继电器驱动失败
#define ALARM_ID_K4_DRIVE_FAILED                         	(3026)	//K4继电器驱动失败
#define ALARM_ID_K5_DRIVE_FAILED                         	(3027)	//K5继电器驱动失败
#define ALARM_ID_K6_DRIVE_FAILED                         	(3028)	//K6继电器驱动失败
#define ALARM_ID_K7_DRIVE_FAILED                         	(3029)	//K7继电器驱动失败
#define ALARM_ID_K8_DRIVE_FAILED                         	(3030)	//K8继电器驱动失败
#define ALARM_ID_K9_DRIVE_FAILED                         	(3031)	//K9继电器驱动失败
#define ALARM_ID_K10_DRIVE_FAILED                        	(3032)	//K10继电器驱动失败
#define ALARM_ID_K11_DRIVE_FAILED                        	(3033)	//K11继电器驱动失败
#define ALARM_ID_K12_DRIVE_FAILED                        	(3034)	//K12继电器驱动失败
#define ALARM_ID_K13_DRIVE_FAILED                        	(3035)	//K13继电器驱动失败
#define ALARM_ID_K14_DRIVE_FAILED                        	(3036)	//K14继电器驱动失败
#define ALARM_ID_K15_DRIVE_FAILED                        	(3037)	//K15继电器驱动失败
#define ALARM_ID_K16_DRIVE_FAILED                        	(3038)	//K16继电器驱动失败
#define ALARM_ID_EVENT_END_FLAG                          	(3039)	//事件信号结束位置


//---------------------------------------------------------------------------------------------//
//------------------------------------------开关量表（信号状态）---------------------------------------//
//---------------------------------------------------------------------------------------------//
#define SIGNAL_STATUS_BEGIN_FLAG                         	(4000)	//信号状态开始标志
#define SIGNAL_STATUS_POWER_ON                           	(4001)	//开机
#define SIGNAL_STATUS_POWER_OFF                          	(4002)	//关机
#define SIGNAL_STATUS_UPDATA                             	(4003)	//升级
#define SIGNAL_STATUS_FACTORY_TEST                       	(4004)	//生产测试
#define SIGNAL_STATUS_WDG_TEST                           	(4005)	//看门狗测试
#define SIGNAL_STATUS_K1                                 	(4006)	//K1继电器状态
#define SIGNAL_STATUS_K2                                 	(4007)	//K2继电器状态
#define SIGNAL_STATUS_K3                                 	(4008)	//K3继电器状态
#define SIGNAL_STATUS_K4                                 	(4009)	//K4继电器状态
#define SIGNAL_STATUS_K5                                 	(4010)	//K5继电器状态
#define SIGNAL_STATUS_K6                                 	(4011)	//K6继电器状态
#define SIGNAL_STATUS_K7                                 	(4012)	//K7继电器状态
#define SIGNAL_STATUS_K8                                 	(4013)	//K8继电器状态
#define SIGNAL_STATUS_K9                                 	(4014)	//K9继电器状态
#define SIGNAL_STATUS_K10                                	(4015)	//K10继电器状态
#define SIGNAL_STATUS_K11                                	(4016)	//K11继电器状态
#define SIGNAL_STATUS_K12                                	(4017)	//K12继电器状态
#define SIGNAL_STATUS_K13                                	(4018)	//K13继电器状态
#define SIGNAL_STATUS_K14                                	(4019)	//K14继电器状态
#define SIGNAL_STATUS_K15                                	(4020)	//K15继电器状态
#define SIGNAL_STATUS_K16                                	(4021)	//K16继电器状态
#define SIGNAL_STATUS_K1_FB                              	(4022)	//K1继电器反馈状态
#define SIGNAL_STATUS_K2_FB                              	(4023)	//K2继电器反馈状态
#define SIGNAL_STATUS_K3_FB                              	(4024)	//K3继电器反馈状态
#define SIGNAL_STATUS_K4_FB                              	(4025)	//K4继电器反馈状态
#define SIGNAL_STATUS_K5_FB                              	(4026)	//K5继电器反馈状态
#define SIGNAL_STATUS_K6_FB                              	(4027)	//K6继电器反馈状态
#define SIGNAL_STATUS_K7_FB                              	(4028)	//K7继电器反馈状态
#define SIGNAL_STATUS_K8_FB                              	(4029)	//K8继电器反馈状态
#define SIGNAL_STATUS_K9_FB                              	(4030)	//K9继电器反馈状态
#define SIGNAL_STATUS_K10_FB                             	(4031)	//K10继电器反馈状态
#define SIGNAL_STATUS_K11_FB                             	(4032)	//K11继电器反馈状态
#define SIGNAL_STATUS_K12_FB                             	(4033)	//K12继电器反馈状态
#define SIGNAL_STATUS_K13_FB                             	(4034)	//K13继电器反馈状态
#define SIGNAL_STATUS_K14_FB                             	(4035)	//K14继电器反馈状态
#define SIGNAL_STATUS_K15_FB                             	(4036)	//K15继电器反馈状态
#define SIGNAL_STATUS_K16_FB                             	(4037)	//K16继电器反馈状态
#define SIGNAL_STATUS_END_FLAG                           	(4038)	//信号状态结束标志




#endif
