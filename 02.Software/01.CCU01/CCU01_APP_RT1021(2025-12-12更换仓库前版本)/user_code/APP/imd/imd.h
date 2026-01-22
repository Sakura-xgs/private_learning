/*
 * iso.h
 *
 *  Created on: 2024年9月25日
 *      Author: qjwu
 */

#ifndef APP_IMD_IMD_H_
#define APP_IMD_IMD_H_

#define CRC_OFFSET           		6U
#define CRC_LEN              		2U

#define IMD_ADD_A         	 		0x1U
#define IMD_ADD_B            		0x2U

#define IMD_SEND_TIMEOUT   	 		500U
#define IMD_WAIT_TIMEOUT    		20U

#define MIN_INSULATION_VALUE 		100U
#define NORMAL_INSULATION_VALUE  	500U

#define IMD_WORKING_VOL             1000U //0.1v

//#define IMD_DEBUG

extern U32 g_uiNow_tick_imd[2];

#endif /* APP_IMD_IMD_H_ */
