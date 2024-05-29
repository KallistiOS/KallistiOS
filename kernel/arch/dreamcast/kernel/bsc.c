#include <arch/bsc.h>
#include <arch/irq.h>
#include <stdio.h>
#include <stdbool.h>

// ROVI - refresh couner overflow interrupt
//RCMI: compare-match interrupt
//interval timer interrupt

#if 0 
BCR1:  a3020008
BCR2:  1
WCR1:  1110111
WCR2:  618066d8
WCR3:  7777777
MCR:   c00a0e1c
PCR:   0
RTCSR: 94
RTCNT: 2d
RTCOR: 5e = 94 * 160ns = 15.040us
            x * 40ns = 15040ns
            x = 15040ns/40ns = 376
        ~ 10200 = slowest refresh @ 40ns
        20480 = refreshing @ 1 TCOR && 0 LMT
RFCR:  340
#endif

#if 0
Bit 2—Refresh Control (RFSH): Specifies refresh control. Selects whether refreshing is
performed for DRAM and synchronous DRAM. When the refresh function is no
#endif

/* External Bus Clock Rate */
#define CKIO        100000000 /* 100Mhz */

/* Register Access Macros */
#define BSC(o, t)   (*((volatile t*)(BSC_BASE + o)))
#define BSC8(o)     BSC(o, uint8_t)
#define BSC16(o)    BSC(o, uint16_t)
#define BSC32(o)    BSC(o, uint32_t)

/* Register Start Address */
#define BSC_BASE    0xff800000  /* P4 Address */

/* Register Offsets */
#define BCR1        0x0  /* Bus Control Register 1 */
#define BCR2        0x4  /* Bus Control Register 2 */
#define WCR1        0x8  /* Wait State Control Register 1 */
#define WCR2        0xc  /* Wait State Control Register 2 */
#define WCR3        0x10 /* Wait State Control Register 3 */
#define MCR         0x14 /* Memory Control Register */
#define PCR         0x18 /* PCMCIA Control Register */
#define RTCSR       0x1c /* Refresh Timer Control/Status Register */ 
#define RTCNT       0x20 /* Refresh Timer Counter */
#define RTCOR       0x24 /* Refresh Time Constant Register */
#define RFCR        0x28 /* Refresh Counter Register */

/* MCR Field Masks */
#define RASD        (1 << 31) /* RAS Down */
#define MRSET       (1 << 30) /* Mode Register Set */
#define TRC         (7 << 27) /* RAS Precharge Time at End of Refresh */
#define TCAS        (1 << 23) /* CAS Negation Period */
#define TPC         (7 << 19) /* RAS Precharge Period */
#define RCD         (3 << 16) /* RAS-CAS Delay */
#define TRWL        (7 << 13) /* Write Precharge Delay */
#define TRAS        (7 << 10) /* CAS-Before-RAS Refresh RAS Assertion Period */
#define BE          (1 << 9)  /* Burst Enable */ 
#define SZ          (3 << 7)  /* Memory Data Size */
#define AMXEXT      (1 << 6)  /* Address Multiplexing */
#define AMX         (3 << 3)  
#define RFSH        (1 << 2)  /* Refresh Control */
#define RMODE       (1 << 1)  /* Refresh Mode */
#define EDOMODE     (1 << 0)  /* EDO Mode */

/* RTSCR Field Masks */
#define CMF         (1 << 7) /* Compare-Match Flag */
#define CMIE        (1 << 6) /* Compare-Match Interrupt Enable */ 
#define CKS         (7 << 3) /* Clock Select Bits */
#define OVF         (1 << 2) /* Refresh Overflow Flag */
#define OVIE        (1 << 1) /* Refresh Count Overflow Interrupt Enable */
#define LMTS        (1 << 0) /* Refresh Count Overflow Limit Select */

/* Refresh Counter Write Register Magic */
#define RTC_MAGIC    0xa500  /* Write data high byte for RTCSR, RTCNT, RTCOR */
#define RFCR_MAGIC   0xa400  /* Write data high 6 bits for RFCR */

/* CKS Field Values (External Bus Clock Divisors) */
typedef enum CLK_SELECT {       /* Clock Rate    => Cycle Time */
    CLK_DISABLED    = (0 << 3), /* 0Mhz          => INF */
    CLK_DIV_4       = (1 << 3), /* 25Mhz         => 40ns */
    CLK_DIV_16      = (2 << 3), /* 6.25Mhz       => 160ns */
    CLK_DIV_64      = (3 << 3), /* 1562.5Khz     => 640ns */
    CLK_DIV_256     = (4 << 3), /* 390.625Khz    => 2.560us */
    CLK_DIV_1024    = (5 << 3), /* 97.65625Khz   => 10.240us */
    CLK_DIV_2048    = (6 << 3), /* 48.828125Khz  => 20.480us */
    CLK_DIV_4096    = (7 << 3)  /* 24.4140625Khz => 40.960us */
} CLK_SELECT;

