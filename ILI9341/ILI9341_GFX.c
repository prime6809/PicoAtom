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

#include "ILI9341_Driver.h"
#include "ILI9341_Text.h"
#include "ILI9341_GFX.h"
#include "ILI9341_DMA.h"
#include "status.h"
#include <stdlib.h>
#include <math.h>

/* Used by FloodFill */
typedef struct
{
	uint16_t	X;
	uint16_t	Y;
} stack_t;

#define STACK_ELEMENTS	100

#define LOOKFG		0
#define LOOKSTOP	1
/* End FloodFill */

/*Draw hollow circle at X,Y location with specified radius and colour. X and Y represent circles center */
void ILI9341_Draw_Hollow_Circle(uint16_t X, uint16_t Y, uint16_t Radius, uint16_t Colour)
{
	int x = Radius-1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (Radius << 1);

    while (x >= y)
    {
        ILI9341_Draw_Pixel(X + x, Y + y, Colour);
        ILI9341_Draw_Pixel(X + y, Y + x, Colour);
        ILI9341_Draw_Pixel(X - y, Y + x, Colour);
        ILI9341_Draw_Pixel(X - x, Y + y, Colour);
        ILI9341_Draw_Pixel(X - x, Y - y, Colour);
        ILI9341_Draw_Pixel(X - y, Y - x, Colour);
        ILI9341_Draw_Pixel(X + y, Y - x, Colour);
        ILI9341_Draw_Pixel(X + x, Y - y, Colour);

        if (err <= 0)
        {
            y++;
            err += dy;
            dy += 2;
        }
        if (err > 0)
        {
            x--;
            dx += 2;
            err += (-Radius << 1) + dx;
        }
    }
}

/*Draw filled circle at X,Y location with specified radius and colour. X and Y represent circles center */
void ILI9341_Draw_Filled_Circle(uint16_t X, uint16_t Y, uint16_t Radius, uint16_t Colour)
{
	int x = Radius;
    int y = 0;
    int xChange = 1 - (Radius << 1);
    int yChange = 0;
    int radiusError = 0;

    while (x >= y)
    {
        for (int i = X - x; i <= X + x; i++)
        {
            ILI9341_Draw_Pixel(i, Y + y,Colour);
            ILI9341_Draw_Pixel(i, Y - y,Colour);
        }
        for (int i = X - y; i <= X + y; i++)
        {
            ILI9341_Draw_Pixel(i, Y + x,Colour);
            ILI9341_Draw_Pixel(i, Y - x,Colour);
        }

        y++;
        radiusError += yChange;
        yChange += 2;
        if (((radiusError << 1) + xChange) > 0)
        {
            x--;
            radiusError += xChange;
            xChange += 2;
        }
    }
		//Really slow implementation, will require future overhaul
		//TODO:	https://stackoverflow.com/questions/1201200/fast-algorithm-for-drawing-filled-circles	
}

/*Draw a hollow rectangle between positions X0,Y0 and X1,Y1 with specified colour*/
void ILI9341_Draw_Hollow_Rectangle_Coord(uint16_t X0, uint16_t Y0, uint16_t X1, uint16_t Y1, uint16_t Colour)
{
	uint16_t 	X_length = 0;
	uint16_t 	Y_length = 0;
	uint8_t		Negative_X = ((X1 - X0) < 0) ? 1 : 0;
	uint8_t 	Negative_Y = ((Y1 - Y0) < 0) ? 1 : 0;
	
	X_length = Negative_X ? (X0 - X1) : (X1 - X0);

	ILI9341_Draw_Horizontal_Line(X0, Y0, X_length, Colour);
	ILI9341_Draw_Horizontal_Line(X0, Y1, X_length, Colour);

	Y_length = Negative_Y ? (Y0 - Y1) : (Y1 - Y0);

	ILI9341_Draw_Vertical_Line(X0, Y0, Y_length, Colour);
	ILI9341_Draw_Vertical_Line(X1, Y0, Y_length, Colour);
	
	if((X_length > 0)||(Y_length > 0)) 
	{
		ILI9341_Draw_Pixel(X1, Y1, Colour);
	}
}

