/* Fake6502 CPU emulator core v1.1 *******************
 * (c)2011 Mike Chambers (miker00lz@gmail.com)       *
 *****************************************************
 * v1.1 - Small bugfix in BIT opcode, but it was the *
 *        difference between a few games in my NES   *
 *        emulator working and being broken!         *
 *        I went through the rest carefully again    *
 *        after fixing it just to make sure I didn't *
 *        have any other typos! (Dec. 17, 2011)      *
 *                                                   *
 * v1.0 - First release (Nov. 24, 2011)              *
 *****************************************************
 * LICENSE: This source code is released into the    *
 * public domain, but if you use it please do give   *
 * credit. I put a lot of effort into writing this!  *
 *                                                   *
 *****************************************************
 * Fake6502 is a MOS Technology 6502 CPU emulation   *
 * engine in C. It was written as part of a Nintendo *
 * Entertainment System emulator I've been writing.  *
 *                                                   *
 * A couple important things to know about are two   *
 * defines in the code. One is "UNDOCUMENTED" which, *
 * when defined, allows Fake6502 to compile with     *
 * full support for the more predictable             *
 * undocumented instructions of the 6502. If it is   *
 * undefined, undocumented opcodes just act as NOPs. *
 *                                                   *
 * The other define is "NES_CPU", which causes the   *
 * code to compile without support for binary-coded  *
 * decimal (BCD) support for the ADC and SBC         *
 * opcodes. The Ricoh 2A03 CPU in the NES does not   *
 * support BCD, but is otherwise identical to the    *
 * standard MOS 6502. (Note that this define is      *
 * enabled in this file if you haven't changed it    *
 * yourself. If you're not emulating a NES, you      *
 * should comment it out.)                           *
 *                                                   *
 * If you do discover an error in timing accuracy,   *
 * or operation in general please e-mail me at the   *
 * address above so that I can fix it. Thank you!    *
 *                                                   *
 *****************************************************
 * Usage:                                            *
 *                                                   *
 * Fake6502 requires you to provide two external     *
 * functions:                                        *
 *                                                   *
 * uint8_t read6502(uint16_t address)                *
 * void write6502(uint16_t address, uint8_t value)   *
 *                                                   *
 * You may optionally pass Fake6502 the pointer to a *
 * function which you want to be called after every  *
 * emulated instruction. This function should be a   *
 * void with no parameters expected to be passed to  *
 * it.                                               *
 *                                                   *
 * This can be very useful. For example, in a NES    *
 * emulator, you check the number of clock ticks     *
 * that have passed so you can know when to handle   *
 * APU events.                                       *
 *                                                   *
 * To pass Fake6502 this pointer, use the            *
 * hookexternal(void *funcptr) function provided.    *
 *                                                   *
 * To disable the hook later, pass NULL to it.       *
 *****************************************************
 * Useful functions in this emulator:                *
 *                                                   *
 * void reset6502()                                  *
 *   - Call this once before you begin execution.    *
 *                                                   *
 * void exec6502(uint32_t tickcount)                 *
 *   - Execute 6502 code up to the next specified    *
 *     count of clock ticks.                         *
 *                                                   *
 * void step6502()                                   *
 *   - Execute a single instrution.                  *
 *                                                   *
 * void irq6502()                                    *
 *   - Trigger a hardware IRQ in the 6502 core.      *
 *                                                   *
 * void nmi6502()                                    *
 *   - Trigger an NMI in the 6502 core.              *
 *                                                   *
 * void hookexternal(void *funcptr)                  *
 *   - Pass a pointer to a void function taking no   *
 *     parameters. This will cause Fake6502 to call  *
 *     that function once after each emulated        *
 *     instruction.                                  *
 *                                                   *
 *****************************************************
 * Useful variables in this emulator:                *
 *                                                   *
 * uint32_t clockticks6502                           *
 *   - A running total of the emulated cycle count.  *
 *                                                   *
 * uint32_t instructions                             *
 *   - A running total of the total emulated         *
 *     instruction count. This is not related to     *
 *     clock cycle timing.                           *
 *                                                   *
 *****************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "fake6502.h"
#include "status.h"
#include "str_util.h"

//6502 defines
#define UNDOCUMENTED //when this is defined, undocumented opcodes are handled.
                     //otherwise, they're simply treated as NOPs.

//#define NES_CPU      //when this is defined, the binary-coded decimal (BCD)
                     //status flag is not honored by ADC and SBC. the 2A03
                     //CPU in the Nintendo Entertainment System does not
                     //support BCD operation.

#define FLAG_CARRY     0x01
#define FLAG_ZERO      0x02
#define FLAG_INTERRUPT 0x04
#define FLAG_DECIMAL   0x08
#define FLAG_BREAK     0x10
#define FLAG_CONSTANT  0x20
#define FLAG_OVERFLOW  0x40
#define FLAG_SIGN      0x80

#define BASE_STACK     0x100

#define saveaccum(n) a = (uint8_t)((n) & 0x00FF)


//flag modifier macros
#define setcarry() 			status |= FLAG_CARRY
#define clearcarry() 		status &= (~FLAG_CARRY)
#define setzero() 			status |= FLAG_ZERO
#define clearzero() 		status &= (~FLAG_ZERO)
#define setinterrupt() 		status |= FLAG_INTERRUPT
#define clearinterrupt() 	status &= (~FLAG_INTERRUPT)
#define setdecimal() 		status |= FLAG_DECIMAL
#define cleardecimal() 		status &= (~FLAG_DECIMAL)
#define setoverflow() 		status |= FLAG_OVERFLOW
#define clearoverflow() 	status &= (~FLAG_OVERFLOW)
#define setsign() 			status |= FLAG_SIGN
#define clearsign() 		status &= (~FLAG_SIGN)

#define setflags(flags)		status |= (flags)
#define clearflags(flags)	status &= ~(flags)
#define flagset(flag)		((status & flag) ? 1 : 0)

//flag calculation macros
#define zerocalc(n) {\
    if ((n) & 0x00FF) clearzero();\
        else setzero();\
}

#define signcalc(n) {\
    if ((n) & 0x0080) setsign();\
        else clearsign();\
}

#define carrycalc(n) {\
    if ((n) & 0xFF00) setcarry();\
        else clearcarry();\
}

#define overflowcalc(n, m, o) { /* n = result, m = accumulator, o = memory */ \
    if (((n) ^ (uint16_t)(m)) & ((n) ^ (o)) & 0x0080) setoverflow();\
        else clearoverflow();\
}


