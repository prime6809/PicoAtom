/* 
	matrix_kbd.h
	
	Definitions for handling a matrix keyboard output. 
	
	2009-04-27, P.Harvey-Smith.
*/

#define	KEY_DOWN	1
#define KEY_UP		0

#include <stdint.h>

// Type to hold the pointer to the callback routine to output keys to the 
// keyboard matrix, this takes, two paramters
// the first is the code to output, and can be any value, the second will
// be a 1 for a keypress and a 0 for a key release.
//
typedef void (*output_key_t)(uint8_t KeyCode, uint8_t State);

// initialise the matrix routines and set the output callback
void matrix_init(output_key_t	output,
				 output_key_t	callback);

// Check for an available scancode and output it.
void matrix_check_output(void);