/*Draw a filled rectangle between positions X0,Y0 and X1,Y1 with specified colour*/
void ILI9341_Draw_Filled_Rectangle_Coord(uint16_t X0, uint16_t Y0, uint16_t X1, uint16_t Y1, uint16_t Colour)
{
	uint16_t 	X_length = 0;
	uint16_t 	Y_length = 0;
	uint8_t		Negative_X = ((X1 - X0) < 0) ? 1 : 0;
	uint8_t 	Negative_Y = ((Y1 - Y0) < 0) ? 1 : 0;
	
	uint16_t X0_true = 0;
	uint16_t Y0_true = 0;
	
	X_length = Negative_X ? (X0 - X1) : (X1 - X0);
	Y_length = Negative_Y ? (Y0 - Y1) : (Y1 - Y0);
	X0_true  = Negative_X ? X1 : X0;
	Y0_true  = Negative_Y ? Y1 : Y0;

	ILI9341_Draw_Rectangle(X0_true, Y0_true, X_length, Y_length, Colour);	
}

#define SwapXY(_a,_b)	do { int16_t _sw; _sw=_a; _a=_b; _b=_sw; } while (0)

/* Arbitary line plotting between to points, using Bresenham's line algorithm */
/* https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm */
void ILI9341_PlotLine(int16_t X0,
					  int16_t Y0,
					  int16_t X1,
					  int16_t Y1,
					  uint16_t Colour)

{
	int16_t	dx;
	int16_t	dy;
	int16_t	xi;
	int16_t yi;
	int16_t	D;
	int16_t x,y;

	// Set current graphics co-ordinates to line destination
	ILI9341_Vars.GPosX=X1;
	ILI9341_Vars.GPosY=Y1;

	// Line is vertical use optimized version.
	if (X0 == X1)
	{
		if(Y0 > Y1)
			ILI9341_Draw_Vertical_Line(X0,Y1,(Y0-Y1)+1,Colour);
		else
			ILI9341_Draw_Vertical_Line(X0,Y0,(Y1-Y0)+1,Colour);
	}

	// Line is horizontal use optimized version.
	if (Y0 == Y1)
	{
		if(X0 > X1)
			ILI9341_Draw_Horizontal_Line(X1,Y0,(X0-X1)+1,Colour);
		else
			ILI9341_Draw_Horizontal_Line(X0,Y0,(X1-X0)+1,Colour);
	}

	// Determine if X difference greater than Y difference
	if (abs(Y1 - Y0) < abs(X1 - X0))
	{
		// Check if first point is further left than second,
		// if so swap them.
		if (X0 > X1)
		{
			SwapXY(X0,X1);
			SwapXY(Y0,Y1);
		}

		// Work out X and Y differences
		// initialize y increment
		dx = X1-X0;
		dy = Y1-Y0;
		yi = 1;

		// invert if Y0 is further down than Y1, so we
		// need to decrement Y
		if (dy < 0)
		{
			yi = -1;
			dy = -dy;
		}
		D = 2*dy - dx;
		y = Y0;

		// loop drawing pixels
		for(x=X0; x<=X1; x++)
		{
			ILI9341_Draw_Pixel(x,y,Colour);
			if (D > 0)
			{
				y=y+yi;
				D=D-(2*dx);
			}
			D=D+2*dy;
		}

	}
	else
	{
		// Check if first point is further down than second,
		// if so swap them.
		if (Y0 > Y1)
		{
			SwapXY(X0,X1);
			SwapXY(Y0,Y1);
		}

		// Work out X and Y differences
		// initialize x increment
		dx = X1-X0;
		dy = Y1-Y0;
		xi = 1;

		// invert if X0 is further right than X1, so we
		// need to decrement X
		if(dx < 0)
		{
			xi = -1;
			dx = -dx;
		}
		D = 2*dx - dy;
		x = X0;

		// loop drawing pixels
		for(y=Y0; y<=Y1; y++)
		{
			ILI9341_Draw_Pixel(x,y,Colour);
		    if (D > 0)
		    {
		       x = x + xi;
		       D = D - 2*dy;
		    }
		    D = D + 2*dx;
		}
	}
}

