/*
 * timer.c
 *
 *  Created on: Jan 27, 2013
 *      Author: vic
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer.h"
#include "utility.h"

// run a timer that expires every 30 seconds
#define TIMER_REG_RELOAD 34286
#define TIMER_VAL_RELOAD 30
uint8_t timer_value = TIMER_VAL_RELOAD;

void timer_init(){
    TCCR1A = 0;
    TCCR1B = _BV(CS12); // prescaler = 256
    TCCR1C = 0;
    TCNT1 = TIMER_REG_RELOAD;
    TIMSK1 = _BV(TOIE1);
}

void timer_restart(){
    timer_value = TIMER_VAL_RELOAD;
}

uint8_t timer_expired(){
    if(timer_value == 0){
        return 1;
    }
    return 0;
}

// occurs once per second
ISR(TIMER1_OVF_vect){
    TCNT1 = TIMER_REG_RELOAD;
    if(timer_value > 0){
        timer_value--;
    }
}

