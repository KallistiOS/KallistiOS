/* sh-stub.c -- debugging stub for the Hitachi-SH.

 NOTE!! This code has to be compiled with optimization, otherwise the
 function inlining which generates the exception handlers won't work.

*/

/*   This is originally based on an m68k software stub written by Glenn
     Engel at HP, but has changed quite a bit.

     Modifications for the SH by Ben Lee and Steve Chamberlain

     Modifications for KOS by Megan Potter (earlier) and Richard Moats (more
     recently).
*/

/****************************************************************************

        THIS SOFTWARE IS NOT COPYRIGHTED

   HP offers the following for use in the public domain.  HP makes no
   warranty with regard to the software or it's performance and the
   user accepts the software "AS IS" with all faults.

   HP DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD
   TO THIS SOFTWARE INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

****************************************************************************/


/* Remote communication protocol.

   A debug packet whose contents are <data>
   is encapsulated for transmission in the form:

    $ <data> # CSUM1 CSUM2

    <data> must be ASCII alphanumeric and cannot include characters
    '$' or '#'.  If <data> starts with two characters followed by
    ':', then the existing stubs interpret this as a sequence number.

    CSUM1 and CSUM2 are ascii hex representation of an 8-bit
    checksum of <data>, the most significant nibble is sent first.
    the hex digits 0-9,a-f are used.

   Receiver responds with:

    +   - if CSUM is correct and ready for next packet
    -   - if CSUM is incorrect

   <data> is as follows:
   All values are encoded in ascii hex digits.

    Request     Packet

    read registers  g
    reply       XX....X     Each byte of register data
                    is described by two hex digits.
                    Registers are in the internal order
                    for GDB, and the bytes in a register
                    are in the same order the machine uses.
            or ENN      for an error.

    write regs  GXX..XX     Each byte of register data
                    is described by two hex digits.
    reply       OK      for success
            ENN     for an error

        write reg   Pn...=r...  Write register n... with value r...,
                    which contains two hex digits for each
                    byte in the register (target byte
                    order).
    reply       OK      for success
            ENN     for an error
    (not supported by all stubs).

    read mem    mAA..AA,LLLL    AA..AA is address, LLLL is length.
    reply       XX..XX      XX..XX is mem contents
                    Can be fewer bytes than requested
                    if able to read only part of the data.
            or ENN      NN is errno

    write mem   MAA..AA,LLLL:XX..XX
                    AA..AA is address,
                    LLLL is number of bytes,
                    XX..XX is data
    reply       OK      for success
            ENN     for an error (this includes the case
                    where only part of the data was
                    written).

    cont        cAA..AA     AA..AA is address to resume
                    If AA..AA is omitted,
                    resume at same address.

    step        sAA..AA     AA..AA is address to resume
                    If AA..AA is omitted,
                    resume at same address.

    last signal     ?               Reply the current reason for stopping.
                                        This is the same reply as is generated
                    for step or cont : SAA where AA is the
                    signal number.

    There is no immediate reply to step or cont.
    The reply comes when the machine stops.
    It is       SAA     AA is the "signal number"

    or...       TAAn...:r...;n:r...;n...:r...;
                    AA = signal number
                    n... = register number
                    r... = register contents
    or...       WAA     The process exited, and AA is
                    the exit status.  This is only
                    applicable for certains sorts of
                    targets.
    kill request    k

    toggle debug    d       toggle debug flag (see 386 & 68k stubs)
    reset       r       reset -- see sparc stub.
    reserved    <other>     On other requests, the stub should
                    ignore the request and send an empty
                    response ($#<checksum>).  This way
                    we can extend the protocol and GDB
                    can tell whether the stub it is
                    talking to uses the old or the new.
    search      tAA:PP,MM   Search backwards starting at address
                    AA for a match with pattern PP and
                    mask MM.  PP and MM are 4 bytes.
                    Not supported by all stubs.

    general query   qXXXX       Request info about XXXX.
    general set QXXXX=yyyy  Set value of XXXX to yyyy.
    query sect offs qOffsets    Get section offsets.  Reply is
                    Text=xxx;Data=yyy;Bss=zzz
    console output  Otext       Send text to stdout.  Only comes from
                    remote target.

    Responses can be run-length encoded to save space.  A '*' means that
    the next character is an ASCII encoding giving a repeat count which
    stands for that many repetitions of the character preceding the '*'.
    The encoding is n+29, yielding a printable character where n >=3
    (which is where rle starts to win).  Don't use an n > 126.

    So
    "0* " means the same as "0000".  */

