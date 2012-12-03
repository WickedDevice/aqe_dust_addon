/*
 * timebase.h
 *
 *  Created on: Dec 3, 2012
 *      Author: vic
 */

#ifndef TIMEBASE_H_
#define TIMEBASE_H_

// timer 0 is set up as a 1ms time base
#define TIMER0_1MS_OVERFOW_PRESCALER 3     // 8MHz / 64 = 125 kHz
#define TIMER0_1MS_OVERFLOW_TCNT     131   // 255 - 131 + 1 = 125 ticks

void timebase_reset();
void timebase_update();
uint32_t timebase_now();

#endif /* TIMEBASE_H_ */
