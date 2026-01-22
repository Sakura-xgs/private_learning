/*
 * boot.c
 *
 *  Created on: 2024年8月21日
 *      Author: Bono
 */

#include "boot.h"
#include "SignalManage.h"
#include "hal_sys_IF.h"
#include "hal_can_IF.h"
#include "can_fifo.h"
#include "pdu_can.h"
#include "fsl_cache.h"
#include "relay_ctrl.h"

SemaphoreHandle_t uartUpgradeBinarySemaphore = NULL;
TaskHandle_t NormalUpdataTask_Handler = NULL;

BOOT_STEP_CMD enumPreUpdataStep = UPDATE_UNKNOW_CMD;

extern SOFT_VER cst_sw_no;
extern HARD_VER cst_hw_no;

__RODATA(APPINTEG) volatile const U8 g_app_integ_tab[16] =
{'A','P','P','I','N','T','E','G',' ',' ',' ',' ',' ',' ',' ',' '};

__RODATA(BOOT_VERSION) volatile const U8 g_boot_version[16] =
{'P','D','U','0','1','_','V','1',' ',' ',' ',' ',' ',' ',' ',' '};

void Boot_Init(void)
{
	S32 s32SigVal = 0;

	s32SigVal = (cst_sw_no.sw_major_version[0]<<24) | (cst_sw_no.sw_major_version[1]<<16) | (cst_sw_no.sw_major_version[2]<<8) | (cst_sw_no.sw_major_version[3]);
	(void)SetSigVal(PDU_SAM_SIG_ID_SW_VERSION_1_2_3_4, s32SigVal);
	s32SigVal = (cst_sw_no.sw_major_version[4]<<24) | (cst_sw_no.sw_major_version[5]<<16) | (cst_sw_no.sw_major_version[6]<<8) | (cst_sw_no.sw_major_version[7]);
	(void)SetSigVal(PDU_SAM_SIG_ID_SW_VERSION_5_6_7_8, s32SigVal);
	s32SigVal = (cst_sw_no.sw_major_version[8]<<24) | (cst_sw_no.sw_major_version[9]<<16) | (cst_sw_no.sw_major_version[10]<<8) | (cst_sw_no.sw_major_version[11]);
	(void)SetSigVal(PDU_SAM_SIG_ID_SW_VERSION_9_10_11_12, s32SigVal);
	s32SigVal = (cst_sw_no.sw_major_version[12]<<24) | (cst_sw_no.sw_minor_version[0]<<16) | (cst_sw_no.sw_minor_version[1]<<8) | (cst_sw_no.sw_minor_version[2]);
	(void)SetSigVal(PDU_SAM_SIG_ID_SW_VERSION_13_14_15_16, s32SigVal);

	s32SigVal = (cst_hw_no.hw_version[0]<<24) | (cst_hw_no.hw_version[1]<<16) | (cst_hw_no.hw_version[2]<<8) | (cst_hw_no.hw_version[3]);
	(void)SetSigVal(PDU_SAM_SIG_ID_HW_VERSION_1_2_3_4, s32SigVal);
	s32SigVal = (cst_hw_no.hw_version[4]<<24) | (cst_hw_no.hw_version[5]<<16) | (cst_hw_no.hw_version[6]<<8) | (cst_hw_no.hw_version[7]);
	(void)SetSigVal(PDU_SAM_SIG_ID_HW_VERSION_5_6_7_8, s32SigVal);
	s32SigVal = (cst_hw_no.hw_version[8]<<24) | (cst_hw_no.hw_version[9]<<16) | (cst_hw_no.hw_version[10]<<8) | (cst_hw_no.hw_version[11]);
	(void)SetSigVal(PDU_SAM_SIG_ID_HW_VERSION_9_10_11_12, s32SigVal);
	s32SigVal = (cst_hw_no.hw_version[12]<<24) | (cst_hw_no.hw_version[13]<<16) | (cst_hw_no.hw_version[14]<<8) | (cst_hw_no.hw_version[15]);
	(void)SetSigVal(PDU_SAM_SIG_ID_HW_VERSION_13_14_15_16, s32SigVal);
	assert(g_app_integ_tab[0] != 0);
	assert(g_boot_version[0] != 0);
	assert(cst_sw_no.sw_major_version[0] != 0);
	SetSysNormalModeFg();
}

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