// Because this uses floats and trig functions it is slow for drawing complete circles,
// in that case use ILI9341_Draw_Hollow_Circle or ILI9341_Draw_Filled_Circle
void ILI9341_DrawArc(int16_t CentreX,
	  	  	 	 	 int16_t CentreY,
					 int16_t Radius,
					 float StartAngle,
					 float EndAngle,
					 uint16_t Colour)

{
	float step;
	int	x,y;

	if (EndAngle < StartAngle)
		EndAngle += (2*PI);

	for(step = StartAngle; step < EndAngle; step += 0.005)
	{
		x = (int) (CentreX + cos(step) * Radius);
		y = (int) (CentreY + sin(step) * Radius);
		ILI9341_Draw_Pixel(x,y,Colour);
	}
}


void ILI9341_PlotLineTo(int16_t X1, int16_t Y1,  uint16_t Colour)
{
	ILI9341_PlotLine(ILI9341_Vars.GPosX,ILI9341_Vars.GPosY,X1,Y1,Colour);
}

#define FLOOD_LOG 	0

#if FLOOD_LOG == 1
#define Push(x,y)	do { if (SP < STACK_ELEMENTS) { SP++; Stack[SP].X=x; Stack[SP].Y=y;} log0("Push SP=%d\n",SP); } while (0)
#define Pop(x,y)	do { if (SP > -1) { x=Stack[SP].X; y=Stack[SP].Y; SP--; } log0("Pop  SP=%d, x=%d, y=%d\n",SP,x,y); } while (0)
#else
#define Push(x,y)	do { if (SP < STACK_ELEMENTS) { SP++; Stack[SP].X=x; Stack[SP].Y=y;} } while (0)
#define Pop(x,y)	do { if (SP > -1) { x=Stack[SP].X; y=Stack[SP].Y; SP--; } } while (0)
#endif

