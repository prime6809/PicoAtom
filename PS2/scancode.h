/*
	Scancode table for the AtomClone keyboard interface. 

 PCScanCodeTo Acorn Atom keyboard emulation routines.

 Adapted from PCScanCodeTo Dragon, P.Harvey-Smith, 2008-12-06.
 
 The Atom keyboard data is then output to a Zarlink MT8816 8x16 
 crosspoint switch.

 The outputs of this crosspoint switch are then connected to the 
 Atom Row and column inputs, which are scanned by the Atom's 8255
 using port B1..7 (and C6) as pulled up inputs, and port A0..3
 as outputs connected to a 74LS45 or 74LS145, to select the row 
 to scan. This gives 10 rows and 9 columns, however, the 9th
 column only has one key connected to it, which is either grounded
 or pulled up (the REPT key), so this can just be driven by a spare
 AVR IO line.

 As the Atom has 10 rows, these will be connected to X0..9 of the 
 MT8816, and the columns to Y0..7 (and an IO line).

 The Atom keyboard is laid out as follows :-

74LS145	Keyboard columns								
Output		PB0		PB1		PB2		PB3		PB4		PB5		PB6		PB7 	PC6
0			        3		-		G		Q		Esc			
1			        2		,		F		P		Z			
2			U/D		1		;		E		O		Y			
3			L/R		0		:		D		N		X			
4			Lock	Del		9		C		M		W			
5			^		Copy	8		B		L		V			
6			]		Enter	7		A		K		U			
7			\				6		@		J		T			
8			[				5		/		I		S			
9			spc				4		.		H		R			
 									
 GND														Ctrl	Shift	Rept

Fot the following key definition constants, the high nibble and 0x70 represent the 
ROW of the pressed key, and the low nibble represents the COLUMN. Bit 7 is ignored, 
as this is used by the matrix handling code to flag a key pressed or released.
*/

#ifndef __SCANCODES__
#define __SCANCODES__

#include <inttypes.h>
#include "ps2scancode.h"

#define ATOM_KEY_UD			0x02
#define ATOM_KEY_LR			0x03
#define ATOM_KEY_LOCK		0x04
#define ATOM_KEY_CLARET		0x05
#define ATOM_KEY_RBRACK		0x06
#define ATOM_KEY_BSLASH		0x07
#define ATOM_KEY_LBRACK		0x08
#define ATOM_KEY_SPACE		0x09

#define ATOM_KEY_3			0x10
#define ATOM_KEY_2			0x11
#define ATOM_KEY_1			0x12
#define ATOM_KEY_0			0x13
#define ATOM_KEY_DELETE		0x14
#define ATOM_KEY_COPY		0x15
#define ATOM_KEY_ENTER		0x16

#define ATOM_KEY_MINUS		0x20
#define ATOM_KEY_COMMA		0x21
#define ATOM_KEY_SEMICOLON 	0x22
#define ATOM_KEY_COLON 		0x23
#define ATOM_KEY_9 			0x24
#define ATOM_KEY_8 			0x25
#define ATOM_KEY_7 			0x26
#define ATOM_KEY_6 			0x27
#define ATOM_KEY_5 			0x28
#define ATOM_KEY_4 			0x29

#define ATOM_KEY_G 			0x30
#define ATOM_KEY_F 			0x31
#define ATOM_KEY_E 			0x32
#define ATOM_KEY_D 			0x33
#define ATOM_KEY_C 			0x34
#define ATOM_KEY_B 			0x35
#define ATOM_KEY_A 			0x36
#define ATOM_KEY_AT 		0x37
#define ATOM_KEY_SLASH 		0x38
#define ATOM_KEY_POINT 		0x39

#define ATOM_KEY_Q 			0x40
#define ATOM_KEY_P 			0x41
#define ATOM_KEY_O 			0x42
#define ATOM_KEY_N 			0x43
#define ATOM_KEY_M 			0x44
#define ATOM_KEY_L 			0x45
#define ATOM_KEY_K 			0x46
#define ATOM_KEY_J 			0x47
#define ATOM_KEY_I 			0x48
#define ATOM_KEY_H 			0x49

#define ATOM_KEY_ESC 		0x50
#define ATOM_KEY_Z 			0x51
#define ATOM_KEY_Y 			0x52
#define ATOM_KEY_X 			0x53
#define ATOM_KEY_W 			0x54
#define ATOM_KEY_V 			0x55
#define ATOM_KEY_U 			0x56
#define ATOM_KEY_T 			0x57
#define ATOM_KEY_S 			0x58
#define ATOM_KEY_R			0x59

#define ATOM_KEY_CTRL 		0x6A
#define ATOM_KEY_SHIFT 		0x7A
#define ATOM_KEY_REPT 		0x8A

#define ATOM_KEY_BREAK		0xFF

#define SHIFT_KEY	ATOM_KEY_SHIFT
#define RESET_KEY	ATOM_KEY_BREAK


