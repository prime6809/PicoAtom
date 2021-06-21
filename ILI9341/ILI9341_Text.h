/*
 * ILI9341_Text.h
 *
 *  Created on: 28 Jul 2018
 *      Author: afra
 */

#include "ILI9341_Driver.h"
#include "fonts.h"

#ifndef ILI9341_ILI9341_TEXT_H_
#define ILI9341_ILI9341_TEXT_H_

// Flags bit values
#define INVERSE_FONT		0x0001
#define SCROLL_SCREEN		0x0002
#define TRANSPARENT_PRINT	0x0004

#define DGN_DEBUG			0x80
#define DGN_SAVE			0x40
#define DGN_INTERSECT		0x20

#define DEFAULT_INK			BLACK
#define DEFAULT_PAPER		WHITE

typedef struct
{
	font_t 		*Font; 			// Current Font.
	uint16_t	Flags; 			// Flags, inverse etc.
	uint16_t	Ink;			// Current text colour
	uint16_t	Paper;			// Current background colour.
	uint16_t	PrintX;			// Current print position X
	uint16_t	PrintY;			// Current print position Y
	uint16_t	TextScale;		// Current font scale

	uint16_t	GPosX;			// Current graphics pos X
	uint16_t	GPosY;			// Current graphics pos Y
	uint16_t	GInk;			// Current grapics colour colour
	uint16_t	GPaper;			// Current grapics background colour.

} screen_vars_t;

extern screen_vars_t	ILI9341_Vars;

#define ILI9341_FlagIsOn(Flag)	(ILI9341_Vars.Flags & Flag)

#define ILI9341_InverseOn()		(ILI9341_Vars.Flags |= INVERSE_FONT)
#define ILI9341_InverseOff()	(ILI9341_Vars.Flags &= ~INVERSE_FONT)
#define ILI9341_InverseIsOn()	(ILI9341_Vars.Flags & INVERSE_FONT)

#define ILI9341_ScrollOn()		(ILI9341_Vars.Flags |= SCROLL_SCREEN)
#define ILI9341_ScrollOff()		(ILI9341_Vars.Flags &= ~SCROLL_SCREEN)
#define ILI9341_ScrollIsOn()	(ILI9341_Vars.Flags & SCROLL_SCREEN)

#define ILI9341_TransparentOn()		(ILI9341_Vars.Flags |= TRANSPARENT_PRINT)
#define ILI9341_TransparentOff()	(ILI9341_Vars.Flags &= ~TRANSPARENT_PRINT)
#define ILI9341_TransparentIsOn()	(ILI9341_Vars.Flags & TRANSPARENT_PRINT)

#define ILI9341_DgnDebugOn()	(ILI9341_Vars.Flags |= DGN_DEBUG)
#define ILI9341_DgnDebugOff()	(ILI9341_Vars.Flags &= ~DGN_DEBUG)
#define ILI9341_DgnDebugIsOn()	(ILI9341_Vars.Flags & DGN_DEBUG)

#define ILI9341_AllOff()		ILI9341_Vars.Flags=0

void ILI9341_Clear_Screen(uint16_t	Foreground, uint16_t Background);
void ILI9341_Draw_Char(char Character, uint16_t X, uint16_t Y, uint16_t Colour, uint16_t Size, uint16_t Background_Colour);
void ILI9341_Draw_Text(const char* Text, uint16_t X, uint16_t Y, uint16_t Colour, uint16_t Size, uint16_t Background_Colour);

#define ILI9341_Draw_Char_Def(Character, X, Y, Size)	ILI9341_Draw_Char(Character, X, Y, ILI9341_Vars.Ink, Size, ILI9341_Vars.Paper);
#define ILI9341_Draw_Text_Def(Text, X, Y, Size)			ILI9341_Draw_Text(Text, X, Y, ILI9341_Vars.Ink, Size, ILI9341_Vars.Paper);

void ILI9341_Print_NewLine(void);
//uint8_t ILI9341_Print_CharWillFit(void);
void ILI9341_Print_RawChar(char Character);
void ILI9341_Print_CharNL(char 		Character,
						  uint8_t	DoNewline);
void ILI9341_Print_Text(const char* Text);
void ILI9341_Print_Coloured_Text(uint16_t	Colour, const char* Text);
void ILI9341_Print_TextCount(const char	*Text,
							 uint16_t	MaxLen);


#define ILI9341_Print_Char(ch)		ILI9341_Print_CharNL(ch,1)

#define ILI9341_CharWidth()			(ILI9341_Vars.Font->Width * ILI9341_Vars.TextScale)
#define ILI9341_CharHeight()		(ILI9341_Vars.Font->Height * ILI9341_Vars.TextScale)
#define ILI9341_Print_CharWillFit()	((ILI9341_Vars.PrintX + ILI9341_CharWidth()) <= LCD_WIDTH)
#define ILI9341_GotoXY(x,y)			do { ILI9341_Vars.PrintX = x * ILI9341_Vars.TextScale * ILI9341_Vars.Font->Width; ILI9341_Vars.PrintY = y * ILI9341_Vars.TextScale * ILI9341_Vars.Font->Height; } while(0)

#define ILI9341_SetFont(NewFont)	ILI9341_Vars.Font=(font_t *)NewFont

#endif /* ILI9341_ILI9341_TEXT_H_ */