//6502 CPU registers
uint16_t pc;
uint8_t sp, a, x, y, status;


//helper variables
uint32_t instructions = 0; //keep track of total instructions executed
uint32_t clockticks6502 = 0, clockgoal6502 = 0;
uint16_t oldpc, ea, reladdr, value, result;
uint8_t opcode, oldstatus;

//externally supplied functions
extern uint8_t read6502(uint16_t address);
extern void write6502(uint16_t address, uint8_t value);

// PHS - tracing
#if TRACE_ENABLED == 1

#define OP_STR_LEN		16
#define TRACE_BUF_LEN	64

uint8_t 	DoTrace		= 0;		// should we trace?
uint16_t	Location;				// Used to calculate location in realtive to x,y addressing
uint16_t	OldPC;					// Old value of PC
char		OpStr[OP_STR_LEN+1];

void TraceProc(char *op);
#define Trace(op)			do {if (DoTrace) TraceProc(op); } while(0)
#define TraceOP(format,...)	do {if (DoTrace) snprintf(OpStr,OP_STR_LEN,format,##__VA_ARGS__); } while(0)
#define TracePC()			OldPC=pc
#else
#define Trace(op)
#define TraceOP(format,...)
#define TracePC()
#endif

//a few general functions used by various other functions
void push16(uint16_t pushval) {
    write6502(BASE_STACK + sp, (pushval >> 8) & 0xFF);
    write6502(BASE_STACK + ((sp - 1) & 0xFF), pushval & 0xFF);
    sp -= 2;
}

void push8(uint8_t pushval) {
    write6502(BASE_STACK + sp--, pushval);
}

uint16_t pull16() {
    uint16_t temp16;
    temp16 = read6502(BASE_STACK + ((sp + 1) & 0xFF)) | ((uint16_t)read6502(BASE_STACK + ((sp + 2) & 0xFF)) << 8);
    sp += 2;
    return(temp16);
}

uint8_t pull8() {
    return (read6502(BASE_STACK + ++sp));
}

void reset6502() {
    pc = (uint16_t)read6502(0xFFFC) | ((uint16_t)read6502(0xFFFD) << 8);
    a = 0;
    x = 0;
    y = 0;
    sp = 0xFD;
    status |= FLAG_CONSTANT;
}


static void (*addrtable[256])();
static void (*optable[256])();
uint8_t penaltyop, penaltyaddr;

//addressing mode functions, calculates effective addresses
static void imp()
{ //implied
	TraceOP(" ");
}

