/*
 * interpolation.c
 *
 *  Created on: Oct 21, 2012
 *      Author: vic
 */

#include "interpolation.h"
#include "egg_bus.h"
#include <float.h>

#define INTERPOLATION_X_INDEX 0
#define INTERPOLATION_Y_INDEX 1
#define INTERPOLATION_TERMINATOR 0xff

// these are the conversion factors required to turn into floating point values (multiply table values by these)
const float x_scaler[EGG_BUS_NUM_HOSTED_SENSORS] = {0.07f};
const float y_scaler[EGG_BUS_NUM_HOSTED_SENSORS] = {52.0f};
const float independent_scaler[EGG_BUS_NUM_HOSTED_SENSORS] = {0.01f};
const uint32_t independent_scaler_inverse[EGG_BUS_NUM_HOSTED_SENSORS] = { 100 };

// get_x_or_get_y = 0 returns x value from table, get_x_or_get_y = 1 returns y value from table
uint8_t getTableValue(uint8_t sensor_index, uint8_t table_index, uint8_t get_x_or_get_y){

    // the values MUST be provided in ascending order of x-value
    const uint8_t dust_ppb[][2] = {
            {0,0},
            {28,19},
            {56,39},
            {81,58},
            {104,77},
            {126,96},
            {145,115},
            {162,135},
            {178,154},
            {193,173},
            {206,193},
            {219,212},
            {230,231},
            {242,251},
            {INTERPOLATION_TERMINATOR, INTERPOLATION_TERMINATOR}
    };

    // sensor index 0 is the dust sensor
    uint8_t return_value = 0;
    if(sensor_index == 0){
        return_value = dust_ppb[table_index][get_x_or_get_y];
    }

    return return_value;
}

uint8_t * get_p_x_scaler(uint8_t sensor_index){
    return (uint8_t *) &(x_scaler[sensor_index]);
}

uint8_t * get_p_y_scaler(uint8_t sensor_index){
    return (uint8_t *) &(y_scaler[sensor_index]);
}

uint8_t * get_p_independent_scaler(uint8_t sensor_index){
    return (uint8_t *) &(independent_scaler[sensor_index]);
}

uint32_t get_independent_scaler_inverse(uint8_t sensor_index){
    return independent_scaler_inverse[sensor_index];
}
