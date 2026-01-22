/*
 * RKN_can_comm.h
 *
 *  Created on: 2025年4月15日
 *      Author: qjwu
 */

#ifndef APP_SECC_RKN_CAN_COMM_H_
#define APP_SECC_RKN_CAN_COMM_H_

#pragma pack(1)

//BEM.59-62 CR_PLC_EvErrCode sent By EV
enum
{
	EV_RESSTEMPERATUREINHIBIT 		= 0x01,//EV充电失败RESS温度限制
	EV_FAILED_EVSHIFTPOSITION		= 2, //EV端连接器移位
	EV_CHARGERCONNECTORLOCKFAULT	= 3, //充电器连接器锁定故障
	EV_FAILED_EVRESSMALFUNCTION		= 4, //EV故障
	EV_CHARGINGCURRENTDIFFERENTIAL	= 5, //EV充电失败充电电流差
	EV_CHARGINGVOLTAGEOUTOFRANGE	= 6, //EV充电失败电压超出范围
	RESERVED_A						= 7,
	RESERVED_B						= 8,
	RESERVED_C						= 9,
	EV_CHARGINGSYSTEMINCOMPATIBILITY= 10,//充电系统不兼容
	EV_NODATA						= 11 //没有数据
};

//BEM.56-58 ErrorCode by CP or PP state
enum
{
    GQ_ERR_CP_STATE_ZERO               = 0x1,
	GQ_ERR_PP_UNPLUG                   = 0x2,
	GQ_ERR_CP_STATE_A                  = 0x3,
	GQ_ERR_CP_STATE_B                  = 0x4,
	GQ_ERR_CP_STATE_C                  = 0x5,
};