static void acc()
{ //accumulator
	TraceOP(" ");
}

static void imm()
{ //immediate
    ea = pc++;
	TraceOP("#%02X",(uint8_t)ea);
}

static void zp()
{ //zero-page
    ea = (uint16_t)read6502((uint16_t)pc++);
	TraceOP("%04X",ea);
}

static void zpx()
{ //zero-page,X
#if TRACE_ENABLED == 0
	ea = ((uint16_t)read6502((uint16_t)pc++) + (uint16_t)x) & 0xFF; //zero-page wraparound
#else
	Location = (uint16_t)read6502((uint16_t)pc++);
	ea = (Location  + (uint16_t)x) & 0xFF; //zero-page wraparound

	TraceOP("%04X,x",Location);
#endif
}

static void zpy()
{ //zero-page,Y
#if TRACE_ENABLED == 0
    ea = ((uint16_t)read6502((uint16_t)pc++) + (uint16_t)y) & 0xFF; //zero-page wraparound
#else
	Location = (uint16_t)read6502((uint16_t)pc++);
	ea = (Location + (uint16_t)y) & 0xFF; //zero-page wraparound

	TraceOP("%04X,y",Location);
#endif
}

static void rel()
{ //relative for branch ops (8-bit immediate value, sign-extended)
    reladdr = (uint16_t)read6502(pc++);
    if (reladdr & 0x80) reladdr |= 0xFF00;

    TraceOP("%04X",(uint16_t)(pc+reladdr));
}

static void abso()
{ //absolute
    ea = (uint16_t)read6502(pc) | ((uint16_t)read6502(pc+1) << 8);
    pc += 2;

    TraceOP("%04X",ea);
}

static void absx()
{ //absolute,X
    uint16_t startpage;
    ea = ((uint16_t)read6502(pc) | ((uint16_t)read6502(pc+1) << 8));
    startpage = ea & 0xFF00;
    ea += (uint16_t)x;

    if (startpage != (ea & 0xFF00)) { //one cycle penlty for page-crossing on some opcodes
        penaltyaddr = 1;
    }

    pc += 2;

    TraceOP("%04X,x",ea);
}

static void absy()
{ //absolute,Y
    uint16_t startpage;
    ea = ((uint16_t)read6502(pc) | ((uint16_t)read6502(pc+1) << 8));
    startpage = ea & 0xFF00;
    ea += (uint16_t)y;

    if (startpage != (ea & 0xFF00)) { //one cycle penlty for page-crossing on some opcodes
        penaltyaddr = 1;
    }

    pc += 2;

    TraceOP("%04X,y",ea);
}

static void ind()
{ //indirect
    uint16_t eahelp, eahelp2;
    eahelp = (uint16_t)read6502(pc) | (uint16_t)((uint16_t)read6502(pc+1) << 8);
    eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //replicate 6502 page-boundary wraparound bug
    ea = (uint16_t)read6502(eahelp) | ((uint16_t)read6502(eahelp2) << 8);
    pc += 2;

    TraceOP("(%04X)",eahelp);
}

static void indx()
{ // (indirect,X)
    uint16_t eahelp;

#if TRACE_ENABLED == 0
    eahelp = (uint16_t)(((uint16_t)read6502(pc++) + (uint16_t)x) & 0xFF); //zero-page wraparound for table pointer
#else
    Location=(uint16_t)(uint16_t)read6502(pc++);
    eahelp = (uint16_t)((Location + (uint16_t)x) & 0xFF); //zero-page wraparound for table pointer

    TraceOP("(%04x,x)",eahelp);
#endif
    ea = (uint16_t)read6502(eahelp & 0x00FF) | ((uint16_t)read6502((eahelp+1) & 0x00FF) << 8);

}

static void indy()
{ // (indirect),Y
    uint16_t eahelp, eahelp2, startpage;
    eahelp = (uint16_t)read6502(pc++);
    eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //zero-page wraparound
    ea = (uint16_t)read6502(eahelp) | ((uint16_t)read6502(eahelp2) << 8);
    startpage = ea & 0xFF00;
    ea += (uint16_t)y;

    if (startpage != (ea & 0xFF00)) { //one cycle penlty for page-crossing on some opcodes
        penaltyaddr = 1;
    }

    TraceOP("(%04x),y",eahelp2);
}

static uint16_t getvalue() {
    if (addrtable[opcode] == acc) return((uint16_t)a);
        else return((uint16_t)read6502(ea));
}

