/*
 * eth.c
 *
 *  Created on: 2024年11月2日
 *      Author: Bono
 */

#include "eth.h"
#include "FreeRTOS.h"
//#include "task.h"
#include "lwip/opt.h"
#include "lwip/tcp.h"
#include "lwip/sys.h"
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/errno.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/ip_addr.h"
#include "lwip/dns.h"
#include "lwip/sockets.h"
#include "lwip/priv/sockets_priv.h"
#include "lwip/igmp.h"
#include "lwip/inet.h"
#include "lwip/tcp.h"
#include "lwip/raw.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/priv/tcpip_priv.h"
#include "lwip/mld6.h"

#include "stdio.h"
#include <string.h> /* memset */
#include <stdlib.h> /* atoi */
#include "tcp_client.h"
#include "TcpBoot.h"
#include "boot.h"
#include "SignalManage.h"
#include "emergency_fault_IF.h"
#include "secc_logandupgrade.h"
#include "uart_comm.h"

//static __BSS(SRAM_OC) recev_packet name_recv  = {0};
static __BSS(SRAM_OC) RecBufferTypeDef ETH_TcpRecBuffer;
static U8 Tcp_ClientFirstCreat = 0;
TaskHandle_t tcp_client_Task_Handler = NULL;
static SemaphoreHandle_t ETH_Rec_Binary_Semaphore = NULL;

//static err_t user_eth_recv(int fd, void *data, int len)
//{
//    char *c;
//    int i;
//    int done;
//
//    done = 0;
//    c = (char *)data;
//
//     a telnet communication packet is ended with an enter key,
//       in socket, here is to check whether the last packet is complete
//    for(i = 0; i < len && !done; i++)
//    {
//        //done = ((c[i] == '\r') || (c[i] == '\n'));
//    	done = 1;
//    }
//
//     when the packet length received is no larger than MAX_NAME_SIZE
//    if(0 == name_recv.done)
//    {
//         havn't received the end of the packet, so the received data length
//           is the configured socket reception limit--MAX_NAME_SIZE
//        if(0 == done)
//        {
//            memcpy(name_recv.bytes, data, MAX_NAME_SIZE);
//            name_recv.length = MAX_NAME_SIZE;
//            name_recv.done = 1;
//             have received the end of the packet
//        }
//        else
//        {
//            memcpy(name_recv.bytes, data, len);
//            name_recv.length = len;
//        }
//    }
//
//    if(1 == done)
//    {
//        if((c[len - 2] != '\r') || (c[len - 1] != '\n'))
//        {
//             limit the received data length to MAX_NAME_SIZE - 2('\r' and '\n' will be put into the buffer)
//            if(((c[len - 1] == '\r') || (c[len - 1] == '\n')) && ((len + 1) <= MAX_NAME_SIZE))
//            {
//                 calculate the buffer size to be sent(including '\r' and '\n')
//                name_recv.length += 1;
//            }
//            else if((len + 2) <= MAX_NAME_SIZE)
//            {
//                name_recv.length += 2;
//            }
//            else
//            {
//                name_recv.length = MAX_NAME_SIZE;
//            }
//
//             save the received data to name_recv.bytes
//            name_recv.bytes[name_recv.length - 2] = '\r';
//            name_recv.bytes[name_recv.length - 1] = '\n';
//        }
//
//        char u8TempBuf[] = "ETH: msg received.";
//
//        send(fd, u8TempBuf, sizeof(u8TempBuf), 0);
//        send(fd, name_recv.bytes, name_recv.length, 0);
//
//        name_recv.done = 0;
//        name_recv.length = 0;
//    }
//
//    return ERR_OK;
//}

#define IP_S_ADDR0   172
#define IP_S_ADDR1   30
#define IP_S_ADDR2   1
#define IP_S_ADDR3   1

#define SERVER_PORT  8000

#define MAX_BUF_SIZE 50

