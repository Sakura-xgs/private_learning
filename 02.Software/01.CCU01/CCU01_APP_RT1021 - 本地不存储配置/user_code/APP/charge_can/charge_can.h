/*
 * charge_can.h
 *
 *  Created on: 2024年8月26日
 *      Author: Bono
 */

#ifndef APP_CHARGE_CAN_CHARGE_CAN_H_
#define APP_CHARGE_CAN_CHARGE_CAN_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define CANTYPE 1U
#define ETHTYPE 2U

typedef uint32_t (*PinReadStatus)(void);

enum
{
	PRODUCTION_MODE_CONTROL = 0x00,
	RELAY_DO_CONTROL        = 0x02,
	RELAY_DI_CHECK			= 0x03,
	TEMP_CHECK				= 0x04,
	UART_COMM_CHECK			= 0x05,
	PWM_CHECK				= 0x06,
	SOFTWARE_CHECK			= 0x07,
	EEPROM_CHECK			= 0x08,
	WATCHDOG_CHECK			= 0x09,
	RTC_CHECK               = 0x0B,
	DEVICE_ADDRESS_CHECK    = 0x0C,
	DEVICE_SN_WRITE			= 0x0D,
	DEVICE_SN_READ			= 0x0E,
	DEVICE_LED_CONTROL		= 0x0F
};

extern void Charge_Can_Comm_Init_task(void * pvParameters);
void ProductionProcess(const U8* pucBuf, const U8 ucGun_id, const U8 ComType);

#endif /* APP_CHARGE_CAN_CHARGE_CAN_H_ */