//BEM.48-55 CR_Plc_ErrorCode
enum
{
	GQ_SIG_ERROR_CP                         = 0x01, //SIG错误CP
	GQ_RECEIVED_ALL_STOP                    = 0x02, //收到所有停止
	GQ_J1772_ERROR_UNKNOWN_STATE            = 0x05, //J1772 错误未知状态
	GQ_CHADEMO_ERROR_UNKNOWN_STATE          = 0x06, //CHADEMO 错误未知状态
	GQ_CHADEMO_RX_ERROR_UNKNOWN_STATE       = 0x07, //CHADEMO RX 错误未知状态
	GQ_CHADEMO_TX_ERROR_UNKNOWN_STATE       = 0x08, //CHADEMO TX 错误未知状态
	GQ_INIT_ERROR_UNKNOWN_STATE             = 0x09, //初始化错误未知状态
	GQ_WAIT_ERROR_UNKNOWN_STATE             = 0x0A, //等待错误未知状态
	GQ_SLAC_ERROR_UNKNOWN_STATE             = 0x0B, //SLAC 错误未知状态
	GQ_SDP_ERROR_UNKNOWN_STATE              = 0x0C, //SDP 错误未知状态
	GQ_SDP_BIND_ERROR_UNKNOWN_STATE         = 0x0D, //SDP 绑定错误未知状态
	GQ_V2G_ACCEPT_UNKNOWN_STATE             = 0x0E, //V2G 接受未知状态
	GQ_V2G_HANDSHAKE_UNKNOWN_STATE          = 0x0F, //V2G 握手未知状态
	GQ_INIT_ERROR_GENERAL     				= 0x10, //初始化错误一般
	GQ_INIT_ERROR_IFADDR      				= 0x11, //初始化错误 IFADDR
	GQ_INIT_ERROR_THREAD                    = 0x12, //错误线程
	GQ_INIT_ERROR_OPENCHANNEL 				= 0x13, //初始化错误打开通道
	GQ_INIT_ERROR_KEY         				= 0x14, //初始化错误键
	GQ_SLAC_ERROR_GENERAL                  	= 0x20, //SLAC 错误一般
	GQ_SLAC_ERROR_TIMER_INIT				= 0x21, //SLAC 定时器初始化错误
	GQ_SLAC_ERROR_TIMER_TIMEOUT				= 0x22, //SLAC 定时器超时
	GQ_SLAC_ERROR_TIMER_MISC				= 0x23, //计时器选项
	GQ_SLAC_ERROR_PARAM_TIMEOUT            	= 0x24, //SLAC 错误参数超时
	GQ_SLAC_ERROR_PARAM_SOCKET             	= 0x25, //SLAC 错误插座参数
	GQ_SLAC_ERROR_START_ATTEN_CHAR_TIMEOUT 	= 0x26, //SLAC 错误开始关注字符超时
	GQ_SLAC_ERROR_MNBC_SOUND_TIMEOUT       	= 0x27, //SLAC“MNBC SOUD”错误超时
	GQ_SLAC_ERROR_ATTEN_CHAR_TIMEOUT       	= 0x28, //SLAC“ATTEN CHAR”错误超时
	GQ_SLAC_ERROR_ATTEN_CHAR_SOCKET        	= 0x29, //SLAC“ATTEN CHAR”错误插座
	GQ_SLAC_ERROR_VALIDATE_1_TIMEOUT       	= 0x2a, //SLAC 错误验证 1次超时
	GQ_SLAC_ERROR_VALIDATE_1_SOCKET        	= 0x2b, //SLAC 错误验证 1次插座
	GQ_SLAC_ERROR_VALIDATE_2_TIMEOUT       	= 0x2c, //SLAC 错误验证 2次超时
	GQ_SLAC_ERROR_VALIDATE_2_SOCKET        	= 0x2d, //SLAC 错误验证 2次插座
	GQ_SLAC_ERROR_BCB_TOGGLE_TIMEOUT       	= 0x2e, //SLAC 错误 BCB 切换超时
	GQ_SLAC_ERROR_MATCH_TIMEOUT            	= 0x2f, //SLAC 错误匹配超时
	GQ_SLAC_ERROR_MATCH_SOCKET             	= 0x30, //SLAC 错误匹配套插座
	GQ_SLAC_ERROR_READ_SOCKET              	= 0x31, //SLAC 错误读取插座
	GQ_SLAC_ERROR_LINK_DETECT              	= 0x33, //SLAC 错误链接检测
	GQ_SDP_ERROR_GENERAL 					= 0x40, //一般性错误
	GQ_SDP_ERROR_INIT_SOCKET    			= 0x41, //SDP 错误初始化套接字
	GQ_SDP_ERROR_INIT_SOCKOPT1				= 0x42, //初始化SOCKOPT1错误
	GQ_SDP_ERROR_INIT_SOCKOPT2				= 0x43, //初始化SOCKOPT2错误
	GQ_SDP_ERROR_INIT_BIND      			= 0x44, //SDP 错误初始化绑定
	GQ_SDP_ERROR_THREAD_SOCKET1				= 0x45, //SDP错误线程套接字1
	GQ_SDP_ERROR_THREAD_SOCKET2				= 0x46, //SDP错误线程套接字2
	GQ_SDP_ERROR_TIMEOUT        			= 0x47, //SDP 错误超时
	GQ_DIN_ERROR_GENERAL            		= 0x50, //DIN 错误一般
	GQ_DIN_ERROR_INIT_SOCKET				= 0x51, //初始化套接字
	GQ_DIN_ERROR_INIT_SOCKOPT_I             = 0x52, //sockopt初始化失败
	GQ_DIN_ERROR_INIT_BIND					= 0x53, //初始化绑定
	GQ_DIN_ERROR_INIT_LISTEN				= 0x54, //初始化监听
	GQ_DIN_ERROR_INIT_SELECT				= 0x55, //初始化选择
	GQ_DIN_ERROR_INIT_ACCEPT				= 0x56, //初始化接收
	GQ_DIN_ERROR_TIMEOUT            		= 0x57, //DIN 错误超时
	GQ_DIN_ERROR_V2GTP_HEADER       		= 0x58, //V2G报文长度不对
	GQ_DIN_ERROR_V2GTP_HEADER_LEN   		= 0x59, //V2G报文头长度不对
	GQ_DIN_ERROR_DECODE_EXI         		= 0x5a, //错误EXI解码
	GQ_DIN_ERROR_CREATE_RESPONSE    		= 0x5b, //错误响应
	GQ_DIN_ERROR_ENCODE_EXI         		= 0x5c, //错误EXI编码
	GQ_DIN_ERROR_V2GTP_HEADER_WRITE 		= 0x5d, //错误V2GTP 标头写入
	GQ_DIN_ERROR_SOCKET_EXCEPTION   		= 0x5e, //错误连接套字节异常
	GQ_DIN_ERROR_SOCKET_SEND        		= 0x5f, //错误发送连接
	GQ_DIN_ERROR_NO_PROTOCOL        		= 0x60, //没有兼容的充电协议
	GQ_DIN_SHUTDOWN_BY_EVSE         		= 0x70, //EVSE关机
	GQ_DIN_FAILED_RESPONSE            		= 0x80, //响应失败
	GQ_DIN_SEQUENCE_ERROR             		= 0x81, //顺序错误
	GQ_DIN_SIGNATURE_ERROR            		= 0x82, //签名验证失败
	GQ_DIN_UNKNOWN_SESSION            		= 0x83, //无法识别的会话
	GQ_DIN_NO_REQUEST_FROM_EV       		= 0x89, //没有收到车端请求
	//GQ_V2G_COMM_PORT_MAY_LOST            = 0x90, //v2g通讯丢失
	GQ_CP_STATE_B_2SECONDS_I               	= 0x91,
	GQ_CP_STATE_C_2SECONDS_I               	= 0x92,
	GQ_DETECT_CP_ZERO_I                    	= 0xA1,
	GQ_DETECT_PP_UNPLUG_I                  	= 0xA2,
	GQ_DETECT_CP_STATE_A_I                  = 0xA3,
	GQ_DETECT_CP_STATE_B_I                  = 0xA4,
	GQ_DETECT_CP_STATE_C_I                  = 0xA5,
	GQ_DETECT_CP_STATE_EF_I                 = 0xA6,
	GQ_DETECT_CP_STATE_D                  	= 0xA7,
	GQ_DETECT_PP_S3_PRESSED               	= 0xA8,
	GQ_CAN_LOSS_DETECT_ERROR        		= 0xB0, //can检测丢失
	GQ_CAN_CRMAA_TIMEOUT_ERROR       		= 0xB1, //CRMAA超时
	GQ_CAN_CRM00_TIMEOUT_ERROR       		= 0xB2, //CRM00超时
	GQ_CAN_CTSCML_TIMEOUT_ERROR      		= 0xB3, //CTS/CML超时
	GQ_CAN_CROAA_TIMEOUT_ERROR      		= 0xB4, //CROAA超时
	GQ_CAN_CST_TIMEOUT_ERROR      			= 0xB5, //CST超时
	GQ_CAN_CCS_TIMEOUT_ERROR      			= 0xB6, //CCS超时
	GQ_DIN_STOP_BY_EV_I                 	= 0xE0, //EV端停止
	GQ_ISO_PAUSE_BY_EV            			= 0xE1, //EV端暂停绝缘
	//20210831
	GQ_DIN_EMERGENCY_STOP_BY_SECC     		= 0xE4, //SECC故障停止
	GQ_DIN_STOP_BY_CHARGER               	= 0xE5, //充电器停止
};

