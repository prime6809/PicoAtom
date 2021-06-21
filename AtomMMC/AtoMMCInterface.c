#include "platform.h"

#include "atmmc2.h"
#if PLATFORM == PLATFORM_PIC
#include "..\shared\config.h"
#endif

#include <string.h>
#include <delays.h>

#include "atmmc2io.h"
#include "integer.h"
#include "buildnumber.h"
#include "status.h"

#define FLAG_BLENABLED 0x80
#define FLAG_IRQENABLED 0x20

char windowData[512];

unsigned char globalData[256];

#ifdef INCLUDE_SDDOS
unsigned char sectorData[512];
#endif

uint8_t	InterfaceAddr;
uint8_t	InterfaceData;

BYTE configByte;
BYTE blVersion;


void AtoMMCInit(void)
{
	CardType = 0;     // no card
	CLOCKINIT();

	REDLEDOFF();
	GREENLEDOFF();
	configByte = ReadEEPROM(EE_SYSFLAGS);
	configByte=0x40;
//	configByte=0x00;
	at_initprocessor();
}

void AtoMMCProcess(void)
{
	at_process();
}

void AtoMMCWrite(uint16_t	Addr,
				 uint8_t	Data)
{
	InterfaceAddr=(Addr & ADDRESS_MASK);
	InterfaceData=Data;

	logc0(DEBUG_ATOMMMC_IF,"AtoMMCWrite(%02X,%02X)\n",InterfaceAddr,InterfaceData);
	at_process();
}

// Read the byte from the MMC interface, we store this in a temp variable as
// at_process() may change the value of InterfaceData (which will then be
// returned by the next read call).
uint8_t AtoMMCRead(uint16_t	Addr)
{
	uint8_t	Result;
	InterfaceAddr=(Addr & ADDRESS_MASK) | AtomRWMask;

	Result=InterfaceData;

	logc0(DEBUG_ATOMMMC_IF,"AtoMMCRead(%02X)=%02X\n",InterfaceAddr,InterfaceData);
	at_process(); 

	return Result;
}

void WriteEEPROM(BYTE address, BYTE val)
{
#if 0
	EEADR = address;
   EEDATA = val;
   EECON1bits.EEPGD = 0;
   EECON1bits.CFGS = 0;
   EECON1bits.WREN = 1;
   EECON2 = 0x55;
   EECON2 = 0xAA;
   EECON1bits.WR = 1;
   while(EECON1bits.WR)
   {
      _asm nop _endasm;
   }
   EECON1bits.WREN = 0;
#endif
}


BYTE ReadEEPROM(BYTE address)
{
#if 0
	EEADR = address;
   EECON1bits.EEPGD = 0;
   EECON1bits.CFGS = 0;
   EECON1bits.RD = 1;
   return EEDATA;
#endif
   return 0;
}