U16 crc16_ccitt_xmodem(const U8* data, U32 len, U16 u16CrcInitVal)
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

unsigned int crc32(U8 *data, U32 Len, U32 uCurCRC)
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

void Board_Addr_Init(void)
{
	S32 addr = 0, hw_ver = 0;
	U8 addr1 = 0, addr2 = 0, addr3 = 0, addr4 = 0;
	U8 ver1 = 0, ver2 = 0;

	ver1 = GET_ADDR1_STATUS();
	ver1 = GET_ADDR2_STATUS();
	addr1 = GET_ADDR1_STATUS();
	addr2 = GET_ADDR2_STATUS();
	addr3 = GET_ADDR3_STATUS();
	addr4 = GET_ADDR4_STATUS();

	hw_ver |= (ver1 << 0);
	hw_ver |= (ver2 << 1);
	addr |= (addr1 << 0);
	addr |= (addr2 << 1);
	addr |= (addr3 << 2);
	addr |= (addr4 << 3);

	(void)SetSigVal(PDU_SAM_SIG_ID_HW_VERSION_PIN, hw_ver);
	(void)SetSigVal(PDU_SAM_SIG_ID_BOARD_ID, addr);
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

BOOL blDetcetUpdataStep(BOOT_STEP_CMD RecStep)
{
    BOOL blRet = TRUE;

    switch (enumPreUpdataStep)
    {
        case UPDATE_UNKNOW_CMD:
            if (UPDATE_REQUEST_CMD != RecStep)
            {
                blRet = FALSE;
            }
            break;

        case UPDATE_REQUEST_CMD:
            if (UPDATE_READY_CMD != RecStep)
            {
                blRet = FALSE;
            }
            break;

        case UPDATE_READY_CMD:
            if (UPDATE_MIDDLE_CMD != RecStep)
            {
                blRet = FALSE;
            }
            break;

        case UPDATE_MIDDLE_CMD:
            if ((UPDATE_MIDDLE_CMD != RecStep)
                && (UPDATE_CHECKSUM_CMD != RecStep))
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

BOOL UpdataFlashFunc(const P_U8 pu8Data, const U16 u16DataLen, const U16 u16MsgNo)
{
	BOOL blRet = FALSE;
	U32 u32FlashBeginAddr = 0;
	U8 u8FlashWrBuf[MSG_UPDATE_MAX_DATALEN] = {0};
	U8 u8FlashRdBuf[MSG_UPDATE_MAX_DATALEN] = {0};
	U16 u16DataCnt = 0;
    U32 u32WrErrCnt = 0;

	u32FlashBeginAddr = NEW_APP_FLASH_ADDR_START + MSG_UPDATE_MAX_DATALEN*u16MsgNo;	//起始操作地址

	//转存升级数据
	for (u16DataCnt = 0; u16DataCnt < MSG_UPDATE_MAX_DATALEN; u16DataCnt++)
	{
		u8FlashWrBuf[u16DataCnt] = pu8Data[u16DataCnt];
	}

	//升级过程中，不写入APP完整标志
	if(u32FlashBeginAddr == NEW_APP_INTEG_FLASH_ADDR_START)
	{
		return TRUE;
	}

	flexspi_flash_buffer_write_a_page(u8FlashWrBuf, u32FlashBeginAddr);

	flexspi_flash_buffer_read(u8FlashRdBuf, u32FlashBeginAddr, MSG_UPDATE_MAX_DATALEN);

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

BOOL WriteAppInterFg2Flash(U32 u32FlashAddr, U32 u32FileTotalBytes)
{
	U8 u8FlashBuf[FLASH_PAGE_SIZE] = {0};
	BOOL blRet = FALSE;
	U8 i = 0;

	memset(u8FlashBuf, 0xFF, FLASH_PAGE_SIZE);
	u8FlashBuf[i++] = 'A';
	u8FlashBuf[i++] = 'P';
	u8FlashBuf[i++] = 'P';
	u8FlashBuf[i++] = 'I';
	u8FlashBuf[i++] = 'N';
	u8FlashBuf[i++] = 'T';
	u8FlashBuf[i++] = 'E';
	u8FlashBuf[i++] = 'G';
	u8FlashBuf[i++] = (u32FileTotalBytes>>24)&0xFF;
	u8FlashBuf[i++] = (u32FileTotalBytes>>16)&0xFF;
	u8FlashBuf[i++] = (u32FileTotalBytes>>8)&0xFF;
	u8FlashBuf[i++] = (u32FileTotalBytes)&0xFF;
	flexspi_flash_buffer_write_a_page(u8FlashBuf, u32FlashAddr);

	return TRUE;
}

BOOL CheckExtFlashAppIntegFg(void)
{
	U8 i = 0;
	U8 u8FlashBuf[16] = {0};
	U8 app_integ_tab[APP_INTEG_SIZE] = {'A','P','P','I','N','T','E','G'};

	DCACHE_InvalidateByRange(str_APP_INTEG_FLAG_ADDR, sizeof(u8FlashBuf));

	flexspi_flash_buffer_read(u8FlashBuf, str_APP_INTEG_FLAG_ADDR, sizeof(u8FlashBuf));

	for(i=0;i<APP_INTEG_SIZE;i++)
	{
		if(app_integ_tab[i]!=u8FlashBuf[i])
		{
			return FALSE;
		}
	}

	return TRUE;
}

void UpdataAckFunc(U8 u8MsgTargetAddr, BOOT_STEP_CMD enumStep, UPDATA_RET enumRet, ACK_ERR_TYPE emunAckType, U32 u32TotalCrc)
{
	flexcan_frame_t Frame = {0};
    S32 s32BoardId = 0;

	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32BoardId);

	Frame.dataByte0 = (U8)enumRet;
	Frame.dataByte1 = GET_HIGH_4_BYTE(u32TotalCrc);
	Frame.dataByte2 = GET_MIDDLE_HIGH_4_BYTE(u32TotalCrc);
	Frame.dataByte3 = GET_MIDDLE_LOW_4_BYTE(u32TotalCrc);
	Frame.dataByte4 = GET_LOW_4_BYTE(u32TotalCrc);

	Frame.dataByte7 = (U8)emunAckType;

	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT(((U32)(enumStep)<<16) | ((U32)(u8MsgTargetAddr)<<8) | ((U32)s32BoardId));
	Frame.length = 8;

	if((s32BoardId > 0) && (s32BoardId < 255))
	{
		vTaskDelay(s32BoardId*RESPON_DELAY_TIME);	/* PDU错位回复，解决升级主机可能接收丢帧的问题 */
	}
	PduCanSendFrame(&Frame);
}

void UpdataEnterReplyFunc(U8 u8MsgTargetAddr)
{
	flexcan_frame_t Frame = {0};
    U8 i = 0;
    S32 s32BoardId = 0;

	//获取板号
	(void)GetSigVal(PDU_SAM_SIG_ID_BOARD_ID, &s32BoardId);

	Frame.dataByte0 = 0xAA;

	Frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;
	Frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
	Frame.id = FLEXCAN_ID_EXT(((U32)(UPDATE_REQUEST_CMD)<<16) | ((U32)(u8MsgTargetAddr)<<8) | ((U32)s32BoardId));
	Frame.length = 8;

	if((s32BoardId > 0) && (s32BoardId < 255))
	{
		vTaskDelay(s32BoardId*RESPON_DELAY_TIME);	/* PDU错位回复，解决升级主机可能接收丢帧的问题 */
	}
	PduCanSendFrame(&Frame);
}

void CanUpdateFailed(void)
{
	enumPreUpdataStep = UPDATE_UNKNOW_CMD;
	g_blUpdataFuncEnFg = FALSE;
//	hal_can_filter_init(UPGRADE_CAN);
	ResumePduCanSendTaskFunc();
	(void)SetSigVal(SIGNAL_STATUS_UPDATA, FALSE);
	(void)SetSigVal(PDU_SAM_SIG_ID_UPDATE_STATE, NO_UPGRADE);

	SuspendNormalUpdateTaskFunc();
}

/******************************************************************************
* 名  	称:  UpdataFunc
* 功  	能:  升级主程序
* 入口参数:  无
* 出口参数:  无
*******************************************************************************/
void UpdataFunc(void)
{
	UPDATA_RET enumRet = UPDATA_SUCCESS;
	ACK_ERR_TYPE enumAckType = ACK_OK;
	static U16 u16CalSinglePackCrc = 0;
	static U32 u32CalTotalPackCrc = 0xFFFFFFFF;
	static U32 u32CalTotalBytes = 0;
	strCAN_REC_BUF *g_CanRecBuf;
	S32 s32BorNo = 0;
	S32 s32BmsNum = 0;
	S32 s32CanUpdateType = 0;

	(void)GetSigVal(PDU_SAM_SIG_ID_UPDATE_STATE, &s32CanUpdateType);

	if(s32CanUpdateType == PDU_CAN1_UPGRADE)
	{
		g_CanRecBuf = &g_Can1RecBuf;
	}
	else
	{
		return;
	}

	//进行升级步骤判断，一次升级过程只能依次处理完整的升级流程
	if(g_CanRecBuf->enumUpdataStep < enumPreUpdataStep)
	{
		return;
	}

	AllLedToggle();

	//记录当前的升级步骤
	enumPreUpdataStep = g_CanRecBuf->enumUpdataStep;

	switch(g_CanRecBuf->enumUpdataStep)
	{
		case UPDATE_REQUEST_CMD:		//升级请求
//			hal_can_updata_filter_init(UPGRADE_CAN);
			SuspendPduCanSendTaskFunc();
			UpdataEnterReplyFunc(g_CanRecBuf->u8MsgSourceAddr);

			u32CalTotalPackCrc = 0xFFFFFFFF;
			u32CalTotalBytes = 0;
		break;

		case UPDATE_READY_CMD:			//升级准备
			if(TRUE != ExtFlashClrBackupApp())
			{
				enumRet = UPDATA_FAIL;
				enumAckType = ACK_ERR_FLASH_NOT_REARY;
				break;
			}

			if(IsAllRelayCutOff() == FALSE)
			{
				enumRet = UPDATA_FAIL;
				enumAckType = ACK_ERR_RELAY_CLOSED;
				break;
			}

			vTaskDelay(100);

			u32CalTotalPackCrc = 0xFFFFFFFF;
			u32CalTotalBytes = 0;
		break;

		case UPDATE_MIDDLE_CMD:			//传输数据包
			//计算单个数据包的校验值
			u16CalSinglePackCrc = crc16_ccitt_xmodem(g_CanRecBuf->u8RxData, g_CanRecBuf->u16PackDataNum, 0);
			u32CalTotalBytes += g_CanRecBuf->u16PackDataNum;

			if((g_CanRecBuf->u16PackCrc == u16CalSinglePackCrc) && ((g_CanRecBuf->u16PackDataNum == MSG_UPDATE_MAX_DATALEN) || (g_CanRecBuf->u16PackNum == g_u32TotalPackCount)))	//crc校验通过，数据包长度校验通过，最后一个数据包不参与长度校验
			{
				if(TRUE != UpdataFlashFunc(g_CanRecBuf->u8RxData, g_CanRecBuf->u16PackDataNum, g_CanRecBuf->u16PackNum))
				{
					enumRet = UPDATA_FAIL;
					enumAckType = ACK_ERR_FLASH_WRITE_ERR;
				}

				//计算累计数据包的校验值
				u32CalTotalPackCrc = crc32(g_CanRecBuf->u8RxData, g_CanRecBuf->u16PackDataNum, u32CalTotalPackCrc);
			}
			else
			{
				enumRet = UPDATA_FAIL;
				enumAckType = ACK_ERR_SINGLE_PACK_CRC_ERR;
			}
		break;

		case UPDATE_CHECKSUM_CMD:		//固件文件校验码包
			if((~u32CalTotalPackCrc) == g_CanRecBuf->u32FileCrc)
			{
				if(g_CanRecBuf->u32FileByteNums > u32CalTotalBytes
				&& g_CanRecBuf->u32FileByteNums <= u32CalTotalBytes + FLASH_SECTOR_SIZE)
				{
					u32CalTotalBytes = g_CanRecBuf->u32FileByteNums;
				}
				//升级成功后，写入APP完整的标识
				if(TRUE == WriteAppInterFg2Flash(str_APP_INTEG_FLAG_ADDR, u32CalTotalBytes))
				{
					if (TRUE != CheckExtFlashAppIntegFg())
					{
						enumRet = UPDATA_FAIL;
						enumAckType = ACK_ERR_NO_APPINTEG;
					}
				}
				else
				{
					enumRet = UPDATA_FAIL;
					enumAckType = ACK_ERR_WRITE_APPINTEG_ERR;
				}
			}
			else
			{
				enumRet = UPDATA_FAIL;
				enumAckType = ACK_ERR_TOTAL_PACK_CRC_ERR;
			}
		break;

		default:
			enumRet = UPDATA_FAIL;
			enumAckType = ACK_ERR_UNKNOW_ERR;
		break;
	}

	if(UPDATE_REQUEST_CMD != g_CanRecBuf->enumUpdataStep)
	{
		//每个应答均上报CRC32
		UpdataAckFunc(g_CanRecBuf->u8MsgSourceAddr, g_CanRecBuf->enumUpdataStep, enumRet, enumAckType, u32CalTotalPackCrc);

		if((UPDATE_CHECKSUM_CMD == g_CanRecBuf->enumUpdataStep)
		&& (UPDATA_SUCCESS == enumRet)
		&& (s32CanUpdateType == PDU_CAN1_UPGRADE))
		{
			vTaskDelay(200);

			SystemResetFunc();//重启

			CanUpdateFailed();
		}
	}
}

/*!
    \brief      Normal Updata task
    \param[in]  pvParameters not used
    \param[out] none
    \retval     none
*/
void Updata_Function(void)
{
	S32 s32UpdateType = 0;
    BaseType_t err = pdFALSE;

    (void)SetSigVal(SIGNAL_STATUS_UPDATA, TRUE);
	(void)GetSigVal(PDU_SAM_SIG_ID_UPDATE_STATE, &s32UpdateType);

	if((s32UpdateType == PDU_CAN1_UPGRADE)
	&& (NULL != g_CanUpdata_Binary_Semaphore))
    {
        err = xSemaphoreTake(g_CanUpdata_Binary_Semaphore, COMM_TIME_OUT_VAL);

        if(pdTRUE == err)
        {
			UpdataFunc();
        }
		else
		{
			CanUpdateFailed();
		}
    }
    else if(pdFALSE == err)
    {
		vTaskDelay(300);
    }
}

void Normal_Updata_Task(void * pvParameters)
{
	BOOL blRelayStatus = FALSE;
	U8 i = 0;
    for( ;; )
    {
		if(SYS_STATUS_NORMAL != GetSysRunModeFg())
		{
			vTaskDelete(NULL);
		}

    	blRelayStatus = FALSE;
    	for(i = 0; i < MAX_RELAY_NUM; i++)
    	{
    		if(TRUE == GetMsgVal(SIGNAL_STATUS_K1 + i))
    		{
    			blRelayStatus = TRUE;
    		}
    	}

    	if(blRelayStatus == FALSE)
    	{
    		Updata_Function();
    	}
    	else
    	{
    		CanUpdateFailed();
    	}
    }
}

void SuspendNormalUpdateTaskFunc(void)
{
	if((NormalUpdataTask_Handler != NULL)
    && (eSuspended != eTaskGetState(NormalUpdataTask_Handler)))
	{
		vTaskSuspend(NormalUpdataTask_Handler);
	}
}

void ResumeNormalUpdataTaskFunc(void)
{
	if((NormalUpdataTask_Handler != NULL)
	&& (eSuspended == eTaskGetState(NormalUpdataTask_Handler)))
	{
		vTaskResume(NormalUpdataTask_Handler);
	}
}

void UpdataFuncInitTask(void * pvParameters)
{
	volatile UBaseType_t uxHighWaterMark;

	vTaskDelay(300);

	taskENTER_CRITICAL();

	xTaskCreate(Normal_Updata_Task, "NORMAL_UPDATA", configMINIMAL_STACK_SIZE*7, NULL, NORMAL_UPDATA_TASK_PRIO,	&NormalUpdataTask_Handler);
	SuspendNormalUpdateTaskFunc();

	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

	vTaskDelete(NULL);

	taskEXIT_CRITICAL();
}
