/*
 * 6847.c
 *
 *  Created on: 8 Aug 2018
 *      Author: phill
 */

#include "6847.h"
#include "fonts.h"
#include "ILI9341_Driver.h"
#include "ILI9341_GFX.h"
#include "ILI9341_DMA.h"
#include "ILI9341_Text.h"
#include "main.h"
#include "Atom.h"

#define DEBUG_FREQ 0

mc6847_t	MC6847_Screen;

// xres and y res are the mode's resolution
// bpp is bits per pixel, 2 for 4 colour modes, 1 for 2 colour modes
// xmul and ymul are the multiplication factors to go from the mode's resolution
// to the maximum 256x192
typedef struct
{
	uint16_t	xres;		// Width in pixels
	uint8_t		yres;		// Width in pixels
	uint8_t 	perline;	// Bytes per line
	uint8_t		bpp;		// Bits per pixel
	uint8_t		pmul;		// Pixels / byte (8/bpp)
	uint8_t		xmul;		// X multiplier to translate this resolution to max (255x192)
	uint8_t		ymul;		// Y multiplier to translate this resolution to max (255x192)
	uint16_t	memuse;		// memory required.
} g_mode_t;

static const g_mode_t ModeTable[] =
{
//		xres	yres	pline	bpp		pmul,	xmul	ymul	memused
	{	64,		64,		16,		2,		8/2,	4,		3,		1024	},			// CG1
	{	128,	64,		16,		1,		8/1,	4,		3,		1024	},			// RG1
	{	128,	64,		32,		2,		8/2,	2,		3,		2048	},			// CG1
	{	128,	96,		32,		1,		8/1,	2,		2,		1536	},			// RG2
	{	128,	96,		32,		2,		8/2,	2,		2,		3072	},			// CG2
	{	128,	192,	32,		1,		8/1,	2,		1,		3072	},			// RG3
	{	128,	192,	32,		2,		8/2,	2,		1,		6144	},			// CG3
	{	256,	192,	32,		1,		8/1,	1,		1,		6144	}			// RG4
};

static const uint16_t CGColours[2][4] =
{
	{	GREEN,	YELLOW,	BLUE,		RED 	},
	{	WHITE,	CYAN,	MAGENTA,	ORANGE 	}
};

static const uint16_t RGColours[2][2] =
{
	{	BLACK,	GREEN 	},
	{	BLACK,	WHITE	}
};

void MC6847_UpdateText(void);
void MC6847_UpdateGraph(void);
void MC6847_AtomChar(uint16_t	Offset, uint8_t Char);
void MC6847_DragonChar(uint16_t	Offset, uint8_t Char);
void MC6847_UpdateGraphByte(uint16_t	Offset, uint8_t	Byte);

#define TIMER_120HZ	1

struct repeating_timer fs_timer;
bool fs_timer_callback(struct repeating_timer *t);

void MC6847_InitTimers(void)
{
	// Initialize 120Hz signal for frame sync
	cancel_repeating_timer(&fs_timer);	// Incase it was already running.....
	add_repeating_timer_us(-8333, fs_timer_callback, NULL, &fs_timer);

#if DEBUG_FREQ == 1
	/* Config PA4 as an output so we can toggle to debug frequency */

	gpio_init(DEBUG_6847_PIN);
	gpio_set_dir(DEBUG_6847_PIN,true);
#endif
}

bool fs_timer_callback(struct repeating_timer *t) 
{
	//printf("Repeat at %lld\n", time_us_64());

	MC6847_Screen.FS = MC6847_Screen.FS ^ 0x01;

#if DEBUG_FREQ == 1
	gpio_xor_mask(1ul << DEBUG_6847_PIN);
#endif

	matrix_check_output();

	return true;
}

void MC6847_Init(uint8_t	*ScreenMem,
				 uint16_t	MemSize,
				 uint16_t	InitFlags)
{
	MC6847_Screen.Flags=InitFlags;
	MC6847_Screen.ScreenMem=ScreenMem;
	MC6847_Screen.MemSize=MemSize;
	MC6847_InitTimers();

	ILI9341_SetFont(&m6847_font);
	ILI9341_Clear_Screen(BLACK,GREEN);
	ILI9341_AllOff();
}

void MC6847_Update(void)
{
	if (MC6847_IsText() || MC6847_IsSemi())
		MC6847_UpdateText();
	else
		MC6847_UpdateGraph();
}

void MC6847_UpdateByte(uint16_t		Offset,
					   uint8_t		Byte)
{
	if(Offset>MC6847_Screen.MemSize)
		return;

	MC6847_Screen.ScreenMem[Offset]=Byte;

	if (!MC6847_IsGraphics())
	{
		if (MC6847_IsSet(MC6847_ATOM))
			MC6847_AtomChar(Offset,Byte);

		if (MC6847_IsSet(MC6847_DRAGON))
			MC6847_DragonChar(Offset,Byte);
	}
	else
	{
		MC6847_UpdateGraphByte(Offset,Byte);

		//log0("MC6847_UpdateByte: Graphics not yet implemented!\n");
	}
}

