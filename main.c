
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "delays.h"
#include "ILI9341_Driver.h"
#include "ILI9341_GFX.h"
#include "ILI9341_DMA.h"
#include "ILI9341_Text.h"
#include "ILI9341_Dragon.h"
#include "6847.h"
#include "fake6502.h"
#include "Atom.h"
#include "ps2kbd.h"
#include "matrix_kbd.h"

#include <stdio.h>
#include <stdlib.h>

void key_callback(uint8_t	scancode,
				          uint8_t	state);
/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
    /* Initialize clocks */
    uint sys_freq = 200000;
    //uint sys_freq = 250000;
    if (sys_freq > 250000)
    {
        vreg_set_voltage(VREG_VOLTAGE_1_25);
    }
    set_sys_clock_khz(sys_freq, true);


	/* Initialize all configured peripherals */
    stdio_init_all();

	cls();
	printf("IL9341 Acorn Atom Emulator\n");

	printf("Init TFT\n");
	ILI9341_Init(50 * 1000 * 1000); // SPI pus frequency

	ILISelect();
	log0("ID4:1:%02X\n",ILI9341_ReadReg(LCD_READ_ID4,1));
	log0("ID4:2:%02X\n",ILI9341_ReadReg(LCD_READ_ID4,2));
	log0("ID4:3:%02X\n",ILI9341_ReadReg(LCD_READ_ID4,3));

	log0("Pixel format : %02X\n",ILI9341_ReadReg(LCD_RDDCOLMOD,1));

	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
	ILI9341_Clear_Screen(BLACK,GREEN);

	printf("PS/2 Keyboard init\n");
	ps2_kbd_init();
  
	matrix_init(&Atom_output_key,&key_callback);

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		AtomInit();
		AtomGo();
	}
}

void key_callback(uint8_t	scancode,
				  uint8_t	state)
{
	if(state==KEY_DOWN)
	{
		switch(scancode)
		{
			case SCAN_CODE_F10 :
				log0("\n");
				break;
		}
	}
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

