/*
* delays.c
 *
 *  Created on: 18 Jul 2018
 *      Author: phill harvey-smith.
 */

#include "delays.h"

void _delay_init(void)
{
}

void _delay_us(uint32_t us)
{
	busy_wait_us_32(us);
}

void _delay_ms(uint32_t ms)
{
	while (ms > 0)
	{
		_delay_us(1000);
		ms--;
	}
}

void _delay_s(uint32_t s)
{
	while (s > 0)
	{
		_delay_ms(1000);
		s--;
	}
}
