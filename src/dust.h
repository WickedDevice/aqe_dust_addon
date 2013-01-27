/*
 * dust.h
 *
 *  Created on: Dec 2, 2012
 *      Author: vic
 */

#ifndef DUST_H_
#define DUST_H_

#include <avr/io.h>

#define DUST_PORT_REG PORTB
#define DUST_PIN_REG  PINB
#define DUST_DDR_REG  DDRB
#define DUST_PIN 0

#define DUST_IS_HIGH (0 != (DUST_PIN_REG & _BV(DUST_PIN)))

void dust_init();
uint32_t get_dust_occupancy();
void dust_process();
void dust_clear();


#endif /* DUST_H_ */