// Flood fill an area by specifying the starting point, the fill colour and border colour.
// The basic algorithim for this was adapted from that found at :
// https://lodev.org/cgtutor/floodfill.html#Scanline_Floodfill_Algorithm_With_Stack
// Notes : stack could probably be smaller.
void ILI9341_FloodFill(uint16_t X,
	   	      	  	   uint16_t Y,
					   uint16_t Colour,
					   uint16_t StopColour)
{
	bgr_t		LookFor[2];					// Foreground and border colours as BGR

	stack_t		Stack[STACK_ELEMENTS];		// Stack to store lines to scan
	int16_t		SP=-1;						// Stack pointer

	bgr_t		Current[ILI9341_SCREEN_WIDTH];	// Buffers for current / previous / next
	bgr_t		Next[ILI9341_SCREEN_WIDTH];		// screen lines
	bgr_t		Prev[ILI9341_SCREEN_WIDTH];

	uint16_t	XCo,YCo;					// Current X and Y co-ordinates

	uint8_t		SpanAbove;					// Do we need to scan above the current line
	uint8_t		SpanBelow;					// Do we need to scan below the current line
	uint8_t		AboveStop;					// Is the pixel in the above line the stop colour?
	uint8_t		BelowStop;					// Is the pixel in the below line the stop colour?
	uint8_t		AboveColour;				// Is the pixel in the above line the fill colour?
	uint8_t		BelowColour;				// Is the pixel in the below line the fill colour?

	int16_t		Pos;						// Current point on line

	SetPixelFormat(PX_18_IF);
	// Read current pixels at origin.
	ILI9341_ReadScreen(0,0,320,1,(uint8_t *)Current,6,BLOCK_DUMMY_SEND);

	// Set the colours we are looking at, so we get them in BGR format
	SetPixelFormat(PX_DEFAULT);
	ILI9341_Draw_Pixel(0,0,Colour);
	ILI9341_Draw_Pixel(1,0,StopColour);

	SetPixelFormat(PX_18_IF);

	// Get BGR values to look for
	ILI9341_ReadScreen(0,0,320,1,(uint8_t *)LookFor,sizeof(LookFor),BLOCK_DUMMY_SEND);
	// restore original pixels from the screen
	ILI9341_WriteScreen(0,0,320,1,(uint8_t *)Current,6,BLOCK_DUMMY_RECEIVE);

	// stack initial paint co-ordinates
	Push(X,Y);

	// procedd whilst we have items on the stack
	while(SP > -1)
	{
		// recover search point
		Pop(XCo,YCo);

		// begin searching along the line at X point
		Pos=XCo;

		// read a line of pixels
		ILI9341_ReadScreen(0,YCo,320,YCo+1,(uint8_t *)Current,LCD_WIDTH*3,BLOCK_DUMMY_SEND);

		// if not beginning of screen also read previous
		if(YCo > 0)
			ILI9341_ReadScreen(0,YCo-1,320,YCo,(uint8_t *)Prev,LCD_WIDTH*3,BLOCK_DUMMY_SEND);

		// if not end of screen also read next
		if(YCo < LCD_HEIGHT)
			ILI9341_ReadScreen(0,YCo+1,320,YCo+2,(uint8_t *)Next,LCD_WIDTH*3,BLOCK_DUMMY_SEND);

		// At this point we will have the current line plus the previous & next in memory

		// find leftmost edge that is either screen edge or bounding colour of current line
		while ((Pos >= 0) && !BGRIsEqual(&LookFor[LOOKSTOP],&Current[Pos]))
			Pos--;

		// point at first pixel to colour, init span flags
		Pos++;
		SpanAbove = SpanBelow = 0;

		// proceed until we hit the edge of the screen or the bounding colour
		while ((Pos < LCD_WIDTH) && !BGRIsEqual(&LookFor[LOOKSTOP],&Current[Pos]))
		{
			// colour the current pixel, with new colour
			BGRAssign(&Current[Pos],&LookFor[LOOKFG]);

			// set flags based on lines above & below current one
			AboveStop=BGRIsEqual(&LookFor[LOOKSTOP],&Prev[Pos]);
			BelowStop=BGRIsEqual(&LookFor[LOOKSTOP],&Next[Pos]);
			AboveColour=BGRIsEqual(&LookFor[LOOKFG],&Prev[Pos]);
			BelowColour=BGRIsEqual(&LookFor[LOOKFG],&Next[Pos]);

			//if we find a pixel in the line above, that is not the bounding colour
			// push it's co-ordinates onto the stack, and flag that we have done so
			// so that we do it only once for each line section
			if(!SpanAbove && (YCo > 0) && !AboveStop && !AboveColour)
			{
				Push(Pos,YCo-1);
				SpanAbove=1;
			}
			else if (SpanAbove && (YCo > 0) && AboveStop)
			{
				SpanAbove=0;
			}

			// likewise for the line below
			if(!SpanBelow && (YCo < LCD_HEIGHT-1) && !BelowStop && !BelowColour)
			{
		        Push(Pos, YCo+1);
		        SpanBelow = 1;
			}
			else if(SpanBelow && (YCo < LCD_HEIGHT-1) && BelowStop)
			{
				SpanBelow = 0;
			}

			// move to next pixel to the right
			Pos++;
		}

		// all done on this line write back to display
		ILI9341_WriteScreen(0,YCo,320,YCo+1,(uint8_t *)Current,LCD_WIDTH*3,BLOCK_DUMMY_RECEIVE);
	}

	// Reset pixel format
	SetPixelFormat(PX_DEFAULT);
}





