#ifndef _MMC

#include "integer.h"
#include "main.h"

//#ifndef BUILD_FOR_EMULATOR

#if (PLATFORM==PLATFORM_PIC)

#define SPI_CS_PIN PORTCbits.RC2
#define SPI_CS_TRIS TRISCbits.TRISC2

#define SPI_DIN_PIN PORTCbits.RC5
#define SPI_DIN_TRIS TRISCbits.TRISC5

#define SPI_DOUT_PIN PORTCbits.RC4
#define SPI_DOUT_TRIS TRISCbits.TRISC4

#define SPI_SCK_PIN   PORTCbits.RC3
#define SPI_SCK_TRIS  TRISCbits.TRISC3

#define SELECT()   SPI_CS_PIN=0
#define DESELECT() SPI_CS_PIN=1
#define MMC_SEL()  SPI_CS_PIN==0

#elif (PLATFORM==PLATFORM_AVR)


/* SPI stuff : PHS 2010-06-08 */
#define SPIPORT		PORTB
#define SPIPIN		PINB
#define	SPIDDR		DDRB
#define SPI_SS		4
#define SPI_MOSI	5
#define SPI_MISO	6
#define	SPI_SCK		7

#define SPI_MASK	((1<<SPI_SS) | (1<<SPI_MOSI) | (1<<SPI_MISO) | (1<<SPI_SCK))
#define SPI_SS_MASK	(1<<SPI_SS)

#define AsertSS()	{ SPIPORT &= ~SPI_SS_MASK; };
#define ClearSS()	{ SPIPORT |= SPI_SS_MASK; };
#define WaitSPI()	{ while(!(SPSR & (1<<SPIF))); }; 

#define SELECT()   { SPIPORT &= ~SPI_SS_MASK; }
#define DESELECT() { SPIPORT |= SPI_SS_MASK; }
#define MMC_SEL()  (SPI_CS_PIN & SPI_SS_MASK)==0

#elif (PLATFORM==PLATFORM_STM32)
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_spi.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f105xc.h"

// SPI2 cannot be remapped
/**SPI2 GPIO Configuration
 PB13   ------> SPI2_SCK
 PB14   ------> SPI2_MISO
 PB15   ------> SPI2_MOSI
*/
#define DISK_SPI_SCK	LL_GPIO_PIN_13
#define DISK_SPI_MISO	LL_GPIO_PIN_14
#define DISK_SPI_MOSI	LL_GPIO_PIN_15
#define DISK_SPI_GPIO	GPIOB

#define DISK_SPI		SPI2
#define DISK_CLK_EN		LL_APB1_GRP1_PERIPH_SPI2

#define DISK_CS_PIN		LL_GPIO_PIN_12
#define DISK_CS_GPIO	GPIOB

#define SELECT()		LL_GPIO_ResetOutputPin(DISK_CS_GPIO, DISK_CS_PIN)
#define DESELECT()		LL_GPIO_SetOutputPin(DISK_CS_GPIO, DISK_CS_PIN)

#elif (PLATFORM == PLATFORM_PIPICO)
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"

// SD SPI pins defined in main.h
#if 0
#define DISK_SPI_SCK	14
#define DISK_SPI_MISO	12
#define DISK_SPI_MOSI	11

#define DISK_SPI		spi1

#define DISK_CS_PIN		13
#endif 

#define SELECT()        do { asm volatile("nop \n nop \n nop");  gpio_put(SDCS_Pin, 0); asm volatile("nop \n nop \n nop"); } while (0)  
#define DESELECT()		do { asm volatile("nop \n nop \n nop");  gpio_put(SDCS_Pin, 1); asm volatile("nop \n nop \n nop"); } while (0)

#else


#define SELECT()
#define DESELECT()
#define MMC_SEL()

#endif


/* Card type flags (CardType) */
#define CT_MMC 0x01 /* MMC ver 3 */
#define CT_SD1 0x02 /* SD ver 1 */
#define CT_SD2 0x04 /* SD ver 2 */
#define CT_SDC (CT_SD1|CT_SD2) /* SD */
#define CT_BLOCK 0x08 /* Block addressing */

#define _MMC
#endif

