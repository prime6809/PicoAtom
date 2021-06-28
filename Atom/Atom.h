/*
 * Atom.h
 *
 *  Created on: 9 Aug 2018
 *      Author: phill
 */
#include "main.h"
#include "delays.h"
#include "ILI9341_Driver.h"
#include "ILI9341_GFX.h"
#include "ILI9341_DMA.h"
#include "ILI9341_Text.h"
#include "ILI9341_Dragon.h"
#include "6847.h"
#include "fake6502.h"
#include "abasic.h"
#include "afloat.h"
#include "atommc2-2.9-e000.h"
#include "sdromraw.h"
#include "akernel.h"
#include "matrix_kbd.h"

#include <stdio.h>
#include <stdlib.h>

#ifndef ATOM_ATOM_H_
#define ATOM_ATOM_H_

#define ATOM_MAXRAM		1

#define ATOM_ZERO_SIZE	1024
#define ATOM_ZERO_BASE	0x0000

#if ATOM_MAXRAM == 1
#define ATOM_MAIN_SIZE	(31*1024)
#define ATOM_MAIN_BASE	0x0400
#define ATOM_VID_SIZE	(8*1024)
#define ATOM_VID_BASE	0x8000
#else
#define ATOM_MAIN_SIZE	(5*1024)
#define ATOM_MAIN_BASE	0x2800
#define ATOM_VID_SIZE	(6*1024)
#define ATOM_VID_BASE	0x8000
#endif

#define ATOM_EXT_BASE	0xA000
#define ATOM_IO_BASE	0xB000
#define ATOM_BAS_BASE	0xC000
#define ATOM_FP_BASE	0xD000
#define ATOM_DOS_BASE	0xE000
#define ATOM_SYS_BASE	0xF000

#define ATOM_8255_BASE	ATOM_IO_BASE
#define ATOM_8255_SIZE	0x400

#define ATOM_PL8_BASE	ATOM_IO_BASE+0x400
#define ATOM_PL8_SIZE	0x400

#define ATOM_6522_BASE	ATOM_IO_BASE+0x800
#define ATOM_6522_SIZE	0x400

#define ATOM_IO_SIZE	4096

#define ATOM_FS_INPUT	0x80

#define ATOM_ROM_SIZE	4096

#define ATOM_KEYROWS		11
#define ATOM_MOD_ROW		10
#define ATOM_KEYROW_MASK	0x0F
#define ATOM_KEYS_MASK		0x3F
#define ATOM_KEYS_MOD_MASK	0xC0
#define ATOM_KEYS_REPT_MASK	0x01

// Masks of 6847 control bits in PortA
#define ATOM_AG_MASK		0x10
#define ATOM_GM0_MASK		0x20
#define ATOM_GM1_MASK		0x40
#define ATOM_GM2_MASK		0x80

#define ATOM_PORT_VID_MASK	(ATOM_AG_MASK | ATOM_GM0_MASK | ATOM_GM1_MASK | ATOM_GM2_MASK)

// Masks of 6847 control bits in PortC
#define ATOM_SPEAKER_MASK	0x04
#define ATOM_CSS_MASK		0x08

// Set to true by VDG FS callback, to flag that the CPU should run.
extern volatile bool Run6502;

// Set to true to throttle the emulated CPU to approx 1MHz, set to false to let it
// run as fast as it can.....
extern volatile bool Throttle6502;

void AtomInit(void);
void AtomGo(void);
void AtomReadKey(void);
void ResetMachine(void);
void Atom_output_key(uint8_t	KeyCode, uint8_t	State);
void AtomHook(void);
void ScreenSaverPoll(void);

// VIA 6522 registers

#define DATAB       0x00
#define DATAA       0x01
#define DDRB        0x02
#define DDRA        0x03
#define T1CL        0x04
#define T1CH        0x05
#define T1LL        0x06
#define T1LH        0x07
#define T2CL        0x08
#define T2CH        0x09
#define SHIFT       0x0A
#define AUXCR       0x0B
#define PCR         0x0C
#define INTFR       0x0D
#define INTER       0x0E
#define DATAAA      0x0F

// VIA AUX register bitmaps,
// Shift register not currently emulated, so not defined
#define PB7OUT      0x80
#define T1INTC      0x40
#define CDOWN_PB6   0x20

#define LATCHEN     0x01

// Interrupt enable / flag register
#define INTANY      0x80
#define INTT1       0x40
#define INTT2       0x20

// screensaver countdown in seconds
#define SCREENSAVER_COUNT (60 * 5)

#endif /* ATOM_ATOM_H_ */