/* unused
static uint16_t getvalue16() {
    return((uint16_t)read6502(ea) | ((uint16_t)read6502(ea+1) << 8));
}
*/

static void putvalue(uint16_t saveval) {
    if (addrtable[opcode] == acc) a = (uint8_t)(saveval & 0x00FF);
        else write6502(ea, (saveval & 0x00FF));
}


//instruction handler functions
// Fixes copied from MAME / MESS source - PHS.
static void adc()
{
    penaltyop = 1;
    value = getvalue();

#ifndef NES_CPU
    if (flagset(FLAG_DECIMAL))
    {
    	uint8_t	ahigh,alow;
    	uint8_t	carry_in = flagset(FLAG_CARRY) ? 1 : 0;

    	clearflags(FLAG_SIGN | FLAG_OVERFLOW | FLAG_ZERO | FLAG_CARRY);

    	// calculate LSN
    	alow = (a & 0xF) + (value & 0xF) + carry_in;

    	// calculate half carry to MSN
    	if (alow > 9)
    		alow+=6;

    	// calculate MSN, merging half carry
    	ahigh = (a >> 4) + (value >> 4) + (alow > 0xF);

    	// Set flags as needed
    	if(!(uint8_t)(a + value + carry_in))
    		setflags(FLAG_ZERO);
    	else if (ahigh & 8)
    		setflags(FLAG_SIGN);

    	if(~(a^value) & (a^(ahigh << 4)) & 0x80)
    		setflags(FLAG_OVERFLOW);

    	if(ahigh > 9)
    		ahigh+=6;

    	if (ahigh > 0xF)
    		setflags(FLAG_CARRY);

    	result = (ahigh << 4) | (alow & 0xF);
    }
    else
#endif
    {
    	result = (uint16_t)a + value + (uint16_t)(status & FLAG_CARRY);

    	carrycalc(result);
    	zerocalc(result);
    	overflowcalc(result, a, value);
    	signcalc(result);
    }
    saveaccum(result);

    Trace("adc");
}

static void and()
{
    penaltyop = 1;
    value = getvalue();
    result = (uint16_t)a & value;
   
    zerocalc(result);
    signcalc(result);
   
    saveaccum(result);

    Trace("and");
}

static void asl()
{
    value = getvalue();
    result = value << 1;

    carrycalc(result);
    zerocalc(result);
    signcalc(result);
   
    putvalue(result);

    Trace("asl");
}

static void bcc()
{
    if ((status & FLAG_CARRY) == 0)
    {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
            else clockticks6502++;
    }

    Trace("bcc");
}

static void bcs()
{
    if ((status & FLAG_CARRY) == FLAG_CARRY)
    {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
            else clockticks6502++;
    }

    Trace("bcs");
}

static void beq()
{
    if ((status & FLAG_ZERO) == FLAG_ZERO)
    {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
            else clockticks6502++;
    }
    Trace("beq");
}

static void bit()
{
    value = getvalue();
    result = (uint16_t)a & value;
   
    zerocalc(result);
    status = (status & 0x3F) | (uint8_t)(value & 0xC0);

    Trace("bit");
}

static void bmi()
{
    if ((status & FLAG_SIGN) == FLAG_SIGN)
    {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00))
        	clockticks6502 += 2; //check if jump crossed a page boundary
        else
        	clockticks6502++;
    }
    Trace("bmi");
}

static void bne()
{
    if ((status & FLAG_ZERO) == 0)
    {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00))
        	clockticks6502 += 2; //check if jump crossed a page boundary
        else
        	clockticks6502++;
    }
    Trace("bne");
}

static void bpl()
{
    if ((status & FLAG_SIGN) == 0)
    {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00))
        	clockticks6502 += 2; //check if jump crossed a page boundary
        else
        	clockticks6502++;
    }
    Trace("bpl");
}

static void brk()
{
    pc++;
    push16(pc); //push next instruction address onto stack
    push8(status | FLAG_BREAK); //push CPU status to stack
    setinterrupt(); //set interrupt flag
    pc = (uint16_t)read6502(0xFFFE) | ((uint16_t)read6502(0xFFFF) << 8);

    Trace("brk");
}

static void bvc()
{
    if ((status & FLAG_OVERFLOW) == 0)
    {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00))
        	clockticks6502 += 2; //check if jump crossed a page boundary
        else
        	clockticks6502++;
    }
    Trace("bvc");
}

