/* * Atom.c
 *
 *  Created on: 9 Aug 2018
 *      Author: phill
 */

#include "Atom/Atom.h"
#include "AtoMMCInterface.h"

#define HOOKS 	0

uint8_t	ZeroPage[ATOM_ZERO_SIZE];
uint8_t MainRam[ATOM_MAIN_SIZE];
uint8_t VidRam[ATOM_VID_SIZE];

//FATFS		FatFS[FF_VOLUMES];

uint8_t Keyrows[ATOM_KEYROWS];
typedef struct
{
	uint8_t	PortA;
	uint8_t	PortB;
	uint8_t	PortC;
	uint8_t	Control;
} i8255_t;

i8255_t	i8255;

#define VIA_REGS	16
#define DEBUG_VIA	0

typedef struct
{
	uint8_t	regs[VIA_REGS];
} via6522_t;

via6522_t via6522;

#define LOAD_BUFF_LEN	4
uint8_t	LoadCharBuff[LOAD_BUFF_LEN];

//extern uint16_t pc;

#define AddressInRange(addr, begin, end)	((addr >= begin) && (addr < end))
#define AddressInBlock(addr, begin, size)	((addr >= begin) && (addr < (begin+size)))

uint8_t Read8255(uint16_t	Offset);
void Write8255(uint16_t	Offset, uint8_t Value);
void Reset6522(void);
uint8_t Read6522(uint16_t	Offset);
void Write6522(uint16_t	Offset, uint8_t Value);

struct repeating_timer atom_timer;
bool atom_timer_callback(struct repeating_timer *t);

#define DEBUG_FREQ 		1
#define DEBUG_ATOM_PIN	17

volatile uint32_t clockticks6502last = 0;
volatile bool Exec6502;

void AtomInit(void)
{
	uint8_t				Idx;

	// Set speaker as output	
	gpio_init(SPK_PIN);
	gpio_set_dir(SPK_PIN, true);

	// Set keys to all released
  	for(Idx=0; Idx < ATOM_KEYROWS; Idx++)
		Keyrows[Idx]=0xFF;

	// Initialize 1Hz signal for speed measurement
	cancel_repeating_timer(&atom_timer);	// Incase it was already running.....
	add_repeating_timer_us(-100, atom_timer_callback, NULL, &atom_timer);
	//add_repeating_timer_ms(-1000, atom_timer_callback, NULL, &atom_timer);

#if DEBUG_FREQ == 1
	gpio_init(DEBUG_ATOM_PIN);
	gpio_set_dir(DEBUG_ATOM_PIN,true);
#endif

  	ResetMachine();
}

void ResetMachine(void)
{
	// Initialize display
	MC6847_Init(VidRam,ATOM_VID_SIZE,MC6847_ATOM);
	memset(VidRam,32,ATOM_VID_SIZE);

	// Initialize AtoMMC emulation
	log0("AtoMMCInit()\n");
	AtoMMCInit();

	// Initialize any CPU hooks
	hookexternal(&AtomHook);

	log0("Reset6522()\n");
	Reset6522();

	// Reset the emulated CPU
	log0("reset6502()\n");
	reset6502();
}

void AtomGo(void)
{
	while (1)
	{
		gpio_put(DEBUG_ATOM_PIN,Exec6502);
		if (Exec6502)
		{
			exec6502(200);
			Exec6502=false;
		}
	}
}

void AtomHook(void)
{
#if HOOKS == 1
	// Hook output byte to tape
	if(pc==0xFC7C)
	{
		log0("%02X\n",a);
		pc=0xFCBC; 		// Redirect to an RTS
	}

	// Hook input byte from tape
	if(pc==0xFBEE)
	{
		RxBuff(LoadCharBuff,LOAD_BUFF_LEN-1);
		sscanf(LoadCharBuff,"%02X\n",&a);
		pc=0xFCBC; 		// Redirect to an RTS
	}

	// Hook to ignore leader tone....
	if (pc==0xFB91)
	{
		pc=0xFBB4;
	}
#endif
}

