/*
 * hal_eth_IF.h
 *
 *  Created on: 2024年9月3日
 *      Author: Bono
 */

#ifndef HAL_HAL_ETH_HAL_ETH_IF_H_
#define HAL_HAL_ETH_HAL_ETH_IF_H_

#define SERVER_PORT 	8000
#ifndef SERVERIP_ADDR0
#define SERVERIP_ADDR0 	172
#endif
#ifndef SERVERIP_ADDR1
#define SERVERIP_ADDR1 	30
#endif
#ifndef SERVERIP_ADDR2
#define SERVERIP_ADDR2 	1
#endif
#ifndef SERVERIP_ADDR3
#define SERVERIP_ADDR3 	1
#endif

#define HAL_ENET_PHY_RESET_GPIO			(GPIO1)
#define HAL_ENET_PHY_RESET_GPIO_PIN		(4U)

#define HAL_ENET_PHY_RESET()                                                      \
    GPIO_WritePinOutput(HAL_ENET_PHY_RESET_GPIO, HAL_ENET_PHY_RESET_GPIO_PIN, 0); \
    SDK_DelayAtLeastUs(10000, CLOCK_GetFreq(kCLOCK_CpuClk));                      \
    GPIO_WritePinOutput(HAL_ENET_PHY_RESET_GPIO, HAL_ENET_PHY_RESET_GPIO_PIN, 1); \
    SDK_DelayAtLeastUs(100, CLOCK_GetFreq(kCLOCK_CpuClk))



extern void MDIO_Init(void);
extern void Lwip_Init(void *arg);
extern void GetMacAddress(uint8_t Buf[]);

#endif /* HAL_HAL_ETH_HAL_ETH_IF_H_ */
