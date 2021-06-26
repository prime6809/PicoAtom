
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

#if DEVEL
#define NOW "Devel: compiled at " __TIME__ " on " __DATE__ 
#else
#define NOW "Release: compiled at " __TIME__ " on " __DATE__ 
#endif

volatile uint32_t old_clockticks6502 = 0;                     
volatile uint32_t old_instructions = 0;

struct repeating_timer monitor_timer;
bool monitor_timer_callback(struct repeating_timer *t);
bool monitor_on = false;

void key_callback(uint8_t	scancode,
				  uint8_t	state);


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
    printf("Pico Acorn Atom Emulator\n");
    printf(NOW "\n");

    printf("Init TFT\n");
    ILI9341_Init(50 * 1000 * 1000); // SPI pus frequency

    ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
    ILI9341_Clear_Screen(BLACK,GREEN);

    printf("PS/2 Keyboard init\n");
    ps2_kbd_init();
  
    matrix_init(&Atom_output_key,&key_callback);

	cancel_repeating_timer(&monitor_timer);	// Incase it was already running.....
	add_repeating_timer_ms(-1000, monitor_timer_callback, NULL, &monitor_timer);

    AtomInit();
    AtomGo();

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
    
    }
}

void key_callback(uint8_t	scancode,
				  uint8_t	state)
{
	if(state==KEY_DOWN)
	{
		switch(scancode)
		{
#if TRACE_ENABLED            
			case SCAN_CODE_F9 :
                ToggleTrace();
				log0("Trace: %d\n",DoTrace);
				break;
#endif
			case SCAN_CODE_F10 :
				Throttle6502 = !Throttle6502;
                log0("CPU throttled : %d\n",Throttle6502);
				break;

            case SCAN_CODE_F11 :
                monitor_on = !monitor_on;
                log0("Frequency monitor: %d\n",monitor_on);
                break;
		}
	}
}

bool monitor_timer_callback(struct repeating_timer *t)
{
    if(monitor_on)
        log0("6502: %ld, %ld\n",(clockticks6502 - old_clockticks6502),(instructions - old_instructions));

    old_clockticks6502=clockticks6502;
    old_instructions=instructions;

    return true;
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