#include <dc/scif.h>
#include <dc/fs_dcload.h>
#include <arch/gdb.h>
#include <arch/types.h>
#include <arch/irq.h>
#include <arch/arch.h>
#include <arch/cache.h>

#include <stddef.h>
#include <string.h>

/* Hitachi SH architecture instruction encoding masks */

#define COND_BR_MASK   0xff00
#define UCOND_DBR_MASK 0xe000
#define UCOND_RBR_MASK 0xf0df
#define TRAPA_MASK     0xff00

#define COND_DISP      0x00ff
#define UCOND_DISP     0x0fff
#define UCOND_REG      0x0f00

/* Hitachi SH instruction opcodes */

#define BF_INSTR       0x8b00
#define BFS_INSTR      0x8f00
#define BT_INSTR       0x8900
#define BTS_INSTR      0x8d00
#define BRA_INSTR      0xa000
#define BSR_INSTR      0xb000
#define JMP_INSTR      0x402b
#define JSR_INSTR      0x400b
#define RTS_INSTR      0x000b
#define RTE_INSTR      0x002b
#define TRAPA_INSTR    0xc300
#define SSTEP_INSTR    0xc320

/* Hitachi SH processor register masks */

#define T_BIT_MASK     0x0001

/*
 * BUFMAX defines the maximum number of characters in inbound/outbound
 * buffers. At least NUMREGBYTES*2 are needed for register packets.
 */
#define BUFMAX 1024

/*
 * Number of bytes for registers
 */
#define NUMREGBYTES 41*4

/*
 * Modes for packet dcload packet transmission
 */

#define DCL_SEND       0x1
#define DCL_RECV       0x2
#define DCL_SENDRECV   0x3

/*
 * typedef
 */
typedef void (*Function)();

/*
 * Forward declarations
 */

static int hex(char);
static char *mem2hex(char *, char *, uint32);
static char *hex2mem(char *, char *, uint32);
static int hexToInt(char **, uint32 *);
static unsigned char *getpacket(void);
static void putpacket(char *);
static int computeSignal(int exceptionVector);

static void hardBreakpoint(int, int, uint32, int, char*);
static void putDebugChar(char);
static char getDebugChar(void);
static void flushDebugChannel(void);

void gdb_breakpoint(void);


static int dofault;  /* Non zero, bus errors will raise exception */

/* debug > 0 prints ill-formed commands in valid packets & checksum errors */
static int remote_debug;

/* map from KOS register context order to GDB sh4 order */

#define KOS_REG(r)      offsetof(irq_context_t, r)

static uint32 kosRegMap[] = {
    KOS_REG(r[0]), KOS_REG(r[1]), KOS_REG(r[2]), KOS_REG(r[3]),
    KOS_REG(r[4]), KOS_REG(r[5]), KOS_REG(r[6]), KOS_REG(r[7]),
    KOS_REG(r[8]), KOS_REG(r[9]), KOS_REG(r[10]), KOS_REG(r[11]),
    KOS_REG(r[12]), KOS_REG(r[13]), KOS_REG(r[14]), KOS_REG(r[15]),

    KOS_REG(pc), KOS_REG(pr), KOS_REG(gbr), KOS_REG(vbr),
    KOS_REG(mach), KOS_REG(macl), KOS_REG(sr),
    KOS_REG(fpul), KOS_REG(fpscr),

    KOS_REG(fr[0]), KOS_REG(fr[1]), KOS_REG(fr[2]), KOS_REG(fr[3]),
    KOS_REG(fr[4]), KOS_REG(fr[5]), KOS_REG(fr[6]), KOS_REG(fr[7]),
    KOS_REG(fr[8]), KOS_REG(fr[9]), KOS_REG(fr[10]), KOS_REG(fr[11]),
    KOS_REG(fr[12]), KOS_REG(fr[13]), KOS_REG(fr[14]), KOS_REG(fr[15])
};

