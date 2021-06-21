#include "platform.h"
#if (PLATFORM==PLATFORM_PIC)
#include <p18cxxx.h>
#elif (PLATFORM==PLATFORM_AVR)
#include <avr/io.h>
#elif (PLATFORM==PLATFORM_STM32)
#elif (PLATFORM==PLATFORM_PIPICO)
#endif
#include "mmcio.h"

#include "atmmc2io.h"
#include "diskio.h"
#include "hexout.h"
#include "status.h"


/* Definitions for MMC/SDC command */
#define CMD0   (0x40+0)    /* GO_IDLE_STATE */
#define CMD1   (0x40+1)    /* SEND_OP_COND (MMC) */
#define ACMD41 (0xC0+41)   /* SEND_OP_COND (SDC) */
#define CMD8   (0x40+8)    /* SEND_IF_COND */
#define CMD16  (0x40+16)   /* SET_BLOCKLEN */
#define CMD17  (0x40+17)   /* READ_SINGLE_BLOCK */
#define CMD24  (0x40+24)   /* WRITE_BLOCK */
#define CMD55  (0x40+55)   /* APP_CMD */
#define CMD58  (0x40+58)   /* READ_OCR */

#define DEBUG_SD  0

void INIT_SPI(void)
{
#if (PLATFORM==PLATFORM_PIC)
   SPI_CS_TRIS = 0;
   DESELECT();

   SPI_DIN_TRIS = 1;

   SPI_DOUT_TRIS = 0;
   SPI_DOUT_PIN = 1;

   SPI_SCK_TRIS = 0;
   SPI_SCK_PIN = 0;
#elif (PLATFORM==PLATFORM_AVR)
	// SCK, MOSI and SS as outputs, MISO as output
	SPIDDR |= ((1<<SPI_SCK) | (1<<SPI_MOSI) | (1<<SPI_SS));
	SPIDDR &= ~(1<<SPI_MISO);
	
	// initialize SPI interface
	// master mode
	SPCR |= (1<<MSTR);
	
	// select clock phase positive-going in middle of data
	// Data order MSB first
	// switch to f/4 2X = f/2 bitrate
    // SPCR &= ~((1<<CPOL) | (1<<DORD) | (1<<SPR0) | (1<<SPR1));
	
	SPCR &= ~((1<<SPR0) | (1<<SPR1));
	SPSR |= (1<<SPI2X);
	SPCR |= (1<<SPE);
	
	ClearSS();
	SPIPORT |= (1<<SPI_SCK);	// set SCK hi
#elif (PLATFORM==PLATFORM_STM32)
	LL_SPI_InitTypeDef SPI_InitStruct;

	LL_GPIO_InitTypeDef GPIO_InitStruct;
	/* Peripheral clock enable */
	LL_APB1_GRP1_EnableClock(DISK_CLK_EN);

	/**SPI2 GPIO Configuration
	  PB13   ------> SPI2_SCK
	  PB14   ------> SPI2_MISO
	  PB15   ------> SPI2_MOSI
	 */
	// Clock and Master out are outputs
	GPIO_InitStruct.Pin = DISK_SPI_MOSI | DISK_SPI_SCK;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	LL_GPIO_Init(DISK_SPI_GPIO, &GPIO_InitStruct);

	// Master input is an input!
	GPIO_InitStruct.Pin = DISK_SPI_MISO;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_FLOATING;
	LL_GPIO_Init(DISK_SPI_GPIO, &GPIO_InitStruct);

	// Chip select.
	GPIO_InitStruct.Pin = DISK_CS_PIN;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	LL_GPIO_Init(DISK_CS_GPIO, &GPIO_InitStruct);

	SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
	SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
	SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
	SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_LOW;
	SPI_InitStruct.ClockPhase = LL_SPI_PHASE_1EDGE;
	SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
	SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV4;
	SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
	SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
	SPI_InitStruct.CRCPoly = 10;
	LL_SPI_Init(DISK_SPI, &SPI_InitStruct);

	LL_SPI_Enable(DISK_SPI);
#elif  (PLATFORM==PLATFORM_PIPICO)
   logc0(DEBUG_SD,"SD_SPI_Init:Init SPI\n");
   // This example will use SPI0 at 0.5MHz.
   spi_init(SD_SPI, 10 * 1000 * 1000);
   spi_set_format(SD_SPI,  8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
   gpio_set_function(SD_SPI_MOSI_Pin, GPIO_FUNC_SPI);
   gpio_set_function(SD_SPI_MISO_Pin, GPIO_FUNC_SPI);
   gpio_set_function(SD_SPI_SCK_Pin, GPIO_FUNC_SPI);
   //gpio_pull_up(SD_SPI_MISO_Pin);
   // Make the SPI pins available to picotool
   bi_decl(bi_3pins_with_func(SD_SPI_MISO_Pin, SD_SPI_MOSI_Pin, SD_SPI_SCK_Pin, GPIO_FUNC_SPI));

	logc0(DEBUG_SD,"SD_SPI_Init:Init CS\n");
   // Chip select is active-low, so we'll initialise it to a driven-high state
   gpio_init(SDCS_Pin);
   gpio_set_dir(SDCS_Pin, GPIO_OUT);
   gpio_put(SDCS_Pin, 1);
   // Make the CS pin available to picotool
   bi_decl(bi_1pin_with_name(SDCS_Pin, "SPI CS"));
#endif
}


BYTE XFER_SPI(BYTE output)
{
#if (PLATFORM==PLATFORM_PIC)
   char BitCount;
   static char input;
   BitCount = 8;
   input = output;

   // SCK idles low
   // Data output after falling edge of SCK
   // Data sampled before rising edge of SCK
   if(input&0x80)
      SPI_DOUT_PIN = 1;
   else
      SPI_DOUT_PIN = 0;                // Set Dout to MSB of data
   Nop();                          // Adjust for jitter
   Nop();
   do                              // Loop 8 times
   {
      STATUSbits.C = 0;       // Set the carry bit according
      if(SPI_DIN_PIN)          // to the Din pin
         STATUSbits.C = 1;
      SPI_SCK_PIN = 1;         // Set the SCK pin
      _asm
         rlcf  input,1,1
         _endasm
         //              Rlcf(input);            // Rotate the carry into the data byte
         Nop();                  // Produces a 50% duty cycle clock
      Nop();
      Nop();
      Nop();
      Nop();
      Nop();
      Nop();
      Nop();
      Nop();
      Nop();
      Nop();
      SPI_SCK_PIN = 0;         // Clear the SCK pin
      if(input&0x80)          // the MSB of data
         SPI_DOUT_PIN = 1;
      else
         SPI_DOUT_PIN = 0;        // Set Dout to the next bit according to

      BitCount--;             // Count iterations through loop
   } while(BitCount);

   return input;
#elif (PLATFORM==PLATFORM_AVR)
	SPDR = output;						// send to SPI
	loop_until_bit_is_set(SPSR,SPIF);	// wait for it to be sent
	return SPDR;						// return received byte
#elif (PLATFORM==PLATFORM_STM32)
	LL_SPI_TransmitData8(DISK_SPI,output);			// send to SPI

	while (!LL_SPI_IsActiveFlag_RXNE(DISK_SPI))		// Wait for received byte (should always drop through)
		{}

	while (LL_SPI_IsActiveFlag_BSY(DISK_SPI))
		{}

	return LL_SPI_ReceiveData8(DISK_SPI);
#elif (PLATFORM == PLATFORM_PIPICO)
   BYTE result;

   spi_write_read_blocking(SD_SPI, &output, &result, 1);

   return result;
#else 
	return 0;
#endif
}





/*--------------------------------------------------------------------------

Module Private Functions

---------------------------------------------------------------------------*/

BYTE CardType;


/*-----------------------------------------------------------------------*/
/* Deselect the card and release SPI bus                                 */
/*-----------------------------------------------------------------------*/

static
void release_spi (void)
{
   DESELECT();
   XFER_SPI(0xff);
}


/*-----------------------------------------------------------------------*/
/* Send a command packet to MMC                                          */
/*-----------------------------------------------------------------------*/

static
BYTE send_cmd (
               BYTE cmd,      /* Command byte */
               DWORD arg      /* Argument */
               )
{
   BYTE n, res;

   union
   {
      BYTE b[4];
      DWORD d;
   }
   argbroke;
   argbroke.d = arg;

   if (cmd & 0x80) { /* ACMD<n> is the command sequense of CMD55-CMD<n> */
      cmd &= 0x7F;
      res = send_cmd(CMD55, 0);
      if (res > 1) return res;
   }

   /* Select the card */
   DESELECT();
   XFER_SPI(0xff);
   SELECT();
   XFER_SPI(0xff);

   /* Send a command packet */
   XFER_SPI(cmd);                   /* Start + Command index */
   XFER_SPI(argbroke.b[3]);         /* Argument[31..24] */
   XFER_SPI(argbroke.b[2]);         /* Argument[23..16] */
   XFER_SPI(argbroke.b[1]);         /* Argument[15..8] */
   XFER_SPI(argbroke.b[0]);         /* Argument[7..0] */
   n = 0x01;                        /* Dummy CRC + Stop */
   if (cmd == CMD0) n = 0x95;       /* Valid CRC for CMD0(0) */
   if (cmd == CMD8) n = 0x87;       /* Valid CRC for CMD8(0x1AA) */
   XFER_SPI(n);

   /* Receive a command response */
   n = 10;                       /* Wait for a valid response in timeout of 10 attempts */
   do {
      res = XFER_SPI(0xff);
   } while ((res & 0x80) && --n);

   return res;       /* Return with the response value */
}


/*--------------------------------------------------------------------------

Public Functions

---------------------------------------------------------------------------*/


DSTATUS mmc_status(void)
{
   if (CardType == 0)
      return STA_NODISK;

   // return STA_NOINIT;
   // return STA_NODISK;
   // return STA_PROTECTED;

   return 0;
}


/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS mmc_initialize (void)
{
   BYTE n, cmd, ty, ocr[4];
   WORD tmr;

   GREENLEDON();

   printf("mmc_initialize()\n");
   INIT_SPI();

#if _WRITE_FUNC
   if (MMC_SEL) disk_writep(0, 0);     /* Finalize write process if it is in progress */
#endif
   for (n = 11; n; --n) XFER_SPI(0xff);   /* Dummy clocks */

   ty = 0;
   if (send_cmd(CMD0, 0) == 1)
   {
      /* Enter Idle state */
      if (send_cmd(CMD8, 0x1AA) == 1)
      {  /* SDv2 */
         for (n = 0; n < 4; n++)
         {
            ocr[n] = XFER_SPI(0xff);      /* Get trailing return value of R7 resp */
         }
         if (ocr[2] == 0x01 && ocr[3] == 0xAA)
         {
            /* The card can work at vdd range of 2.7-3.6V */
            for (tmr = 12000; tmr && send_cmd(ACMD41, 1UL << 30); tmr--) ; /* Wait for leaving idle state (ACMD41 with HCS bit) */

            if (tmr && send_cmd(CMD58, 0) == 0)
            {
               /* Check CCS bit in the OCR */
               for (n = 0; n < 4; n++) ocr[n] = XFER_SPI(0xff);
               ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2; /* SDv2 (HC or SC) */
            }
         }
      }
      else
      {                    /* SDv1 or MMCv3 */
         if (send_cmd(ACMD41, 0) <= 1)
         {
            ty = CT_SD1; cmd = ACMD41; /* SDv1 */
         }
         else
         {
            ty = CT_MMC; cmd = CMD1;   /* MMCv3 */
         }
         for (tmr = 25000; tmr && send_cmd(cmd, 0); tmr--) ;   /* Wait for leaving idle state */

         if (!tmr || send_cmd(CMD16, 512) != 0)       /* Set R/W block length to 512 */
         {
            ty = 0;
         }
      }
   }
   CardType = ty;
   release_spi();

   GREENLEDOFF();

   printf("SD type:%d\n",ty);
   return ty ? 0 : STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read sector                                                           */
/*-----------------------------------------------------------------------*/

DRESULT mmc_readsector (BYTE *buff, DWORD lba)
{
   DRESULT res;
   BYTE rc;
   WORD bc;

   GREENLEDON();

   if (!(CardType & CT_BLOCK)) lba *= 512;      /* Convert to byte address if needed */

   res = RES_ERROR;
   if (send_cmd(CMD17, lba) == 0) {    /* READ_SINGLE_BLOCK */

      bc = 30000;
      do {                    /* Wait for data packet in timeout of 100ms */
         rc = XFER_SPI(0xff);
      } while (rc == 0xFF && --bc);

      if (rc == 0xFE)
      {
         /* A data packet arrived */
         for (bc = 0; bc < 512; ++bc)
         {
            buff[bc] = XFER_SPI(0xff);
         }
         /* Skip CRC */
         XFER_SPI(0xff);
         XFER_SPI(0xff);

         res = RES_OK;
      }
   }

   release_spi();

   GREENLEDOFF();
   return res;
}


#if 0
DRESULT mmc_readsector_halp(BYTE *buff, DWORD lba, BYTE top)
{
   DRESULT res;
   BYTE rc;
   WORD bc;

   GREENLEDON();

   if (!(CardType & CT_BLOCK)) lba *= 512;      /* Convert to byte address if needed */

   res = RES_ERROR;
   if (send_cmd(CMD17, lba) == 0) {    /* READ_SINGLE_BLOCK */

      bc = 30000;
      do {                    /* Wait for data packet in timeout of 100ms */
         rc = XFER_SPI(0xff);
      } while (rc == 0xFF && --bc);

      if (rc == 0xFE)
      {
         if (top)
         {
            for (bc = 0; bc < 256; ++bc)
            {
               XFER_SPI(0xff);
            }
         }

         /* A data packet arrived */
         for (bc = 0; bc < 256; ++bc)
         {
            buff[bc] = XFER_SPI(0xff);
         }

         if (!top)
         {
            for (bc = 0; bc < 256; ++bc)
            {
               XFER_SPI(0xff);
            }
         }

         /* Skip CRC */
         XFER_SPI(0xff);
         XFER_SPI(0xff);

         res = RES_OK;
      }
   }

   release_spi();

   GREENLEDOFF();

   return res;
}
#endif

/*-----------------------------------------------------------------------*/
/* Write sector                                                  */
/*-----------------------------------------------------------------------*/

DRESULT mmc_writesector(BYTE *buff, DWORD sa)
{
   DRESULT res;
   WORD bc;

   GREENLEDON();

   res = RES_ERROR;

   if (!(CardType & CT_BLOCK)) sa *= 512; /* Convert to byte address if needed */

   if (send_cmd(CMD24, sa) == 0)
   {
      XFER_SPI(0xFF);
      XFER_SPI(0xFE);

      for (bc = 0; bc < 512; ++bc)
      {
         XFER_SPI(buff[bc]);
      }

      XFER_SPI(0);
      XFER_SPI(0);

      if ((XFER_SPI(0xff) & 0x1F) == 0x05)
      {
         /* Receive data resp and wait for end of write process in timeout of 300ms */
         for (bc = 65000; XFER_SPI(0xff) != 0xFF && bc; bc--) ;   /* Wait ready */
         if (bc) res = RES_OK;

         release_spi();
      }
   }

   GREENLEDOFF();

   return res;
}
