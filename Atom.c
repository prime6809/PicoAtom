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

#define DEBUG_VIA		0
#define DEBUG_VIA_T1	1
#define DEBUG_VIA_T2	1

typedef struct Atom
{
	uint16_t	Timer;
	uint16_t	Latch;
} via6522_timer_t;

typedef struct
{
	uint8_t	DataA;
	uint8_t	DataB;
	uint8_t DDRAR;
	uint8_t DDRBR;

	via6522_timer_t	Timer1;
	via6522_timer_t	Timer2;

	uint8_t	ShiftR;
	uint8_t PeriperalC;
	uint8_t AuxC;
	uint8_t IntFlag;
	uint8_t IntEnable;

} via6522_t;

via6522_t via6522;

char * VIARegNames[] = { "DatB", "DatA", "DDRB", "DDRA", "T1CL", "T1CH", "T1LL", "T1LH", "T2CL", "T2CH", "SR", "ACR", "PCR", "IFR", "IER", "DA2" };

#define LOAD_BUFF_LEN	4
uint8_t	LoadCharBuff[LOAD_BUFF_LEN];

uint32_t AtomOldTick = 0;

//extern uint16_t pc;

#define AddressInRange(addr, begin, end)	((addr >= begin) && (addr < end))
#define AddressInBlock(addr, begin, size)	((addr >= begin) && (addr < (begin+size)))

uint8_t Read8255(uint16_t	Offset);
void Write8255(uint16_t	Offset, uint8_t Value);
void Reset6522(void);
uint8_t Read6522(uint16_t	Offset);
void Write6522(uint16_t	Offset, uint8_t Value);
void Update6522(uint32_t Ticks);

struct repeating_timer atom_timer;
bool atom_timer_callback(struct repeating_timer *t);

#define DEBUG_FREQ 		0
#define DEBUG_ATOM_PIN	17
#define DEBUG_EXEC_PIN	18

#define CYCLES_PER_TIMER	(32*262)
//#define CYCLES_PER_TIMER	(64)

volatile uint32_t clockticks6502last = 0;
volatile bool Run6502;
volatile bool Throttle6502 = 1;

// Screensaver
#define SS_STOPED	0
#define SS_START	1
#define SS_RUN		1
#define SS_STOP		2

volatile uint32_t   countdown   = SCREENSAVER_COUNT;
volatile uint8_t    screensaver = SS_STOPED;

void ScreenSaverStart(void);
void ScreenSaverRun(void);
void ScreenSaverEnd(void);

void AtomInit(void)
{
	uint8_t				Idx;

	log0("Atom Init\n");
	// Set speaker as output	
	gpio_init(SPK_PIN);
	gpio_set_dir(SPK_PIN, true);

	// Set keys to all released
  	for(Idx=0; Idx < ATOM_KEYROWS; Idx++)
		Keyrows[Idx]=0xFF;

#if 0
	// Initialize 1Hz signal for speed measurement
	cancel_repeating_timer(&atom_timer);	// Incase it was already running.....
	add_repeating_timer_us(-100, atom_timer_callback, NULL, &atom_timer);
	//add_repeating_timer_ms(-1000, atom_timer_callback, NULL, &atom_timer);
#endif 

#if DEBUG_FREQ == 1
	gpio_init(DEBUG_ATOM_PIN);
	gpio_set_dir(DEBUG_ATOM_PIN,true);
	gpio_init(DEBUG_EXEC_PIN);
	gpio_set_dir(DEBUG_EXEC_PIN,true);
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
	Run6502 = false;
}

void AtomGo(void)
{
	log0("AtomGo()\n");

	while (1)
	{
		if ((Run6502) && (SS_STOPED == screensaver))
		{
			Run6502 = Throttle6502 ? false : true;
			exec6502(CYCLES_PER_TIMER);
		}
		else if (SS_RUN == screensaver)
		{
			ScreenSaverRun();
		} 
		else if (SS_STOP == screensaver)
		{
			ScreenSaverEnd();
		}
	}
}