#undef KOS_REG

typedef struct {
    short *memAddr;
    short oldInstr;
}
stepData;

static irq_context_t *irq_ctx;
static stepData instrBuffer;
static char stepped;
static const char hexchars[] = "0123456789abcdef";
static char remcomInBuffer[BUFMAX], remcomOutBuffer[BUFMAX];

static char in_dcl_buf[BUFMAX], out_dcl_buf[BUFMAX];
static int using_dcl = 0, in_dcl_pos = 0, out_dcl_pos = 0, in_dcl_size = 0;

static char __pure highhex(int  x) {
    return hexchars[(x >> 4) & 0xf];
}

static char __pure lowhex(int  x) {
    return hexchars[x & 0xf];
}

/*
 * Assembly macros
 */

#define BREAKPOINT()   __asm__("trapa	#0xff"::);


/*
 * Routines to handle hex data
 */

static int __pure hex(char ch) {
    if((ch >= 'a') && (ch <= 'f'))
        return (ch - 'a' + 10);

    if((ch >= '0') && (ch <= '9'))
        return (ch - '0');

    if((ch >= 'A') && (ch <= 'F'))
        return (ch - 'A' + 10);

    return (-1);
}

/* convert the memory, pointed to by mem into hex, placing result in buf */
/* return a pointer to the last char put in buf (null) */
static char * mem2hex(char *mem, char *buf, uint32 count) {
    uint32 i;
    int ch;

    for(i = 0; i < count; i++) {
        ch = *mem++;
        *buf++ = highhex(ch);
        *buf++ = lowhex(ch);
    }

    *buf = 0;
    return (buf);
}

/* convert the hex array pointed to by buf into binary, to be placed in mem */
/* return a pointer to the character after the last byte written */

static char * hex2mem(char *buf, char *mem, uint32 count) {
    uint32 i;
    unsigned char ch;

    for(i = 0; i < count; i++) {
        ch = hex(*buf++) << 4;
        ch = ch + hex(*buf++);
        *mem++ = ch;
    }

    return (mem);
}

/**********************************************/
/* WHILE WE FIND NICE HEX CHARS, BUILD AN INT */
/* RETURN NUMBER OF CHARS PROCESSED           */
/**********************************************/
static int hexToInt(char **ptr, uint32 *intValue) {
    int numChars = 0;
    int hexValue;

    *intValue = 0;

    while(**ptr) {
        hexValue = hex(**ptr);

        if(hexValue >= 0) {
            *intValue = (*intValue << 4) | hexValue;
            numChars++;
        }
        else
            break;

        (*ptr)++;
    }

    return (numChars);
}

/*
 * Routines to get and put packets
 */

/* scan for the sequence $<data>#<checksum>     */

static unsigned char * getpacket(void) {
    unsigned char *buffer = (unsigned char *)(&remcomInBuffer[0]);
    unsigned char checksum;
    unsigned char xmitcsum;
    int count;
    char ch;

    while(1) {
        /* wait around for the start character, ignore all other characters */
        while((ch = getDebugChar()) != '$')
            ;

    retry:
        checksum = 0;
        xmitcsum = -1;
        count = 0;

        /* now, read until a # or end of buffer is found */
        while(count < (BUFMAX-1)) {
            ch = getDebugChar();

            if(ch == '$')
                goto retry;

            if(ch == '#')
                break;

            checksum = checksum + ch;
            buffer[count] = ch;
            count = count + 1;
        }

        buffer[count] = 0;

        if(ch == '#') {
            ch = getDebugChar();
            xmitcsum = hex(ch) << 4;
            ch = getDebugChar();
            xmitcsum += hex(ch);

            if(checksum != xmitcsum) {
                putDebugChar('-');    /* failed checksum */
            }
            else {
                putDebugChar('+');    /* successful transfer */

//        printf("get_packet() -> %s\n", buffer);

                /* if a sequence char is present, reply the sequence ID */
                if(buffer[2] == ':') {
                    putDebugChar(buffer[0]);
                    putDebugChar(buffer[1]);

                    return &buffer[3];
                }

                return &buffer[0];
            }
        }
    }
}