#if 0
DC BIOS default values
RTCSR: 94
RTCNT: 5d
RTCOR: 5e
RFCR: 232
#endif

/* Interrupt Priority Register access */
#define IPR(o)          (*((volatile uint16_t *)(IPR_BASE + o)))
#define IPR_BASE        0xffd00004          /* Base Address */
#define IPRB            0x4                 /* Interrupt Priority Register B offset */
#define IPRB_REF_BIT    8                   /* Mask for IRB WDT IRQ priority field */
#define IPRB_REF        (0xf << IPRB_REF_BIT) /* IRB WDT IRQ priority field (3 bits) */


static void *compare_match_userdata = NULL;
static bsc_callback_t compare_match_isr = NULL;
static void *overflow_userdata = NULL;
static bsc_callback_t overflow_isr = NULL;

static void bsc_compare_match_isr_wrapper(irq_t, irq_context_t *) {
    compare_match_isr(compare_match_userdata);
    BSC16(RTCSR) = RTC_MAGIC | (BSC16(RTCSR) & ~CMF);
}

static void bsc_overflow_isr_wrapper(irq_t, irq_context_t *) { 
    overflow_isr(overflow_userdata);
    BSC16(RTCSR) = RTC_MAGIC | (BSC16(RTCSR) & ~OVF);
}

static void bsc_log_config(void) { 
    printf("\n\n");
    printf("BCR1:  %x\n", BSC32(BCR1));
    printf("BCR2:  %x\n", BSC16(BCR2));
    printf("WCR1:  %x\n", BSC32(WCR1));
    printf("WCR2:  %x\n", BSC32(WCR2));
    printf("WCR3:  %x\n", BSC32(WCR3));
    printf("MCR:   %x\n", BSC32(MCR));
    printf("PCR:   %x\n", BSC16(PCR));
    printf("RTCSR: %x\n", BSC16(RTCSR));
    printf("RTCNT: %x\n", BSC16(RTCNT));
    printf("RTCOR: %x\n", BSC16(RTCOR));
    printf("RFCR:  %x\n", BSC16(RFCR));
    printf("\n\n");
    fflush(stdout);
}

void bsc_set_isrs(uint8_t priority, 
                  bsc_callback_t comp_match_callback, void *comp_match_data,
                  bsc_callback_t overflow_callback, void *overflow_data) {

    printf("Initializing!\n");

    bsc_log_config();


    compare_match_userdata = comp_match_data;
    overflow_userdata      = overflow_data;

    compare_match_isr = comp_match_callback;
    overflow_isr      = overflow_callback;

    /* Register our interrupt handlers */
    irq_set_handler(EXC_REF_RCMI, bsc_compare_match_isr_wrapper);
    irq_set_handler(EXC_REF_ROVI, bsc_overflow_isr_wrapper);

    //BSC16(RTCNT) = RTC_MAGIC;
    //BSC16(RTCOR) = RTC_MAGIC | 255;
    //BSC16(RFCR)  = RFCR_MAGIC;

    printf("Set RTCNT, RTCOR, RFCR\n");

    /* Unmask the WDTIT interrupt, giving it a new priority */
    IPR(IPRB) = IPR(IPRB) | ((priority & 0xf) << IPRB_REF_BIT);

    printf("Set IPRB!\n");

#if 1
    BSC16(RTCSR) = RTC_MAGIC | BSC16(RTCSR) |
                    OVIE | CMIE;
                    //CLK_DIV_4;
#else 
    BSC16(RTCSR) = rtcsr.bytes;
#endif

for(unsigned i = 0; i < 500; ++i) { 
    printf("RTCSR: %x\n", BSC16(RTCSR));
    printf("RTCNT: %x\n", BSC16(RTCNT));
   // printf("RTCOR: %x\n", BSC16(RTCOR));
    printf("RFCR: %x\n", BSC16(RFCR));
}
}



void bsc_init(void) {}

void bsc_shutdown(void) {
        //BSC16(RTCSR) = RTC_MAGIC | CLK_DISABLED;
    IPR(IPRB) = IPR(IPRB) & ~(IPRB_REF);
    irq_set_handler(EXC_REF_RCMI, NULL);
    irq_set_handler(EXC_REF_ROVI, NULL);

    uint8_t bsc = BSC16(RTCSR);
    bsc &= ~(OVIE | CMIE);
    bsc |= RTC_MAGIC;
    BSC16(RTCSR) = bsc;
    //BSC16(RTCNT) = RTC_MAGIC;
   // BSC16(RFCR)  = RFCR_MAGIC;
}


