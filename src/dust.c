/*
 * dust.c
 *
 *  Created on: Dec 2, 2012
 *      Author: vic
 */

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "dust.h"

uint32_t num_high_samples;
uint32_t num_low_samples;

void dust_init(){
    // input with pullups disabled
    DUST_DDR_REG = ~_BV(DUST_PIN);
    DUST_PORT_REG = ~_BV(DUST_PIN);

    dust_clear();
}

uint32_t get_dust_occupancy(){
    uint32_t denominator = (num_high_samples + num_low_samples);
    denominator /= 10000;
    return num_low_samples / denominator;
}

void dust_process(){
    if(DUST_IS_HIGH){
        num_high_samples++;
    }
    else{
        num_low_samples++;
    }
}

void dust_clear(){
    num_high_samples = 0;
    num_low_samples = 0;
}