/* send the packet in buffer. */

static void putpacket(register char *buffer) {
    register  int checksum;

    /*  $<packet info>#<checksum>. */
//  printf("put_packet() <- %s\n", buffer);
    do {
        char *src = buffer;
        putDebugChar('$');
        checksum = 0;

        while(*src) {
            int runlen;

            /* Do run length encoding */
            for(runlen = 0; runlen < 100; runlen ++) {
                if(src[0] != src[runlen] || runlen == 99) {
                    if(runlen > 3) {
                        int encode;
                        /* Got a useful amount */
                        putDebugChar(*src);
                        checksum += *src;
                        putDebugChar('*');
                        checksum += '*';
                        checksum += (encode = runlen + ' ' - 4);
                        putDebugChar(encode);
                        src += runlen;
                    }
                    else {
                        putDebugChar(*src);
                        checksum += *src;
                        src++;
                    }

                    break;
                }
            }
        }


        putDebugChar('#');
        putDebugChar(highhex(checksum));
        putDebugChar(lowhex(checksum));
        flushDebugChannel();
    }
    while(getDebugChar() != '+');
}


/*
 * this function takes the SH-1 exception number and attempts to
 * translate this number into a unix compatible signal value
 */
static int __pure computeSignal(int exceptionVector) {
    int sigval;

    switch(exceptionVector) {
        case EXC_ILLEGAL_INSTR:
        case EXC_SLOT_ILLEGAL_INSTR:
            sigval = 4;
            break;
        case EXC_DATA_ADDRESS_READ:
        case EXC_DATA_ADDRESS_WRITE:
            sigval = 10;
            break;

        case EXC_TRAPA:
            sigval = 5;
            break;

        default:
            sigval = 7;       /* "software generated"*/
            break;
    }

    return (sigval);
}

static void doSStep(void) {
    short *instrMem;
    int displacement;
    int reg;
    unsigned short opcode, br_opcode;

    instrMem = (short *) irq_ctx->pc;

    opcode = *instrMem;
    stepped = 1;

    br_opcode = opcode & COND_BR_MASK;

    if(br_opcode == BT_INSTR || br_opcode == BTS_INSTR) {
        if(irq_ctx->sr & T_BIT_MASK) {
            displacement = (opcode & COND_DISP) << 1;

            if(displacement & 0x80)
                displacement |= 0xffffff00;

            /*
               * Remember PC points to second instr.
               * after PC of branch ... so add 4
               */
            instrMem = (short *)(irq_ctx->pc + displacement + 4);
        }
        else {
            /* can't put a trapa in the delay slot of a bt/s instruction */
            instrMem += (br_opcode == BTS_INSTR) ? 2 : 1;
        }
    }
    else if(br_opcode == BF_INSTR || br_opcode == BFS_INSTR) {
        if(irq_ctx->sr & T_BIT_MASK) {
            /* can't put a trapa in the delay slot of a bf/s instruction */
            instrMem += (br_opcode == BFS_INSTR) ? 2 : 1;
        }
        else {
            displacement = (opcode & COND_DISP) << 1;

            if(displacement & 0x80)
                displacement |= 0xffffff00;

            /*
               * Remember PC points to second instr.
               * after PC of branch ... so add 4
               */
            instrMem = (short *)(irq_ctx->pc + displacement + 4);
        }
    }
    else if((opcode & UCOND_DBR_MASK) == BRA_INSTR) {
        displacement = (opcode & UCOND_DISP) << 1;

        if(displacement & 0x0800)
            displacement |= 0xfffff000;

        /*
         * Remember PC points to second instr.
         * after PC of branch ... so add 4
         */
        instrMem = (short *)(irq_ctx->pc + displacement + 4);
    }
    else if((opcode & UCOND_RBR_MASK) == JSR_INSTR) {
        reg = (char)((opcode & UCOND_REG) >> 8);

        instrMem = (short *) irq_ctx->r[reg];
    }
    else if(opcode == RTS_INSTR)
        instrMem = (short *) irq_ctx->pr;
    else if(opcode == RTE_INSTR)
        instrMem = (short *) irq_ctx->r[15];
    else if((opcode & TRAPA_MASK) == TRAPA_INSTR)
        instrMem = (short *)((opcode & ~TRAPA_MASK) << 2);
    else
        instrMem += 1;

    instrBuffer.memAddr = instrMem;
    instrBuffer.oldInstr = *instrMem;
    *instrMem = SSTEP_INSTR;
    icache_flush_range((uint32)instrMem, 2);
}