void tcp_client_task(void *arg)
{
	static __BSS(SRAM_OC) U8 ucTcp_rec_buf[MAX_NAME_SIZE];

	int  ret, recvnum;
    static int self_port = 8000;
    struct sockaddr_in svr_addr, clt_addr;
    ip_addr_t ipaddr;

    Tcp_ClientFirstCreat = 1;

    IP4_ADDR(&ipaddr, IP_S_ADDR0, IP_S_ADDR1, IP_S_ADDR2, IP_S_ADDR3);

    while(1)
    {
        svr_addr.sin_family = AF_INET;
        svr_addr.sin_port = htons(SERVER_PORT);
        svr_addr.sin_addr.s_addr = ipaddr.addr;

        /* create the socket */
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0)
        {
        	my_printf(USER_ERROR, "%s:%d create the socket failed\n", __FILE__, __LINE__);
            PileAlarmSet(ALARM_ID_PCU_COMM_LOST_ERROR);
        	vTaskDelay(1000);
            continue;
        }
        if(self_port < 9000)
        {
        	self_port++;
        }
        else
        {
        	self_port=8000;
        }

        clt_addr.sin_family = AF_INET;
        clt_addr.sin_port = htons(self_port);
        clt_addr.sin_addr.s_addr = htons(INADDR_ANY);
        ret = bind(sockfd, (struct sockaddr *)&clt_addr, sizeof(clt_addr));

        if(ret < 0)
        {
        	TcpClose();
            my_printf(USER_ERROR, "%s:%d bind the socket failed\n", __FILE__, __LINE__);
            PileAlarmSet(ALARM_ID_PCU_COMM_LOST_ERROR);
            vTaskDelay(1000);
            continue;
        }

        /* connect */
        ret = connect(sockfd, (struct sockaddr *)&svr_addr, sizeof(svr_addr));
        if(ret < 0)
        {
        	TcpClose();
            my_printf(USER_ERROR, "%s:%d client connect failed:%d.%d.%d.%d\n", __FILE__, __LINE__, IP_S_ADDR0, IP_S_ADDR1, IP_S_ADDR2, IP_S_ADDR3);
            PileAlarmSet(ALARM_ID_PCU_COMM_LOST_ERROR);
            vTaskDelay(2000);
            continue;
        }

        //连接成功后置位登录标志位，立即登录
        if (-1 != sockfd)
        {
        	g_sMsg_control.sLogin_control.bLogin_timeup_flag = TRUE;
            my_printf(USER_INFO, "connect server success:%d.%d.%d.%d\n", IP_S_ADDR0, IP_S_ADDR1, IP_S_ADDR2, IP_S_ADDR3);
        }

        while(-1 != sockfd)
        {
            /* reveive packets, and limit a reception to MAX_BUF_SIZE bytes */
            recvnum = recv(sockfd, ucTcp_rec_buf, MAX_NAME_SIZE, 0);
            if(recvnum <= 0)
            {
            	my_printf(USER_ERROR, "%s:%d client recv return = %d\n", __FILE__, __LINE__, recvnum);
				//触发PCU连接断开
				PileAlarmSet(ALARM_ID_PCU_COMM_LOST_ERROR);
				break;
            }

    		if(recvnum < MAX_NAME_SIZE)
    		{
				U16 FreeSize = ETH_REC_RINGBUF_SIZE - ETH_TcpRecBuffer.u16ReachPos;//剩余的空间
				U16 CpyLen = (FreeSize >= (U16)recvnum) ? recvnum : FreeSize;
				(void)memcpy(&ETH_TcpRecBuffer.u8DealData[ETH_TcpRecBuffer.u16ReachPos], ucTcp_rec_buf, CpyLen);
				ETH_TcpRecBuffer.u16ReachPos += CpyLen;
				if(ETH_TcpRecBuffer.u16ReachPos == ETH_REC_RINGBUF_SIZE)
				{
					ETH_TcpRecBuffer.u16ReachPos=0;
					if(CpyLen != (U16)recvnum)
					{
						(void)memcpy(&ETH_TcpRecBuffer.u8DealData[ETH_TcpRecBuffer.u16ReachPos], &ucTcp_rec_buf[CpyLen], (U16)recvnum - CpyLen);
						ETH_TcpRecBuffer.u16ReachPos += ((U16)recvnum - CpyLen);
					}
				 }

				if(NULL != ETH_Rec_Binary_Semaphore)
				{
					(void)xSemaphoreGive(ETH_Rec_Binary_Semaphore);
				}
    		}

    		if (g_sMsg_control.sComm_Unans.ucCharging_unansNum > 2)
    		{
    			my_printf(USER_INFO, "receive tcp data = %d\n", recvnum);
    		}
        }

        TcpClose();
        vTaskDelay(10000);
    }
}

static void PCU_EventAnalysis(void)
{
	if ((ETH_TcpRecBuffer.u8AnalysisData[3] == 0x80U) || (ETH_TcpRecBuffer.u8AnalysisData[3] == 0x00U))
	{
		if (ETH_TcpRecBuffer.u8AnalysisData[2] == g_sStorage_data.ucPile_num)
		{
			U16 u16DataLen = 0;

			u16DataLen = ((U16)ETH_TcpRecBuffer.u8AnalysisData[7] << 8) | (ETH_TcpRecBuffer.u8AnalysisData[8] + 9U);
			CommunicationParse(ETH_TcpRecBuffer.u8AnalysisData, u16DataLen);
		}
	}
}

