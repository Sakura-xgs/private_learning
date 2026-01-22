/*
 * PublicDefine.h
 *
 *  Created on: 2024年8月19日
 *      Author: Bono
 */

#ifndef PUBLICDEFINE_H_
#define PUBLICDEFINE_H_
#include "uart_comm.h"
#include "stdbool.h"

#define CHECK_PTR_NULL_NO_RETURN(ptr) do { \
    if ((ptr) == NULL) {          \
        printf("Error: Pointer is null at line %d in file %s\n", __LINE__, __FILE__);  \
        return;       \
    }                             \
} while(0)

#define CHECK_PTR_NULL(ptr) do { \
    if ((ptr) == NULL) {          \
        printf("Error: Pointer is null at line %d in file %s\n", __LINE__, __FILE__);  \
        return 0;       \
    }                             \
} while(0)

#define CHECK_MSG_LEN(limit_len, input_len) do { \
    if ((limit_len) < (input_len)) {                 \
        printf("Error: input length error at line %d in file %s\n", \
                 __LINE__, __FILE__); \
        return 0;                      \
    }                                            \
} while(0)

#define CHECK_MSG_LEN_NO_RETURN(limit_len, input_len) do { \
    if ((limit_len) < (input_len)) {                 \
        printf("Error: input length error at line %d in file %s\n", \
                 __LINE__, __FILE__); \
        return;                      \
    }                                            \
} while(0)

typedef unsigned char 		bl;
typedef unsigned char 		BOOL;
typedef unsigned char 		U8;
typedef unsigned short int 	U16;
typedef unsigned long int 	U32;
typedef signed char 		S8;
typedef short int 			S16;
typedef long int 			S32;
typedef unsigned char 		byte;
typedef unsigned short int 	word;
typedef unsigned int 		dword;
typedef U8 * 				P_U8;
typedef S8 * 				P_S8;
typedef U16 * 				P_U16;
typedef S16 * 				P_S16;
typedef U32 * 				P_U32;
typedef S32 * 				P_S32;
typedef float 				F32;
typedef void * 				PVOID;

#ifndef null
  #define null (0)
#endif

#ifndef NULL
  #define NULL (0)
#endif

#ifndef TRUE
	#define TRUE  1
#endif
#ifndef FALSE
	#define FALSE 0
#endif

#ifndef EVENT_HAPPEN
	#define EVENT_HAPPEN TRUE
#endif
#ifndef EVENT_CANCEL
	#define EVENT_CANCEL FALSE
#endif

#define  GET_LOW_BYTE(a)						    ((U8)(a))
#define  GET_HIGH_BYTE(a)						    ((U8)(a >> 8))
#define  GET_LOW_4_BYTE(a)						    ((U8)(a))
#define  GET_MIDDLE_LOW_4_BYTE(a)					((U8)(a >> 8))
#define  GET_MIDDLE_HIGH_4_BYTE(a)					((U8)(a >> 16))
#define  GET_HIGH_4_BYTE(a)						    ((U8)(a >> 24))

/* Task priorities. */
#define INIT_TASK_PRIO                  ( tskIDLE_PRIORITY + 3 )
#define LED_TASK_PRIO                   ( tskIDLE_PRIORITY + 3 )
#define SAVE_EEPROM_TASK_PRIO           ( tskIDLE_PRIORITY + 4 )
#define GENERAL_TASK_PRIO				( tskIDLE_PRIORITY + 4 )
#define STACK_OVER_FLOW_BUF_LEN			(5)

#define uPRINTF  						debug_printf


typedef struct
{
	U16 m_u16Signal;
	S32 m_u32DefaultVal;
	S32 m_u32MaxVal;
	S32 m_u32MinVal;
	U16 m_u16CanZoom;
	S16 m_s16OffSet;
}tag_signal;

typedef struct
{
    U32 u32SigId;
    S32 s32SigVal;
}EEP_MSG;

enum
{
    K1 = 0,
	K2,
	K3,
	K4,
	K5,
	K6,
	K7,
	K8,
	K9,
	K10,
	K11,
	K12,
	K13,
	K14,
	K15,
	K16,
	MAX_RELAY_NUM
};

enum
{
    PWM_LED1 = 0,
	PWM_LED2,
	PWM_LED3,
	USER_LED,
	MAX_PWM_LED_NUM
};

typedef enum
{
	SYS_STATUS_IDLE				= 0x0000,	//未定义系统工作状态
	SYS_STATUS_NORMAL			= 0x55AA,	//系统在工作模式下
	SYS_STATUS_SLEEP			= 0x5A5A,	//系统在休眠模式下
	SYS_STATUS_SHUTTING_DOWN	= 0xAA55,	//系统即将关机
	SYS_STATUS_POWER_OFF		= 0xAAA5,	//系统完成关机
}SYS_WORKING_STATYS;

#endif /* PUBLICDEFINE_H_ */
