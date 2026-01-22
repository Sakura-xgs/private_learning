/*
 * hal_adc_IF.h
 *
 *  Created on: 2024年12月16日
 *      Author: Bono
 */

#ifndef HAL_HAL_ADC_HAL_ADC_IF_H_
#define HAL_HAL_ADC_HAL_ADC_IF_H_


extern U16 HAL_TEMP_AdToTemp_3380(U16 TempData);
extern U16 HAL_TEMP_AdToTemp_3435(U16 TempData);
extern U16 HAL_TEMP_AdToTemp_PT1000(U16 TempData);
U16 HAL_TEMP_AdToTemp_UnKnow(U16 TempData);

#endif /* HAL_HAL_ADC_HAL_ADC_IF_H_ */