// Read handler for emulated 6502
uint8_t read6502(uint16_t address)
{
	uint8_t	Result;

	// RAM, Zero page, Main RAM, Video RAM
	if(AddressInBlock(address,0x000,ATOM_ZERO_SIZE))
		Result=ZeroPage[address];
	else if (AddressInBlock(address,ATOM_MAIN_BASE,ATOM_MAIN_SIZE))
		Result=MainRam[address-ATOM_MAIN_BASE];
	else if (AddressInBlock(address,ATOM_VID_BASE,ATOM_VID_SIZE))
		Result=VidRam[address-ATOM_VID_BASE];

	// ROMS, system, basic, floating point, AtoMMC.
	else if (AddressInBlock(address,ATOM_EXT_BASE,ATOM_ROM_SIZE))
		Result=sdromraw_rom[address-ATOM_EXT_BASE];
	else if (AddressInBlock(address,ATOM_BAS_BASE,ATOM_ROM_SIZE))
		Result=abasic_rom[address-ATOM_BAS_BASE];
	else if (AddressInBlock(address,ATOM_FP_BASE,ATOM_ROM_SIZE))
		Result=afloat_rom[address-ATOM_FP_BASE];
	else if (AddressInBlock(address,ATOM_DOS_BASE,ATOM_ROM_SIZE))
		Result=atommc2_2_9_e000_rom[address-ATOM_DOS_BASE];
	else if (AddressInBlock(address,ATOM_SYS_BASE,ATOM_ROM_SIZE))
			Result=akernel_rom[address-ATOM_SYS_BASE];

	// 8255
	else if (AddressInBlock(address,ATOM_8255_BASE,ATOM_8255_SIZE))
		Result=Read8255(address);
	else if (AddressInBlock(address,ATOM_PL8_BASE,ATOM_PL8_SIZE))
		Result=AtoMMCRead(address-ATOM_PL8_BASE);
	else if (AddressInBlock(address,ATOM_6522_BASE,ATOM_6522_SIZE))
			Result=AtoMMCRead(address-ATOM_6522_BASE);

	else if (AddressInBlock(address,0xBD00,0x100))
		Result=0xBF;
	else if (AddressInBlock(address,ATOM_IO_BASE,ATOM_IO_SIZE))
	{
		log0("Atom IO read %04X pc=%04X\n",address,pc);
		Result=0;
	}
	else
		Result=address >> 8;

	return Result;
}

// Write handler for emulated 6502
void write6502(uint16_t address, uint8_t value)
{
	if(AddressInBlock(address,0x000,ATOM_ZERO_SIZE))
		ZeroPage[address]=value;
	else if (AddressInBlock(address,ATOM_MAIN_BASE,ATOM_MAIN_SIZE))
		MainRam[address-ATOM_MAIN_BASE]=value;
	else if (AddressInBlock(address,ATOM_VID_BASE,ATOM_VID_SIZE))
	{
		MC6847_UpdateByte((address-ATOM_VID_BASE),value);
	}
	else if (AddressInBlock(address,ATOM_8255_BASE,ATOM_8255_SIZE))
		Write8255(address,value);
	else if (AddressInBlock(address,ATOM_PL8_BASE,ATOM_PL8_SIZE))
		AtoMMCWrite(address,value);
	else if (AddressInBlock(address,ATOM_6522_BASE,ATOM_6522_SIZE))
			Write6522(address,value);
	else if (AddressInBlock(address,ATOM_IO_BASE,ATOM_VID_SIZE))
		log0("Atom IO write %04X, %02X, pc=%04X\n",address,value,pc);
}

uint8_t Read8255(uint16_t	Offset)
{
	uint8_t Result = 0;

	switch (Offset & 0x0003)
	{
		case 0x00	:
			Result=i8255.PortA;
			break;
		case 0x01	:
			// Combine modifier and normal keys, shift & ctrl appear in all rows
			//Keyrow=PortA_8255 && 0x0F;
			Result=(Keyrows[(i8255.PortA & ATOM_KEYROW_MASK)] & ATOM_KEYS_MASK) | (Keyrows[ATOM_MOD_ROW] & ATOM_KEYS_MOD_MASK);
//			if (Result!=0xFF)
//				log0("%s Keyrows[%02d]=%02X\n",regdump(),(i8255.PortA & ATOM_KEYROW_MASK),Result);
			break;
		case 0x02	:
			// Rept key on bit 6
			Result = (Keyrows[ATOM_MOD_ROW] & ATOM_KEYS_REPT_MASK) << 6;

			// frame sync on bit 7
			if(MC6847_Screen.FS)
				Result |= ATOM_FS_INPUT;
			else
				Result &= ~ATOM_FS_INPUT;
			break;
		case 0x03	: break;
	}

	//log0("8255 read : %04X=%02X\n",Offset,Result);

	return Result;
}

