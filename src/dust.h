/*
 * dust.h
 *
 *  Created on: Dec 2, 2012
 *      Author: vic
 */

#ifndef DUST_H_
#define DUST_H_

void dust_init();
uint32_t get_dust_occupancy();
void dust_enq();
void dust_process();

#endif /* DUST_H_ */