/* Undo the effect of a previous doSStep.  If we single stepped,
   restore the old instruction. */

static void undoSStep(void) {
    if(stepped) {
        short *instrMem;
        instrMem = instrBuffer.memAddr;
        *instrMem = instrBuffer.oldInstr;
        icache_flush_range((uint32)instrMem, 2);
    }

    stepped = 0;
}

/* Handle inserting/removing a hardware breakpoint.
   Using the SH4 User Break Controller (UBC) we can have
   two breakpoints, each set for either instruction and/or operand access.
   Break channel B can match a specific data being moved, but there is
   no GDB remote protocol spec for utilizing this functionality. */

#define LREG(r, o) (*((uint32*)((r)+(o))))
#define WREG(r, o) (*((uint16*)((r)+(o))))
#define BREG(r, o) (*((uint8*)((r)+(o))))

static void hardBreakpoint(int set, int brktype, uint32 addr, int length, char* resBuffer) {
    char* const ucb_base = (char*)0xff200000;
    static const int ucb_step = 0xc;
    static const char BAR = 0x0, BAMR = 0x4, BBR = 0x8, /*BASR = 0x14,*/ BRCR = 0x20;

    static const uint8 bbrBrk[] = {
        0x0,  /* type 0, memory breakpoint -- unsupported */
        0x14, /* type 1, hardware breakpoint */
        0x28, /* type 2, write watchpoint */
        0x24, /* type 3, read watchpoint */
        0x2c  /* type 4, access watchpoint */
    };

    uint8 bbr = 0;
    char* ucb;
    int i;

    if(length <= 8)
        do {
            bbr++;
        }
        while(length >>= 1);

    bbr |= bbrBrk[brktype];

    if(addr == 0) {  /* GDB tries to watch 0, wasting a UCB channel */
        strcpy(resBuffer, "OK");
    }
    else if(brktype == 0) {
        /* we don't support memory breakpoints -- the debugger
           will use the manual memory modification method */
        *resBuffer = '\0';
    }
    else if(length > 8) {
        strcpy(resBuffer, "E22");
    }
    else if(set) {
        WREG(ucb_base, BRCR) = 0;

        /* find a free UCB channel */
        for(ucb = ucb_base, i = 2; i > 0; ucb += ucb_step, i--)
            if(WREG(ucb, BBR) == 0)
                break;

        if(i) {
            LREG(ucb, BAR) = addr;
            BREG(ucb, BAMR) = 0x4; /* no BASR bits used, all BAR bits used */
            WREG(ucb, BBR) = bbr;
            strcpy(resBuffer, "OK");
        }
        else
            strcpy(resBuffer, "E12");
    }
    else {
        /* find matching UCB channel */
        for(ucb = ucb_base, i = 2; i > 0; ucb += ucb_step, i--)
            if(LREG(ucb, BAR) == addr && WREG(ucb, BBR) == bbr)
                break;

        if(i) {
            WREG(ucb, BBR) = 0;
            strcpy(resBuffer, "OK");
        }
        else
            strcpy(resBuffer, "E06");
    }
}