void MC6847_PrintSemiOrText(uint16_t	Offset,
							uint8_t 	Char)
{
	uint16_t	Colour;
	uint8_t		colour_sel;
	uint8_t		to_print;
	uint8_t		XCo;
	uint8_t		YCo;

	if (MC6847_IsSet(MC6847_INV))
		ILI9341_InverseOn();
	else
		ILI9341_InverseOff();

	if (!MC6847_IsSet(MC6847_AS))
	{
		// Text chars
		ILI9341_SetFont(&m6847_font);

		if(MC6847_IsSet(MC6847_CSS))
			colour_sel = 0x07;
		else
			colour_sel = 0x00;

		to_print = MC6847_ToASCII(Char & MC6847_TEXT_MASK);
	}
	else if (MC6847_IsSet(MC6847_AS) && MC6847_IsSet(MC6847_INT_EXT))
	{
		// Semigraphics 6
		ILI9341_SetFont(&m6847_sg6_font);

		colour_sel = ((Char & MC6847_SG6_COLOUR_MASK) >> 6);

		if(MC6847_IsSet(MC6847_CSS))
			colour_sel |= 0x04;

		to_print = Char & MC6847_SG6_CHAR_MASK;
	}
	else
	{
		// Semigraphics 4
		ILI9341_SetFont(&m6847_sg4_font);

		colour_sel = (Char & MC6847_SG4_COLOUR_MASK) >> 4;
		to_print = Char & MC6847_SG4_CHAR_MASK;
	}

	switch (colour_sel)
	{
		case 0x00	: Colour=GREEN;   break;
		case 0x01	: Colour=YELLOW;  break;
		case 0x02	: Colour=BLUE; 	  break;
		case 0x03	: Colour=RED;     break;
		case 0x04	: Colour=WHITE;   break;
		case 0x05	: Colour=CYAN; 	  break;
		case 0x06	: Colour=MAGENTA; break;
		case 0x07	: Colour=ORANGE;  break;
		default 	: Colour=GREEN;   break;
	}

	ILI9341_Vars.Ink=Colour;
	ILI9341_Vars.Paper=BLACK;

	YCo = Offset / MC6847_TEXT_COL;
	XCo = Offset % MC6847_TEXT_COL;

	if (YCo < MC6847_TEXT_ROW)
	{
		ILI9341_GotoXY(XOFFSET+XCo,YOFFSET+YCo);
		ILI9341_Print_RawChar(to_print);
	}
}

void MC6847_AtomChar(uint16_t	Offset,
					 uint8_t 	Char)

{
	if(Char & 0x40)
		MC6847_Screen.Flags |= MC6847_AS | MC6847_INT_EXT;
	else
		MC6847_Screen.Flags &= ~(MC6847_AS | MC6847_INT_EXT);

	if(Char & 0x80)
		MC6847_Screen.Flags |= MC6847_INV;
	else
		MC6847_Screen.Flags &= ~(MC6847_INV);

	MC6847_PrintSemiOrText(Offset,Char);
}

void MC6847_DragonChar(uint16_t	Offset,
					   uint8_t 	Char)
{
 	if(Char & 0x40)
 		MC6847_Screen.Flags |= MC6847_INV;
	else
		MC6847_Screen.Flags &= ~MC6847_INV;

	if(Char & 0x80)
		MC6847_Screen.Flags |= MC6847_AS;
	else
		MC6847_Screen.Flags &= ~(MC6847_AS);

	MC6847_PrintSemiOrText(Offset,Char);
}


void MC6847_UpdateText(void)
{
	uint8_t	Row;
	uint8_t	Col;
	uint8_t	Char;

	ILI9341_SetFont(&m6847_font);
	ILI9341_AllOff();

	ILI9341_Vars.TextScale=1;

	for(Col=0; Col < MC6847_TEXT_COL; Col++)
	{
		for (Row=0; Row < MC6847_TEXT_ROW; Row++)
		{
			Char=MC6847_Screen.ScreenMem[(Row*MC6847_TEXT_COL)+Col];
			ILI9341_GotoXY(XOFFSET+Col,YOFFSET+Row);

			if (MC6847_IsSet(MC6847_ATOM))
				MC6847_AtomChar((Row*32)+Col,Char);

			if (MC6847_IsSet(MC6847_DRAGON))
				MC6847_DragonChar((Row*32)+Col,Char);
		}
	}
}