static void bvs()
{
    if ((status & FLAG_OVERFLOW) == FLAG_OVERFLOW)
    {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00))
        	clockticks6502 += 2; //check if jump crossed a page boundary
        else
        	clockticks6502++;
    }
    Trace("bvs");
}

static void clc()
{
    clearcarry();
    Trace("clc");
}

static void cld()
{
    cleardecimal();
    Trace("cld");
}

static void cli()
{
    clearinterrupt();
    Trace("cli");
}

static void clv()
{
    clearoverflow();
    Trace("clv");
}

static void cmp()
{
    penaltyop = 1;
    value = getvalue();
    result = (uint16_t)a - value;
   
    if (a >= (uint8_t)(value & 0x00FF))
    	setcarry();
    else
    	clearcarry();

    if (a == (uint8_t)(value & 0x00FF))
    	setzero();
    else
    	clearzero();

    signcalc(result);

    Trace("cmp");
}

static void cpx()
{
    value = getvalue();
    result = (uint16_t)x - value;
   
    if (x >= (uint8_t)(value & 0x00FF))
    	setcarry();
    else
    	clearcarry();

    if (x == (uint8_t)(value & 0x00FF))
    	setzero();
    else
    	clearzero();

    signcalc(result);

    Trace("cpx");
}

static void cpy()
{
    value = getvalue();
    result = (uint16_t)y - value;
   
    if (y >= (uint8_t)(value & 0x00FF))
    	setcarry();
    else
    	clearcarry();

    if (y == (uint8_t)(value & 0x00FF))
    	setzero();
    else
    	clearzero();

    signcalc(result);

    Trace("cpy");
}

static void dec()
{
    value = getvalue();
    result = value - 1;
   
    zerocalc(result);
    signcalc(result);
   
    putvalue(result);

    Trace("dec");
}

static void dex()
{
    x--;
   
    zerocalc(x);
    signcalc(x);

    Trace("dex");
}

static void dey()
{
    y--;
   
    zerocalc(y);
    signcalc(y);

    Trace("dey");
}

static void eor()
{
    penaltyop = 1;
    value = getvalue();
    result = (uint16_t)a ^ value;
   
    zerocalc(result);
    signcalc(result);
   
    saveaccum(result);

    Trace("eor");
}

static void inc()
{
    value = getvalue();
    result = value + 1;
   
    zerocalc(result);
    signcalc(result);
   
    putvalue(result);

    Trace("inc");
}

static void inx()
{
    x++;
   
    zerocalc(x);
    signcalc(x);

    Trace("inx");
}

static void iny()
{
    y++;
   
    zerocalc(y);
    signcalc(y);

    Trace("iny");
}

static void jmp()
{
    pc = ea;

    Trace("jmp");
}

static void jsr()
{
    push16(pc - 1);
    pc = ea;

    Trace("jsr");
}

static void lda()
{
    penaltyop = 1;
    value = getvalue();
    a = (uint8_t)(value & 0x00FF);
   
    zerocalc(a);
    signcalc(a);

    Trace("lda");
}

static void ldx()
{
    penaltyop = 1;
    value = getvalue();
    x = (uint8_t)(value & 0x00FF);
   
    zerocalc(x);
    signcalc(x);

    Trace("ldx");
}

static void ldy()
{
    penaltyop = 1;
    value = getvalue();
    y = (uint8_t)(value & 0x00FF);
   
    zerocalc(y);
    signcalc(y);

    Trace("ldy");
}

static void lsr()
{
    value = getvalue();
    result = value >> 1;
   
    if (value & 1)
    	setcarry();
    else
    	clearcarry();

    zerocalc(result);
    signcalc(result);
   
    putvalue(result);

    Trace("lsr");
}

static void nop()
{
    switch (opcode) {
        case 0x1C:
        case 0x3C:
        case 0x5C:
        case 0x7C:
        case 0xDC:
        case 0xFC:
            penaltyop = 1;
            break;
    }

    Trace("nop");
}

static void ora()
{
    penaltyop = 1;
    value = getvalue();
    result = (uint16_t)a | value;
   
    zerocalc(result);
    signcalc(result);
   
    saveaccum(result);

    Trace("ora");
}

static void pha()
{
    push8(a);

    Trace("pha");
}

static void php()
{
    push8(status | FLAG_BREAK);

    Trace("php");
}

static void pla()
{
    a = pull8();
   
    zerocalc(a);
    signcalc(a);

    Trace("pla");
}

static void plp()
{
    status = pull8() | FLAG_CONSTANT;

    Trace("plp");
}