void AtomHook(void)
{
	uint32_t TicksPast = clockticks6502 - AtomOldTick;

	AtomOldTick = clockticks6502;

	Update6522(TicksPast);

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
		Result=Read6522(address);

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
	via6522.DataA = 0;
	via6522.DataB = 0;
	via6522.DDRAR = 0;
	via6522.DDRBR = 0;
	via6522.Timer1 = (via6522_timer_t){0,0};
	via6522.Timer2 = (via6522_timer_t){0,0};
	via6522.ShiftR = 0;
	via6522.PeriperalC = 0;
	via6522.AuxC = 0;
	via6522.IntFlag = 0;
	via6522.IntEnable = 0;

	Run6502=0;
}

char *VIARegName(uint16_t Offset)
{
	return VIARegNames[(Offset & 0x0f)];
}

uint8_t Read6522(uint16_t	Offset)
{
	uint8_t Result = 0xFF;

	switch (Offset & 0x0F)
	{
		case DATAB	:
			Result = via6522.DataB; 
			break;

		case DATAA	: break;
			Result = via6522.DataA; 
			break;

		case DDRB	: break;
			Result = via6522.DDRBR; 
			break;

		case DDRA	: break;
			Result = via6522.DDRAR; 
			break;

		case T1CL	: 
			Result=via6522.Timer1.Timer & 0xFF;
			break;

		case T1CH	: 
			Result=via6522.Timer1.Timer >> 8;
			via6522.IntFlag &= ~INTT1;
			break;

		case T1LL	: 
			Result=via6522.Timer1.Latch & 0xFF;
			break;

		case T1LH	:
			Result=via6522.Timer1.Latch >> 8; 
			break;

		case T2CL	: 
			Result=via6522.Timer2.Timer & 0xFF;
			break;

		case T2CH	:
			Result=via6522.Timer2.Timer >> 8;
			break;

		case SHIFT	: 
			Result=via6522.ShiftR;
			break;

		case AUXCR	:
			Result=via6522.AuxC; 
			break;

		case PCR	: 
			Result=via6522.PeriperalC;
			break;

		case INTFR	:
			Result=via6522.IntFlag; 
			break;

		case INTER	:
			Result=via6522.IntEnable | 0x80; 
			break;

		case DATAAA	:
			Result=via6522.DataA; 
			break;
	}

	logc0(DEBUG_VIA,"Read6522(%04X)[%s]=%02X\n",Offset,VIARegName(Offset),Result);

	return Result;
}

void Write6522(uint16_t	Offset, uint8_t Value)
{
	uint16_t ValueH	= Value << 8;
	logc0(DEBUG_VIA,"Write 6522: %04X [%s],%02X\n",Offset,VIARegName(Offset),Value);

	switch (Offset & 0x0F)
	{
		case DATAB	:
			via6522.DataB = Value; 
			break;

		case DATAA	: break;
			via6522.DataA = Value; 
			break;

		case DDRB	: break;
			via6522.DDRBR = Value; 
			break;

		case DDRA	: break;
			via6522.DDRAR = Value; 
			break;

		case T1CL	: 
			via6522.Timer1.Latch = (via6522.Timer1.Latch & 0xFF00) | Value;
			break;

		case T1CH	: 
			via6522.Timer1.Latch = (via6522.Timer1.Latch & 0x00FF) | ValueH;
			via6522.Timer1.Timer = via6522.Timer1.Latch;
			via6522.IntFlag &= INTT1;
			break;

		case T1LL	: 
			via6522.Timer1.Latch = (via6522.Timer1.Latch & 0xFF00) | Value;
			break;

		case T1LH	:
			via6522.Timer1.Latch =  (via6522.Timer1.Latch & 0x00FF) | ValueH;
			break;

		case T2CL	: 
			via6522.Timer2.Latch = (via6522.Timer2.Latch & 0xFF00) | Value;
			break;

		case T2CH	:
			via6522.Timer2.Timer = (via6522.Timer2.Timer & 0x00FF) | ValueH;
			break;

		case SHIFT	: 
			via6522.ShiftR = Value;
			break;

		case AUXCR	:
			via6522.AuxC = Value; 
			break;

		case PCR	: 
			via6522.PeriperalC = Value;
			break;

		case INTFR	:
			via6522.IntFlag  = Value; 
			break;

		case INTER	:
			if (Value & 0x80)
				via6522.IntEnable = via6522.IntEnable | (Value & 0x7F);
			else
				via6522.IntEnable = via6522.IntEnable & ~(Value & 0x7F);
			break;

		case DATAAA	:
			via6522.DataA = Value; 
			break;
	}

}

