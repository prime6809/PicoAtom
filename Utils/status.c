/*
	Status.c
	
	Functions for logging program status to the serial port, to
	be used for debugging pruposes etc.
	
	2008-03-21, P.Harvey-Smith.
	
*/

#include <stdio.h>
#include <ctype.h>
#include "TerminalCodes.h"
#include "status.h"

void cls(void)
{
	printf(ESC_ERASE_DISPLAY);
	printf(ESC_CURSOR_POS(0,0));
}

int FreeRam(void) 
{
	return 0;
}