/*
 * ILI9341_Dragon.h
 *
 *  Created on: 6 Aug 2018
 *      Author: phill
 */

#ifndef ILI9341_ILI9341_DRAGON_H_
#define ILI9341_ILI9341_DRAGON_H_

// DgnXXXXX functions like the ones in Dragon / CoCo basic.
void ILI9341_DgnLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, uint8_t Flags);
void ILI9341_DgnLineTo(int16_t X1, int16_t Y1, uint8_t Flags);
void ILI9341_DgnColour(uint16_t	Foreground, uint16_t Background);
void ILI9341_DgnCircle(uint16_t X, uint16_t Y, uint16_t Radius, uint16_t Colour, uint8_t HWRatio, float	Start,float	End);
void ILI9341_DgnPaint(uint16_t X, uint16_t Y, uint16_t Colour, uint16_t StopColour);

#endif /* ILI9341_ILI9341_DRAGON_H_ */