void Update6522(uint32_t Ticks)
{
	uint8_t flags_set;
	// detect timers reaching zero (or -ve), we have to do this here like this
	// as via6522.timerX use unsigned ints which will wrap......
	bool uft1	= via6522.Timer1.Timer <= Ticks;
	bool uft2	= via6522.Timer2.Timer <= Ticks;
	
	via6522.Timer1.Timer -= Ticks;
	via6522.Timer2.Timer -= Ticks;

	// If T1 or T2 has reached 0 set the relevant interrupt flag.
	via6522.IntFlag |= uft1 ? INTT1 : 0;
	via6522.IntFlag |= uft2 ? INTT2 : 0;
	flags_set = via6522.IntFlag & (INTT1 | INTT2);

	// Check if any int flags are set *AND* their int enables
	if ((flags_set & (via6522.IntEnable & (INTT1 | INTT2))) ||
	    (flags_set && (via6522.IntEnable & INTANY)))
	{
		irq6502();
	}	
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

	countdown = SCREENSAVER_COUNT;

	if(SS_RUN == screensaver)
	{
		screensaver = SS_STOP;
	}	
}

void ScreenSaverPoll(void)
{
    if ((0 == countdown) && (!screensaver))
    {
		ScreenSaverStart();
    }
    else
        countdown--;
}

void ScreenSaverStart(void)
{
	screensaver = SS_START;
	log0("Screensaver on!\n");
	ILI9341_Clear_Screen(GREEN,BLACK);
	ILI9341_SetFont(&beeb_font);
	ILI9341_Vars.TextScale = 2;
}

uint16_t RandomColour()
{
	uint16_t	Result = rand() % 8;

	switch (Result)
	{
		case 0x00	: Result=GREEN;   break;
		case 0x01	: Result=YELLOW;  break;
		case 0x02	: Result=BLUE; 	  break;
		case 0x03	: Result=RED;     break;
		case 0x04	: Result=WHITE;   break;
		case 0x05	: Result=CYAN; 	  break;
		case 0x06	: Result=MAGENTA; break;
		case 0x07	: Result=ORANGE;  break;
		default 	: Result=GREEN;   break;
	}
	return Result;
}

#define OUTBUF_LEN	32

void ScreenSaverRun(void)
{
	char 	Text[] = " PicoAtom - press a key.....";
	char	OutBuf[OUTBUF_LEN+1] = "\0";
	int		ScreenWidth 	= ILI9341_ScreenCharWidth();
	int 	ScreenHeight 	= ILI9341_ScreenCharHeight();
	int 	charpos;
	int		firstchar;
	int 	lastchar;
	int		XPos;
	int 	YPos			= rand() % ScreenHeight;

	log0("ScreenWidth=%d, ScreenHeight=%d\n",ScreenWidth,ScreenHeight);

	ILI9341_Vars.Ink=BLACK;
	ILI9341_Vars.Paper=RandomColour();

	for(charpos = 0-strlen(Text); (charpos < ScreenWidth) && (SS_RUN == screensaver); charpos++)
	{
		_delay_ms(125);	

		XPos		= (charpos < 0 ) ? 0 : charpos;
		firstchar 	= (charpos >= 0) ? 0 : charpos*-1;
		lastchar	= firstchar+(ScreenWidth-(XPos + firstchar))+1; 
		
		snprintf(OutBuf,lastchar,"%s",&Text[firstchar]);

		ILI9341_GotoXY(XPos,YPos);
		ILI9341_Print_Text(OutBuf);
	}
}

void ScreenSaverEnd(void)
{
	log0("Screensaver off!\n");
	MC6847_ReInit();
	MC6847_Update();

	screensaver = SS_STOPED;
}
