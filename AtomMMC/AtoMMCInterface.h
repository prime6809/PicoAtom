/*
 * AtoMMCInterface.h
 *
 *  Created on: 24 Aug 2018
 *      Author: afra
 */

#include "integer.h"

#ifndef ATOMMMC_ATOMMCINTERFACE_H_
#define ATOMMMC_ATOMMCINTERFACE_H_

void AtoMMCInit(void);
void AtoMMCWrite(uint16_t	Addr, uint8_t	Data);
uint8_t AtoMMCRead(uint16_t	Addr);
void WriteEEPROM(BYTE address, BYTE val);
BYTE ReadEEPROM(BYTE address);

#endif /* ATOMMMC_ATOMMCINTERFACE_H_ */
