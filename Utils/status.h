/*
	Status.h
	
	Functions for logging program status to the serial port, to
	be used for debugging pruposes etc.
	
	2008-03-21, P.Harvey-Smith.

	Some functions and macros borrowed from Dean Camera's LURFA 
	USB libraries.
	
*/

#include <stdbool.h>
#include <stdio.h>
#include "TerminalCodes.h"

#ifndef __STATUS_DEFINES__
#define __STATUS_DEFINES__

#define log0(format,...) printf(format,##__VA_ARGS__)
#define logc0(cond,format,...) 	if (cond) printf(format,##__VA_ARGS__)

void cls(void);

int FreeRam(void);

#endif
