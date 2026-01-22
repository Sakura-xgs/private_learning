/*
 * hal_eth.c
 *
 *  Created on: 2024年9月3日
 *      Author: Bono
 */

#include "eth.h"
#include "peripherals.h"
#include "PublicDefine.h"
#include "fsl_iomuxc.h"
#include "hal_eth_IF.h"
#include "SignalManage.h"

#include "TcpBoot.h"
#include "boot.h"
#include "hal_eth.h"

/* IP address configuration. */
#ifndef configIP_ADDR0
#define configIP_ADDR0 172
#endif
#ifndef configIP_ADDR1
#define configIP_ADDR1 168
#endif
#ifndef configIP_ADDR2
#define configIP_ADDR2 1
#endif
#ifndef configIP_ADDR3
#define configIP_ADDR3 105
#endif

/* Netmask configuration. */
#ifndef configNET_MASK0
#define configNET_MASK0 255
#endif
#ifndef configNET_MASK1
#define configNET_MASK1 255
#endif
#ifndef configNET_MASK2
#define configNET_MASK2 255
#endif
#ifndef configNET_MASK3
#define configNET_MASK3 0
#endif

/* Gateway address configuration. */
#ifndef configGW_ADDR0
#define configGW_ADDR0 172
#endif
#ifndef configGW_ADDR1
#define configGW_ADDR1 168
#endif
#ifndef configGW_ADDR2
#define configGW_ADDR2 1
#endif
#ifndef configGW_ADDR3
#define configGW_ADDR3 1
#endif

/* Ethernet configuration. */
#define EXAMPLE_ENET         ENET
#define EXAMPLE_PHY_ADDRESS  0x02U
#define EXAMPLE_PHY_OPS      &phyksz8081_ops
//extern phy_ksz8081_resource_t lwIP_netif0_phy_resource;
#define EXAMPLE_PHY_RESOURCE &lwIP_netif0_phy_resource
#define EXAMPLE_CLOCK_FREQ   CLOCK_GetFreq(kCLOCK_IpgClk)
/* Must be after include of app.h */
#if !defined(configMAC_ADDR) || !defined(configMAC_ADDR1)
#include "fsl_silicon_id.h"
#endif

/*! @brief Stack size of the temporary lwIP initialization thread. */
#define INIT_THREAD_STACKSIZE 1024

/*! @brief Priority of the temporary lwIP initialization thread. */
#define INIT_THREAD_PRIO DEFAULT_THREAD_PRIO

#ifndef EXAMPLE_NETIF_INIT_FN
#define EXAMPLE_NETIF_INIT_FN ethernetif0_init
#endif

#if defined(BOARD_NETWORK_USE_DUAL_ENET)
#define BOARD_PHY_COUNT 2
#else
#define BOARD_PHY_COUNT 1
#endif
//static struct netif netif;

static void LWIP_IP_Reinit(void);
static void lwip_netif_status_callback(const struct netif *my_netif);

void MDIO_Init(void)
{
    (void)CLOCK_EnableClock(s_enetClock[ENET_GetInstance(ENET)]);
    ENET_SetSMI(ENET, CLOCK_GetFreq(kCLOCK_IpgClk), false);
}

status_t MDIO_Write(uint8_t phyAddr, uint8_t regAddr, uint16_t data)
{
    return ENET_MDIOWrite(ENET, phyAddr, regAddr, data);
}

status_t MDIO_Read(uint8_t phyAddr, uint8_t regAddr, uint16_t *pData)
{
    return ENET_MDIORead(ENET, phyAddr, regAddr, pData);
}

static void lwip_netif_status_callback(const struct netif *my_netif)
{
	/* initilaize the tcp server: telnet 8000 */
	user_eth_init();
}

static void stack_init(void)
{
	static phy_handle_t phyHandle;
    ip4_addr_t netif0_ipaddr, netif0_netmask, netif0_gw;
    static struct netif s_netif0;
    ethernetif_config_t enet0_config = {.phyHandle   = &phyHandle,
                                        .phyAddr     = EXAMPLE_PHY_ADDRESS,
                                        .phyOps      = EXAMPLE_PHY_OPS,
                                        .phyResource = EXAMPLE_PHY_RESOURCE,
                                        .srcClockHz  = EXAMPLE_CLOCK_FREQ,
#ifdef configMAC_ADDR
                                        .macAddress = configMAC_ADDR
#endif
    };
#ifndef configMAC_ADDR
    (void)SILICONID_ConvertToMacAddr(&enet0_config.macAddress);
#endif

    tcpip_init(NULL, NULL);

    IP4_ADDR(&netif0_ipaddr, configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3);
    IP4_ADDR(&netif0_netmask, configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3);
    IP4_ADDR(&netif0_gw, configGW_ADDR0, configGW_ADDR1, configGW_ADDR2, configGW_ADDR3);
    (void)netifapi_netif_add(&s_netif0, &netif0_ipaddr, &netif0_netmask, &netif0_gw, &enet0_config, EXAMPLE_NETIF_INIT_FN,
                       &tcpip_input);
    netifapi_netif_set_up(&s_netif0);

#if defined(BOARD_NETWORK_USE_DUAL_ENET)
    ip4_addr_t netif1_ipaddr, netif1_netmask, netif1_gw;
    ethernetif_config_t enet1_config = {.phyHandle   = &phyHandle1,
                                        .phyAddr     = EXAMPLE_PHY1_ADDRESS,
                                        .phyOps      = EXAMPLE_PHY1_OPS,
                                        .phyResource = EXAMPLE_PHY1_RESOURCE,
                                        .srcClockHz  = EXAMPLE_CLOCK_FREQ,
#ifdef configMAC_ADDR1
                                        .macAddress = configMAC_ADDR1
#endif
    };
    static struct netif s_netif1;
#ifndef configMAC_ADDR1
    (void)SILICONID_ConvertToMacAddr(&enet1_config.macAddress);
#endif
    IP4_ADDR(&netif1_ipaddr, configIP1_ADDR0, configIP1_ADDR1, configIP1_ADDR2, configIP1_ADDR3);
    IP4_ADDR(&netif1_netmask, configNET1_MASK0, configNET1_MASK1, configNET1_MASK2, configNET1_MASK3);
    IP4_ADDR(&netif1_gw, configGW1_ADDR0, configGW1_ADDR1, configGW1_ADDR2, configGW1_ADDR3);
    netifapi_netif_add(&s_netif1, &netif1_ipaddr, &netif1_netmask, &netif1_gw, &enet1_config, EXAMPLE_NETIF1_INIT_FN,
                       tcpip_input);
    netifapi_netif_set_up(&s_netif1);
#else
    /*
     * Single netif is used, set is as default to avoid
     * the need to append zone indices to link-local IPv6 addresses.
     */
    netifapi_netif_set_default(&s_netif0);
#endif /* defined(BOARD_NETWORK_USE_DUAL_ENET) */

    struct netif *netif_array[BOARD_PHY_COUNT];
    netif_array[0] = &s_netif0;
#if defined(BOARD_NETWORK_USE_DUAL_ENET)
    netif_array[1] = &s_netif1;
#endif

    while (ethernetif_wait_linkup_array(netif_array, BOARD_PHY_COUNT, 5000) != ERR_OK)
    {
        PRINTF("PHY Auto-negotiation failed. Please check the cable connection and link partner setting.\r\n");
    }
}

