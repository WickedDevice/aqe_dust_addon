/*
 * timebase.c
 *
 *  Created on: Dec 3, 2012
 *      Author: vic
 */
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "timebase.h"


// double buffered timestamp
uint32_t  _timestamp_ms = 0;
volatile uint8_t   _timebase_update_flag = 0;

void timebase_reset(){
    // set up the timer0 overflow interrupt for 1ms
    TCNT0  = TIMER0_1MS_OVERFLOW_TCNT;
    TIMSK0 = _BV(TOIE0);

    // start timer0
    TCCR0A = TIMER0_1MS_OVERFOW_PRESCALER;

    _timestamp_ms = 0;
    _timebase_update_flag = 0;
}


// fires once per millisecond, don't block
// can't miss TWI interrupts for anything
ISR(TIMER0_OVF_vect, ISR_NOBLOCK){
    TCNT0 = TIMER0_1MS_OVERFLOW_TCNT;
    _timebase_update_flag = 1;
}


uint32_t timebase_now(){
    return _timestamp_ms;
}

void timebase_update(){
    if(_timebase_update_flag){
        _timebase_update_flag = 0;
        _timestamp_ms++;
    }
}

