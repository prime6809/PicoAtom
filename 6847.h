/*
 * 6847.h
 *
 *  Created on: 8 Aug 2018
 *      Author: phill
 */
#include "main.h"
#include "delays.h"
#include "status.h"

#ifndef mc6847_H_
#define mc6847_H_

// Motorola mc6847 emulation.

typedef struct
{
	uint8_t		*ScreenMem;		// Screen memory
	uint16_t	Flags;			// Screen flags A/G, A/S etc.
	uint16_t	MemSize;		// Size of screen buffer
	uint16_t	Line;			// Line no for HS/VS emulation
	uint8_t		HS;				// Horizontal Sync
	uint8_t		FS;				// Frame sync
} mc6847_t;

extern mc6847_t	MC6847_Screen;

#define DEBUG_6847_PIN		16

// Flags that control operation, on the real chip these are mostly io lines
#define MC6847_GM0			0x0001
#define MC6847_GM1			0x0002
#define MC6847_GM2			0x0004

#define MC6847_MODE_MASK	(MC6847_GM0 | MC6847_GM1 | MC6847_GM2)

#define MC6847_AG			0x0010
#define MC6847_AS			0x0020
#define MC6847_CSS			0x0040
#define MC6847_INV			0x0080
#define MC6847_INT_EXT		0x0100

#define MC6847_ATOM			0x1000
#define MC6847_DRAGON		0x2000

#define MC6847_TEXT_MASK		0x7F
#define MC6847_SG_MASK			0x80
#define MC6847_SG4_COLOUR_MASK	0x70
#define MC6847_SG4_CHAR_MASK	0x0F
#define MC6847_SG6_COLOUR_MASK	0xC0
#define MC6847_SG6_CHAR_MASK	0x3F

#define XOFFSET					32
#define YOFFSET					24

#define MC6847_GetMode()		(MC6847_Screen.Flags & MC6847_MODE_MASK)
#define MC6847_IsGraphics()		(MC6847_Screen.Flags & MC6847_AG)
#define MC6847_IsSemi()			(MC6847_Screen.Flags & MC6847_AS)
#define MC6847_IsText()			((MC6847_Screen.Flags & (MC6847_AS | MC6847_AG)) == 0)

#define MC6847_IsSet(flag)				((MC6847_Screen.Flags & flag) == flag)
#define MC6847_SetFlag(flag)			MC6847_Screen.Flags |= flag
#define MC6847_ResetFlag(flag)			MC6847_Screen.Flags &= ~flag
#define MC6847_CondSetFlag(cond,flag)	do { if (cond) MC6847_Screen.Flags |= flag; else MC6847_Screen.Flags &= ~flag; } while (0)

#define MC6847_GetCSS()			((MC6847_Screen.Flags & MC6847_CSS) ? 1 : 0)

#define MC6847_MEM_SIZE		1024*6

#define MC6847_TEXT_ROW		16
#define MC6847_TEXT_COL		32

#define MC6847_SCANLINES	262

void MC6847_Init(uint8_t *ScreenMem, uint16_t MemSize, uint16_t InitFlags);
void MC6847_Update(void);
void MC6847_UpdateByte(uint16_t	Offset, uint8_t	Byte);
void MC6847_WriteASCII(char *ToWrite, uint16_t AtPos);
uint8_t MC6847_ToASCII(uint8_t	VDGChar);
uint8_t MC6847_ToVDG(uint8_t	ASCIIChar);
void MC6847_SetMode(void);

#endif /* 6847_H_ */
