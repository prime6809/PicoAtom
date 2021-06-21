/*
 * IL9341_DMA.c
 *
 *  Created on: 6 Aug 2018
 *      Author: phill
 */

#include "ILI9341_Driver.h"
#include "ILI9341_DMA.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "hardware/dma.h"

extern uint8_t ILI9341_BUS_Get(void);
extern void ILI9341_BUS_Send(unsigned char Data);
extern uint8_t ILI9341_SPI_Get(void);
extern void ILI9341_SPI_Send(unsigned char Data);

#if ILI9341_USE_SPI == 1
#define ILI9341_Send(Data)	ILI9341_SPI_Send(Data)
#define ILI9341_Get()		ILI9341_SPI_Get()
#else
#define ILI9341_Send(Data)	ILI9341_BUS_Send(Data)
#define ILI9341_Get()		ILI9341_BUS_Get()
#endif

#define BLOCK_LINES	1

#if ILI9341_USE_DMA == 1

static int 		dma_channel_rx;
static int 		dma_channel_tx;

static uint8_t	Dummy_rx = LCD_NOP;
static uint8_t	Dummy_tx = LCD_NOP;

void ILI9341_ConfigDMA(void)
{
	// Claim a channel for transmit and receive
	dma_channel_rx = dma_claim_unused_channel(true);
	dma_channel_tx = dma_claim_unused_channel(true);

	// Get default channel configs
	dma_channel_config channel_config_rx = dma_channel_get_default_config(dma_channel_rx);
	dma_channel_config channel_config_tx = dma_channel_get_default_config(dma_channel_tx);

	// Set datarequest to the request output of the configured SPI 
	// Set transfer size to 8 bits
	// Set write increment  
    channel_config_set_dreq(&channel_config_tx, spi_get_index(ILI9341_SPI) ? DREQ_SPI1_TX : DREQ_SPI0_TX);
 	channel_config_set_transfer_data_size(&channel_config_tx, DMA_SIZE_8);
    channel_config_set_write_increment(&channel_config_tx, false);
    
	dma_channel_configure(dma_channel_tx, &channel_config_tx,
                          &spi_get_hw(spi_default)->dr,		// write address SPI dataregsier
                          &Dummy_tx, 						// read address
                          0, 								// element count (each element is of size transfer_data_size)
                          false); 							// don't start yet

	// Set datarequest to the request output of the configured SPI 
	// Set transfer size to 8 bits
	// Set read increment  
    channel_config_set_dreq(&channel_config_rx, spi_get_index(ILI9341_SPI) ? DREQ_SPI1_RX : DREQ_SPI0_RX);
 	channel_config_set_transfer_data_size(&channel_config_rx, DMA_SIZE_8);
    channel_config_set_read_increment(&channel_config_rx, false);
    
	dma_channel_configure(dma_channel_rx, &channel_config_rx,
                          &Dummy_rx, 						// write address
                          &spi_get_hw(ILI9341_SPI)->dr, 	// read address SPI data register
                          0, 								// element count (each element is of size transfer_data_size)
                          false); 							// don't start yet

}

// Send or recieve a block of memory by DMA to the SPI
void ILI9341_DMA_SendReceive(uint8_t	*Buffer,
							 uint32_t	Count,
							 uint8_t	Flags)
{
	// Get current configurations for send and receive channels
	dma_channel_config channel_config_rx = dma_get_channel_config(dma_channel_rx);
	dma_channel_config channel_config_tx = dma_get_channel_config(dma_channel_tx);
	
	// Setup the length and address of receive buffer
	
	// if BLOCK_DUMMY_RECEIVE is set the receive channel is just receiving filler bytes not
	// containing any data so we just throw them away.
	// Otherwise we put them in the buffer.
	if(DMAFlagSet(Flags,BLOCK_DUMMY_RECEIVE))
	{
		channel_config_set_write_increment(&channel_config_rx, false);
		dma_channel_configure(dma_channel_rx, &channel_config_rx, 
								&Dummy_rx, 						// Dummy read address
								&spi_get_hw(ILI9341_SPI)->dr,	// SPI data register
								Count,							// no of bytes to transfer
								false);							// don't start yet
	}
	else
	{
		channel_config_set_write_increment(&channel_config_rx, true);
		dma_channel_configure(dma_channel_rx, &channel_config_rx, 
								Buffer, 						// Buffer to receive the data
								&spi_get_hw(ILI9341_SPI)->dr,	// SPI data register
								Count,							// no of bytes to transfer
								false);							// don't start yet
	}

	// Setup the length of the transmit buffer
	
	// If BLOCK_DUMMY_SEND is set we are just sending filler bytes to cause the 
	// SPI to transmit it's data back to use, so point at dummy transmit byte.
	if(DMAFlagSet(Flags,BLOCK_DUMMY_SEND))
	{
		channel_config_set_read_increment(&channel_config_tx, false);
		dma_channel_configure(dma_channel_tx, &channel_config_tx, 
								&spi_get_hw(ILI9341_SPI)->dr, 	// SPI data register
								&Dummy_tx,						// Dummy read address
								Count,							// no of bytes to transfer
								false);							// don't start yet
	}
	else
	{
		channel_config_set_read_increment(&channel_config_tx, true);
		dma_channel_configure(dma_channel_tx, &channel_config_tx, 
								&spi_get_hw(ILI9341_SPI)->dr, 	// SPI data register
								Buffer,							// Buffer to get data to transmit
								Count,							// no of bytes to transfer
								false);							// don't start yet
	}

    // start them exactly simultaneously to avoid races (in extreme cases the FIFO could overflow)
    dma_start_channel_mask((1u << dma_channel_tx) | (1u << dma_channel_rx));

	// Wait for the DMA to complete
    dma_channel_wait_for_finish_blocking(dma_channel_rx);
	dma_channel_wait_for_finish_blocking(dma_channel_tx);
}