void MC6847_UpdateGraphByte(uint16_t	Offset,
					   	    uint8_t		Byte)
{
	uint8_t		Mode 	= MC6847_GetMode();
	uint8_t		XCo		= (Offset % ModeTable[Mode].perline) * ModeTable[Mode].pmul * ModeTable[Mode].xmul;
	uint8_t 	YCo		= (Offset / ModeTable[Mode].perline) * ModeTable[Mode].ymul;
	uint8_t		PxNo;
	uint8_t		PxRep;
	uint8_t		Pixels 	= Byte;
	uint16_t	Colour;
	uint8_t		LineNo;

	uint16_t	LineBuff[32];	// Enough for a bytes worth of pixels at maximum xmul
	uint8_t		LinePtr	= 0;

	if (Offset > ModeTable[Mode].memuse)
		return;

	for(PxNo=0; PxNo < ModeTable[Mode].pmul; PxNo++)
	{
		if (ModeTable[Mode].bpp==2)
			Colour=SwapColour(CGColours[MC6847_GetCSS()][(Pixels & 0xC0) >> 6]);
		else
			Colour=SwapColour(RGColours[MC6847_GetCSS()][(Pixels & 0x80) >> 7]);

		Pixels=Pixels << ModeTable[Mode].bpp;
		for(PxRep=0; PxRep < ModeTable[Mode].xmul; PxRep++)
			LineBuff[LinePtr++]=Colour;
	}

	for(LineNo=YCo; LineNo < YCo+ModeTable[Mode].ymul; LineNo++)
	{
		ILI9341_WriteScreen(XOFFSET+XCo,YOFFSET+LineNo,XOFFSET+XCo+LinePtr,YOFFSET+LineNo,(uint8_t *)LineBuff,LinePtr*2,BLOCK_DUMMY_RECEIVE);
	}
}

void MC6847_UpdateGraph(void)
{
	uint8_t		Mode 	= MC6847_GetMode();
	uint8_t		XCo;
	uint8_t 	YCo;
	uint16_t	PxPtr	= 0;
	uint8_t		Pixels;
	uint8_t		PxNo;
	uint8_t		PxRep;
	uint16_t	Colour;
	uint16_t	LineBuff[255];	// Enough for a line of pixels
	uint8_t		LinePtr;
	uint8_t		LineNo;

	for(YCo=0; YCo < ModeTable[Mode].yres; YCo++)
	{
		LinePtr=0;
		for(XCo=0; XCo < ModeTable[Mode].perline; XCo++)
		{
			Pixels=MC6847_Screen.ScreenMem[PxPtr++];
			for(PxNo=0; PxNo < ModeTable[Mode].pmul; PxNo++)
			{
				if (ModeTable[Mode].bpp==2)
					Colour=SwapColour(CGColours[MC6847_GetCSS()][(Pixels & 0xC0) >> 6]);
				else
					Colour=SwapColour(RGColours[MC6847_GetCSS()][(Pixels & 0x80) >> 7]);

				Pixels=Pixels << ModeTable[Mode].bpp;

				for(PxRep=0; PxRep < ModeTable[Mode].xmul; PxRep++)
					LineBuff[LinePtr++]=SwapColour(Colour);
			}
		}
		for(LineNo=(YCo*ModeTable[Mode].ymul); LineNo < (YCo*ModeTable[Mode].ymul)+ModeTable[Mode].ymul; LineNo++)
		{
			ILI9341_WriteScreen(XOFFSET,YOFFSET+LineNo,XOFFSET+255,YOFFSET+LineNo+1,(uint8_t *)LineBuff,512,BLOCK_DUMMY_RECEIVE);
		}
	}
}

uint8_t MC6847_ToASCII(uint8_t	VDGChar)
{
	uint8_t Result;

	VDGChar &= 0x3F;

	if (((VDGChar >= 0 ) && (VDGChar < 31 )))
		Result=VDGChar+64;
//	else if (((VDGChar >= 32 ) && (VDGChar < 63 )))
//		Result=VDGChar+32;
	else
		Result=VDGChar;

	return Result;
}

uint8_t MC6847_ToVDG(uint8_t	ASCIIChar)
{
	uint8_t Result;

	if ((ASCIIChar >= 32 ) && (ASCIIChar < 64 ))
		Result=ASCIIChar+64;
//	else if (((ASCIIChar >= 64 ) && (ASCIIChar < 96 )))
//		Result=ASCIIChar+32;
	else if (((ASCIIChar >= 96 ) && (ASCIIChar < 128 )))
		Result=ASCIIChar-96;
	else
		Result=ASCIIChar;

	return Result;
}

void MC6847_WriteASCII(char 	*ToWrite,
					   uint16_t AtPos)
{
	uint8_t	Idx;

	for(Idx=0; Idx < strlen(ToWrite); Idx++)
		MC6847_Screen.ScreenMem[AtPos++]=MC6847_ToVDG(ToWrite[Idx]);
}

void MC6847_SetMode(void)
{
	if(!MC6847_IsGraphics())
		MC6847_UpdateText();
	else
		MC6847_UpdateGraph();
}
