
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
//	ILI9341 Driver library for STM32
//-----------------------------------
//
//	While there are other libraries for ILI9341 they mostly require either interrupts, DMA or both for fast drawing
//	The intent of this library is to offer a simple yet still reasonably fast alternatives for those that
//	do not wish to use interrupts or DMA in their projects.
//
//	Library is written for STM32 HAL library and supports STM32CUBEMX. To use the library with Cube software
//	you need to tick the box that generates peripheral initialization code in their own respective .c and .h file
//
//
//-----------------------------------
//	Performance
//-----------------------------------
//	Settings:	
//	--SPI @ 50MHz 
//	--STM32F746ZG Nucleo board
//	--Redraw entire screen
//
//	++		Theoretical maximum FPS with 50Mhz SPI calculated to be 40.69 FPS
//	++		320*240 = 76800 pixels, each pixel contains 16bit colour information (2x8)
//	++		Theoretical Max FPS: 1/((320*240*16)/50000000)
//
//	With ART Accelerator, instruction prefetch, CPI ICACHE and CPU DCACHE enabled:
//
//	-FPS:									39.62
//	-SPI utilization:			97.37%
//	-MB/Second:						6.09
//
//	With ART Accelerator, instruction prefetch, CPI ICACHE and CPU DCACHE disabled:
//
//	-FPS:									35.45
//	-SPI utilization:			87.12%
//	-MB/Second:						5.44
//	
//	ART Accelerator, instruction prefetch, CPI ICACHE and CPU DCACHE settings found in MXCUBE under "System-> CORTEX M7 button"
//
//
//
//-----------------------------------
//	How to use this library
//-----------------------------------
//
//	-generate SPI peripheral and 3 GPIO_SPEED_FREQ_VERY_HIGH GPIO outputs
//	 		++Library reinitializes GPIOs and SPIs generated by gpio.c/.h and spi.c/.h using MX_X_Init(); calls
//			++reinitialization will not clash with previous initialization so generated initializations can be laft as they are
//	-If using MCUs other than STM32F7 you will have to change the #include "stm32f7xx_hal.h" in the ILI9341_STM32_Driver.h to your respective .h file
//	-define your HSPI_INSTANCE in ILI9341_STM32_Driver.h
//	-define your CS, DC and RST outputs in ILI9341_STM32_Driver.h
//	-check if ILI9341_SCREEN_HEIGHT and ILI9341_SCREEN_WIDTH match your LCD size
//			++Library was written and tested for 320x240 screen size. Other sizes might have issues**
//	-in your main program initialize LCD with ILI9341_Init();
//	-library is now ready to be used. Driver library has only basic functions, for more advanced functions see ILI9341_GFX library	
//
//-----------------------------------
//
// Ported to run on Raspbery Pi Pico
//
// 2021-06, Phill Harvey-Smith.
//

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"

#include "ILI9341_Driver.h"
#include "ILI9341_DMA.h"
#include "status.h"
#include "hexout.h"

#define DEBUG_LCD	0

/* Global Variables ------------------------------------------------------------------*/
volatile uint16_t LCD_HEIGHT = ILI9341_SCREEN_HEIGHT;
volatile uint16_t LCD_WIDTH	 = ILI9341_SCREEN_WIDTH;

