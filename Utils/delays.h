/*
 * delays.h
 *
 *  Created on: 18 Jul 2018
 *      Author: phill harvey-smith
 */

#include <stdint.h>
#include "pico/stdlib.h"

#ifndef DELAYS_H_
#define DELAYS_H_

void _delay_init(void);
void _delay_us(uint32_t us);
void _delay_ms(uint32_t ms);
void _delay_s(uint32_t s);



#endif /* DELAYS_H_ */
