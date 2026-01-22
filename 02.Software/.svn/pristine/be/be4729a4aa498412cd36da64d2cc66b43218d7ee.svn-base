#ifndef __MODBUS_H_
#define __MODBUS_H_

#include "PublicDefine.h"

#define MODBUS_DEFAULT_ADDR  0x01  //默认从机地址

#define MODBUS_SEND_BUF_LEN    128

typedef void (*Modbus_sendData_funcptr)(uint8_t *data, uint16_t len); //modbus发送数据函数指针

//modbus发送缓冲区结构体
typedef struct{
    uint8_t u8buf[MODBUS_SEND_BUF_LEN];
    uint16_t u16DataCnt;
}Modbus_SendBuf_t;

//modbus协议帧结构体
typedef struct{
    uint8_t  addr;
    uint8_t fun_id;
    uint16_t start_addr;
    uint16_t reg_num;
    uint16_t data_len;
    uint16_t crc;
}ModbusFrame_t;


//modbus协议操作结构体
typedef struct{
    const uint8_t self_addr;  //从机设备地址，初始化时赋值指定
    ModbusFrame_t frame;
    Modbus_SendBuf_t sendBuf;
    Modbus_sendData_funcptr sendData;    //数据发送函数接口，初始化赋值
    uint16_t reg_num;  //寄存器数量，初始化时赋值指定
    uint16_t* regs;  //模拟寄存器地址，初始化时赋值指定
}Modbus_t;

#define MODBUS_EXPORT(x, xSelfAddr, xSendDataFuncPtr, xRegsPtr, xRegNum)  \
Modbus_t x = {                                                             \
    .self_addr = xSelfAddr,                                               \
    .sendData = xSendDataFuncPtr,                                         \
    .regs = xRegsPtr,                                                     \
    .reg_num = xRegNum,                                                   \
};

BOOL Modbus_ParaseFrame(uint8_t *recvBuf, uint16_t recvLen, Modbus_t *modbus);

#endif