static void rol()
{
    value = getvalue();
    result = (value << 1) | (status & FLAG_CARRY);
   
    carrycalc(result);
    zerocalc(result);
    signcalc(result);
   
    putvalue(result);

    Trace("rol");
}

static void ror()
{
    value = getvalue();
    result = (value >> 1) | ((status & FLAG_CARRY) << 7);
   
    if (value & 1)
    	setcarry();
    else
    	clearcarry();

    zerocalc(result);
    signcalc(result);
   
    putvalue(result);

    Trace("ror");
}

static void rti()
{
    status = pull8();
    value = pull16();
    pc = value;

    Trace("rti");
}

static void rts()
{
    value = pull16();
    pc = value + 1;

    Trace("rts");
}

static void sbc()
{
    penaltyop = 1;
#ifndef NES_CPU
    if(flagset(FLAG_DECIMAL))
    {
        value 					= getvalue() & 0x00FF;
    	uint8_t		carry_in 	= flagset(FLAG_CARRY) ?  0 : 1;
    	uint16_t	diff 		= a - value - carry_in;
    	uint8_t		ahigh,alow;
    	uint8_t		halfcarry	= 0;

    	clearflags(FLAG_SIGN | FLAG_OVERFLOW | FLAG_ZERO | FLAG_CARRY);

    	// calc lsn
    	alow = (a & 0xF) - (value & 0xF) - carry_in;

    	// lsn overflow, deal with carry.
    	if(alow & 0x10)
    	{
    		alow -= 6;
    		halfcarry=1;
    		alow &= 0x0F;
    	}

    	// calc msn
    	ahigh = (a >> 4) - (value >> 4);
    	if (halfcarry)
    		ahigh--;

    	if(!(uint8_t)diff)
    		setflags(FLAG_ZERO);
    	else if (diff & 0x80)
    		setflags(FLAG_SIGN);

    	if((a^value) & (a^diff) & 0x80)
    		setflags(FLAG_OVERFLOW);

    	if(!(diff & 0xff00))
    		setflags(FLAG_CARRY);

    	if(ahigh & 0x10)
    	{
    		ahigh -= 6;
    		ahigh &= 0x0F;
    	}
    	result = (ahigh << 4) | (alow & 0xF);
    }
    else
#endif
    {
        value = getvalue() ^ 0x00FF;

    	result = (uint16_t)a + value + (uint16_t)(status & FLAG_CARRY);

    	carrycalc(result);
    	zerocalc(result);
    	overflowcalc(result, a, value);
    	signcalc(result);
    }

    saveaccum(result);

    Trace("sbc");
}

static void sec()
{
    setcarry();

    Trace("sec");
}

static void sed()
{
    setdecimal();

    Trace("sed");
}

static void sei()
{
    setinterrupt();

    Trace("sei");
}

static void sta()
{
    putvalue(a);

    Trace("sta");
}

static void stx()
{
    putvalue(x);

    Trace("stx");
}

static void sty()
{
    putvalue(y);

    Trace("sty");
}

static void tax()
{
    x = a;
   
    zerocalc(x);
    signcalc(x);

    Trace("tax");
}

static void tay()
{
    y = a;
   
    zerocalc(y);
    signcalc(y);

    Trace("tay");
}

static void tsx()
{
    x = sp;
   
    zerocalc(x);
    signcalc(x);

    Trace("tsx");
}

static void txa()
{
    a = x;
   
    zerocalc(a);
    signcalc(a);

    Trace("txa");
}

static void txs()
{
    sp = x;

    Trace("tsx");
}

static void tya()
{
    a = y;
   
    zerocalc(a);
    signcalc(a);

    Trace("tya");
}

//undocumented instructions
#ifdef UNDOCUMENTED
static void lax()
{
	lda();
    ldx();

    Trace("lax");
}

static void sax()
{
	sta();
	stx();
	putvalue(a & x);
	if (penaltyop && penaltyaddr) clockticks6502--;

    Trace("sax");
}

static void dcp()
{
	dec();
    cmp();
    if (penaltyop && penaltyaddr) clockticks6502--;

    Trace("dcp");
}

static void isb()
{
	inc();
    sbc();
    if (penaltyop && penaltyaddr) clockticks6502--;

    Trace("isb");
}

static void slo()
{
	asl();
    ora();
    if (penaltyop && penaltyaddr) clockticks6502--;

    Trace("slo");
}

static void rla()
{
	rol();
    and();
    if (penaltyop && penaltyaddr) clockticks6502--;

    Trace("rla");
}