#define ILI9341_Block_SendReceive(buff,count,flags)	ILI9341_DMA_SendReceive(buff,count,flags)
#define ILI9341_BlockConfig() ILI9341_ConfigDMA()
#else
void ILI9341_ConfigNonDMA(void)
{
}

// Send or recieve a block of memory programattically the SPI
void ILI9341_NonDMA_SendReceive(uint8_t		*Buffer,
							    uint32_t	Count,
							    uint8_t		Flags)
{
	uint32_t	Index;


	// If dummy send is set we are rceiving
	if(DMAFlagSet(Flags,BLOCK_DUMMY_SEND))
	{
		for(Index=0; Index < Count; Index++)
			Buffer[Index]=ILI9341_Get();
	}
	else
	{
		for(Index=0; Index < Count; Index++)
			ILI9341_Send(Buffer[Index]);
	}
}

#define ILI9341_Block_SendReceive(buff,count,flags)	ILI9341_NonDMA_SendReceive(buff,count,flags)
#define ILI9341_BlockConfig() ILI9341_ConfigNonDMA()
#endif

void ILI9341_BlockInit(void)
{
	ILI9341_BlockConfig();
}

void ILI9341_ReadScreen(uint16_t X0,
						uint16_t Y0,
						uint16_t X1,
						uint16_t Y1,
						uint8_t	*Buff,
						uint32_t NoBytes,
						uint8_t	Flags)
{
	ILISelect();

	// Set address of source line
	ILI9341_Set_AddressRW(X0,Y0,X1,Y1,0);
	ILI9341_Read_Data();	// skip dummy byte

	// Recieve the data by DMA
	ILI9341_Block_SendReceive(Buff,NoBytes,Flags);

	// Idle the display
	ILIDeSelect();
	ILISelect();
	ILI9341_Write_Command(LCD_NOP);

	ILIDeSelect();
}

void ILI9341_WriteScreen(uint16_t X0,
		   	   	   	   	 uint16_t Y0,
						 uint16_t X1,
						 uint16_t Y1,
						 uint8_t *Buff,
						 uint32_t NoBytes,
						 uint8_t  Flags)
{
	ILISelect();

	// Set the address of the destination line
	ILI9341_Set_AddressRW(X0,Y0,X1,Y1,1);
	ILISetData();

	// Send the data by DMA
	ILI9341_Block_SendReceive(Buff,NoBytes,Flags);
	ILIDeSelect();
	ILISelect();

	// Idle the display
	ILI9341_Write_Command(LCD_NOP);

	ILIDeSelect();
}

void ILI9341_ReadPixels(uint16_t X0, uint16_t Y0, uint16_t X1, uint16_t Y1,	uint8_t	 *Buff,	uint32_t NoBytes,uint8_t	Flags)
{
	ILISelect();
	// Set the pixel format to 24 bit RGB
	SetPixelFormat(PX_18_IF);

	ILI9341_ReadScreen(X0,Y0,X1,Y1,Buff,NoBytes,Flags);

	// Reset the pixel format.
	SetPixelFormat(PX_DEFAULT);
	ILIDeSelect();
}

void ILI9341_WritePixels(uint16_t X0, uint16_t Y0, uint16_t X1, uint16_t Y1,	uint8_t	 *Buff,	uint32_t NoBytes,uint8_t	Flags)
{
	ILISelect();
	// Set the pixel format to 24 bit RGB
	SetPixelFormat(PX_18_IF);

	ILI9341_WriteScreen(X0,Y0,X1,Y1,Buff,NoBytes,Flags);

	// Reset the pixel format.
	SetPixelFormat(PX_DEFAULT);
	ILIDeSelect();
}

void ILI9341_Scroll_Screen(uint16_t	Lines)
{
	uint16_t	LineNo;							// Line number being moved
	uint16_t	NoBytes;						// Number of bytes to transfer
	uint8_t		LineBuff[(320*3)*BLOCK_LINES];	// write buffer

	// Set the pixel format to 24 bit RGB
	SetPixelFormat(PX_18_IF);

	// Calculate the number of bytes to transfer at once.
	NoBytes=(LCD_WIDTH*3)*BLOCK_LINES;

	// Loop over display lines to be moved
	for(LineNo=Lines;LineNo < LCD_HEIGHT ;LineNo+=BLOCK_LINES)
	{
		ILI9341_ReadScreen(0,LineNo,LCD_WIDTH,LineNo+BLOCK_LINES,LineBuff,NoBytes,BLOCK_DUMMY_SEND);
		ILI9341_WriteScreen(0,(LineNo-Lines),LCD_WIDTH,(LineNo-Lines)+BLOCK_LINES,LineBuff,NoBytes,BLOCK_DUMMY_RECEIVE);
	}

	// Reset the pixel format.
	SetPixelFormat(PX_DEFAULT);
	ILIDeSelect();
}



