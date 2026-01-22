#include "modbus.h"
#include <string.h>


static uint16_t Cal_CRC16(uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++) 
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) 
        {
            if (crc & 0x0001) 
            {
                crc >>= 1;
                crc ^= 0xA001;
            } 
            else {
                crc >>= 1;
            }
        }
    }
    return crc;
}


static void Modbus_SendResponse(Modbus_t *modbus)
{
    modbus->sendBuf.u16DataCnt = 0; //清空接收缓冲区

    modbus->sendBuf.u8buf[modbus->sendBuf.u16DataCnt++] = modbus->frame.addr;
    modbus->sendBuf.u8buf[modbus->sendBuf.u16DataCnt++] = modbus->frame.fun_id;

    switch (modbus->frame.fun_id)
    {
    case 0x03: //读保持寄存器
        modbus->sendBuf.u8buf[modbus->sendBuf.u16DataCnt++] = modbus->frame.reg_num * 2;
        for (uint8_t i = 0; i < modbus->frame.reg_num; i++) {
            modbus->sendBuf.u8buf[modbus->sendBuf.u16DataCnt++] = (modbus->regs[modbus->frame.start_addr + i] >> 8) & 0xff; //高字节
            modbus->sendBuf.u8buf[modbus->sendBuf.u16DataCnt++] = modbus->regs[modbus->frame.start_addr + i] & 0xff; //低字节
        }
        break;
    case 0x06: //写单个寄存器
        modbus->sendBuf.u8buf[modbus->sendBuf.u16DataCnt++] = (modbus->frame.start_addr>>8) & 0xff;
        modbus->sendBuf.u8buf[modbus->sendBuf.u16DataCnt++] = modbus->frame.start_addr & 0xff;
        modbus->sendBuf.u8buf[modbus->sendBuf.u16DataCnt++] = (modbus->regs[modbus->frame.start_addr]>>8) & 0xff;
        modbus->sendBuf.u8buf[modbus->sendBuf.u16DataCnt++] = modbus->regs[modbus->frame.start_addr] & 0xff;        
        break;    
    case 0x10: //写多个寄存器
        modbus->sendBuf.u8buf[modbus->sendBuf.u16DataCnt++] = (modbus->frame.start_addr>>8) & 0xff;
        modbus->sendBuf.u8buf[modbus->sendBuf.u16DataCnt++] = modbus->frame.start_addr & 0xff;
        modbus->sendBuf.u8buf[modbus->sendBuf.u16DataCnt++] = (modbus->frame.reg_num >> 8) & 0xff;
        modbus->sendBuf.u8buf[modbus->sendBuf.u16DataCnt++] = modbus->frame.reg_num & 0xff;
        break;

    default:
        break;
    }

    uint16_t crc = Cal_CRC16(modbus->sendBuf.u8buf, modbus->sendBuf.u16DataCnt);
    modbus->sendBuf.u8buf[modbus->sendBuf.u16DataCnt++] = crc & 0xff;
    modbus->sendBuf.u8buf[modbus->sendBuf.u16DataCnt++] = (crc >> 8) & 0xff;

    modbus->sendData(modbus->sendBuf.u8buf, modbus->sendBuf.u16DataCnt);
}

BOOL Modbus_ParaseFrame(uint8_t *recvBuf, uint16_t recvLen, Modbus_t *modbus)
{
    if(recvLen < 4) return FALSE;   //小于最小帧长度

    //判断设备地址
    modbus->frame.addr = recvBuf[0];
    if(modbus->frame.addr != modbus->self_addr) {
        return FALSE;
    }

    //crc校验
    uint16_t crc = Cal_CRC16(recvBuf, recvLen - 2);
    if(((crc & 0xff) != recvBuf[recvLen - 2]) || ((crc >> 8) != recvBuf[recvLen - 1])){
        return FALSE;
    }

    modbus->frame.fun_id = recvBuf[1];
    modbus->frame.start_addr = (recvBuf[2] << 8) | recvBuf[3];

    switch(modbus->frame.fun_id)
    {
        case 0x03: //读保持寄存器
            modbus->frame.reg_num = (recvBuf[4] << 8) | recvBuf[5];
            if(modbus->frame.reg_num > modbus->reg_num) {
                return FALSE;  //请求的寄存器数量超过最大数量
            }
            break;
        case 0x06: //写单个寄存器
            modbus->regs[modbus->frame.start_addr] = (recvBuf[4] << 8) | recvBuf[5];
            break;
        case 0x10: //写多个寄存器
            modbus->frame.reg_num = (recvBuf[4] << 8) | recvBuf[5];
            if(modbus->frame.reg_num > modbus->reg_num) {
                return FALSE;  //请求的寄存器数量超过最大数量
            }
            modbus->frame.data_len = recvBuf[6];
            for(uint8_t i = 0; i < modbus->frame.reg_num; i++) {
                modbus->regs[modbus->frame.start_addr + i] = (recvBuf[7 + i * 2] << 8) | recvBuf[8 + i * 2];
            }
            // memcpy(&modbus->regs[modbus->frame.start_addr], &recvBuf[7], modbus->frame.data_len);
            break;
        default:
            return FALSE;
    }

    Modbus_SendResponse(modbus);  //发送响应

    return TRUE;
}



