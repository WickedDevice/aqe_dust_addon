/*
 * timer.h
 *
 *  Created on: Jan 27, 2013
 *      Author: vic
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>

void timer_init();
void timer_restart();
uint8_t timer_expired();


#endif /* TIMER_H_ */
