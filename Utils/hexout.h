#include <stdint.h>
#include <stdbool.h>

#ifndef __HEXOUT_H__
#define __HEXOUT_H__

#define HEXLINELEN  16
#define HEXBUFLEN   ((HEXLINELEN * 3) + 1)
#define CHARBUFLEN  (HEXLINELEN + 1)

#define MIN_PRINT   0x20

typedef struct 
{
    char        hexbuf[HEXBUFLEN+1];        // Buffer for hex chars
    char        charbuf[CHARBUFLEN+1];      // Buffer for ASCII equivilents
    uint32_t    offset;                     // offset
    uint8_t     bufptr;                     // Buffer pointer
} hexbuf_t;

// Initialize buffer for hex printing
void HexBegin(hexbuf_t  *buf,
              uint32_t  offs,
              bool      dohead);

// Begin the next line of output
void HexBeginLine(hexbuf_t  *buf);             

// Add a character to the buffer and output line if full
void HexAdd(hexbuf_t    *buf,
            uint8_t     newchar);

// print any trailing characters in buffer
void HexEnd(hexbuf_t    *buf);

// Output a string of characters
void HexOutStr(hexbuf_t     *buf,
               char         *str);

// Print hex heading
void HexHead(void);

// Dump an area of memory
void HexDump(const uint8_t 	*Buf, 
				   uint16_t Length);


// Dump an area of memory, with headdings
void HexDumpHead(const uint8_t 	*Buf, 
				       uint16_t Length);
#endif