/*Draws a full screen picture from flash. Image converted from RGB .jpeg/other to C array using online converter*/
//USING CONVERTER: http://www.digole.com/tools/PicturetoC_Hex_converter.php
//65K colour (2Bytes / Pixel)
void ILI9341_Draw_Image(const char* Image_Array, uint8_t Orientation)
{
	unsigned char Temp_small_buffer[BURST_MAX_SIZE];
	uint32_t counter = 0;
	uint32_t Line;
	uint32_t Idx;

	if(Orientation == SCREEN_HORIZONTAL_1)
	{
		ILI9341_Set_Rotation(SCREEN_HORIZONTAL_1);
		ILI9341_Set_AddressRW(0,0,ILI9341_SCREEN_WIDTH,ILI9341_SCREEN_HEIGHT,1);
			
		ILISetData();
		ILISelect();
		
		counter = 0;
		for(Line = 0; Line < ILI9341_SCREEN_WIDTH*ILI9341_SCREEN_HEIGHT*2/BURST_MAX_SIZE; Line++)
		{			
				for(Idx = 0; Idx < BURST_MAX_SIZE; Idx++)
				{
					Temp_small_buffer[Idx]	= Image_Array[counter+Idx];
				}						
				ILI9341_Transmit(Temp_small_buffer, BURST_MAX_SIZE);

				counter += BURST_MAX_SIZE;			
		}
		ILIDeSelect();
	}
	else if(Orientation == SCREEN_HORIZONTAL_2)
	{
		ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
		ILI9341_Set_AddressRW(0,0,ILI9341_SCREEN_WIDTH,ILI9341_SCREEN_HEIGHT,1);
			
		ILISetData();
		ILISelect();
		
		for(Line = 0; Line < ILI9341_SCREEN_WIDTH*ILI9341_SCREEN_HEIGHT*2/BURST_MAX_SIZE; Line++)
		{			
				for(Idx = 0; Idx < BURST_MAX_SIZE; Idx++)
				{
					Temp_small_buffer[Idx]	= Image_Array[counter+Idx];
				}						
				ILI9341_Transmit(Temp_small_buffer, BURST_MAX_SIZE);

				counter += BURST_MAX_SIZE;			
		}
		ILIDeSelect();
	}
	else if(Orientation == SCREEN_VERTICAL_2)
	{
		ILI9341_Set_Rotation(SCREEN_VERTICAL_2);
		ILI9341_Set_AddressRW(0,0,ILI9341_SCREEN_HEIGHT,ILI9341_SCREEN_WIDTH,1);
			
		ILISetData();
		ILISelect();
		
		for(Line = 0; Line < ILI9341_SCREEN_WIDTH*ILI9341_SCREEN_HEIGHT*2/BURST_MAX_SIZE; Line++)
		{			
				for(Idx = 0; Idx < BURST_MAX_SIZE; Idx++)
				{
					Temp_small_buffer[Idx]	= Image_Array[counter+Idx];
				}						
				ILI9341_Transmit(Temp_small_buffer, BURST_MAX_SIZE);

				counter += BURST_MAX_SIZE;			
		}
		ILIDeSelect();
	}
	else if(Orientation == SCREEN_VERTICAL_1)
	{
		ILI9341_Set_Rotation(SCREEN_VERTICAL_1);
		ILI9341_Set_AddressRW(0,0,ILI9341_SCREEN_HEIGHT,ILI9341_SCREEN_WIDTH,1);
			
		ILISetData();
		ILISelect();
		
		for(Line = 0; Line < ILI9341_SCREEN_WIDTH*ILI9341_SCREEN_HEIGHT*2/BURST_MAX_SIZE; Line++)
		{			
				for(Idx = 0; Idx < BURST_MAX_SIZE; Idx++)
				{
					Temp_small_buffer[Idx]	= Image_Array[counter+Idx];
				}						

				ILI9341_Transmit(Temp_small_buffer, BURST_MAX_SIZE);

				counter += BURST_MAX_SIZE;			
		}
		ILIDeSelect();
	}
}

