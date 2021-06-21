/* PS/2 keyboard routines by Jurre Hanema, version 1.0
 * ps2kbd.c
 *
 * Released under "THE BEER-WARE LICENSE" (Revision 42):
 * <kipmans@gmail.com> wrote this file. As long as you retain this notice you can 
 * do whatever you want with this stuff. If we meet some day, and you think this 
 * stuff is worth it, you can buy me a beer in return.
 */

/*
    Modified March-October 2009, P.Harvey-Smith to more suit the needs of the 
    Atom/BBC/Electron keyboard interfaces.

	2010-07-03, P.Harvey-Smith.
	Changed keyboard queue handling to use a proper ring buffer rather than a stack!
	
	2010-07-04, P.Harvey-Smith.
	Changed nested if statements to be a switch as I think this makes the code clearer
	if a little more verbose !
	Receive code now checks parity bit and only queues scancode if parity is valid.
	
	2011-04-27, Changed all externally callable routines to have the prefix
	ps2_kbd_

	2018-08-10, Ported code to STM32F10x

	2018-08-27, Changed Queue / Dequeue functions to do the pointer manipulation in a
	single line to help mitigate race condition between interupt queuing scancodes and
	main routine dequeuing them.

	2021-06-17, Ported to Raspbery Pi Pico.

*/

#include "main.h"
#include "ps2kbd.h"
#include "status.h"
#include "delays.h"
#include "status.h"

#define DEBUG_PS2	0

#define LED_PIN		PICO_DEFAULT_LED_PIN	
#define LED_MASK 	(1ul << LED_PIN)

volatile uint8_t	kbd_bit_n = 1;
volatile uint8_t	kbd_n_bits = 0;
volatile uint8_t	kbd_buffer = 0;
volatile uint8_t	kbd_queue[KBD_BUFSIZE + 1];
volatile uint8_t	kbd_queue_idx = 0;
volatile uint8_t	kbd_parity;

volatile uint8_t	kbd_queue_insert = 0;
volatile uint8_t	kbd_queue_remove = 0;
volatile uint16_t	kbd_status = 0;

#define KQueueHasSpace()	(((kbd_queue_insert % KBD_BUFSIZE)+1) != (kbd_queue_remove % KBD_BUFSIZE))
#define KQueueEmpty()		((kbd_queue_insert % KBD_BUFSIZE) == (kbd_queue_remove % KBD_BUFSIZE))

void ps2_kbd_update_leds(void);
// Begin actual implementation

void ps2_keyboard_callback(uint gpio, uint32_t events);

void ps2_kbd_init()
{
	kbd_queue[KBD_BUFSIZE] = 0;
	kbd_queue_insert = 0;
	kbd_queue_remove = 0;
	kbd_status = 0;
	kbd_parity = 0;

	// PS/2 Clock pin, opendrain output, so it will be at whatever level
	// the keyboard drives it at *UNLESS* we drive it low.
	gpio_init(PS2_CLK_Pin);
	gpio_set_dir(PS2_CLK_Pin, false);
	gpio_set_pulls(PS2_CLK_Pin, true, false);

	// Set interrupts, enabled on clock pin falling edge
	gpio_set_irq_enabled_with_callback(PS2_CLK_Pin, GPIO_IRQ_EDGE_FALL, true, &ps2_keyboard_callback);

	// Setup PS/2 Data line, in A12
	gpio_init(PS2_DAT_Pin);
	gpio_set_dir(PS2_DAT_Pin, false);
	//gpio_set_pulls(PS2_DAT_Pin, true, false);

	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, true);
	
	kbd_status = KBD_CAPS;

	ps2_kbd_send(KBD_CMD_ALL_MB);
	ps2_kbd_update_leds();
	log0("PS/2 Init done\n");
}


uint8_t ps2_kbd_queue_scancode(volatile uint8_t p)
{
	//log0("PS/2:%02X,%02X\n",p,kbd_queue_insert);
	
	if(KQueueHasSpace())
	{
		kbd_queue[kbd_queue_insert] = p;
		kbd_queue_insert = (kbd_queue_insert+1) % KBD_BUFSIZE;
	} else
		return 0;

	return 1;
}


uint8_t ps2_kbd_get_scancode(void)
{
	uint8_t		result;

	if(!KQueueEmpty())
	{
		result=kbd_queue[kbd_queue_remove];
		kbd_queue_remove = (kbd_queue_remove+1) % KBD_BUFSIZE;
		
		return result;
	} else
		return 0;
}


