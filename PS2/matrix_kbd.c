/* 
	matrix_kbd.h
	
	Definitions for handling a matrix keyboard output. 
		
	2009-07-23, P.Harvey-Smith.
    
    2010-05-03, Fixed so that CTRL-BREAK now works.
	
	2011-04-27, Seperated driver for MT8816 into seperate driver file.
				Changed from using a table containing all the possible scancodes
				both used and unused to a table that only contains used scancodes.
*/

#include "matrix_kbd.h"
#include "ps2kbd.h"
#include "scancode.h"
#include "status.h"

static uint8_t	LastScanCode;
static uint8_t	PrefixCode;

static uint8_t  Pos;
static uint8_t  CodeCount;
static uint8_t	InBreak;

static output_key_t	output_key = NULL;	
static output_key_t	key_callback = NULL;	

extern void ResetMachine(void);

// initialise the matrix routines and set the output callback
void matrix_init(output_key_t	output,
				 output_key_t	callback)
{
	log0("Keyboard matrix init\n");
	LastScanCode=0x00;
	PrefixCode=SCAN_CODE_NO_PREFIX;

	output_key=output;
	key_callback=callback;

    // Init Scancode buffer variables
    Pos=0;
    CodeCount=0;
	InBreak=0;
}

static uint8_t LookupKeys(uint8_t	Scancode,
					      uint8_t 	IsShift)
{
	uint16_t	Offset 	= 0;
	uint8_t		*Table;
	uint8_t		Prefix;
	uint8_t		Code;	
	uint8_t		MatrixCode;	
	uint8_t		Handled	= 0;
	
	// Select which table to use
	if(IsShift)
		Table=(uint8_t  *)ScancodeShiftTable;
	else
		Table=(uint8_t  *)NewScancodeTable;
	
	// Point at initial values
	Prefix 		= Table[Offset++];
	Code		= Table[Offset++];
	MatrixCode	= Table[Offset++];
	
	//log0("LookupKeys(%02X,%d), Table=%05X\n",Scancode,IsShift,Table);
	
	// Scan through shift code table
	while((Prefix!=SCAN_CODE_TERMINATE) && (!Handled))
	{
		//log0("o=%d, p=%02X, c=%02X, z=%02X\n",Offset,Prefix,Code,ZXCode);
		// if code found, output it.
		// If IsShift is true press shift then key
		// on release release key then shift
		if((PrefixCode==Prefix) && (Scancode==Code))
		{
			if(LastScanCode!=SCAN_CODE_RELEASE)
			{
				if(MatrixCode!=RESET_KEY)
				{
					if(IsShift)
						output_key(SHIFT_KEY,KEY_DOWN);

					output_key(MatrixCode,KEY_DOWN);
				}
				
				Handled++;
			}
			else
			{
				if(MatrixCode!=RESET_KEY)
				{
					output_key(MatrixCode,KEY_UP);
					if(IsShift)
						output_key(SHIFT_KEY,KEY_UP);
				}
				else
					ResetMachine();
					
				Handled++;
			}
			Prefix=SCAN_CODE_TERMINATE;
		}
		else
		{
			Prefix 		= Table[Offset++];
			Code		= Table[Offset++];
			MatrixCode	= Table[Offset++];
		}
	}
	
	return Handled;
}

/*
	Check the scancode queue, and if codes are available interpret them, possibly
	outputting keys to the matrix driver.
*/

void matrix_check_output(void)
{
	uint8_t	Scancode;
//	uint8_t	KeyCode;
	uint8_t	KeyValid;
	uint8_t	IsShift;
	
	if(output_key==NULL)
	{
		log0("ERROR, no matrix driver initialised, please initialise first with matrix_init()\n");
		return;
	}
	
	// Check for a waiting scancode
	Scancode=ps2_kbd_get_scancode();
	
	// If it's a valid scancode process it
	if(Scancode!=0)
	{
        //log0("%2X ",Scancode);
		//if(Scancode!=LastScanCode)
        //    log0("Last:%2.2X Scan:%2.2X Prefix:%2.2X Count=%d\n",LastScanCode,Scancode,PrefixCode,CodeCount);
	
		if (CodeCount>0)
		{
        	CodeCount--;
			if(CodeCount==0)
			{
				if(PrefixCode==SCAN_CODE_BESCAPE)
				{
					ResetMachine();
				}
				LastScanCode=0;
				PrefixCode=0;
			}
		}
		else
		{
			switch (Scancode)
			{
                // Check for various escape scancodes and set prefix as needed
				case SCAN_CODE_ESCAPE :
					PrefixCode=SCAN_CODE_ESCAPE;
					break;
			
				// Break key ignore scan codes
				case SCAN_CODE_BESCAPE :
					PrefixCode=SCAN_CODE_BESCAPE;
					CodeCount=BREAK_SEQUENCE_LEN-1;
					break;
								
				case SCAN_CODE_F12 :
					if(LastScanCode!=SCAN_CODE_RELEASE)
						ResetMachine();	// Atomulator compatibility.
					break;
								
				default: 
					if(Scancode<=MAX_SCANCODE)
					{
						// Lookup normal unshifted keys
						IsShift=0;
						KeyValid=LookupKeys(Scancode,IsShift);
						
						// If key was not found then try the shifted table
						if (!KeyValid)
						{
							IsShift=1;
							KeyValid=LookupKeys(Scancode,IsShift);
						}
							
						if(key_callback!=NULL)
						{
							if(LastScanCode!=SCAN_CODE_RELEASE)
								key_callback(Scancode,KEY_DOWN);
							else
								key_callback(Scancode,KEY_UP);
						}
							
						PrefixCode=SCAN_CODE_NO_PREFIX;
					}
					break;
			}
			// Update last scancode
			LastScanCode=Scancode;
		}
	}
}	
