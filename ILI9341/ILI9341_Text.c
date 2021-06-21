/*
 * ILI9341_Text.c
 *
 *  Created on: 28 Jul 2018
 *      Author: afra
 */

#include "ILI9341_Driver.h"
#include "ILI9341_GFX.h"
#include "ILI9341_DMA.h"
#include "ILI9341_Text.h"
#include "fonts.h"
#include "status.h"

// Print variables
screen_vars_t	ILI9341_Vars	=
{
	(font_t *)&font_5x5,	// Default Font
	0,						// Default flags
	DEFAULT_INK,			// Default text font colour
	DEFAULT_PAPER,			// Default text background colour
	0,						// Default print position X
	0,						// Default print position Y
	1,						// Default font scale
	0,						// Default grapics co-ordinates
	0,
	DEFAULT_INK,			// Default graphics colour
	DEFAULT_PAPER			// Default graphics background colour
};

/* Clear screen and set current ink (font colour) and paper (background)     */
/* colours. These can also be changed by changing ILI9341_Vars.Ink and       */
/* ILI9341_Vars.paper.                                                       */
void ILI9341_Clear_Screen(uint16_t	Foreground, uint16_t Background)
{
	ILI9341_Fill_Screen(Background);

	ILI9341_Vars.Paper=Background;
	ILI9341_Vars.Ink=Foreground;
	ILI9341_Vars.PrintX=0;
	ILI9341_Vars.PrintY=0;

	ILI9341_Vars.GPaper=Background;
	ILI9341_Vars.GInk=Foreground;
	ILI9341_Vars.GPosX=0;
	ILI9341_Vars.GPosY=0;
}

/* Draws a character in the current font specified in ILI9341_Vars.Font      */
/* at X,Y location with specified colour, size and Background colour.        */
/* Also takes account of ILI9341_Vars.Flags for inverse settings etc.        */
/* See fonts.h implementation of fonts on what is required for changing to a */
/* different font when switching fonts libraries                             */
void ILI9341_Draw_Char(
		char 		Character,
		uint16_t 	Xpos,
		uint16_t 	Ypos,
		uint16_t 	Colour,
		uint16_t 	Size,
		uint16_t 	Background_Colour)
{
	uint8_t 	function_char;
    uint8_t 	ChrX,ChrY;
    uint8_t		*DataPtr;
    uint8_t		Inverse = (ILI9341_Vars.Font->Flags & FONT_INVERSE);
    uint16_t	Foreground;
    uint16_t	Background;

    if (ILI9341_InverseIsOn())
    {
    	Foreground=Background_Colour;
    	Background=Colour;
    }
    else
    {
    	Foreground=Colour;
    	Background=Background_Colour;
    }

	function_char = Character;

	if ((function_char < ILI9341_Vars.Font->First) || (function_char > ILI9341_Vars.Font->Last))
    	function_char = 0;
    else
        function_char -= ILI9341_Vars.Font->First;

    if (FONT_OR_YX & ILI9341_Vars.Font->Flags)
    	DataPtr=(ILI9341_Vars.Font->Data + ((ILI9341_Vars.Font->Width) * function_char));
	else
    	DataPtr=(ILI9341_Vars.Font->Data + ((ILI9341_Vars.Font->Height) * function_char));

    // Draw pixels
    if(!ILI9341_TransparentIsOn())
    {
    	ILI9341_Draw_Rectangle(Xpos, Ypos, ILI9341_Vars.Font->Width * Size,ILI9341_Vars.Font->Height * Size, Background);
    }

	if (FONT_OR_YX & ILI9341_Vars.Font->Flags)
    {
    	for (ChrX=0; ChrX < ILI9341_Vars.Font->Width; ChrX++)
    	{
    		for (ChrY=0; ChrY < ILI9341_Vars.Font->Height; ChrY++)
    		{
    			if (DataPtr[ChrX] & (1<<ChrY))
    			{
    				if(Size == 1)
    					ILI9341_Draw_Pixel(Xpos+ChrX, Ypos+ChrY, Foreground);
    				else
    					ILI9341_Draw_Rectangle(Xpos+(ChrX*Size), Ypos+(ChrY*Size), Size, Size, Foreground);
    			}
    		}
    	}
    }
    else
    {
    	for (ChrY=0; ChrY < ILI9341_Vars.Font->Height; ChrY++)
    	{
    		for (ChrX=0; ChrX < (ILI9341_Vars.Font->Width) ; ChrX++)
    		{
    			if (((DataPtr[ChrY] & (1<<ChrX)) && !Inverse)  ||
    				((~DataPtr[ChrY] & (1<<ChrX)) && Inverse))
    			{
    				if(Size == 1)
    					ILI9341_Draw_Pixel(Xpos+(ILI9341_Vars.Font->Width-(ChrX+1)), Ypos+ChrY, Foreground);
    				else
    					ILI9341_Draw_Rectangle(Xpos+((ILI9341_Vars.Font->Width-(ChrX+1))*Size), Ypos+(ChrY*Size), Size, Size, Foreground);
    			}
    		}
    	}
    }
}