//BEM.40-47 CR_Plc_ErrorCodeII
enum
{
	GQ_SIG_ERROR_CP_PP_II                     = 0x01, // SIG错误CP/PP
	GQ_INIT_ERROR_UNKNOWN_STATE_II            = 0x09, // 初始化错误未知状态
	GQ_WAIT_ERROR_UNKNOWN_STATE_II            = 0x0A, // 等待错误未知状态
	GQ_SLAC_ERROR_UNKNOWN_STATE_II            = 0x0B, // SLAC 错误未知状态
	GQ_SDP_ERROR_UNKNOWN_STATE_II             = 0x0C, // SDP 错误未知状态
	SDP_BIND_ERROR_UNKNOWN_STATE_II        	  = 0x0D, // SDP 绑定错误未知状态
	GQ_V2G_ACCEPT_UNKNOWN_STATE_II            = 0x0E, // V2G 接受未知状态
	GQ_V2G_HANDSHAKE_UNKNOWN_STATE_II         = 0x0F, // V2G 握手未知状态
	GQ_V2G_ERROR_UNKNOWN_STATE_II             = 0x10, // V2G 错误未知状态
	GQ_INIT_ERROR_IFADDR_II                   = 0x11, // 初始化地址错误
	GQ_INIT_ERROR_OPENCHANNEL_II              = 0x13, // 初始化错误打开通道
	GQ_INIT_ERROR_KEY_II                      = 0x14, // 错误的 KEY
	GQ_SLAC_ERROR_INIT_II                     = 0x20, // SLAC初始化错误
	GQ_SLAC_ERROR_PARAM_TIMEOUT_II            = 0x24, // 数据参数超时
	GQ_SLAC_ERROR_PARAM_SOCKET_II             = 0x25, // 参数连接错误
	SLAC_ERROR_START_ATTEN_CHAR_TIMEOUT_II 	  = 0x26, // 开始计算衰减字符超时
	SLAC_ERROR_MNBC_SOUND_TIMEOUT_II          = 0x27, // MNBC超时
	SLAC_ERROR_ATTEN_CHAR_TIMEOUT_II       	  = 0x28, // 衰减字符超时
	SLAC_ERROR_ATTEN_CHAR_SOCKET_II        	  = 0x29, // 衰减字符连接错误
	SLAC_ERROR_VALIDATE_1_TIMEOUT_II       	  = 0x2A, // 验证1超时
	SLAC_ERROR_VALIDATE_1_SOCKET_II           = 0x2B, // SOCKET-1接口验证
	SLAC_ERROR_VALIDATE_2_TIMEOUT_II          = 0x2C, // 验证2超时
	SLAC_ERROR_VALIDATE_2_SOCKET_II           = 0x2D, // SOCKET-2接口验证
	SLAC_ERROR_BCB_TOGGLE_TIMEOUT_II          = 0x2E, // BCB切换超时
	GQ_SLAC_ERROR_MATCH_TIMEOUT_II            = 0x2F, // 参数匹配超时
	GQ_SLAC_ERROR_MATCH_SOCKET_II             = 0x30, // SOCKET匹配
	GQ_SLAC_ERROR_LINK_DETECT_II              = 0x33, // SLAC 错误链接检测
	GQ_SDP_ERROR_INIT_SOCKET_II               = 0x41, // 初始化SOCKET
	GQ_SDP_ERROR_TIMEOUT_II                   = 0x47, // SDP错误超时
	GQ_DIN_ERROR_TIMEOUT_II                   = 0x57, // 通讯超时
	GQ_DIN_ERROR_V2GTP_HEADER_II              = 0x58, // 错误 V2GTP 标头
	GQ_DIN_ERROR_V2GTP_HEADER_LEN_II          = 0x59, // 错误 V2GTP 标头长度
	GQ_DIN_ERROR_DECODE_EXI_II                = 0x5A, // 错误EXI解码
	GQ_DIN_ERROR_CREATE_RESPONSE_II           = 0x5B, // 错误响应
	GQ_DIN_ERROR_ENCODE_EXI_II                = 0x5C, // 错误EXI编码
	DIN_ERROR_V2GTP_HEADER_WRITE_II           = 0x5D, // 错误V2GTP 标头写入
	GQ_DIN_ERROR_SOCKET_SEND_II               = 0x5F, // 错误发送连接
	GQ_DIN_ERROR_NO_PROTOCOL_II               = 0x60, // 没有兼容的充电协议
	GQ_CHM_TIMEOUT_II                         = 0x70, // CHM超时
	GQ_DIN_FAILED_RESPONSE_II                 = 0x80, // 响应失败
	GQ_DIN_SEQUENCE_ERROR_II                  = 0x81, // 顺序错误
	GQ_DIN_SIGNATURE_ERROR_II                 = 0x82, // 签名验证失败
	GQ_DIN_UNKNOWN_SESSION_II                 = 0x83, // 无法识别的会话
	GQ_DIN_SUDDEN_STOP_BY_SECC_II             = 0x84, // SECC意外关闭
	GQ_DIN_STOP_BY_SECC_II                    = 0x85, // SECC主动关闭
	GQ_DIN_RECEIVED_CST_II                    = 0x86, // 收到桩端CST
	GQ_DIN_STOP_BY_EV_II                      = 0x87, // EV端主动断开
	GQ_DIN_RECEIVED_CEM_II                    = 0x88, // 收到桩端CEM
	GQ_DIN_NO_REQUEST_FROM_EV_II              = 0x89, // 没有收到车端请求
	GQ_DIN_EV_DO_NOT_STOP_II                  = 0x8A, // 车端未停止
	GQ_V2G_COMM_PORT_MAY_LOST_II              = 0x90, // v2g通讯丢失
	GQ_CP_STATE_B_2SECONDS_II                 = 0x91,
	GQ_CP_STATE_C_2SECONDS_II                 = 0x92,
	GQ_DETECT_CP_ZERO_II                      = 0xA1,
	GQ_DETECT_PP_UNPLUG_II                    = 0xA2,
	GQ_DETECT_CP_STATE_A_II                   = 0xA3,
	GQ_DETECT_CP_STATE_B_II                   = 0xA4,
	GQ_DETECT_CP_STATE_C_II                   = 0xA5,
	GQ_DETECT_CP_STATE_EF_II                  = 0xA6,
};

