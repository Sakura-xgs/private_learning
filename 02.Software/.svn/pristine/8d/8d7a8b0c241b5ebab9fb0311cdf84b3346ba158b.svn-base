/*
 * hal_cd4502b.c
 *
 *  Created on: 2024年9月9日
 *      Author: Bono
 */
#include "hal_cd4052b_IF.h"

void hal_cd4052b_choose_channel(CD4052B_CHANNEL channel)
{
	switch (channel)
	{
		case e_0X0Y:
			CHANNEL_B_DISABLE();
			CHANNEL_A_DISABLE();
			break;

		case e_1X1Y:
			CHANNEL_B_DISABLE();
			CHANNEL_A_ENABLE();
			break;

		case e_2X2Y:
			CHANNEL_B_ENABLE();
			CHANNEL_A_DISABLE();
			break;

		case e_3X3Y:
			CHANNEL_B_ENABLE();
			CHANNEL_A_ENABLE();
			break;

		default:
			break;
	}
}
