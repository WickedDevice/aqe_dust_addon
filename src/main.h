/*
 * main.h
 *
 *  Created on: Jul 14, 2012
 *      Author: vic
 */

#ifndef MAIN_H_
#define MAIN_H_

#define VOC_HEATER_POWER_ADC       7
#define VOC_HEATER_FEEDBACK_ADC    1
#define VOC_HEATER_FEEDBACK_RESISTANCE 10L // ohms
#define VOC_HEATER_TARGET_POWER_MW 76L // mW

uint16_t averageADC(uint8_t sensor_index);

#endif /* MAIN_H_ */
