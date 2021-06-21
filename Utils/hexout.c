#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "hexout.h"

// Buffer used by HexDump, HexDumpHead
hexbuf_t HexDumpBuf;

// Initialize buffer for hex printing
void HexBegin(hexbuf_t  *buf,
              uint32_t  offs,
              bool      dohead)
{
    HexBeginLine(buf);
    buf->offset=offs;

    if(dohead) 
    {
        HexHead();
    }
}

void HexBeginLine(hexbuf_t  *buf)
{
    buf->charbuf[0]=0;
    buf->hexbuf[0]=0;
    buf->bufptr=0;
    buf->offset += HEXLINELEN;
}

void HexHead(void)
{
    printf("Offset    00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F ASCII\n");
    printf("---------------------------------------------------------------\n");
}

void HexOut(hexbuf_t    *buf)
{
    printf("%08lX %s %s\n",buf->offset,buf->hexbuf,buf->charbuf);
    HexBeginLine(buf);
}

#define ADDBUFLEN   4
// Add a character to the buffer and output line if full
void HexAdd(hexbuf_t    *buf,
            uint8_t     newchar)
{
    char    hex[ADDBUFLEN];

    if (buf->bufptr < HEXLINELEN)
    {
        snprintf(hex,ADDBUFLEN," %02X",newchar);
        strncat(buf->hexbuf,hex,HEXBUFLEN);
        buf->charbuf[buf->bufptr++] = (newchar < MIN_PRINT) ? '.' : newchar;
        buf->charbuf[buf->bufptr]=0;
    }

    if (HEXLINELEN == buf->bufptr)
    {
        HexOut(buf);
    }
}

// print any trailing characters in buffer
void HexEnd(hexbuf_t    *buf)
{
    char    hex[ADDBUFLEN] = "   ";

    if(0 != buf->bufptr)
    {
        while (buf->bufptr < HEXLINELEN)
        {
            strncat(buf->hexbuf,hex,HEXBUFLEN);
            buf->charbuf[buf->bufptr++] =  ' ';
        } 
        buf->charbuf[buf->bufptr]=0;
        
        HexOut(buf);
    }
}

void HexOutStr(hexbuf_t     *buf,
               char         *str)
{
    size_t Idx;

    for(Idx=0; Idx<strlen(str); Idx++)
    {
        HexAdd(buf,str[Idx]);
    }
}

// Dump an area of memory
void HexDump(const uint8_t 	*Buf, 
				   uint16_t Length)
{
    uint16_t    Idx;

    HexBegin(&HexDumpBuf,0,false);

    for(Idx=0; Idx < Length; Idx++)
    {
        HexAdd(&HexDumpBuf,Buf[Idx]);
    }
    HexEnd(&HexDumpBuf);
}

// Dump an area of memory, with headdings
void HexDumpHead(const uint8_t 	*Buf, 
				       uint16_t Length)
{
    HexHead();
    HexDump(Buf,Length);
}