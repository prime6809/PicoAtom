/*
 * ILI9341_Dragon.c
 *
 *  Created on: 6 Aug 2018
 *      Author: phill
 */

#include "ILI9341_Driver.h"
#include "ILI9341_Text.h"
#include "ILI9341_GFX.h"
#include "ILI9341_DMA.h"
#include "status.h"
#include <stdlib.h>
#include <math.h>

// Draw a line using semantics similar to those in Dragon / CoCo basic.
// Flags can be can be a combination of :
// PSET		line is drawn in current grapics foreground colour
// PRESET	line is drawn in current graphics background colour.
// B		draw a box rather than a line
// BF		draw a filled box rather than a line
#define CALC_SLOPE 0

void ILI9341_DgnLine(int16_t X0,
		  	  	  	 int16_t Y0,
					 int16_t X1,
					 int16_t Y1,
					 uint8_t Flags)
{
	uint16_t	Colour;

#if CALC_SLOPE == 1
	float		Slope = 0;
	int16_t		dx,dy;
	int16_t		YIntercept;

	static float	m1,	m2;
	static int16_t	c1,c2;
	float			Xintersect, Yintersect;
	float mcalc;
	int16_t ccalc;


	if(ILI9341_DgnDebugIsOn())
	{
		dx		= (X0-X1);
		dy		= (Y0-Y1); // Since y co-ordinates increse down the screen.

		if(dx != 0)
			Slope	= (float) dy / (float) dx;

		// since y=m*x + c we can re-arrange to drop c out (since we know x and y)
		// so c = y - mx
		YIntercept = Y0 - (Slope * X0);

		if(DgnFlagSet(Flags,DGN_SAVE))
		{
			m1=Slope;
			c1=YIntercept;
		}

		log0("(%d,%d)-(%d,%d), dx=%d, dy=%d m=%f, c=%d eqn : y = %2.2fx + %d\n",X0,Y0,X1,Y1,abs(dx),abs(dy),Slope,YIntercept,Slope,YIntercept);

		if(DgnFlagSet(Flags,DGN_INTERSECT))
		{
			m2=Slope;
			c2=YIntercept;

			// line one is : y1=m1 * X + c1
			// line two is : y1=m2 * X + c2
			//
			// we need to solve the simmultanius equation : m1 * X + c1 = m2 * X + c2 = y
			//

			// first adjust X terms
			if (m2 < 0)
				mcalc = m1 + -m2;
			else
				mcalc = m1 - m2;

			// Now adjust constants
			if (c1 < 0)
				ccalc = c2 + -c1;
			else
				ccalc = c2 - c1;

			// we now have m1 * x = c2, so divide c2 by m1 will give us x at the point of intersection
			Xintersect = ccalc / mcalc;

			// Substitute Xintersect in either equation to get Yintersect
			Yintersect = (m1 * Xintersect) + c1;

			log0("line 1 y = %2.2fx + %d\n",m1,c1);
			log0("line 2 y = %2.2fx + %d\n",m2,c2);

			log0("lines intersect at %2.2f,%2.2f\n",Xintersect,Yintersect);
		}
	}
#endif

	if(DgnFlagSet(Flags,PSET))
		Colour=ILI9341_Vars.GInk;
	else
		Colour=ILI9341_Vars.GPaper;

	//log0("ILI9341_DgnLine(%d,%d,%d,%d,%d)\n",X0,Y0,X1,Y1,Flags);

	if(DgnFlagSet(Flags,BF))
		ILI9341_Draw_Filled_Rectangle_Coord(X0,Y0,X1,Y1,Colour);
	else if(DgnFlagSet(Flags,B))
		ILI9341_Draw_Hollow_Rectangle_Coord(X0,Y0,X1,Y1,Colour);
	else
		ILI9341_PlotLine(X0,Y0,X1,Y1,Colour);
}

// Draw a line using semantics similar to those in Dragon / CoCo basic.
// This is like the version of the line command that ommits the first
// co-ordinate and continues from the last point drawn.
void ILI9341_DgnLineTo(int16_t X1,
					   int16_t Y1,
					   uint8_t Flags)

{
	ILI9341_DgnLine(ILI9341_Vars.GPosX,ILI9341_Vars.GPosY,X1,Y1,Flags);
}

// Similar to Dragon/CoCo basic Color command sets forground and background
// colour for graphics.
void ILI9341_DgnColour(uint16_t	Foreground, uint16_t Background)
{
	ILI9341_Vars.GInk=Foreground;
	ILI9341_Vars.GPaper=Background;
}

void ILI9341_DgnCircle(uint16_t X,
					   uint16_t Y,
					   uint16_t Radius,
					   uint16_t	Colour,
					   uint8_t  HWRatio,
					   float	Start,
					   float	End)
{
	ILI9341_DrawArc(X,Y,Radius,(2*PI*Start),(2*PI*End),Colour);
}

void ILI9341_DgnPaint(uint16_t X,
		   	   	      uint16_t Y,
					  uint16_t Colour,
					  uint16_t StopColour)
{
	ILI9341_FloodFill(X,Y,Colour,StopColour);
}


