/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  */

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/vreg.h"
#include "hardware/spi.h"

#include "hardware/pll.h"
#include "hardware/clocks.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/clocks.h"

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H__
#define __MAIN_H__


/* Private define ------------------------------------------------------------*/

#if 0
#define B1_Pin 			LL_GPIO_PIN_13
#define B1_GPIO_Port 	GPIOC
#define B1_EXTI_IRQn 	EXTI15_10_IRQn

#define DB0_Pin 		LL_GPIO_PIN_0
#define DB1_Pin 		LL_GPIO_PIN_1
#define DB2_Pin 		LL_GPIO_PIN_2
#define DB3_Pin 		LL_GPIO_PIN_3
#define DB4_Pin 		LL_GPIO_PIN_4
#define DB5_Pin 		LL_GPIO_PIN_5
#define DB6_Pin 		LL_GPIO_PIN_6
#define DB7_Pin 		LL_GPIO_PIN_7

#define DB0_GPIO_Port 	GPIOC
#define DB1_GPIO_Port 	GPIOC
#define DB2_GPIO_Port 	GPIOC
#define DB3_GPIO_Port 	GPIOC
#define DB6_GPIO_Port 	GPIOC
#define DB4_GPIO_Port 	GPIOC
#define DB5_GPIO_Port 	GPIOC
#define DB7_GPIO_Port 	GPIOC

#define USART_TX_Pin 	LL_GPIO_PIN_2
#define USART_RX_Pin 	LL_GPIO_PIN_3
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_GPIO_Port GPIOA

#define LD2_Pin LL_GPIO_PIN_5
#define LD2_GPIO_Port GPIOA

#define nRD_IL9341_Pin 	LL_GPIO_PIN_8
#define nWR_IL9341_Pin 	LL_GPIO_PIN_9
#define nCS_IL9341_Pin 	LL_GPIO_PIN_10
#define CD_IL9341_Pin 	LL_GPIO_PIN_11
#define RES_IL9341_Pin 	LL_GPIO_PIN_12

#define nRD_IL9341_GPIO_Port 	GPIOC
#define nWR_IL9341_GPIO_Port 	GPIOC
#define nCS_IL9341_GPIO_Port 	GPIOC
#define CD_IL9341_GPIO_Port 	GPIOC
#define RES_IL9341_GPIO_Port 	GPIOC

#define TMS_Pin 		LL_GPIO_PIN_13
#define TMS_GPIO_Port 	GPIOA
#define TCK_Pin 		LL_GPIO_PIN_14
#define TCK_GPIO_Port 	GPIOA

#define SWO_Pin 		LL_GPIO_PIN_3
#define SWO_GPIO_Port 	GPIOB
#endif

#define PS2_CLK_Pin 		  8
#define PS2_DAT_Pin 		  9

#define SPK_PIN				    14

// SD card CS pin.
#define SDCS_Pin			    13

// SD card SPI pins
#define SD_SPI_MOSI_Pin		11
#define SD_SPI_MISO_Pin   12		
#define SD_SPI_SCK_Pin		10
#define SD_SPI				    spi1

#define LD2On()		LL_GPIO_SetOutputPin(LD2_GPIO_Port, LD2_Pin)
#define LD2Off()	LL_GPIO_ResetOutputPin(LD2_GPIO_Port, LD2_Pin)


/* ########################## Assert Selection ############################## */
/**
  * @brief Uncomment the line below to expanse the "assert_param" macro in the 
  *        HAL drivers code
  */

#ifdef __cplusplus
 extern "C" {
#endif
void _Error_Handler(char *, int);

#define Error_Handler() _Error_Handler(__FILE__, __LINE__)
#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