#undef LREG
#undef WREG

/*
This function does all exception handling.  It only does two things -
it figures out why it was called and tells gdb, and then it reacts
to gdb's requests.

When in the monitor mode we talk a human on the serial line rather than gdb.

*/


static void gdb_handle_exception(int exceptionVector) {
    int sigval, stepping;
    uint32 addr, length;
    char *ptr;

    /* reply to host that an exception has occurred */
    sigval = computeSignal(exceptionVector);
    remcomOutBuffer[0] = 'S';
    remcomOutBuffer[1] = highhex(sigval);
    remcomOutBuffer[2] = lowhex(sigval);
    remcomOutBuffer[3] = 0;

    putpacket(remcomOutBuffer);

    /*
     * Do the thangs needed to undo
     * any stepping we may have done!
     */
    undoSStep();

    stepping = 0;

    while(1) {
        remcomOutBuffer[0] = 0;
        ptr = (char *)getpacket();

        switch(*ptr++) {
            case '?':
                remcomOutBuffer[0] = 'S';
                remcomOutBuffer[1] = highhex(sigval);
                remcomOutBuffer[2] = lowhex(sigval);
                remcomOutBuffer[3] = 0;
                break;
            case 'd':
                remote_debug = !(remote_debug);   /* toggle debug flag */
                break;

            case 'g': {     /* return the value of the CPU registers */
                int i;
                char* outBuf = remcomOutBuffer;

                for(i = 0; i < NUMREGBYTES / 4; i++)
                    outBuf = mem2hex((char *)((uint32)irq_ctx + kosRegMap[i]), outBuf, 4);
            }
            break;

            case 'G': {     /* set the value of the CPU registers - return OK */
                int i;
                char* inBuf = ptr;

                for(i = 0; i < NUMREGBYTES / 4; i++, inBuf += 8)
                    hex2mem(inBuf, (char *)((uint32)irq_ctx + kosRegMap[i]), 4);

                strcpy(remcomOutBuffer, "OK");
            }
            break;

            /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
            case 'm':
                dofault = 0;

                /* TRY, TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
                if(hexToInt(&ptr, &addr))
                    if(*(ptr++) == ',')
                        if(hexToInt(&ptr, &length)) {
                            ptr = 0;
                            mem2hex((char *) addr, remcomOutBuffer, length);
                        }

                if(ptr)
                    strcpy(remcomOutBuffer, "E01");

                /* restore handler for bus error */
                dofault = 1;
                break;

                /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
            case 'M':
                dofault = 0;

                /* TRY, TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
                if(hexToInt(&ptr, &addr))
                    if(*(ptr++) == ',')
                        if(hexToInt(&ptr, &length))
                            if(*(ptr++) == ':') {
                                hex2mem(ptr, (char *) addr, length);
                                icache_flush_range(addr, length);
                                ptr = 0;
                                strcpy(remcomOutBuffer, "OK");
                            }

                if(ptr)
                    strcpy(remcomOutBuffer, "E02");

                /* restore handler for bus error */
                dofault = 1;
                break;

                /* cAA..AA    Continue at address AA..AA(optional) */
                /* sAA..AA   Step one instruction from AA..AA(optional) */
            case 's':
                stepping = 1;
                __fallthrough;
            case 'c': {
                /* tRY, to read optional parameter, pc unchanged if no param */
                if(hexToInt(&ptr, &addr))
                    irq_ctx->pc = addr;

                if(stepping)
                    doSStep();
            }
            return;
            break;

            /* kill the program */
            case 'k':       /* reboot */
                arch_reboot();
                break;

                /* set or remove a breakpoint */
            case 'Z':
            case 'z': {
                int set = (*(ptr - 1) == 'Z');
                int brktype = *ptr++ - '0';

                if(*(ptr++) == ',')
                    if(hexToInt(&ptr, &addr))
                        if(*(ptr++) == ',')
                            if(hexToInt(&ptr, &length)) {
                                hardBreakpoint(set, brktype, addr, length, remcomOutBuffer);
                                ptr = 0;
                            }

                if(ptr)
                    strcpy(remcomOutBuffer, "E02");
            }
            break;
        }           /* switch */

        /* reply to the request */
        putpacket(remcomOutBuffer);
    }
}


