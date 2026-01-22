/*
 * hal_eth_IF.h
 *
 *  Created on: 2024年9月3日
 *      Author: Bono
 */

#ifndef HAL_HAL_ETH_HAL_ETH_IF_H_
#define HAL_HAL_ETH_HAL_ETH_IF_H_


#define HAL_ENET_PHY_RESET_GPIO			(GPIO1)
#define HAL_ENET_PHY_RESET_GPIO_PIN		(4U)

#define HAL_ENET_PHY_RESET()                                                      \
    GPIO_WritePinOutput(HAL_ENET_PHY_RESET_GPIO, HAL_ENET_PHY_RESET_GPIO_PIN, 0); \
    SDK_DelayAtLeastUs(10000, CLOCK_GetFreq(kCLOCK_CpuClk));                      \
    GPIO_WritePinOutput(HAL_ENET_PHY_RESET_GPIO, HAL_ENET_PHY_RESET_GPIO_PIN, 1); \
    SDK_DelayAtLeastUs(100, CLOCK_GetFreq(kCLOCK_CpuClk))



extern void MDIO_Init(void);
extern void Lwip_Init(void *arg);
void GetMacAddress(uint8_t Buf[]);

#endif /* HAL_HAL_ETH_HAL_ETH_IF_H_ */