static void sre()
{
	lsr();
    eor();
    if (penaltyop && penaltyaddr) clockticks6502--;

    Trace("sre");
}

static void rra()
{
	ror();
    adc();
    if (penaltyop && penaltyaddr) clockticks6502--;

    Trace("rra");
}
#else
#define lax nop
#define sax nop
#define dcp nop
#define isb nop
#define slo nop
#define rla nop
#define sre nop
#define rra nop
#endif


static void (*addrtable[256])() = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */     imp, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm, abso, abso, abso, abso, /* 0 */
/* 1 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 1 */
/* 2 */    abso, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm, abso, abso, abso, abso, /* 2 */
/* 3 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 3 */
/* 4 */     imp, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm, abso, abso, abso, abso, /* 4 */
/* 5 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 5 */
/* 6 */     imp, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm,  ind, abso, abso, abso, /* 6 */
/* 7 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 7 */
/* 8 */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* 8 */
/* 9 */     rel, indy,  imp, indy,  zpx,  zpx,  zpy,  zpy,  imp, absy,  imp, absy, absx, absx, absy, absy, /* 9 */
/* A */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* A */
/* B */     rel, indy,  imp, indy,  zpx,  zpx,  zpy,  zpy,  imp, absy,  imp, absy, absx, absx, absy, absy, /* B */
/* C */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* C */
/* D */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* D */
/* E */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* E */
/* F */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx  /* F */
};

static void (*optable[256])() = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |      */
/* 0 */      brk,  ora,  nop,  slo,  nop,  ora,  asl,  slo,  php,  ora,  asl,  nop,  nop,  ora,  asl,  slo, /* 0 */
/* 1 */      bpl,  ora,  nop,  slo,  nop,  ora,  asl,  slo,  clc,  ora,  nop,  slo,  nop,  ora,  asl,  slo, /* 1 */
/* 2 */      jsr,  and,  nop,  rla,  bit,  and,  rol,  rla,  plp,  and,  rol,  nop,  bit,  and,  rol,  rla, /* 2 */
/* 3 */      bmi,  and,  nop,  rla,  nop,  and,  rol,  rla,  sec,  and,  nop,  rla,  nop,  and,  rol,  rla, /* 3 */
/* 4 */      rti,  eor,  nop,  sre,  nop,  eor,  lsr,  sre,  pha,  eor,  lsr,  nop,  jmp,  eor,  lsr,  sre, /* 4 */
/* 5 */      bvc,  eor,  nop,  sre,  nop,  eor,  lsr,  sre,  cli,  eor,  nop,  sre,  nop,  eor,  lsr,  sre, /* 5 */
/* 6 */      rts,  adc,  nop,  rra,  nop,  adc,  ror,  rra,  pla,  adc,  ror,  nop,  jmp,  adc,  ror,  rra, /* 6 */
/* 7 */      bvs,  adc,  nop,  rra,  nop,  adc,  ror,  rra,  sei,  adc,  nop,  rra,  nop,  adc,  ror,  rra, /* 7 */
/* 8 */      nop,  sta,  nop,  sax,  sty,  sta,  stx,  sax,  dey,  nop,  txa,  nop,  sty,  sta,  stx,  sax, /* 8 */
/* 9 */      bcc,  sta,  nop,  nop,  sty,  sta,  stx,  sax,  tya,  sta,  txs,  nop,  nop,  sta,  nop,  nop, /* 9 */
/* A */      ldy,  lda,  ldx,  lax,  ldy,  lda,  ldx,  lax,  tay,  lda,  tax,  nop,  ldy,  lda,  ldx,  lax, /* A */
/* B */      bcs,  lda,  nop,  lax,  ldy,  lda,  ldx,  lax,  clv,  lda,  tsx,  lax,  ldy,  lda,  ldx,  lax, /* B */
/* C */      cpy,  cmp,  nop,  dcp,  cpy,  cmp,  dec,  dcp,  iny,  cmp,  dex,  nop,  cpy,  cmp,  dec,  dcp, /* C */
/* D */      bne,  cmp,  nop,  dcp,  nop,  cmp,  dec,  dcp,  cld,  cmp,  nop,  dcp,  nop,  cmp,  dec,  dcp, /* D */
/* E */      cpx,  sbc,  nop,  isb,  cpx,  sbc,  inc,  isb,  inx,  sbc,  nop,  sbc,  cpx,  sbc,  inc,  isb, /* E */
/* F */      beq,  sbc,  nop,  isb,  nop,  sbc,  inc,  isb,  sed,  sbc,  nop,  isb,  nop,  sbc,  inc,  isb  /* F */
};

