/*
 * main.c
 *
 *  Created on: Jul 14, 2012
 *      Author: vic
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#define __DELAY_BACKWARD_COMPATIBLE__
#include <util/delay.h>
#include "utility.h"
#include "main.h"
#include "twi.h"
#include "egg_bus.h"
#include "mac.h"
#include "interpolation.h"
#include "dust.h"
#include "timer.h"

//#define INCLUDE_DEBUG_REGISTERS

void onRequestService(void);
void onReceiveService(uint8_t* inBytes, int numBytes);

void setup(void);

uint8_t macaddr[6];

#define DUST_READING_INTERVAL_MS  30000L
#define DUST_READING_INTERVAL_8US (DUST_READING_INTERVAL_MS * 125L)

void main(void) __attribute__((noreturn));
void main(void) {
    setup();
    sei();    // enable interrupts

    // This loop runs forever
    // it can be interrupted at any point by a TWI event
    for (;;) {
        dust_process(); // accumulates the dust variables

        if(timer_expired()){
            STATUS_LED_TOGGLE();
            dust_clear();
            timer_restart();
        }
    }
}

// this gets called when you get an SLA+R
void onRequestService(void){
    uint8_t response[EGG_BUS_MAX_RESPONSE_LENGTH] = { 0 };
    uint8_t response_length = 4; // unless it gets overridden 4 is the default
    uint8_t sensor_index = 0;
    uint8_t sensor_field_offset = 0;
    uint16_t address = egg_bus_get_read_address(); // get the address requested in the SLA+W
    uint16_t sensor_block_relative_address = address - ((uint16_t) EGG_BUS_SENSOR_BLOCK_BASE_ADDRESS);
    uint32_t responseValue = 0;
    switch(address){
    case EGG_BUS_ADDRESS_SENSOR_COUNT:
        response[0] = EGG_BUS_NUM_HOSTED_SENSORS;
        response_length = 1;
        break;
    case EGG_BUS_ADDRESS_MODULE_ID:
        memcpy(response, macaddr, 6);
        response_length = 6;
        break;
    case EGG_BUS_FIRMWARE_VERSION:
        big_endian_copy_uint32_to_buffer(EGG_BUS_FIRMWARE_VERSION_NUMBER, response);
        break;
    default:
        if(address >= EGG_BUS_SENSOR_BLOCK_BASE_ADDRESS){
            sensor_index = sensor_block_relative_address / ((uint16_t) EGG_BUS_SENSOR_BLOCK_SIZE);
            sensor_field_offset = sensor_block_relative_address % ((uint16_t) EGG_BUS_SENSOR_BLOCK_SIZE);
            switch(sensor_field_offset){
            case EGG_BUS_SENSOR_BLOCK_TYPE_OFFSET:
                egg_bus_get_sensor_type(sensor_index, (char *) response);
                response_length = 16;
                break;
            case EGG_BUS_SENSOR_BLOCK_UNITS_OFFSET:
                egg_bus_get_sensor_units(sensor_index, (char *) response);
                response_length = 16;
                break;
            case EGG_BUS_SENSOR_BLOCK_R0_OFFSET:
                big_endian_copy_uint32_to_buffer(egg_bus_get_r0_ohms(sensor_index), response);
                break;
            case EGG_BUS_SENSOR_BLOCK_TABLE_X_SCALER_OFFSET:
                memcpy(&responseValue, get_p_x_scaler(sensor_index), 4);
                big_endian_copy_uint32_to_buffer(responseValue, response);
                break;
                case EGG_BUS_SENSOR_BLOCK_TABLE_Y_SCALER_OFFSET:
                memcpy(&responseValue, get_p_y_scaler(sensor_index), 4);
                big_endian_copy_uint32_to_buffer(responseValue, response);
                break;
            case EGG_BUS_SENSOR_BLOCK_MEASURED_INDEPENDENT_SCALER_OFFSET:
                memcpy(&responseValue, get_p_independent_scaler(sensor_index), 4);
                big_endian_copy_uint32_to_buffer(responseValue, response);
                break;
            case EGG_BUS_SENSOR_BLOCK_MEASURED_INDEPENDENT_OFFSET:
            case EGG_BUS_SENSOR_BLOCK_RAW_VALUE_OFFSET:
                responseValue = get_dust_occupancy(); // num 8us intervals low in 30s interval
                // independent is ratio of time low to interval duration
                //responseValue *= get_independent_scaler_inverse(sensor_index); // scale up numerator
                //responseValue /= DUST_READING_INTERVAL_8US;                    // divide by denominator
                // value should be something like XXXY representing XXX.Y%
                // for example if the value is 346 that means 34.6% occupancy
                big_endian_copy_uint32_to_buffer(responseValue, response);
                break;
            default: // assume its an access to the mapping table entries
                sensor_block_relative_address = (sensor_field_offset - EGG_BUS_SENSOR_BLOCK_COMPUTED_VALUE_MAPPING_TABLE_BASE_OFFSET);
                sensor_block_relative_address >>= 3; // divide by eight - now it is the mapping table index
                response_length = 2;

                *(response)   = getTableValue(sensor_index, sensor_block_relative_address, 0);
                *(response+1) = getTableValue(sensor_index, sensor_block_relative_address, 1);

                break;
            }
        }
        break;
    }

    // write the value back to the master per the protocol requirements
    // the response is always four bytes, most significant byte first
    twi_transmit(response, response_length);
}

// this gets called when you get an SLA+W  then numBytes bytes, then stop
//   numBytes bytes have been buffered in inBytes by the twi library
// it seems quite critical that we not dilly-dally in this function, get in and get out ASAP
void onReceiveService(uint8_t* inBytes, int numBytes){
    uint8_t command = inBytes[0];
    uint16_t address = (((uint16_t) inBytes[1]) << 8) | inBytes[2];
    uint32_t value = 0;
    uint8_t sensor_index = 0;
    uint8_t sensor_field_offset = 0;
    uint16_t sensor_block_relative_address = address - ((uint16_t) EGG_BUS_SENSOR_BLOCK_BASE_ADDRESS);
    uint8_t ii = 0;

    POWER_LED_TOGGLE();
    switch(command){
    case EGG_BUS_COMMAND_READ:
        egg_bus_set_read_address(address);
        break;
    case EGG_BUS_COMMAND_WRITE:
        // The write command always has a 2-byte address
        // then the data in big-endian byte order
        // so numBytes must be at least 4 (command, address high, address low, value byte N-1, ..., value byte 0)
        if(address >= EGG_BUS_SENSOR_BLOCK_BASE_ADDRESS){
            sensor_index = sensor_block_relative_address / ((uint16_t) EGG_BUS_SENSOR_BLOCK_SIZE);
            sensor_field_offset = sensor_block_relative_address % ((uint16_t) EGG_BUS_SENSOR_BLOCK_SIZE);
            switch(sensor_field_offset){
            case EGG_BUS_SENSOR_BLOCK_R0_OFFSET:
                // rebuild the value
                value = inBytes[3];
                for(ii = 4; ii < 7; ii++){
                    value <<= 8;
                    value |= inBytes[ii];
                }

                // store the value
                egg_bus_set_r0_ohms(sensor_index, value);
                break;
            }
        }

        break;
    }
}

void setup(void){
    POWER_LED_INIT();
    STATUS_LED_INIT();
    POWER_LED_ON();
    delay_sec(1);

    unio_init(NANODE_MAC_DEVICE);
    unio_read(macaddr, NANODE_MAC_ADDRESS, 6);

    // TWI Initialize
    twi_setAddress(TWI_SLAVE_ADDRESS);
    twi_attachSlaveTxEvent(onRequestService);
    twi_attachSlaveRxEvent(onReceiveService);
    twi_init();

    dust_init();
    timer_init();

    POWER_LED_OFF();

    blinkLEDs(1, STATUS_LED);
    STATUS_LED_OFF();
}