void Write8255(uint16_t	Offset, uint8_t Value)
{
	//log0("8255 write : %04X,%02X\n",Offset,Value);

	switch (Offset & 0x0003)
	{
		case 0x00	:
			// Only change 6847 bits if they have really changed.
			if((Value & ATOM_PORT_VID_MASK)	!= (i8255.PortA & ATOM_PORT_VID_MASK))
			{
				MC6847_CondSetFlag(Value & ATOM_AG_MASK,MC6847_AG);
				MC6847_CondSetFlag(Value & ATOM_GM0_MASK,MC6847_GM0);
				MC6847_CondSetFlag(Value & ATOM_GM1_MASK,MC6847_GM1);
				MC6847_CondSetFlag(Value & ATOM_GM2_MASK,MC6847_GM2);
				MC6847_SetMode();
			}
			i8255.PortA = Value;

			//log0("Keyrow set to %d at pc=%04X\n",Keyrow,pc);
			break;
		case 0x01	:
			i8255.PortB = Value;
			break;
		case 0x02	:
			// Only change 6847 bits if they have really changed.
			if((Value & ATOM_CSS_MASK)	!= (i8255.PortC & ATOM_CSS_MASK))
				MC6847_CondSetFlag(Value & ATOM_CSS_MASK,MC6847_CSS);

			i8255.PortC = Value;
			break;
		case 0x03	:


			if ((Value & 0x04) == 0x04)
			{
				if (Value & 0x01)
					gpio_put(SPK_PIN, true);
				else
					gpio_put(SPK_PIN, false);
			}

			i8255.Control = Value;
			break;
	}
}

void Reset6522(void)
{
	uint8_t	Idx;

	for(Idx=0; Idx < VIA_REGS; Idx++)
		via6522.regs[Idx]=0;

	Exec6502=0;
}

uint8_t Read6522(uint16_t	Offset)
{
	uint8_t Result = via6522.regs[Offset & 0xF];

	switch (Offset & 0x0F)
	{
		case 0x00	: break;
		case 0x01	: break;
		case 0x02	: break;
		case 0x03	: break;
		case 0x04	: break;
		case 0x05	: break;
		case 0x06	: break;
		case 0x07	: break;
		case 0x08	: break;
		case 0x09	: break;
		case 0x0A	: break;
		case 0x0B	: break;
		case 0x0C	: break;
		case 0x0D	: break;
		case 0x0E	: break;
		case 0x0F	: break;
	}

	logc0(DEBUG_VIA,"Read6522(%04X)=%02X",Offset,Result);

	return Result;
}

void Write6522(uint16_t	Offset, uint8_t Value)
{
	logc0(DEBUG_VIA,"Write 6522: %04X,%02X\n",Offset,Value);

	switch (Offset & 0x0F)
	{
		case 0x00	: break;
		case 0x01	: break;
		case 0x02	: break;
		case 0x03	: break;
		case 0x04	: break;
		case 0x05	: break;
		case 0x06	: break;
		case 0x07	: break;
		case 0x08	: break;
		case 0x09	: break;
		case 0x0A	: break;
		case 0x0B	: break;
		case 0x0C	: break;
		case 0x0D	: break;
		case 0x0E	: break;
		case 0x0F	: break;
	}

	via6522.regs[(Offset & 0x0f)]=Value;
}
// Keycode contains the key column (b6..b4) and row (b3..b0)
// State is the state of the key 1=keydown, 0=keyup.
// The one exception to this is that REPT sets the keycode to 8A
// this gets translated to Row 10, column 0
// Row 10 contains CTRL & shift in bits 6 and 7, but the key reading
// code above always masks out the bottom 6 bits of this row, which
// is then or'd with the masked bottom 6 bits of each other keyrow.
// This simulates the fact that CTRL and shift appear in the top bits
// whatever keyrow is being read.
void Atom_output_key(uint8_t	KeyCode,
				     uint8_t	State)
{
	uint8_t	Col		= (KeyCode & 0x70) >> 4;
	uint8_t Mask 	= 0x01 << Col;

//	log0("Atom_output_key(%02X,%02X), %02X, %02X\n",KeyCode,State,(KeyCode & 0x0F),Mask);

	if(State)
		Keyrows[KeyCode & 0x0F] &= ~Mask;
	else
		Keyrows[KeyCode & 0x0F] |= Mask;
}

bool atom_timer_callback(struct repeating_timer *t) 
{
#if DEBUG_FREQ == 1
//	gpio_xor_mask(1ul << DEBUG_ATOM_PIN);
#endif
	Exec6502 = true;
	//printf("ticks:%ld\n",(clockticks6502 - clockticks6502last));

	//clockticks6502last = clockticks6502;

	return true;
}
