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

uint32_t num_high_samples[2] = {0,0};
uint32_t num_low_samples[2] = {0,0};
uint8_t  dust_write_index = 0;

static inline uint8_t dust_read_index(){
    return 1 - dust_write_index;
}

void dust_init(){
    // input with pullups disabled
    DUST_DDR_REG = ~_BV(DUST_PIN);
    DUST_PORT_REG = ~_BV(DUST_PIN);

    dust_clear();
}

uint32_t get_dust_occupancy(){
    uint32_t ret_value = 0;
    uint32_t denominator = num_high_samples[dust_read_index()] + num_low_samples[dust_read_index()];
    uint32_t offset = 0;
    denominator /= 10000;
    ret_value = num_low_samples[dust_read_index()] / denominator;

    if(ret_value > 0 && ret_value < 10000){
        // offset compensation
        offset = ret_value / 100;
        offset *= 2;
        ret_value -= offset;
    }

    if(ret_value > 10000L){
        ret_value = 10000L;
    }

    return ret_value;
}

void dust_process(){
    if(DUST_IS_HIGH){
        num_high_samples[dust_write_index]++;
    }
    else{
        num_low_samples[dust_write_index]++;
    }
}

void dust_clear(){
    dust_write_index = dust_read_index();
    num_high_samples[dust_write_index] = 0;
    num_low_samples[dust_write_index] = 0;
}
