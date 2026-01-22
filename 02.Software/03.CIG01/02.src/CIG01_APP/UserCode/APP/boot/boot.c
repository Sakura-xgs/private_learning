#include "boot.h"
#include "gd32f4xx.h"
#include "flash_program.h"
#include "hal_uart.h"
#include "MbmsSignal.h"
#include "SignalManage.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"  
#include "io_ctrl.h"


__attribute__((section(".ARM.__at_"str_APP_SOFTWARE_VERSION_ADDR)))volatile const U8 g_u8BootVersion[] = 
{	'V', '5', '.', '0', '4', '.', '1'};

__attribute__((section(".ARM.__at_"str_APP_INTEG_FG_FLASH_ADDR)))volatile const U8 g_u8AppIntegFg[] = 
{'A','P','P','I','N','T','E','G'};


static TaskHandle_t UpdataTask_Handler = NULL;
SemaphoreHandle_t g_USART_1_Updata_Binary_Semaphore = NULL;
SemaphoreHandle_t g_USART_2_Updata_Binary_Semaphore = NULL;

static USART_BOOT_STEP_CMD enumUsartPreUpdataStep = USART_UPDATE_UNKNOW_CMD;

static BOOL g_blUsartUpdataFuncEnFg = FALSE;

static const U16 ccitt16Table[] =
{
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

static const unsigned int CRC32TABLE[] =
{
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
	0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
	0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
	0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
	0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
	0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
	0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
	0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
	0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
	0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
	0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
	0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
	0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
	0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
	0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
	0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
	0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
	0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
	0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
	0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
	0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
	0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

static U16 crc16_ccitt_xmodem(const U8* data, U32 len, U16 u16CrcInitVal)
{
	U16  crc = 0x0000;
	U32  i = 0;

	crc = u16CrcInitVal;

	while ((len--) > 0)
	{
		crc = (U16)((crc << 8) ^ ccitt16Table[((crc >> 8) ^ data[i]) & 0xff]);
		i++;
	}

	return crc;
}

static unsigned int crc32(U8 *data, U32 Len, U32 uCurCRC)
{
	U32 crcValue = 0xFFFFFFFF;
	U32 i = 0;

	crcValue = uCurCRC;

	for (i = 0; i < Len; i++)
	{
		crcValue = CRC32TABLE[(crcValue ^ data[i]) & 0xff] ^ (crcValue >> 8);
	}

	return crcValue;
}


/******************************************************************************
* 名  	称:  SystemResetFunc
* 功  	能:  重启程序
* 入口参数:  无
* 出口参数:  无
*******************************************************************************/
void SystemResetFunc(void)
{
	__set_FAULTMASK(1);			//关闭所有中断
	__disable_irq();
	NVIC_SystemReset();			//复位
	for(;;);
}

/// @brief flash写入APP升级数据包
/// @param pu8Data 
/// @param u16DataLen 
/// @param u16MsgNo 
/// @return 
static BOOL WriteAppPack2Flash(const P_U8 pu8Data, const U16 u16DataLen, const U16 u16MsgNo)
{
	BOOL blRet = FALSE;
	U32 u32FlashBeginAddr = 0;
	U8 u8FlashWrBuf[MSG_UPDATE_MAX_DATALEN] = {0};
	U8 u8FlashRdBuf[MSG_UPDATE_MAX_DATALEN] = {0};
	U16 u16DataCnt = 0;
    U32 u32WrErrCnt = 0;
	
	u32FlashBeginAddr = UPDATA_FLASH_WR_BEGIN_ADDR + MSG_UPDATE_MAX_DATALEN*u16MsgNo;	//起始操作地址

	//转存升级数据
	for (u16DataCnt = 0; u16DataCnt < MSG_UPDATE_MAX_DATALEN; u16DataCnt++)
	{
		u8FlashWrBuf[u16DataCnt] = pu8Data[u16DataCnt];
	}
	
	//升级过程中，不写入APP完整标志(0xff覆盖“A P P I N T E G”标记，后续再写入)
	if(u32FlashBeginAddr == UPDATA_FLASH_WR_BEGIN_ADDR)
	{
		for (u16DataCnt = 0; u16DataCnt < 8; u16DataCnt++)
		{
			u8FlashWrBuf[0x20+u16DataCnt] = 0xFF;
		}
	}

    FlashBufferWrite(u32FlashBeginAddr, u8FlashWrBuf, u16DataLen);
    FlashBufferRead(u32FlashBeginAddr, u8FlashRdBuf, u16DataLen);

	for(u16DataCnt = 0; u16DataCnt < u16DataLen; u16DataCnt++)
	{
		if(u8FlashRdBuf[u16DataCnt] != u8FlashWrBuf[u16DataCnt])
		{
			u32WrErrCnt++;
		}
	}

	//最后才写入完整标志位
	if(0 == u32WrErrCnt)
	{
		blRet = TRUE;
	}
	else
	{
		blRet = FALSE;
	}

	return blRet;	
}


/// @brief flash写入APP完整标志位
/// @param  
/// @return 
static BOOL WriteAppInterFg2Flash(void)
{
	U8 app_integ_tab[8] = {'A','P','P','I','N','T','E','G'};

    FlashBufferWrite(UPDATA_FLASH_WR_BEGIN_ADDR+0x20, app_integ_tab, 8);

	return TRUE;
}

/******************************************************************************
* 名  	称:  CheckAppIntegFg
* 功  	能: 获取APP完整标志
* 入口参数:  无
* 出口参数:  无
*******************************************************************************/
static BOOL CheckExtFlashAppIntegFg(void)
{
	U8 i = 0;
	U8 temp_data[8]={0};
	U8 app_integ_tab[8] = {'A','P','P','I','N','T','E','G'};

    FlashBufferRead(UPDATA_FLASH_WR_BEGIN_ADDR + 0x20, temp_data, 8);

	for(i=0;i<8;i++)
	{
		if(app_integ_tab[i]!=temp_data[i])
		{
			return FALSE;
		}
	}

	return TRUE;
}

/******************************************************************************
* 名  	称:  blDetcetUsartUpdataStep
* 功  	能： 判断Usart升级报文是否按照升级流程要求
* 入口参数:  无
* 出口参数:  无
*******************************************************************************/
static BOOL blDetcetUsartUpdataStep(USART_BOOT_STEP_CMD RecStep)
{
    BOOL blRet = TRUE;

    switch (enumUsartPreUpdataStep)
    {
        case USART_UPDATE_UNKNOW_CMD:
            if (USART_UPDATE_REQUEST_CMD != RecStep
			|| GetMsgVal(MBMS_SAM_SIG_ID_UPDATE_STATE) != NO_UPGRADE)
            {
                blRet = FALSE;
            }
            break;

        case USART_UPDATE_REQUEST_CMD:
            if (USART_UPDATE_READY_CMD != RecStep)
            {
                blRet = FALSE;
            }
            break;

        case USART_UPDATE_READY_CMD:
            if (USART_UPDATE_BEGIN_CMD != RecStep)
            {
                blRet = FALSE;
            }
            break;

        case USART_UPDATE_BEGIN_CMD:
            if (USART_UPDATE_MIDDLE_CMD != RecStep)
            {
                blRet = FALSE;
            }
            break;

        case USART_UPDATE_MIDDLE_CMD:
            if ((USART_UPDATE_MIDDLE_CMD != RecStep) 
                && (USART_UPDATE_END_CMD != RecStep))
            {
                blRet = FALSE;
            }
            break;

        case USART_UPDATE_END_CMD:
            if (USART_UPDATE_CHECKSUM_CMD != RecStep)
            {
                blRet = FALSE;
            }
            break;
        
        default:
            blRet = FALSE;
            break;
    }

    return blRet;
}

/// @brief 判断是否需要升级
/// @param  
/// @return 
BOOL blCheck_Usart1_UpdateMsg(void)
{
	BaseType_t xHigherPriorityTaskWoken;
	BOOL blRet = FALSE;
	U16 u16ReadCrc = 0;
	U16 u16CalCrc = 0;
	U16 u16DataType = 0;
	S32 s32UpdateType = 0;

	(void)GetSigVal(MBMS_SAM_SIG_ID_UPDATE_STATE, &s32UpdateType);
	
	if((s32UpdateType != NO_UPGRADE)
	&& (s32UpdateType != UART1_UPGRADE))
	{
		return blRet;
	}

	if((0xAA == g_Usart_1_CommBuf.u8Recvbuf[0])
	&& (0x55 == g_Usart_1_CommBuf.u8Recvbuf[1])
	&& (BOOT_DEVICE_ADDR == g_Usart_1_CommBuf.u8Recvbuf[2])
	&& (0x50 == g_Usart_1_CommBuf.u8Recvbuf[3])
	&& (g_Usart_1_CommBuf.u16RecvDataCnt > 7))
	{
		u16CalCrc = crc16_ccitt_xmodem(&g_Usart_1_CommBuf.u8Recvbuf[3], (g_Usart_1_CommBuf.u16RecvDataCnt-7), 0);
		u16ReadCrc = (U16)g_Usart_1_CommBuf.u8Recvbuf[g_Usart_1_CommBuf.u16RecvDataCnt-1-3] | ((U16)g_Usart_1_CommBuf.u8Recvbuf[g_Usart_1_CommBuf.u16RecvDataCnt-1-2]<<8);

		if(u16CalCrc == u16ReadCrc)
		{
			u16DataType = (U16)g_Usart_1_CommBuf.u8Recvbuf[5] | ((U16)g_Usart_1_CommBuf.u8Recvbuf[6])<<8;

			//判断升级报文是否按照升级流程要求
			if(TRUE != blDetcetUsartUpdataStep((USART_BOOT_STEP_CMD)u16DataType))
			{
				return blRet;
			}

			blRet = TRUE;

			if(FALSE == g_blUsartUpdataFuncEnFg)
			{
				g_blUsartUpdataFuncEnFg = TRUE;
				(void)SetSigVal(MBMS_SAM_SIG_ID_UPDATE_STATE, UART1_UPGRADE);
				xTaskResumeFromISR(UpdataTask_Handler);  //唤醒升级任务
			}

    		if(NULL != g_USART_1_Updata_Binary_Semaphore)
            {
                /* release binary semaphores. */
                (void)xSemaphoreGiveFromISR(g_USART_1_Updata_Binary_Semaphore, &xHigherPriorityTaskWoken);
				/* make a task switch if necessary. */
            	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }				
		}
	}

	return blRet;
}

/// @brief 判断是否需要升级
/// @param  
/// @return 
BOOL blCheck_Usart2_UpdateMsg(void)
{
	BaseType_t xHigherPriorityTaskWoken;
	BOOL blRet = FALSE;
	U16 u16ReadCrc = 0;
	U16 u16CalCrc = 0;
	U16 u16DataType = 0;
	S32 s32UpdateType = 0;

	(void)GetSigVal(MBMS_SAM_SIG_ID_UPDATE_STATE, &s32UpdateType);
	
	if((s32UpdateType != NO_UPGRADE)
	&& (s32UpdateType != UART2_UPGRADE))
	{
		return blRet;
	}

	if((0xAA == g_Usart_2_CommBuf.u8Recvbuf[0])
	&& (0x55 == g_Usart_2_CommBuf.u8Recvbuf[1])
	&& (BOOT_DEVICE_ADDR == g_Usart_2_CommBuf.u8Recvbuf[2])
	&& (0x50 == g_Usart_2_CommBuf.u8Recvbuf[3])
	&& (g_Usart_2_CommBuf.u16RecvDataCnt > 7))
	{
		u16CalCrc = crc16_ccitt_xmodem(&g_Usart_2_CommBuf.u8Recvbuf[3], (g_Usart_2_CommBuf.u16RecvDataCnt-7), 0);
		u16ReadCrc = (U16)g_Usart_2_CommBuf.u8Recvbuf[g_Usart_2_CommBuf.u16RecvDataCnt-1-3] | ((U16)g_Usart_2_CommBuf.u8Recvbuf[g_Usart_2_CommBuf.u16RecvDataCnt-1-2]<<8);

		if(u16CalCrc == u16ReadCrc)
		{
			u16DataType = (U16)g_Usart_2_CommBuf.u8Recvbuf[5] | ((U16)g_Usart_2_CommBuf.u8Recvbuf[6])<<8;

			//判断升级报文是否按照升级流程要求
			if(TRUE != blDetcetUsartUpdataStep((USART_BOOT_STEP_CMD)u16DataType))
			{
				return blRet;
			}

			blRet = TRUE;

			if(FALSE == g_blUsartUpdataFuncEnFg)
			{
				g_blUsartUpdataFuncEnFg = TRUE;
				(void)SetSigVal(MBMS_SAM_SIG_ID_UPDATE_STATE, UART2_UPGRADE);
				xTaskResumeFromISR(UpdataTask_Handler);    //唤醒升级任务
			}

    		if(NULL != g_USART_2_Updata_Binary_Semaphore)
            {
                /* release binary semaphores. */
                (void)xSemaphoreGiveFromISR(g_USART_2_Updata_Binary_Semaphore, &xHigherPriorityTaskWoken);
				/* make a task switch if necessary. */
				portYIELD_FROM_ISR(xHigherPriorityTaskWoken);				
            }			
		}
	}

	return blRet;
}


/******************************************************************************
* 名  	称:  BOOT_ENTER_REPLY
* 功  	能:  进入BOOT应答
* 入口参数:  无
* 出口参数:  无
*******************************************************************************/
static void BOOT_ENTER_REPLY(void)
{
    U16 i = 0;
    U16 j = 0;
    U16 u16Crc = 0;
	U8 u8Sendbuf[USART_COMM_BUF_LEN] = {0};

	//发送回应数据包
	u8Sendbuf[i++] = 0xAA;
	u8Sendbuf[i++] = 0x55;
	u8Sendbuf[i++] = BOOT_DEVICE_ADDR;			//发送本机设备地址
	u8Sendbuf[i++] = 0x50;     			//发送功能码
	u8Sendbuf[i++] = 0x07;				//返回字节个数(寄存器长度为2字节)
	u8Sendbuf[i++] = 0x01;
	u8Sendbuf[i++] = 0x00;

	for(j = 0; j < 5; j++)				//取版本信息前5个byte
	{
		u8Sendbuf[i++] = g_u8BootVersion[j];
	}

	u16Crc = crc16_ccitt_xmodem(&u8Sendbuf[3], (i-3), 0);

	u8Sendbuf[i++] = (U8)u16Crc;
	u8Sendbuf[i++] = (U8)(u16Crc>>8);

	u8Sendbuf[i++] = 0x0D;
	u8Sendbuf[i++] = 0x0A;

	if((GetMsgVal(MBMS_SAM_SIG_ID_UPDATE_STATE) == UART1_UPGRADE))
	{
		usart1_dma_send(u8Sendbuf,i);
	}
	else if((GetMsgVal(MBMS_SAM_SIG_ID_UPDATE_STATE) == UART2_UPGRADE))
	{
		usart2_dma_send(u8Sendbuf,i);
	}
}

/******************************************************************************
* 名  	称:  BOOT_DISENTER_REPLY
* 功  	能:  BMS进入BOOT应答
* 入口参数:  无
* 出口参数:  无
*******************************************************************************/
static void BOOT_DISENTER_REPLY(void)
{
    U16 i = 0;
    U16 j = 0;
    U16 u16Crc = 0;
	U8 u8Sendbuf[USART_COMM_BUF_LEN] = {0};

	//发送回应数据包
	u8Sendbuf[i++] = 0xAA;
	u8Sendbuf[i++] = 0x55;
	u8Sendbuf[i++] = BOOT_DEVICE_ADDR;			//发送本机设备地址
	u8Sendbuf[i++] = 0x50;     			//发送功能码
	u8Sendbuf[i++] = 0x07;				//返回字节个数(寄存器长度为2字节)
	u8Sendbuf[i++] = 0x01;
	u8Sendbuf[i++] = 0x00;

	for(j = 0; j < 5; j++)				//取版本信息前5个byte
	{
		u8Sendbuf[i++] = 0xFF;
	}

	u16Crc = crc16_ccitt_xmodem(&u8Sendbuf[3], (i-3), 0);

	u8Sendbuf[i++] = (U8)u16Crc;
	u8Sendbuf[i++] = (U8)(u16Crc>>8);

	u8Sendbuf[i++] = 0x0D;
	u8Sendbuf[i++] = 0x0A;

	if((GetMsgVal(MBMS_SAM_SIG_ID_UPDATE_STATE) == UART1_UPGRADE))
	{
		usart1_dma_send(u8Sendbuf,i);
	}
	else if((GetMsgVal(MBMS_SAM_SIG_ID_UPDATE_STATE) == UART2_UPGRADE))
	{
		usart2_dma_send(u8Sendbuf,i);
	}	
}

static void BOOT_UPDATE_ACK(BOOL blFg)
{
    U16 i = 0;
    U16 u16Crc = 0;
	U8 u8Sendbuf[USART_COMM_BUF_LEN] = {0};

	//发送回应数据包
	u8Sendbuf[i++] = 0xAA;
	u8Sendbuf[i++] = 0x55;
	u8Sendbuf[i++] = BOOT_DEVICE_ADDR;			//发送本机设备地址
	u8Sendbuf[i++] = 0x50;     			//发送功能码
	u8Sendbuf[i++] = 0x01;				//返回字节个数(寄存器长度为2字节)
	u8Sendbuf[i++] = blFg;

	u16Crc = crc16_ccitt_xmodem(&u8Sendbuf[3], (i-3), 0);

	u8Sendbuf[i++] = (U8)u16Crc;
	u8Sendbuf[i++] = (U8)(u16Crc>>8);

	u8Sendbuf[i++] = 0x0D;
	u8Sendbuf[i++] = 0x0A;

	if((GetMsgVal(MBMS_SAM_SIG_ID_UPDATE_STATE) == UART1_UPGRADE))
	{
		usart1_dma_send(u8Sendbuf,i);
	}
	else if((GetMsgVal(MBMS_SAM_SIG_ID_UPDATE_STATE) == UART2_UPGRADE))
	{
		usart2_dma_send(u8Sendbuf,i);
	}	
}


/// @brief 在备份域寄存器中写入app升级标志位
/// @param  
static void WriteUpdataFg(void)
{
	rtc_lwoff_wait();
    bkp_write_data(RTC_BKP10, (uint16_t)0x55AA);	
}

void UsartUpdataTimeOutFunc(void)
{
	(void)SetSigVal(MBMS_SAM_SIG_ID_UPDATE_STATE, NO_UPGRADE);
	enumUsartPreUpdataStep = USART_UPDATE_UNKNOW_CMD;
	g_blUsartUpdataFuncEnFg = FALSE;

}

/******************************************************************************
* 名  	称:  UsartUpdataFunc
* 功  	能:  升级主程序
* 入口参数:  无
* 出口参数:  无
*******************************************************************************/
static void UsartUpdataFunc(void)
{
	BOOL blRet = FALSE;
	U16 u16DataType = 0;
	U16 u16DataCnt = 0;
	U16 u16DataLen = 0;
	U16 u16MsgNo = 0;
	U8 u8DataBuf[USART_COMM_BUF_LEN] = {0};
	U32 u32GetTotalPackCrc = 0;
	static U32 u32CalTotalPackCrc = 0;
	S32 s32UpdateType = 0;

	(void)GetSigVal(MBMS_SAM_SIG_ID_UPDATE_STATE, &s32UpdateType);
	if(s32UpdateType == UART2_UPGRADE)
	{
		u16DataType = (U16)g_Usart_2_CommBuf.u8Recvbuf[5] | ((U16)g_Usart_2_CommBuf.u8Recvbuf[6])<<8;
	}
	else if(s32UpdateType == UART1_UPGRADE)
	{
		u16DataType = (U16)g_Usart_1_CommBuf.u8Recvbuf[5] | ((U16)g_Usart_1_CommBuf.u8Recvbuf[6])<<8;		
	}

	//进行升级步骤判断，一次升级过程只能依次处理完整的升级流程
	if (u16DataType < enumUsartPreUpdataStep)
	{
		return;
	}
	
	enumUsartPreUpdataStep = (USART_BOOT_STEP_CMD)u16DataType;

	switch(u16DataType)
	{
		case USART_UPDATE_REQUEST_CMD:		//升级请求
			if((s32UpdateType == UART2_UPGRADE)
			&& (g_Usart_2_CommBuf.u8Recvbuf[9] == g_u8BootVersion[0])
			&& (g_Usart_2_CommBuf.u8Recvbuf[10] == g_u8BootVersion[1])
			&& (g_Usart_2_CommBuf.u8Recvbuf[11] == g_u8BootVersion[2])
			&& (g_Usart_2_CommBuf.u8Recvbuf[12] == g_u8BootVersion[3])
			&& (g_Usart_2_CommBuf.u8Recvbuf[13] == g_u8BootVersion[4]))
			{
				BOOT_ENTER_REPLY();		
			}
			else if((s32UpdateType == UART1_UPGRADE)
			&& (g_Usart_1_CommBuf.u8Recvbuf[9] == g_u8BootVersion[0])
			&& (g_Usart_1_CommBuf.u8Recvbuf[10] == g_u8BootVersion[1])
			&& (g_Usart_1_CommBuf.u8Recvbuf[11] == g_u8BootVersion[2])
			&& (g_Usart_1_CommBuf.u8Recvbuf[12] == g_u8BootVersion[3])
			&& (g_Usart_1_CommBuf.u8Recvbuf[13] == g_u8BootVersion[4]))
			{
				BOOT_ENTER_REPLY();					
			}
			else
			{
				BOOT_DISENTER_REPLY();
			}
		break;

		case USART_UPDATE_READY_CMD:			//升级准备

			blRet = FlashClearBackupApp();  //清除app升级包flash区域

			vTaskDelay(100);

			u32CalTotalPackCrc = 0xFFFFFFFF;
		break;

		case USART_UPDATE_BEGIN_CMD:			//起始包
		case USART_UPDATE_MIDDLE_CMD:			//中间包
		case USART_UPDATE_END_CMD:				//结束包
			if(s32UpdateType == UART2_UPGRADE)
			{
				u16DataLen = g_Usart_2_CommBuf.u8Recvbuf[4] - 4;	//更新数据字节数 = 上报的长度 - CRC（2字节） - 帧尾标志（2字节）
				u16MsgNo = (U16)g_Usart_2_CommBuf.u8Recvbuf[7] | ((U16)g_Usart_2_CommBuf.u8Recvbuf[8])<<8;

				for(u16DataCnt = 0; u16DataCnt < u16DataLen; u16DataCnt++)
				{
					/*+9是因为在数据包前段含有9个其他信息：
					帧头标志（2字节）、目标地址（1字节）、命令（1字节）、
					数据长度（1字节）、升级包类型（2字节）、升级包计数器（2字节）*/
					u8DataBuf[u16DataCnt] = g_Usart_2_CommBuf.u8Recvbuf[u16DataCnt+9];
				}
			}
			else if(s32UpdateType == UART1_UPGRADE)
			{
				u16DataLen = g_Usart_1_CommBuf.u8Recvbuf[4] - 4;	//更新数据字节数 = 上报的长度 - CRC（2字节） - 帧尾标志（2字节）
				u16MsgNo = (U16)g_Usart_1_CommBuf.u8Recvbuf[7] | ((U16)g_Usart_1_CommBuf.u8Recvbuf[8])<<8;

				for(u16DataCnt = 0; u16DataCnt < u16DataLen; u16DataCnt++)
				{
					/*+9是因为在数据包前段含有9个其他信息：
					帧头标志（2字节）、目标地址（1字节）、命令（1字节）、
					数据长度（1字节）、升级包类型（2字节）、升级包计数器（2字节）*/
					u8DataBuf[u16DataCnt] = g_Usart_1_CommBuf.u8Recvbuf[u16DataCnt+9];
				}	
			}

			blRet = WriteAppPack2Flash(u8DataBuf, u16DataLen, u16MsgNo-1);

			//计算累计数据包的校验值
			u32CalTotalPackCrc = crc32(u8DataBuf, u16DataLen, u32CalTotalPackCrc);	
		break;

		case USART_UPDATE_CHECKSUM_CMD:		//固件文件校验码包
			if(s32UpdateType == UART2_UPGRADE)
			{
				u32GetTotalPackCrc = (U32)g_Usart_2_CommBuf.u8Recvbuf[9] 
										| ((U32)g_Usart_2_CommBuf.u8Recvbuf[10])<<8
										| ((U32)g_Usart_2_CommBuf.u8Recvbuf[11])<<16
										| ((U32)g_Usart_2_CommBuf.u8Recvbuf[12])<<24;
			}
			else if(s32UpdateType == UART1_UPGRADE)
			{
				u32GetTotalPackCrc = (U32)g_Usart_1_CommBuf.u8Recvbuf[9] 
										| ((U32)g_Usart_1_CommBuf.u8Recvbuf[10])<<8
										| ((U32)g_Usart_1_CommBuf.u8Recvbuf[11])<<16
										| ((U32)g_Usart_1_CommBuf.u8Recvbuf[12])<<24;	
			}

			if((~u32CalTotalPackCrc) == u32GetTotalPackCrc)
			{
				//升级成功后，写入APP完整的标识
				if(TRUE == WriteAppInterFg2Flash())
				{
					if (TRUE == CheckExtFlashAppIntegFg())
					{
						blRet = TRUE;
					}
				}
			}
		break;

		default:
		break;
	}

	if(USART_UPDATE_REQUEST_CMD != (USART_BOOT_STEP_CMD)u16DataType)
	{
		BOOT_UPDATE_ACK(blRet);

		if((USART_UPDATE_CHECKSUM_CMD == (USART_BOOT_STEP_CMD)u16DataType)  //校验通过则进入升级完成流程
			&& (blRet == TRUE))
		{	
			
			vTaskDelay(500);

			WriteUpdataFg();
			
			SystemResetFunc();//重启
		}
	}
}

static void SuspendUpdateTaskFunc(void)
{
	if((UpdataTask_Handler != NULL)
        && (eSuspended != eTaskGetState(UpdataTask_Handler)))
	{
		vTaskSuspend(UpdataTask_Handler);
	}
}



static void UpdataFunc(void)
{
	S32 s32UpdateType = 0;
    BaseType_t err = pdFALSE;	

	(void)GetSigVal(MBMS_SAM_SIG_ID_UPDATE_STATE, &s32UpdateType);

	if((s32UpdateType == UART2_UPGRADE)
	&& NULL != g_USART_2_Updata_Binary_Semaphore)
    {
        err = xSemaphoreTake(g_USART_2_Updata_Binary_Semaphore, COMM_TIME_OUT_VAL);    

        if(pdTRUE == err)
        {
			UsartUpdataFunc();
        }
		else
		{
			UsartUpdataTimeOutFunc();
			SuspendUpdateTaskFunc();
		}
    }
	else if((s32UpdateType == UART1_UPGRADE)
		&& NULL != g_USART_1_Updata_Binary_Semaphore)
    {
        err = xSemaphoreTake(g_USART_1_Updata_Binary_Semaphore, COMM_TIME_OUT_VAL);    

        if(pdTRUE == err)
        {
			UsartUpdataFunc();
        }
		else
		{
			UsartUpdataTimeOutFunc();
			SuspendUpdateTaskFunc();
		}
    }	
    else if(pdFALSE == err)
    {
		vTaskDelay(300);      
    }   	
}

/// @brief 升级任务,默认开机被挂起，升级信号触发后启动
/// @param pvParameters 
static void UpdataTask(void *pvParameters)
{
	volatile UBaseType_t uxHighWaterMark;

	while(1)
	{
		DisableAllGun(); //关闭所有DO输出

		UpdataFunc();

		uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
	}
}



void UpdataFuncInitTask(void *pvParameters)
{
	volatile UBaseType_t uxHighWaterMark;

    g_USART_2_Updata_Binary_Semaphore = xSemaphoreCreateBinary(); 
    g_USART_1_Updata_Binary_Semaphore = xSemaphoreCreateBinary();   	
	
    vTaskDelay(300);        //延时用于等待硬件稳定

	taskENTER_CRITICAL();

	xTaskCreate(UpdataTask,         "NORMAL_UPDATA",        configMINIMAL_STACK_SIZE*7, NULL,   tskIDLE_PRIORITY + 5,	&UpdataTask_Handler);
	vTaskSuspend(UpdataTask_Handler);

	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
	
	taskEXIT_CRITICAL();

	vTaskDelete(NULL);
	
	
}


