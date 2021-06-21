/* 
	PS/2 keyboard scancodes.
	
	2011-04-27, P.Harvey-Smith.
*/

#ifndef __PS2SCANCODE__
#define __PS2SCANCODE__

// normal scancodes 

#define SCAN_CODE_A	0x1C
#define SCAN_CODE_B	0x32
#define SCAN_CODE_C	0x21
#define SCAN_CODE_D	0x23
#define SCAN_CODE_E	0x24
#define SCAN_CODE_F	0x2B
#define SCAN_CODE_G	0x34
#define SCAN_CODE_H	0x33
#define SCAN_CODE_I	0x43
#define SCAN_CODE_J	0x3B
#define SCAN_CODE_K	0x42
#define SCAN_CODE_L	0x4B
#define SCAN_CODE_M	0x3A
#define SCAN_CODE_N	0x31
#define SCAN_CODE_O	0x44
#define SCAN_CODE_P	0x4D
#define SCAN_CODE_Q	0x15
#define SCAN_CODE_R	0x2D
#define SCAN_CODE_S	0x1B
#define SCAN_CODE_T	0x2C
#define SCAN_CODE_U	0x3C
#define SCAN_CODE_V	0x2A
#define SCAN_CODE_W	0x1D
#define SCAN_CODE_X	0x22
#define SCAN_CODE_Y	0x35
#define SCAN_CODE_Z	0x1A

#define SCAN_CODE_0	0x45
#define SCAN_CODE_1	0x16
#define SCAN_CODE_2	0x1E
#define SCAN_CODE_3	0x26
#define SCAN_CODE_4	0x25
#define SCAN_CODE_5	0x2E
#define SCAN_CODE_6	0x36
#define SCAN_CODE_7	0x3D
#define SCAN_CODE_8	0x3E
#define SCAN_CODE_9	0x46

#define SCAN_CODE_TILDE		0x0E
#define SCAN_CODE_MINUS		0x4E
#define SCAN_CODE_EQUAL		0x55
#define SCAN_CODE_BSLASH	0x5D
#define SCAN_CODE_BACKSPACE	0x66
#define SCAN_CODE_SPACE		0x29
#define SCAN_CODE_TAB		0x0D
#define SCAN_CODE_CAPSLOCK	0x58
#define SCAN_CODE_LSHIFT	0x12
#define SCAN_CODE_RSHIFT	0x59
#define SCAN_CODE_LCTRL		0x14
#define SCAN_CODE_LALT		0x11
#define SCAN_CODE_ENTER		0x5A
#define SCAN_CODE_ESC		0x76

#define SCAN_CODE_F1		0x05
#define SCAN_CODE_F2		0x06
#define SCAN_CODE_F3		0x04
#define SCAN_CODE_F4		0x0C
#define SCAN_CODE_F5		0x03
#define SCAN_CODE_F6		0x0B
#define SCAN_CODE_F7		0x83
#define SCAN_CODE_F8		0x0A
#define SCAN_CODE_F9		0x01
#define SCAN_CODE_F10		0x09
#define SCAN_CODE_F11		0x78
#define SCAN_CODE_F12		0x07

#define SCAN_CODE_SCROLL	0x7E
#define SCAN_CODE_LBRACK	0x54
#define SCAN_CODE_NUMLOCK	0x77
#define SCAN_CODE_KPSTAR	0x7C
#define SCAN_CODE_KPMINUS	0x7B
#define SCAN_CODE_KPPLUS	0x79
#define SCAN_CODE_KPPOINT	0x71
#define SCAN_CODE_KP0		0x70
#define SCAN_CODE_KP1		0x69
#define SCAN_CODE_KP2		0x72
#define SCAN_CODE_KP3		0x7A
#define SCAN_CODE_KP4		0x6B
#define SCAN_CODE_KP5		0x73
#define SCAN_CODE_KP6		0x74
#define SCAN_CODE_KP7		0x6C
#define SCAN_CODE_KP8		0x75
#define SCAN_CODE_KP9		0x7D
#define SCAN_CODE_RBRACK	0x5B
#define SCAN_CODE_SEMICOLON	0x4C
#define SCAN_CODE_QUOTE		0x52
#define SCAN_CODE_COMMA		0x41
#define SCAN_CODE_POINT		0x49
#define SCAN_CODE_SLASH		0x4A

// escaped scancodes, escaped by SCAN_CODE_ESCAPE, defined below


#define SCAN_CODE_LGUI		0x1F
#define SCAN_CODE_RCTRL		0x14
#define SCAN_CODE_RGUI		0x27
#define SCAN_CODE_RALT		0x11
#define SCAN_CODE_APPS		0x2F
#define SCAN_CODE_INSERT	0x70
#define SCAN_CODE_HOME		0x6C
#define SCAN_CODE_PGUP		0x7D
#define SCAN_CODE_DELETE	0x71
#define SCAN_CODE_END		0x69
#define SCAN_CODE_PGDN		0x7A
#define SCAN_CODE_UARROW	0x75
#define SCAN_CODE_LARROW	0x6B
#define SCAN_CODE_DARROW	0x72
#define SCAN_CODE_RARROW	0x74
#define SCAN_CODE_KPSLASH	0x4A
#define SCAN_CODE_KPENTER	0x5A

// ACPI also escaped by SCAN_CODE_ESCAPE

#define SCAN_CODE_POWER		0x37
#define SCAN_CODE_SLEEP		0x3F
#define SCAN_CODE_WAKE		0x5E

// Multimedia also escaped by SCAN_CODE_ESCAPE

#define SCAN_CODE_NEXTTRK	0x4D
#define SCAN_CODE_PREVTRK	0x15
#define SCAN_CODE_STOP		0x3B
#define SCAN_CODE_PLAYPAUSE	0x34
#define SCAN_CODE_MUTE		0x23
#define SCAN_CODE_VOLUP		0x32
#define SCAN_CODE_VOLDN		0x21
#define SCAN_CODE_MEDIASEL	0x50
#define SCAN_CODE_EMAIL		0x48
#define SCAN_CODE_CALC		0x2B
#define SCAN_CODE_MYCOMP	0x40
#define SCAN_CODE_WWWSEARCH	0x10
#define SCAN_CODE_WWWHOME	0x3A
#define SCAN_CODE_WWWBACK	0x38
#define SCAN_CODE_WWWFORWD	0x30
#define SCAN_CODE_WWWSTOP	0x28
#define SCAN_CODE_WWWREFR	0x20
#define SCAN_CODE_WWWFAVS	0x18


// Special scancodes
#define SCAN_CODE_NO_PREFIX	0x00	// Normal, no prefix
#define SCAN_CODE_ESCAPE	0xE0	// Escape the next scancode (if not release)
#define SCAN_CODE_BESCAPE	0xE1	// Escape code for break
#define SCAN_CODE_RELEASE	0xF0	// Release key prefix sent by keyboard.
#define SCAN_CODE_PAUSE		0x7E	// E0,7E is pause / ctrl-break on some keyboards
#define MAX_SCANCODE        0x83    // Maximum valid scancode

#define SCAN_CODE_TERMINATE	0xFF
#define MAX_SCANCODE_LEN    0x08    // Length of max scancode sequence

#define BREAK_SEQUENCE_LEN  0x08    // Length for break sequence

#define ResetScancode	0xF0		// Break scancode
#define ResetPrefixCode	SCAN_CODE_BESCAPE

#endif