static const uint8_t NewScancodeTable[] = {
// Prefix code			Scancode				Keycode
SCAN_CODE_NO_PREFIX,	SCAN_CODE_A,			ATOM_KEY_A,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_B,			ATOM_KEY_B,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_C,			ATOM_KEY_C,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_D,			ATOM_KEY_D,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_E,			ATOM_KEY_E,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_F,			ATOM_KEY_F,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_G,			ATOM_KEY_G,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_H,			ATOM_KEY_H,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_I,			ATOM_KEY_I,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_J,			ATOM_KEY_J,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_K,			ATOM_KEY_K,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_L,			ATOM_KEY_L,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_M,			ATOM_KEY_M,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_N,			ATOM_KEY_N,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_O,			ATOM_KEY_O,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_P,			ATOM_KEY_P,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_Q,			ATOM_KEY_Q,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_R,			ATOM_KEY_R,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_S,			ATOM_KEY_S,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_T,			ATOM_KEY_T,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_U,			ATOM_KEY_U,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_V,			ATOM_KEY_V,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_W,			ATOM_KEY_W,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_X,			ATOM_KEY_X,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_Y,			ATOM_KEY_Y,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_Z,			ATOM_KEY_Z,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_0,			ATOM_KEY_0,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_1,			ATOM_KEY_1,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_2,			ATOM_KEY_2,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_3,			ATOM_KEY_3,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_4,			ATOM_KEY_4,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_5,			ATOM_KEY_5,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_6,			ATOM_KEY_6,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_7,			ATOM_KEY_7,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_8,			ATOM_KEY_8,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_9,			ATOM_KEY_9,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_SPACE,		ATOM_KEY_SPACE,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_POINT,		ATOM_KEY_POINT,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_COMMA,		ATOM_KEY_COMMA,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_SLASH,		ATOM_KEY_SLASH,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_SEMICOLON,	ATOM_KEY_SEMICOLON,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_RBRACK,		ATOM_KEY_RBRACK,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_LBRACK,		ATOM_KEY_LBRACK,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_QUOTE,		ATOM_KEY_AT,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_BSLASH,		ATOM_KEY_BSLASH,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_MINUS,		ATOM_KEY_MINUS,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_EQUAL,		ATOM_KEY_COLON,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_TILDE,		ATOM_KEY_CLARET,

SCAN_CODE_NO_PREFIX,	SCAN_CODE_BACKSPACE,	ATOM_KEY_DELETE,

SCAN_CODE_NO_PREFIX,	SCAN_CODE_LSHIFT,		ATOM_KEY_SHIFT,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_LALT,			ATOM_KEY_SHIFT,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_RSHIFT,		ATOM_KEY_SHIFT,
SCAN_CODE_ESCAPE,		SCAN_CODE_LGUI,			ATOM_KEY_SHIFT,

SCAN_CODE_NO_PREFIX,	SCAN_CODE_LCTRL,		ATOM_KEY_CTRL,
SCAN_CODE_ESCAPE,		SCAN_CODE_RCTRL,		ATOM_KEY_CTRL,

SCAN_CODE_NO_PREFIX,	SCAN_CODE_ENTER,		ATOM_KEY_ENTER,
SCAN_CODE_ESCAPE,		SCAN_CODE_KPENTER,		ATOM_KEY_ENTER,

SCAN_CODE_ESCAPE,		SCAN_CODE_RALT,			ATOM_KEY_COPY,

SCAN_CODE_NO_PREFIX,	SCAN_CODE_ESC,			ATOM_KEY_ESC,

SCAN_CODE_ESCAPE,		SCAN_CODE_UARROW,		ATOM_KEY_UD,
SCAN_CODE_ESCAPE,		SCAN_CODE_RARROW,		ATOM_KEY_LR,

SCAN_CODE_NO_PREFIX,	SCAN_CODE_CAPSLOCK,		ATOM_KEY_LOCK,

SCAN_CODE_NO_PREFIX,	SCAN_CODE_KPPOINT,		ATOM_KEY_REPT,
SCAN_CODE_ESCAPE,		SCAN_CODE_KPPOINT,		ATOM_KEY_REPT,

SCAN_CODE_ESCAPE,		SCAN_CODE_PAUSE,		ATOM_KEY_BREAK,

// Keypad remaps for Bas
SCAN_CODE_NO_PREFIX,	SCAN_CODE_KP8,			ATOM_KEY_UD,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_KP6,			ATOM_KEY_LR,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_KP5,			ATOM_KEY_ENTER,

SCAN_CODE_TERMINATE, 	SCAN_CODE_TERMINATE, 	SCAN_CODE_TERMINATE
};


/*
  The shift key table, this has the same format as the above standard
  key table, however when one of these keys is pressed, the output shift
  key will also be pressed before the key, and released after the key.

  This allows as single PC key to translate to a SHIFT-KEY combination
  on the output matrix.

  A similar method could be used for other modifier keys.
*/

static const uint8_t ScancodeShiftTable[] = {
// Prefix code			Scancode				Keycode

SCAN_CODE_ESCAPE,		SCAN_CODE_DARROW,		ATOM_KEY_UD,
SCAN_CODE_ESCAPE,		SCAN_CODE_LARROW,		ATOM_KEY_LR,

// Keypad remaps for Bas
SCAN_CODE_NO_PREFIX,	SCAN_CODE_KP2,			ATOM_KEY_UD,
SCAN_CODE_NO_PREFIX,	SCAN_CODE_KP4,			ATOM_KEY_LR,

SCAN_CODE_TERMINATE, 	SCAN_CODE_TERMINATE, 	SCAN_CODE_TERMINATE
};

#endif
