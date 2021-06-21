//	MIT License
//
//	Copyright (c) 2017 Matej Artnak
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in all
//	copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//	SOFTWARE.
//
//
//
//-----------------------------------
//	ILI9341 GFX library for STM32
//-----------------------------------
//
//	Very simple GFX library built upon ILI9342_STM32_Driver library.
//	Adds basic shapes, image and font drawing capabilities to ILI9341
//
//	Library is written for STM32 HAL library and supports STM32CUBEMX. To use the library with Cube software
//	you need to tick the box that generates peripheral initialization code in their own respective .c and .h file
//
//
//-----------------------------------
//	How to use this library
//-----------------------------------
//
//	-If using MCUs other than STM32F7 you will have to change the #include "stm32f7xx_hal.h" in the ILI9341_GFX.h to your respective .h file
//
//	If using "ILI9341_STM32_Driver" then all other prequisites to use the library have allready been met
//	Simply include the library and it is ready to be used
//
//-----------------------------------

#include "fonts.h"

#ifndef ILI9341_GFX_H
#define ILI9341_GFX_H

#define HORIZONTAL_IMAGE	0
#define VERTICAL_IMAGE		1

typedef struct
{
	uint8_t	Blue;
	uint8_t	Green;
	uint8_t	Red;
} bgr_t;

#define BGRIsEqual(b1,b2)	 (((b1)->Blue == (b2)->Blue) && ((b1)->Green == (b2)->Green) && ((b1)->Red == (b2)->Red))
#define BGRAssign(b1,b2)	do { (b1)->Blue = (b2)->Blue; (b1)->Green = (b2)->Green; (b1)->Red = (b2)->Red; } while (0)

// DGN flags, OR them together.....
#define PRESET	0		// Draw ponts in background colour
#define PSET	1		// Draw ponts in foreground colour
#define B		2		// Draw a box, rather than a line
#define	BF		4		// Draw a filled box rather than a line

#define PI		3.14159265358979323846	// Maths PI :)

#define DgnFlagSet(Flags,Test)	((Flags & Test)==Test)

void ILI9341_Draw_Hollow_Circle(uint16_t X, uint16_t Y, uint16_t Radius, uint16_t Colour);
void ILI9341_Draw_Filled_Circle(uint16_t X, uint16_t Y, uint16_t Radius, uint16_t Colour);
void ILI9341_Draw_Hollow_Rectangle_Coord(uint16_t X0, uint16_t Y0, uint16_t X1, uint16_t Y1, uint16_t Colour);
void ILI9341_Draw_Filled_Rectangle_Coord(uint16_t X0, uint16_t Y0, uint16_t X1, uint16_t Y1, uint16_t Colour);
void ILI9341_Draw_Filled_Rectangle_Size_Text(uint16_t X0, uint16_t Y0, uint16_t Size_X, uint16_t Size_Y, uint16_t Colour);

void ILI9341_PlotLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1,  uint16_t Colour);
void ILI9341_PlotLineTo(int16_t X1, int16_t Y1,  uint16_t Colour);
void ILI9341_DrawArc(int16_t CentreX, int16_t CentreY, int16_t Radius, float StartAngle, float EndAngle, uint16_t Colour);
void ILI9341_FloodFill(uint16_t X, uint16_t Y, uint16_t Colour, uint16_t StopColour);

//USING CONVERTER: http://www.digole.com/tools/PicturetoC_Hex_converter.php
//65K colour (2Bytes / Pixel)
void ILI9341_Draw_Image(const char* Image_Array, uint8_t Orientation);

#endif