/* This function will generate a breakpoint exception.  It is used at the
   beginning of a program to sync up with a debugger and can be used
   otherwise as a quick means to stop program execution and "break" into
   the debugger. */

void gdb_breakpoint(void) {
    BREAKPOINT();
}


static char getDebugChar(void) {
    int ch;

    if(using_dcl) {
        if(in_dcl_pos >= in_dcl_size) {
            in_dcl_size = dcload_gdbpacket(NULL, 0, in_dcl_buf, BUFMAX);
            in_dcl_pos = 0;
        }

        ch = in_dcl_buf[in_dcl_pos++];
    }
    else {
        /* Spin while nothing is available. */
        while((ch = scif_read()) < 0);

        ch &= 0xff;
    }

    return ch;
}

static void putDebugChar(char ch) {
    if(using_dcl) {
        out_dcl_buf[out_dcl_pos++] = ch;

        if(out_dcl_pos >= BUFMAX) {
            dcload_gdbpacket(out_dcl_buf, out_dcl_pos, NULL, 0);
            out_dcl_pos = 0;
        }
    }
    else {
        /* write the char and flush it. */
        scif_write(ch);
        scif_flush();
    }
}

static void flushDebugChannel(void) {
    /* send the current complete packet and wait for a response */
    if(using_dcl) {
        if(in_dcl_pos >= in_dcl_size) {
            in_dcl_size = dcload_gdbpacket(out_dcl_buf, out_dcl_pos, in_dcl_buf, BUFMAX);
            in_dcl_pos = 0;
        }
        else
            dcload_gdbpacket(out_dcl_buf, out_dcl_pos, NULL, 0);

        out_dcl_pos = 0;
    }
}

static void handle_exception(irq_t code, irq_context_t *context, void *data) {
    (void)data;
    irq_ctx = context;
    gdb_handle_exception(code);
}

static void handle_user_trapa(trapa_t code, irq_context_t *context, void *data) {
    (void)code;
    (void)data;
    irq_ctx = context;
    gdb_handle_exception(EXC_TRAPA);
}

static void handle_gdb_trapa(trapa_t code, irq_context_t *context, void *data) {
    /*
    * trapa 0x20 indicates a software trap inserted in
    * place of code ... so back up PC by one
    * instruction, since this instruction will
    * later be replaced by its original one!
    */
    (void)code;
    (void)data;
    irq_ctx = context;
    irq_ctx->pc -= 2;
    gdb_handle_exception(EXC_TRAPA);
}

void gdb_init(void) {
    if(dcload_gdbpacket(NULL, 0, NULL, 0) == 0)
        using_dcl = 1;
    else
        scif_set_parameters(57600, 1);

    irq_set_handler(EXC_ILLEGAL_INSTR, handle_exception, NULL);
    irq_set_handler(EXC_SLOT_ILLEGAL_INSTR, handle_exception, NULL);
    irq_set_handler(EXC_DATA_ADDRESS_READ, handle_exception, NULL);
    irq_set_handler(EXC_DATA_ADDRESS_WRITE, handle_exception, NULL);
    irq_set_handler(EXC_USER_BREAK_PRE, handle_exception, NULL);

    trapa_set_handler(32, handle_gdb_trapa, NULL);
    trapa_set_handler(255, handle_user_trapa, NULL);

    BREAKPOINT();
}