static const uint32_t ticktable[256] = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */      7,    6,    2,    8,    3,    3,    5,    5,    3,    2,    2,    2,    4,    4,    6,    6,  /* 0 */
/* 1 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 1 */
/* 2 */      6,    6,    2,    8,    3,    3,    5,    5,    4,    2,    2,    2,    4,    4,    6,    6,  /* 2 */
/* 3 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 3 */
/* 4 */      6,    6,    2,    8,    3,    3,    5,    5,    3,    2,    2,    2,    3,    4,    6,    6,  /* 4 */
/* 5 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 5 */
/* 6 */      6,    6,    2,    8,    3,    3,    5,    5,    4,    2,    2,    2,    5,    4,    6,    6,  /* 6 */
/* 7 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 7 */
/* 8 */      2,    6,    2,    6,    3,    3,    3,    3,    2,    2,    2,    2,    4,    4,    4,    4,  /* 8 */
/* 9 */      2,    6,    2,    6,    4,    4,    4,    4,    2,    5,    2,    5,    5,    5,    5,    5,  /* 9 */
/* A */      2,    6,    2,    6,    3,    3,    3,    3,    2,    2,    2,    2,    4,    4,    4,    4,  /* A */
/* B */      2,    5,    2,    5,    4,    4,    4,    4,    2,    4,    2,    4,    4,    4,    4,    4,  /* B */
/* C */      2,    6,    2,    8,    3,    3,    5,    5,    2,    2,    2,    2,    4,    4,    6,    6,  /* C */
/* D */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* D */
/* E */      2,    6,    2,    8,    3,    3,    5,    5,    2,    2,    2,    2,    4,    4,    6,    6,  /* E */
/* F */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7   /* F */
};


void nmi6502() {
    push16(pc);
    push8(status);
    status |= FLAG_INTERRUPT;
    pc = (uint16_t)read6502(0xFFFA) | ((uint16_t)read6502(0xFFFB) << 8);
}

void irq6502() {
    push16(pc);
    push8(status);
    status |= FLAG_INTERRUPT;
    pc = (uint16_t)read6502(0xFFFE) | ((uint16_t)read6502(0xFFFF) << 8);
}

uint8_t callexternal = 0;
void (*loopexternal)();

void exec6502(uint32_t tickcount)
{
    clockgoal6502 += tickcount;
   
    while (clockticks6502 < clockgoal6502)
    {
    	TracePC();
    	opcode = read6502(pc++);
        status |= FLAG_CONSTANT;

        penaltyop = 0;
        penaltyaddr = 0;

        (*addrtable[opcode])();
        (*optable[opcode])();
        clockticks6502 += ticktable[opcode];
        if (penaltyop && penaltyaddr) clockticks6502++;

        instructions++;

        if (callexternal) (*loopexternal)();
    }

}

void step6502()
{
	TracePC();
	opcode = read6502(pc++);
    status |= FLAG_CONSTANT;

    penaltyop = 0;
    penaltyaddr = 0;

    (*addrtable[opcode])();
    (*optable[opcode])();
    clockticks6502 += ticktable[opcode];
    if (penaltyop && penaltyaddr) clockticks6502++;
    clockgoal6502 = clockticks6502;

    instructions++;

    if (callexternal) (*loopexternal)();
}

void hookexternal(void *funcptr) {
    if (funcptr != (void *)NULL) {
        loopexternal = funcptr;
        callexternal = 1;
    } else callexternal = 0;
}

#define REGBUFF_SIZE	64

char *regdump(void)
{
	static char	RegBuff[REGBUFF_SIZE+1];

	snprintf(RegBuff,REGBUFF_SIZE,"PC=%04X, a=%02X, x=%02X, y=%02X, sp=%02x cc=%02X",pc,a,x,y,sp,status);

	return RegBuff;
}

void trace(uint8_t	flag)
{
#if TRACE_ENABLED==1
	DoTrace = flag;
#endif
}

#if TRACE_ENABLED==1
void TraceProc(char *op)
{
	char	TraceBuff[TRACE_BUF_LEN+1];

	if (DoTrace)
	{
	 	  snprintf(TraceBuff,TRACE_BUF_LEN,"%04X %s %s",OldPC,op,OpStr);
	  	  if (DoTrace==2)
		  {
	  		  PadTo(TraceBuff,16);
		  	  strcat(TraceBuff,regdump());
		  }
	  	  log0("%s\n",TraceBuff);
	}
}
#endif