typedef struct
{
    U8 CF_Plc_ReachSoC:2;
    U8 invaild2:2;
    U8 invaild3:2;
    U8 CF_Plc_ChargerSuspend:2;//充电暂停控制

    U8 invaild4:2;
    U8 invaild5:2;
    U8 invaild6:2;
    U8 invaild7:2;

    U8 invaild8:2;
    U8 invaild9:2;
    U8 invaild10:2;
    U8 invaild11:2;

    U8 invaild12:2;
    U8 invaild13:2;
    U8 invaild14:2;
    U8 CF_Plc_FullChgComplete:1;//充满电完成
    U8 CF_Plc_ChgFinished:1;//结束充电请求：1

	U8 Message_Sequence;
	U8 CR_Plc_ErrorCodeII;
    U8 CR_Plc_ErrorCode;
    U8 invaild15:3;
    U8 invaild16:4;
    U8 reserve:1;
}RKN_BST_Data_t;

typedef union
{
	U32 uiTimeout_id;
	struct
	{
		U32 crm_00:2;
		U32 crm_AA:2;
		U32 reserve1:4;
		U32 cml_cts:2;
		U32 cro:2;  //cro AA 超时
		U32 reserve2:4;
		U32 ccs:2;
		U32 cst:2;
		U32 reserve3:4;
		U32 csd:2;
		U32 reserve4:6;
	}sItem;
}RKNTimeout_t;

typedef struct
{
	U8 ucMessage_Sequence;
	U8 ucErrorCodeII;
	U8 ucErrorCode;
	U8 ErrorCode_By_CP:3;
	U8 EVErrorCode_By_EV:4;
	U8 reserve1:1;
}RKNError_t;

typedef struct
{
	RKNTimeout_t sTimeout;
	RKNError_t sError;
}RKN_BEM_Data_t;

typedef struct
{
	RKN_BST_Data_t sBST;
	RKN_BEM_Data_t sBEM;
}Rkn_Stop_Data_t;

#pragma pack()

extern Rkn_Stop_Data_t g_sRkn_stop_data[2];

void RKNSeccInit(void);
g_psCharge_Control_t* GetRknSeccModel(void);
void RKNBemTimeoutProcess(const RKN_BEM_Data_t sBEM_data, const U8 ucGun_id);
void RKNBemErrorProcess(const RKN_BEM_Data_t sBEM_data, const U8 ucGun_id);
void RKNBstMessageProcess(const RKN_BST_Data_t sBST_data, const U8 ucGun_id);

#endif /* APP_SECC_RKN_CAN_COMM_H_ */