void ps2_kbd_send(uint8_t data)
{
	logc0(DEBUG_PS2,"kbd_send(%2X), kbd_status=%4X, kbd_bit_n=%d\n",data,kbd_status,kbd_bit_n);
	
	// This behaviour isn't the most desirable, but it's the easiest and proved to be reliable.
	while(kbd_status & (KBD_SEND | KBD_RECEIVE)) 
	//	_delay_ms(5);
		asm volatile ("nop");	
	
	// Initiate request-to-send, the actual sending of the data
	// is handled in the ISR.
	kbd_status |= KBD_REQ_SEND;
	gpio_set_dir(PS2_CLK_Pin, true);
	gpio_put(PS2_CLK_Pin, false);
	
	_delay_us(120);
	kbd_status &= ~KBD_REQ_SEND;

	kbd_bit_n = 1;
	kbd_status |= KBD_SEND;
	kbd_n_bits = 0;
	kbd_buffer = data;

	// Set the data line to output and drive it low before releasing the
	// clock. This signals a request to send.
	gpio_set_dir(PS2_DAT_Pin,true);
	gpio_put(PS2_DAT_Pin,false);
	
	// Set clock pin high before setting it back to input
	gpio_set_dir(PS2_CLK_Pin, false);
	gpio_put(PS2_CLK_Pin,true);

	// wait for transmit to finish
	while(kbd_status & (KBD_SEND | KBD_RECEIVE)) 
	//	_delay_ms(5);
		asm volatile ("nop");	
}

void ps2_kbd_set_leds(uint8_t	kbleds)
{
	// Send the command to set the LEDS to the keyboard
	ps2_kbd_send(KBD_CMD_SET_LEDS);
	ps2_kbd_send(kbleds);
}

void ps2_kbd_update_leds(void)
{
	uint8_t	val = 0;

	if(kbd_status & KBD_CAPS) val |= 0x04;
	if(kbd_status & KBD_NUMLOCK) val |= 0x02;
	if(kbd_status & KBD_SCROLL) val |= 0x01;
	
	ps2_kbd_set_leds(val);
}


#if 0
unsigned char kbd_do_lookup(const unsigned char *lut, uint8_t sc)
{
	uint8_t	i;
	
	for(i = 0; lut[i]; i += 2)
		if(sc == lut[i])
			return lut[i + 1];
	return 0;
}
#endif 

uint16_t ps2_kbd_get_status()
{
	return kbd_status;
}

void PS2_KBD_INT(void)
{
	uint8_t	bit_in;
	
	if(kbd_status & KBD_SEND)
	{
		gpio_xor_mask(LED_MASK);
		// Send data
		switch (kbd_bit_n)
		{			
			case 9 :					// Parity bit
				if(kbd_n_bits & 0x01)
					gpio_put(PS2_DAT_Pin,false);
				else
					gpio_put(PS2_DAT_Pin,true);
			
				break;
				
			case 10 :					// Stop bit
				gpio_set_dir(PS2_DAT_Pin, false);
				break;
				
			case 11 : 					// ACK bit, set by device
				gpio_set_dir(PS2_DAT_Pin, false);
				kbd_buffer = 0;
				kbd_bit_n = 0;
				kbd_status &= ~KBD_SEND;
				break;
			
			default : 
				if(kbd_buffer & (1 << (kbd_bit_n - 1)))
				{
					gpio_put(PS2_DAT_Pin,true);
					kbd_n_bits++;
				} 
				else
				{
					gpio_put(PS2_DAT_Pin,false);
				}
				break;	
		}
	} 
	else
	{
		// Receive data
		
		bit_in=gpio_get(PS2_DAT_Pin) ? 0x01 : 0x00;
		
		switch (kbd_bit_n)
		{
			case 1 :						// ignore start bit
				break;
					
			case 10 :						// parity bit
				kbd_parity=bit_in;
				break;
							
			case 11 :
				kbd_n_bits&=0x01;
				
				// Only queue code if parity is valid.
				if (((kbd_parity==1) && (kbd_n_bits==0)) ||
				    ((kbd_parity==0) && (kbd_n_bits==1)))
					ps2_kbd_queue_scancode(kbd_buffer);

#if DEBUG_PS2
				log0("scan=%2X, parity=%d, kbd_n_bits=%d\n",kbd_buffer,kbd_parity,kbd_n_bits);
#endif	
				kbd_buffer = 0;
				kbd_bit_n = 0;
				kbd_parity = 0;
				kbd_n_bits = 0;
				break;
				
			default :						// data bits
				if(bit_in)
				{
					kbd_buffer |= (1 << (kbd_bit_n - 2));
					kbd_n_bits++;
				}
		}
		
	}
	
	kbd_bit_n++;
}

void ps2_keyboard_callback(uint gpio, uint32_t events)
{
	if ((PS2_CLK_Pin == gpio) && (events & GPIO_IRQ_EDGE_FALL)  && ((kbd_status & KBD_REQ_SEND)==0))
	{
		PS2_KBD_INT();	
	}
}