// Enough for 8x12 character at scale 2 at 2 bytes per pixel....
#define FAST_CHAR_SIZE	(8*2)*(12*2)*2

void ILI9341_Draw_CharFast(
		char 		Character,
		uint16_t 	Xpos,
		uint16_t 	Ypos,
		uint16_t 	Colour,
		uint16_t 	Size,
		uint16_t 	Background_Colour)
{
	uint16_t	CharBuff[FAST_CHAR_SIZE];
	uint16_t	MemNeeded;

    uint8_t 	ChrX,ChrY;
    uint8_t		ScaleX,ScaleY;
    uint8_t		*DataPtr;
    uint16_t	PixelNo = 0;
    uint8_t		Inverse = (ILI9341_Vars.Font->Flags & FONT_INVERSE);
    uint16_t	Foreground;
    uint16_t	Background;
    uint8_t		Mask;

	MemNeeded=(ILI9341_Vars.Font->Width * ILI9341_Vars.Font->Height * (Size*Size))*2;

	// Pass on to normal slow version if not enough memory, font is YX order fonts,
	// or character is out of range, or transparent is set in options, as fast char can
	// not draw transparent.
	if(((MemNeeded > FAST_CHAR_SIZE) || (ILI9341_Vars.Font->Flags & FONT_OR_YX)) ||
	   ((Character < ILI9341_Vars.Font->First) || (Character > ILI9341_Vars.Font->Last)) ||
	   (ILI9341_Vars.Flags & TRANSPARENT_PRINT))
	{
		ILI9341_Draw_Char(Character,Xpos,Ypos,Colour,Size,Background_Colour);
		return ;
	}

	ILISelect();
	SetPixelFormat(PX_DEFAULT);

	if (ILI9341_InverseIsOn())
    {
    	Foreground=(Background_Colour >> 8) | (Background_Colour << 8);
    	Background=(Colour >> 8) | (Colour << 8);
    }
    else
    {
    	Foreground=(Colour >> 8) | (Colour << 8);
    	Background=(Background_Colour >> 8) | (Background_Colour << 8);
    }

	DataPtr=(ILI9341_Vars.Font->Data + ((ILI9341_Vars.Font->Height) * (Character - ILI9341_Vars.Font->First)));

	for (ChrY=0; ChrY < ILI9341_Vars.Font->Height; ChrY++)
	{
		for(ScaleY=0; ScaleY < Size; ScaleY++)
		{
			Mask=0x80;
			for (ChrX=(ILI9341_Vars.Font->Width); ChrX > 0; ChrX--)
			{
				for(ScaleX=0; ScaleX < Size; ScaleX++)
				{
					if (((DataPtr[ChrY] & Mask) && !Inverse)  || ((~DataPtr[ChrY] & Mask) && Inverse))
						CharBuff[PixelNo++]=Foreground;
					else
						CharBuff[PixelNo++]=Background;
				}
				Mask=Mask >> 1;
			}
		}
	}

	ILI9341_WriteScreen(Xpos,Ypos,(Xpos+(ILI9341_Vars.Font->Width*Size)-1),(Ypos+(ILI9341_Vars.Font->Height*Size)-1),(uint8_t *)CharBuff,MemNeeded,BLOCK_DUMMY_RECEIVE);
}

