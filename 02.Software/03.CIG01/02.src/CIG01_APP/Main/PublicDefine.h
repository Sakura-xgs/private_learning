#ifndef _PUBLICDEFINE_H_
#define _PUBLICDEFINE_H_

#include <stdint.h>

#define assert_param(expr) ((void)0U)

#define ARRAYNUM(arr_name)     (uint32_t)(sizeof(arr_name) / sizeof(*(arr_name)))

typedef unsigned char 		bl; 
typedef unsigned char 		BOOL;
typedef unsigned char 		U8;
typedef unsigned char 		byte;
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


#define SET_BIT(REG, BIT)      ((REG) |= (1U << (BIT)))
#define CLEAR_BIT(REG, BIT)    ((REG) &= ~(1U << (BIT)))
#define READ_BIT(REG, BIT)     (((REG) >> (BIT)) & 1U)
#define TOGGLE_BIT(REG, BIT)   ((REG) ^= (1U << (BIT)))


#define EVENT_HAPPEN                 (1)    	//事件产生
#define EVENT_CANCEL                 (0)    	//事件消失

#define _LINK_T(PRODUCT, AREA, INPUT, OUTPUT, YEAR, MONTH, DAY)  #PRODUCT#AREA#INPUT#OUTPUT#YEAR#MONTH#DAY
#define _SW_NUM(NAME)  #NAME

//软件版本信息
typedef struct
{
	U8 sw_major_version[13];
	U8 sw_minor_version[3];
}SOFT_VER;

typedef struct
{
	U8 hw_version[16];
}HARD_VER;

typedef enum
{
	I2C_OK    = 0,
	I2C_FAIL  = 1
}I2C_Status;

typedef enum
{
	NO_UPGRADE	= 0,
	UART1_UPGRADE = 1,
	UART2_UPGRADE = 2,
}UPGRADE_TYPE;



#endif //_PUBLICDEFINE_H_