static void LWIP_IP_Reinit(void)
{
	S32 iTmp_flag = 0;
	S32 iIp_addr1 = 0, iIp_addr2 = 0;

	//等待eeprom启动完成
	vTaskDelay(1000);

//	(void)GetSigVal(CCU_SET_SIG_ID_GUN_NUM, &iIp_addr1);
//	//无效编号
//    if ((iIp_addr1 < PCU_NUM_MIN) || (iIp_addr1 > PCU_NUM_MAX))
//    {
//    	iIp_addr1 = PCU_NUM_MIN;
//		//设置为默认值1
//		(void)SetSigVal(CCU_SET_SIG_ID_GUN_NUM, PCU_NUM_MIN);
//    }
//
//	(void)GetSigVal(CCU_SET_SIG_ID_GUN_NUM, &iIp_addr2);
//	//无效编号
//    if ((iIp_addr2 < PILE_NUM_MIN) || (iIp_addr2 > PILE_NUM_MAX))
//    {
//    	iIp_addr2 = PILE_NUM_MIN;
//		//设置为默认值1
//		(void)SetSigVal(CCU_SET_SIG_ID_GUN_NUM, PILE_NUM_MIN);
//    }
//    IP4_ADDR(&lwIP_netif0_ipaddr, SERVERIP_ADDR0, SERVERIP_ADDR1, iIp_addr1, iIp_addr2*10);

	(void)GetSigVal(CCU_SET_SIG_ID_GUN_NUM, &iTmp_flag);

	//有效编号
    if ((PILE_NUM_MIN <= iTmp_flag) && (iTmp_flag <= PILE_NUM_MAX))
    {
		IP4_ADDR(&lwIP_netif0_ipaddr, SERVERIP_ADDR0, SERVERIP_ADDR1, SERVERIP_ADDR2, iTmp_flag*10);
    }
    else
    {
		//设置为默认值1
		(void)SetSigVal(CCU_SET_SIG_ID_GUN_NUM, 1);
		IP4_ADDR(&lwIP_netif0_ipaddr, SERVERIP_ADDR0, SERVERIP_ADDR1, SERVERIP_ADDR2, 1*10);
    }
}

void GetMacAddress(U8 Buf[])
{
	(void)memcpy(Buf, &lwIP_netif0_enet_config.macAddress, 6);
}

void Lwip_Init(void *arg)
{
	LWIP_UNUSED_ARG(arg);

	/* Set special address for each chip. */
	(void)SILICONID_ConvertToMacAddr(&lwIP_netif0_enet_config.macAddress);

	/* lwIP module initialization */
	tcpip_init(NULL, NULL);
	/* ENET peripheral initialization */
	/* ENET clock enable */
	(void)CLOCK_EnableClock(s_enetClock[ENET_GetInstance(LWIP_NETIF0_ENET_PERIPHERAL)]);
	/* ENET set SMI (serial management interface) */
	ENET_SetSMI(LWIP_NETIF0_ENET_PERIPHERAL, LWIP_NETIF0_ENET_PERIPHERAL_CLK_FREQ, false);

	//根据桩编号重新适配ip
	LWIP_IP_Reinit();

	/* lwIP network interface initialization */
	(void)netifapi_netif_add(&lwIP_netif0, &lwIP_netif0_ipaddr, &lwIP_netif0_netmask, &lwIP_netif0_gw, &lwIP_netif0_enet_config, ethernetif0_init, tcpip_input);
	(void)netifapi_netif_set_default(&lwIP_netif0);

	//netif_set_status_callback(&lwIP_netif0, lwip_netif_status_callback);

	(void)netifapi_netif_set_up(&lwIP_netif0);
	/* lwIP interface auto-negotiation check */
	while (ethernetif_wait_linkup(&lwIP_netif0, 5000UL) != ERR_OK) {
	  (void)PRINTF("PHY Auto-negotiation failed. Please check the cable connection and link partner setting.\r\n");
	}

	lwip_netif_status_callback(&lwIP_netif0);

	taskENTER_CRITICAL();

	vTaskDelete(NULL);
	taskEXIT_CRITICAL();
}

