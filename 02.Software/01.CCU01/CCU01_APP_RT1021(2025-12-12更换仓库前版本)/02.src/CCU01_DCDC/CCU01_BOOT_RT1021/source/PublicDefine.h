/*
 * PublicDefine.h
 *
 *  Created on: 2024年8月16日
 *      Author: Bono
 */

#ifndef PUBLICDEFINE_H_
#define PUBLICDEFINE_H_

/* 数据类型定义 */
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


/* 常用宏定义 */
#define ARRAYNUM(arr_name)     (uint32_t)(sizeof(arr_name) / sizeof(*(arr_name)))

#define GET_U16_WORD(a)						        ((U16)(a))
#define GET_HIGH_WORD(a)						    ((U16)(a >> 16))

#define GET_LOW_BYTE(a)						        ((U8)(a))
#define GET_HIGH_BYTE(a)						    ((U8)(a >> 8))

#define GET_LOW_4_BYTE(a)						    ((U8)(a))
#define GET_MIDDLE_LOW_4_BYTE(a)					((U8)(a >> 8))
#define GET_MIDDLE_HIGH_4_BYTE(a)					((U8)(a >> 16))
#define GET_HIGH_4_BYTE(a)						    ((U8)(a >> 24))

#define GET_LOW_U16(a)						    	((U16)(a))
#define GET_HIGH_U16(a)						        ((U16)(a >> 16))

#define DIFF_VALUE(a,b)								(((a) > (b)) ? ((a) - (b)):((b) - (a)))


#endif /* PUBLICDEFINE_H_ */
