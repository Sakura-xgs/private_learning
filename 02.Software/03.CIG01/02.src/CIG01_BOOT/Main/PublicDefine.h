#ifndef _PUBLICDEFINE_H_
#define _PUBLICDEFINE_H_


#define assert_param(expr) ((void)0U)

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



#endif //_PUBLICDEFINE_H_