static void PcRecOneData(U8 u8RecDataTmp)
{
	if(ETH_TcpRecBuffer.u8ReadMsgBeginFlag == 0U)
	{
		ETH_TcpRecBuffer.u16ReadMsgTimeOutCount = 0;
		ETH_TcpRecBuffer.u16ReadMsgDataNum = 0;
		if(u8RecDataTmp == 0xAAU)
		{
			ETH_TcpRecBuffer.u8AnalysisData[ETH_TcpRecBuffer.u16ReadMsgDataNum++] = u8RecDataTmp;
			ETH_TcpRecBuffer.u8ReadMsgBeginFlag = 1;
		}
	}
	else
	{
		if((ETH_TcpRecBuffer.u16ReadMsgDataNum == 1U) && (u8RecDataTmp != 0x55U))
		{
			ETH_TcpRecBuffer.u16ReadMsgDataNum = 0;
			ETH_TcpRecBuffer.u8ReadMsgBeginFlag = 0;
		}

		//数据填充
		ETH_TcpRecBuffer.u8AnalysisData[ETH_TcpRecBuffer.u16ReadMsgDataNum++] = u8RecDataTmp;

		if(ETH_TcpRecBuffer.u16ReadMsgDataNum == 9U)//到长度位
		{
			ETH_TcpRecBuffer.u16Datacount=((U16)ETH_TcpRecBuffer.u8AnalysisData[7] << 8) + ETH_TcpRecBuffer.u8AnalysisData[8];//提取数据长度
		}

		//等待数据填充
		if(ETH_TcpRecBuffer.u16Datacount < 400U)
		{
			if(ETH_TcpRecBuffer.u16ReadMsgDataNum >= (ETH_TcpRecBuffer.u16Datacount + 9U))
			{
				PCU_EventAnalysis();
				ETH_TcpRecBuffer.u16ReadMsgDataNum=0;
				ETH_TcpRecBuffer.u8ReadMsgBeginFlag=0;
			}
		}
		else
		{
			ETH_TcpRecBuffer.u16ReadMsgDataNum=0;
			ETH_TcpRecBuffer.u8ReadMsgBeginFlag=0;
			my_printf(USER_ERROR, "%s:%d TCP Datacount= %d\n", __FILE__, __LINE__,ETH_TcpRecBuffer.u16Datacount);
		}
	}
}

static void Tcp_analysis_task(void *arg)
{
	BaseType_t err = pdFALSE;

	while(1)
	{
		err = xSemaphoreTake(ETH_Rec_Binary_Semaphore, portMAX_DELAY);
		if(pdTRUE == err)
		{
			while ((ETH_TcpRecBuffer.u16DealPos != ETH_TcpRecBuffer.u16ReachPos) && (ETH_TcpRecBuffer.u16DealCount < ETH_REC_RINGBUF_SIZE))
			{
				ETH_TcpRecBuffer.u16DealCount+=1U;
				PcRecOneData(ETH_TcpRecBuffer.u8DealData[ETH_TcpRecBuffer.u16DealPos]);
				ETH_TcpRecBuffer.u16DealPos++;
				if(ETH_TcpRecBuffer.u16DealPos >= ETH_REC_RINGBUF_SIZE)
				{
					ETH_TcpRecBuffer.u16DealPos = 0;
				}
			}
			ETH_TcpRecBuffer.u16DealCount=0;
		}
		else
		{
			vTaskDelay(50);
		}
	}
	vTaskDelay(50);
}

static void ethernet_link_check_task(void *arg)
{
	U8 LastLinkStatus = 1,LinkStatus = 0,EthReconnect=0;

	vTaskDelay(2000);

	while(1)
	{
		if (0U != netif_is_link_up(&lwIP_netif0))
		{
			//网线连接正常
			LinkStatus=1;
		}
		else
		{
			//网线连接不正常
			LinkStatus=0;
		}

		if(LastLinkStatus != LinkStatus)
		{
			if((LinkStatus == 1U) && (Tcp_ClientFirstCreat == 1U))
			{
				EthReconnect=1;
			}

			LastLinkStatus = LinkStatus;
		}

		if(EthReconnect == 1U)
		{
			EthReconnect = 0;
			TcpClose();

			my_printf(USER_INFO, "Recreate task:TCP_CLIENT %s", __FILE__);
			taskENTER_CRITICAL();

			vTaskDelete(tcp_client_Task_Handler);

			(void)xTaskCreate(&tcp_client_task, "TCP_CLIENT", 1600U/4U, NULL, TCP_TASK_PRIO, &tcp_client_Task_Handler);

			taskEXIT_CRITICAL();
		}

		vTaskDelay(1000);
	}
}

void user_eth_init(void)
{
	ETH_Rec_Binary_Semaphore = xSemaphoreCreateBinary();

	(void)xTaskCreate(&tcp_client_task, "TCP_CLIENT", 1600U/4U, NULL, TCP_TASK_PRIO, &tcp_client_Task_Handler);
	(void)xTaskCreate(&Tcp_analysis_task, "TCP_ANALYSIS", 1400U/4U, NULL, TCP_TASK_PRIO-2U, NULL);
	//xTaskCreate(ethernet_link_check_task, "ETH_LINK", 900/4, NULL, TCP_TASK_PRIO, NULL);
}
