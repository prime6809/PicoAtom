/*
 * IL9341_DMA.h
 *
 *  Created on: 6 Aug 2018
 *      Author: phill
 */
#include "ILI9341_Driver.h"

#ifndef ILI9341_ILI9341_DMA_H_
#define ILI9341_ILI9341_DMA_H_

#define DEBUG_DMA 1

#define BLOCK_FLAGS_NONE		0x00
#define BLOCK_DUMMY_SEND		0x01
#define BLOCK_DUMMY_RECEIVE	    0x02

#define DMAFlagSet(var,flag)	((var & flag)==flag)

#if ILI9341_USE_DMA == 1
#define DMA_CH(chan)		LL_DMA_CHANNEL_##chan
#define DMA_CH_NAME(chan)	DMA_CH(chan)

#define ILI9341_DMA_IN		DMA_CH_NAME(ILI9341_DMA_CH_IN)
#define ILI9341_DMA_OUT		DMA_CH_NAME(ILI9341_DMA_CH_OUT)

#if 0
#define DMAFlag(flag,chan)			flag##chan
#define ClearDMAChanFlags(ch)		DMAFlag(DMA_ISR_TCIF,ch) | DMAFlag(DMA_ISR_TEIF,ch) | DMAFlag(DMA_ISR_HTIF,ch)
#define ClearDMAFlags(chin,chout)	ILI9341_DMA_CTRL->IFCR |= (ClearDMAChanFlags(chin) | ClearDMAChanFlags(chout))

#define IsActiveFlagTC(ctrl,chan)	((ctrl->ISR & DMAFlag(DMA_ISR_TCIF,chan)) == (DMAFlag(DMA_ISR_TCIF,chan)))
#endif
#endif

// Initialize the block section, for DMA sets up DMA channels, otherwise does nothing!
void ILI9341_BlockInit(void);

// ReadScreenDMA and WriteScreenDMA don't touch the pixel formats before operating on the screen.
// ReadPixelsDMA and WritePixelsDMA *DO*
void ILI9341_ReadScreen(uint16_t X0, uint16_t Y0, uint16_t X1, uint16_t Y1,	uint8_t	 *Buff,	uint32_t NoBytes, uint8_t Flags);
void ILI9341_WriteScreen(uint16_t X0, uint16_t Y0, uint16_t X1, uint16_t Y1,	uint8_t	 *Buff,	uint32_t NoBytes, uint8_t Flags);

void ILI9341_ReadPixels(uint16_t X0, uint16_t Y0, uint16_t X1, uint16_t Y1,	uint8_t	 *Buff,	uint32_t NoBytes, uint8_t Flags);
void ILI9341_WritePixels(uint16_t X0, uint16_t Y0, uint16_t X1, uint16_t Y1,	uint8_t	 *Buff,	uint32_t NoBytes, uint8_t Flags);


void ILI9341_Scroll_Screen(uint16_t	Lines);

#if ILI9341_USE_DMA == 1
#if 0
#define WaitDMA()	while (!IsActiveFlagTC(ILI9341_DMA_CTRL,ILI9341_DMA_CH_IN) && !IsActiveFlagTC(ILI9341_DMA_CTRL,ILI9341_DMA_CH_OUT)) {}
#define WaitSPI()	while (LL_SPI_IsActiveFlag_BSY(ILI9341_SPI) && !LL_SPI_IsActiveFlag_RXNE(ILI9341_SPI)) {}
#endif

//#ifdef DEBUG_DMA
//void ILI9341_DMA_SendReceive(uint8_t	*Buffer, uint32_t	Count, uint8_t	Flags);
//void ILI9341_ConfigDMA(void);
//
//#endif

#else
#endif


#endif /* ILI9341_ILI9341_DMA_H_ */