/* Draws an array of characters at X,Y location with specified font colour,    */
/* size and Background colour                                                  */
void ILI9341_Draw_Text(const char* Text, uint16_t X, uint16_t Y, uint16_t Colour, uint16_t Size, uint16_t Background_Colour)
{
    while (*Text)
    {
        ILI9341_Draw_Char(*Text++, X, Y, Colour, Size, Background_Colour);
        X += ILI9341_Vars.Font->Width*Size;
    }
}

/* The ILI9351_Print_ functions are used to effectively emulate a text mode  */
/* interface where text is printed on a character grid, wraps and moves to   */
/* a new character line when wrapping. This is controlled byt the current    */
/* font dimenstions and the PrintX and PrintY feilds of ILI9341_Vars.        */

/* Move to a new line on the LCD, this sets the PrintX to 0, and advances    */
/* PrintY to the next line, if this is greater than the screen height, then  */
/* it (currently) wrapps the screen                                          */

void ILI9341_Print_NewLine(void)
{
	ILI9341_Vars.PrintX=0;
	ILI9341_Vars.PrintY += ILI9341_CharHeight();
	if ((ILI9341_Vars.PrintY + ILI9341_CharHeight()) > LCD_HEIGHT)
	{
		ILI9341_Vars.PrintY -= ILI9341_CharHeight();

		if (ILI9341_ScrollIsOn())
		{
			ILI9341_Scroll_Screen(ILI9341_Vars.Font->Height * ILI9341_Vars.TextScale);
			ILI9341_Draw_Filled_Rectangle_Coord(0,ILI9341_Vars.PrintY,LCD_WIDTH,LCD_HEIGHT,ILI9341_Vars.Paper);
		}
		else
			ILI9341_Vars.PrintY=0;
	}
}

/* Print the specifed character at the current print position, wrapping      */
/* if needed. Takes care of moving to a new line if a \n is passed           */
void ILI9341_Print_CharNL(char 		Character,
						  uint8_t	DoNewline)
{
	// Check to see if the character will fit on the current line
	// If not move to the next line
	if (!ILI9341_Print_CharWillFit() || ('\n' == Character))
		ILI9341_Print_NewLine();

	if ('\n' != Character)
		ILI9341_Print_RawChar(Character);
}

void ILI9341_Print_RawChar(char Character)
{
	ILI9341_Draw_CharFast(Character, ILI9341_Vars.PrintX, ILI9341_Vars.PrintY, ILI9341_Vars.Ink, ILI9341_Vars.TextScale, ILI9341_Vars.Paper);
	ILI9341_Vars.PrintX += ILI9341_CharWidth();
}

/* Print a string on the LCD taking care of wrapping etc.                    */
void ILI9341_Print_Text(const char* Text)
{
	while (*Text)
		ILI9341_Print_Char(*Text++);
}

void ILI9341_Print_Coloured_Text(uint16_t	Colour, const char* Text)
{
	uint16_t	SaveColour = ILI9341_Vars.Ink;

	ILI9341_Vars.Ink=Colour;
	ILI9341_Print_Text(Text);
	ILI9341_Vars.Ink=SaveColour;
}

void ILI9341_Print_TextCount(const char	*Text,
							 uint16_t	MaxLen)
{
	uint16_t	Count;

	for(Count=0; Count < MaxLen; Count++)
		ILI9341_Print_RawChar(*Text++);
}