#if ILI9341_USE_SPI == 1
/* Initialize SPI */
void ILI9341_SPI_Init(uint baudrate)
{
	printf("ILI9341_SPI_Init:Init SPI, baudrate = %d\n",baudrate);
    spi_init(ILI9341_SPI, baudrate);
    gpio_set_function(ILI9341_SPI_MISO, GPIO_FUNC_SPI);
    gpio_set_function(ILI9341_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(ILI9341_SPI_MOSI, GPIO_FUNC_SPI);
    // Make the SPI pins available to picotool
    bi_decl(bi_3pins_with_func(ILI9341_SPI_MISO, ILI9341_SPI_MOSI, ILI9341_SPI_SCK, GPIO_FUNC_SPI));

	logc0(DEBUG_LCD,"ILI9341_SPI_Init:Init CS\n");
    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_init(ILI9341_SPI_CS);
    gpio_set_dir(ILI9341_SPI_CS, GPIO_OUT);
    gpio_put(ILI9341_SPI_CS, 1);
    // Make the CS pin available to picotool
    bi_decl(bi_1pin_with_name(ILI9341_SPI_CS, "SPI CS"));

	logc0(DEBUG_LCD,"ILI9341_SPI_Init:Init CD\n");
	//Command/Data pin
	gpio_init(ILI9341_SPI_CD);
	gpio_set_dir(ILI9341_SPI_CD, GPIO_OUT);
    gpio_put(ILI9341_SPI_CD, 1);
    
	logc0(DEBUG_LCD,"ILI9341_SPI_Init:Init reset\n");
	//Command/Data pin
	gpio_init(ILI9341_RESET);
	gpio_set_dir(ILI9341_RESET, GPIO_OUT);
    gpio_put(ILI9341_RESET, 1);

	ILIDeSelect();
	ILIClearReset();

    ILI9341_DebugParams();
}

/*Send data (char) to LCD*/
void ILI9341_SPI_Send(unsigned char SPI_Data)
{
	spi_write_blocking(ILI9341_SPI,&SPI_Data,1);
}

inline void ILI9341_SPI_SendMulti(uint8_t 	*SPI_Data,
						   int 		length)
{
	spi_write_blocking(ILI9341_SPI,SPI_Data,length);
}

uint8_t ILI9341_SPI_Get(void)
{
	uint8_t result = LCD_NOP;

	spi_write_read_blocking(ILI9341_SPI,&result,&result,1);

	return result;
}

inline void ILI9341_SPI_GetMulti(uint8_t	*Buffer,
						  		 int		length)
{
	spi_write_read_blocking(ILI9341_SPI,Buffer,Buffer,length);
}

#else
void ILI9341_BUS_Init(void)
{
	uint8_t	Idx;
	LL_GPIO_InitTypeDef GPIO_InitStruct;

	/* Setup Databus */
	GPIO_InitStruct.Pin = ILIDataPins;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_FLOATING;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/* Setup flag pins, CS, RD, WR, C/D */
	GPIO_InitStruct.Pin = nRD_IL9341_Pin | nWR_IL9341_Pin | nCS_IL9341_Pin | CD_IL9341_Pin | RES_IL9341_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	ILIDeSelect();
	ILIClearWriteMode();
	ILIClearReadMode();
	ILISetData();
	ILIClearReset();

	ILI9341_Write_Command(LCD_NOP);
	for(Idx=0;Idx < 4; Idx++)
		ILI9341_BUS_Send(0x00);

	ILIPortWriteMode();
}

void ILI9341_BUS_Send(unsigned char Data)

{
	ILIPortWriteMode();							// Select data bus as outputs
	ILIWriteData(Data);							// write the data
	ILISetWriteMode();							// Set write line low
	//_delay_us(1);								// short delay
	ILIClearWriteMode();						// Clear nWR
}

uint8_t ILI9341_BUS_Get(void)
{
	uint8_t	Result;

	ILIPortReadMode();							// Select data bus as inputs
	ILISetReadMode();							// Strobe read low
	_delay_us(1);								// short delay
	Result=ILIReadData();						// Read the data back
	ILIClearReadMode();							// Clear nRD

	return Result;
}

#endif

#if ILI9341_USE_SPI == 1
#define ILI9341_Send(Data)				ILI9341_SPI_Send(Data)
#define ILI9341_SendMulti(Data,Size)	ILI9341_SPI_SendMulti(Data,Size)
#define ILI9341_HW_Init(baudrate)		ILI9341_SPI_Init(baudrate)
#define ILI9341_Get()					ILI9341_SPI_Get()
#define ILI9341_GetMulti(Buffer,Size)	ILI9341_SPI_GetMulti(Buffer,Size)
#else
#define ILI9341_Send(Data)				ILI9341_BUS_Send(Data)
#define ILI9341_SendMulti(Data,Size)	ILI9341_BUS_SendMulti(Data,Size)
#define ILI9341_HW_Init(baudrate)		ILI9341_BUS_Init()
#define ILI9341_Get()					ILI9341_BUS_Get()
#define ILI9341_GetMulti(Buffer,Size)	ILI9341_BUS_GetMulti(Buffer,Size)
#endif

/* Send command (char) to LCD */
inline void ILI9341_Write_Command(uint8_t Command)
{
	ILISetCommand();
	ILI9341_Send(Command);
}

/* Send Data (char) to LCD */
inline void ILI9341_Write_DataMulti(uint8_t *Data,
							   		int		Size)
{
	ILISetData();
	ILI9341_SendMulti(Data,Size);
}

inline void ILI9341_Write_CommandAndDataM(uint8_t 	Command, 
									      uint8_t 	*Buff, 
										  uint16_t 	Size)
{
	ILI9341_Write_Command(Command);
	ILI9341_Write_DataMulti(Buff,Size);	
}

inline void ILI9341_Write_CommandAndData(uint8_t 	Command, 
									     uint8_t 	Data)
{
	ILI9341_Write_Command(Command);
	ILI9341_Write_DataMulti(&Data,1);	
}

inline void ILI9341_Write_Data(uint8_t Data)
{
	ILI9341_Write_DataMulti(&Data,1);
}


inline uint8_t	ILI9341_Read_Data(void)
{
	uint8_t	Data;
	ILISetData();
	Data=ILI9341_Get();

	return Data;
}

inline void ILI9341_Transmit(uint8_t *Buff, uint16_t Size)
{
	ILI9341_SendMulti(Buff,Size);
}

void ILI9341_TransmitCD(uint8_t *Buff, uint16_t Size, uint8_t	CmdData)
{
	if(CmdData)
		ILISetData();
	else
		ILISetCommand();

	ILI9341_Transmit(Buff,Size);
}

void ILI9341_Receive(uint8_t *Buff, uint16_t Size)
{
	ILISetData();	// Only valid for data !

	ILI9341_GetMulti(Buff,Size);
}

void ILI9341_Write_Command_Get(uint8_t Command, uint8_t *Buff, uint16_t Size)
{
	ILI9341_Write_Command(Command);
	ILI9341_Receive(Buff,Size);
}

void ILI9341_Write_Command_Put(uint8_t Command, uint8_t *Buff, uint16_t Size)
{
	ILI9341_Write_CommandAndDataM(Command, Buff, Size);
	//ILI9341_TransmitCD(Buff,Size,ILIData);
}

/* Set Address - Location block - to draw into */
void ILI9341_Set_AddressRW(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2, uint8_t IsWrite)
{
	uint8_t	addrX[4] = { X1>>8, X1, X2>>8, X2};
	uint8_t	addrY[4] = { Y1>>8, Y1, Y2>>8, Y2};

	ILI9341_Write_CommandAndDataM(LCD_COLUMN_ADDR,addrX,4);

	ILI9341_Write_CommandAndDataM(LCD_PAGE_ADDR, addrY,4);

	if (IsWrite)
		ILI9341_Write_Command(LCD_RAMWR);
	else
		ILI9341_Write_Command(LCD_RAMRD);
}

/*HARDWARE RESET*/
void ILI9341_Reset(void)
{
	ILIClearReset();
	_delay_ms(100);
	ILISetReset();
	_delay_ms(100);
	ILIClearReset();
}

/// @brief  Read parameters on SPI bus ILI9341 displays
/// For those displays that do not have the EXTC pin asserted.
/// This undocumented command overrides the restriction for reading
/// command parameters.
// Reg is the command or register we wish to execute.
// ParamByteNo, is the byte of the parameter we wish to retrieve.
uint8_t ILI9341_ReadReg(uint8_t Reg, uint8_t ParamByteNo)
{
	uint8_t Result;
#if ILI9341_USE_SPI == 0
	uint8_t	Idx;
#endif
	ILISelect();

	// Write undocumented magic command
	ILI9341_Write_CommandAndData(LCD_READREG, LCD_READREG_MAGIC + ParamByteNo);

	// Write and read the reg we want to read.
	ILI9341_Write_Command(Reg);

#if ILI9341_USE_SPI == 1
	Result=ILI9341_Read_Data();
#else
	for(Idx=0; Idx < ParamByteNo+1; Idx++)
		Result=ILI9341_Read_Data();
//	log0("%02X:%08X\n",Idx,Result);
#endif
	ILIDeSelect();

	return Result;
}

/*Set rotation of the screen - changes x0 and y0*/
void ILI9341_Set_Rotation(uint8_t Rotation) 
{
	uint8_t	MACValue;

	switch(Rotation)
	{
		case SCREEN_VERTICAL_1:
			MACValue	= MAC_MX | MAC_BGR;
			LCD_WIDTH 	= 240;
			LCD_HEIGHT 	= 320;
			break;

		case SCREEN_HORIZONTAL_1:
			MACValue	= MAC_MV | MAC_BGR;
			LCD_WIDTH  	= 320;
			LCD_HEIGHT 	= 240;
			break;

		case SCREEN_VERTICAL_2:
			MACValue	= MAC_MY | MAC_BGR;
			LCD_WIDTH  	= 240;
			LCD_HEIGHT 	= 320;
			break;

		case SCREEN_HORIZONTAL_2:
			MACValue	= MAC_MY | MAC_MX | MAC_MV | MAC_BGR;
			LCD_WIDTH  	= 320;
			LCD_HEIGHT 	= 240;
			break;

		default:
			//EXIT IF SCREEN ROTATION NOT VALID!
			return;
			break;
	}

	ILISelect();
	ILI9341_Write_CommandAndData(LCD_MAC, MACValue & MAC_VALID_MASK);

	ILIDeSelect();
}

// Display initialization data
uint8_t PowerA[] 	= {0x39, 0x2C, 0x00, 0x34, 0x02};
uint8_t PowerB[] 	= {0x00, 0xC1, 0x30};
uint8_t TimingA[]	= {0x85, 0x00, 0x78};
uint8_t TimingB[]	= {0x00, 0x00};
uint8_t PowerSeq[]  = {0x64, 0x03, 0x12, 0x81};
uint8_t VCom[]		= {0x3E, 0x28};
uint8_t FrmCtl1[]	= {0x00, 0x18};
uint8_t DFnCtl[]	= {0x08, 0x82, 0x27};
uint8_t PGamma[]	= {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
uint8_t NGamma[]	= {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};

/*Initialize LCD display*/
void ILI9341_Init(uint baudrate)
{
	// Init hardware
	ILI9341_HW_Init(baudrate);

	// init DMA / Block send if in use
	ILI9341_BlockInit();

	ILI9341_Reset();

	//SOFTWARE RESET
	ILISelect();
	ILI9341_Write_Command(LCD_SWRESET);
	_delay_ms(10);

	//POWER CONTROL A
	ILI9341_Write_CommandAndDataM(LCD_POWERA, PowerA, sizeof(PowerA));

	//POWER CONTROL B
	ILI9341_Write_CommandAndDataM(LCD_POWERB, PowerB, sizeof(PowerB));

	//DRIVER TIMING CONTROL A
	ILI9341_Write_CommandAndDataM(LCD_DTCA, TimingA, sizeof(TimingA));

	//DRIVER TIMING CONTROL B
	ILI9341_Write_CommandAndDataM(LCD_DTCB, TimingB, sizeof(TimingB));

	//POWER ON SEQUENCE CONTROL
	ILI9341_Write_CommandAndDataM(LCD_POWER_SEQ, PowerSeq, sizeof(PowerSeq));

	//PUMP RATIO CONTROL
	ILI9341_Write_CommandAndData(LCD_PRC, 0x20);

	//POWER CONTROL,VRH[5:0]
	ILI9341_Write_CommandAndData(LCD_POWER1, 0x23);

	//POWER CONTROL,SAP[2:0];BT[3:0]
	ILI9341_Write_CommandAndData(LCD_POWER2, 0x10);

	//VCM CONTROL
	ILI9341_Write_CommandAndDataM(LCD_VCOM1, VCom, sizeof(VCom));

	//VCM CONTROL 2
	ILI9341_Write_CommandAndData(LCD_VCOM2, 0x86);

	//MEMORY ACCESS CONTROL
	ILI9341_Write_CommandAndData(LCD_MAC, MAC_MX | MAC_BGR);

	//PIXEL FORMAT
	ILI9341_Write_CommandAndData(LCD_PIXEL_FORMAT, PX_DEFAULT);	

	//FRAME RATIO CONTROL, STANDARD RGB COLOR
	ILI9341_Write_CommandAndDataM(LCD_FRMCTR1, FrmCtl1, sizeof(FrmCtl1));

	//DISPLAY FUNCTION CONTROL
	ILI9341_Write_CommandAndDataM(LCD_DFC, DFnCtl, sizeof(DFnCtl));

	//3GAMMA FUNCTION DISABLE
	ILI9341_Write_CommandAndData(LCD_3GAMMA_EN, 0x00);

	//GAMMA CURVE SELECTED
	ILI9341_Write_CommandAndData(LCD_GAMMA, 0x01);

	//POSITIVE GAMMA CORRECTION
	ILI9341_Write_CommandAndDataM(LCD_PGAMMA, PGamma, sizeof(PGamma));

	//NEGATIVE GAMMA CORRECTION
	ILI9341_Write_CommandAndDataM(LCD_NGAMMA, NGamma, sizeof(NGamma));
	//EXIT SLEEP
	ILI9341_Write_Command(LCD_SLEEP_OUT);
	_delay_ms(10);

	//TURN ON DISPLAY
	ILI9341_Write_Command(LCD_DISPLAY_ON);
	_delay_ms(5);

	//STARTING ROTATION
	ILI9341_Set_Rotation(SCREEN_VERTICAL_1);
	ILIDeSelect();
}

//INTERNAL FUNCTION OF LIBRARY
/*Sends block colour information to LCD*/
void ILI9341_Draw_Colour_Burst(uint16_t Colour, uint32_t Size)
{
//SENDS COLOUR
	uint32_t 		Buffer_Size = 0;
	unsigned char 	CShifted = 	Colour>>8;;
	unsigned char 	burst_buffer[BURST_MAX_SIZE];
	uint32_t 		Sending_Size = Size*2;
	uint32_t 		Sending_in_Block;
	uint32_t 		Remainder_from_block;
	uint32_t		Idx;

	Buffer_Size = ((Size*2) < BURST_MAX_SIZE) ?  Size*2 :  BURST_MAX_SIZE;

	memset(burst_buffer,0xFF,Buffer_Size);
	ILISetData();
	ILISelect();

	for(Idx = 0; Idx < Buffer_Size; Idx+=2)
	{
		burst_buffer[Idx] 	= CShifted;
		burst_buffer[Idx+1] = Colour;
	}

	Sending_in_Block 		= Sending_Size/Buffer_Size;
	Remainder_from_block 	= Sending_Size%Buffer_Size;

	if(Sending_in_Block != 0)
	{
		for(Idx = 0; Idx < (Sending_in_Block); Idx++)
		{
			ILI9341_Transmit(burst_buffer, Buffer_Size);
		}
	}

	//REMAINDER!
	ILI9341_Transmit(burst_buffer, Remainder_from_block);
}

//FILL THE ENTIRE SCREEN WITH SELECTED COLOUR (either #define-d ones or custom 16bit)
/*Sets address (entire screen) and Sends Height*Width ammount of colour information to LCD*/
void ILI9341_Fill_Screen(uint16_t Colour)
{
	ILISelect();
	ILI9341_Set_AddressRW(0,0,LCD_WIDTH,LCD_HEIGHT,1);
	ILI9341_Draw_Colour_Burst(Colour, LCD_WIDTH*LCD_HEIGHT);
	ILIDeSelect();
}

//DRAW PIXEL AT XY POSITION WITH SELECTED COLOUR
//
//Location is dependant on screen orientation. x0 and y0 locations change with orientations.
//Using pixels to draw big simple structures is not recommended as it is really slow
//Try using either rectangles or lines if possible
//
void ILI9341_Draw_Pixel(uint16_t X,uint16_t Y,uint16_t Colour) 
{
	unsigned char X_Buffer[4] = {X>>8,X, (X+1)>>8, (X+1)};
	unsigned char Y_Buffer[4] = {Y>>8,Y, (Y+1)>>8, (Y+1)};
	unsigned char C_Buffer[2] = {Colour>>8, Colour};

	if((X >=LCD_WIDTH) || (Y >=LCD_HEIGHT))
		return;	//OUT OF BOUNDS!

	logc0(DEBUG_LCD,"ILI9341_Draw_Pixel(%d,%d,%02X)\n",X,Y,Colour);

	ILISelect();

	// Address and X data
	ILI9341_Write_CommandAndDataM(LCD_COLUMN_ADDR, X_Buffer, 4);

	// Address and Y data
	ILI9341_Write_CommandAndDataM(LCD_PAGE_ADDR, Y_Buffer, 4);

	// Colour data
	ILI9341_Write_CommandAndDataM(LCD_RAMWR, C_Buffer, 2);

	ILIDeSelect();
}

//DRAW RECTANGLE OF SET SIZE AND HEIGTH AT X and Y POSITION WITH CUSTOM COLOUR
//
//Rectangle is hollow. X and Y positions mark the upper left corner of rectangle
//As with all other draw calls x0 and y0 locations dependant on screen orientation
//

void ILI9341_Draw_Rectangle(uint16_t X, uint16_t Y, uint16_t Width, uint16_t Height, uint16_t Colour)
{
	if((X >=LCD_WIDTH) || (Y >=LCD_HEIGHT))
		return;

	if((X+Width-1)>=LCD_WIDTH)
		Width=LCD_WIDTH-X;

	if((Y+Height-1)>=LCD_HEIGHT)
		Height=LCD_HEIGHT-Y;

	ILISelect();
	ILI9341_Set_AddressRW(X, Y, X+Width-1, Y+Height-1,1);
	ILI9341_Draw_Colour_Burst(Colour, Height*Width);
	ILIDeSelect();
}

//DRAW LINE FROM X,Y LOCATION to X+Width,Y LOCATION
void ILI9341_Draw_Horizontal_Line(uint16_t X, uint16_t Y, uint16_t Width, uint16_t Colour)
{
	if((X >=LCD_WIDTH) || (Y >=LCD_HEIGHT))
		return;

	if((X+Width-1)>=LCD_WIDTH)
		Width=LCD_WIDTH-X;

	ILISelect();
	ILI9341_Set_AddressRW(X, Y, X+Width, Y,1);
	ILI9341_Draw_Colour_Burst(Colour, Width);
	ILIDeSelect();
}

//DRAW LINE FROM X,Y LOCATION to X,Y+Height LOCATION
void ILI9341_Draw_Vertical_Line(uint16_t X, uint16_t Y, uint16_t Height, uint16_t Colour)
{
	if((X >=LCD_WIDTH) || (Y >=LCD_HEIGHT))
		return;

	if((Y+Height-1)>=LCD_HEIGHT)
		Height=LCD_HEIGHT-Y;

	ILISelect();
	ILI9341_Set_AddressRW(X, Y, X, Y+Height-1,1);
	ILI9341_Draw_Colour_Burst(Colour, Height);
	ILIDeSelect();
}

void ILI9341_ReadID(uint8_t *buff)
{
	ILISelect();
	ILI9341_Write_Command_Get(LCD_RDDMADCTL,buff,2);	// Get display ID
}

void ILI9341_DebugParams(void)
{
	ILISelect();

	logc0(DEBUG_LCD,"ID4:1:%02X\n",ILI9341_ReadReg(LCD_READ_ID4,1));
	logc0(DEBUG_LCD,"ID4:2:%02X\n",ILI9341_ReadReg(LCD_READ_ID4,2));
	logc0(DEBUG_LCD,"ID4:3:%02X\n",ILI9341_ReadReg(LCD_READ_ID4,3));

	logc0(DEBUG_LCD,"Pixel format : %02X\n",ILI9341_ReadReg(LCD_RDDCOLMOD,1));

	ILIDeSelect();
}