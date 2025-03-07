/* KallistiOS ##version##

   aica_registers.h
   Copyright (C) 2024 Paul Cercueil

   Definitions for the AICA registers.
   This file is meant to be included from both the ARM and SH-4 sides.
*/

#ifndef __ARM_AICA_REGISTERS_H
#define __ARM_AICA_REGISTERS_H

#define AICA_REGISTERS_BASE_SH4 0xa0700000
#define AICA_REGISTERS_BASE_ARM 0x00800000

#ifdef __sh__
#define AICA_REGISTERS_BASE     AICA_REGISTERS_BASE_SH4
#else
#define AICA_REGISTERS_BASE     AICA_REGISTERS_BASE_ARM
#endif

#define SPU_REG32(reg)          *(volatile unsigned int *)(reg)

#define SPU_BIT(bit)            (1u << (bit))
#define SPU_GENMASK(h, l)       ((0xffffffff << (l)) & (0xffffffff >> (31 - (h))))

#define SPU_FIELD_GET(mask, v)  ((v) & (mask)) >> (__builtin_ffs(mask) - 1)
#define SPU_FIELD_PREP(mask, v) (((v) << (__builtin_ffs(mask) - 1)) & (mask))

#define SPU_REG(reg)            (AICA_REGISTERS_BASE + (reg))
#define CHN_REG(chn, reg)       SPU_REG(0x80 * (chn) + (reg))

#define REG_SPU_PLAY_CTRL(chn)  CHN_REG((chn), 0x00)
#define REG_SPU_ADDR_L(chn)     CHN_REG((chn), 0x04)
#define REG_SPU_LOOP_START(chn) CHN_REG((chn), 0x08)
#define REG_SPU_LOOP_END(chn)   CHN_REG((chn), 0x0c)
#define REG_SPU_AMP_ENV1(chn)   CHN_REG((chn), 0x10)
#define REG_SPU_AMP_ENV2(chn)   CHN_REG((chn), 0x14)
#define REG_SPU_PITCH(chn)      CHN_REG((chn), 0x18)
#define REG_SPU_LFO(chn)        CHN_REG((chn), 0x1c)
#define REG_SPU_DSP(chn)        CHN_REG((chn), 0x20)
#define REG_SPU_VOL_PAN(chn)    CHN_REG((chn), 0x24)
#define REG_SPU_LPF1(chn)       CHN_REG((chn), 0x28)
#define REG_SPU_LPF2(chn)       CHN_REG((chn), 0x2c)
#define REG_SPU_LPF3(chn)       CHN_REG((chn), 0x30)
#define REG_SPU_LPF4(chn)       CHN_REG((chn), 0x34)
#define REG_SPU_LPF5(chn)       CHN_REG((chn), 0x38)
#define REG_SPU_LPF6(chn)       CHN_REG((chn), 0x3c)
#define REG_SPU_LPF7(chn)       CHN_REG((chn), 0x40)
#define REG_SPU_LPF8(chn)       CHN_REG((chn), 0x44)

#define REG_SPU_DSP_MIXER(chn)  SPU_REG(0x2000 + 0x4 * (chn))

#define REG_SPU_MASTER_VOL      SPU_REG(0x2800)
#define REG_SPU_BUS_REQUEST     SPU_REG(0x2808)
#define REG_SPU_INFO_REQUEST    SPU_REG(0x280c)
#define REG_SPU_INFO_PLAY_POS   SPU_REG(0x2814)
#define REG_SPU_TIMER0_CTRL     SPU_REG(0x2890)
#define REG_SPU_TIMER1_CTRL     SPU_REG(0x2894)
#define REG_SPU_TIMER2_CTRL     SPU_REG(0x2898)
#define REG_SPU_INT_ENABLE      SPU_REG(0x289c)
#define REG_SPU_INT_SEND        SPU_REG(0x28a0)
#define REG_SPU_INT_RESET       SPU_REG(0x28a4)
#define REG_SPU_FIQ_BIT_0       SPU_REG(0x28a8)
#define REG_SPU_FIQ_BIT_1       SPU_REG(0x28ac)
#define REG_SPU_FIQ_BIT_2       SPU_REG(0x28b0)
#define REG_SPU_SH4_INT_ENABLE  SPU_REG(0x28b4)
#define REG_SPU_SH4_INT_SEND    SPU_REG(0x28b8)
#define REG_SPU_SH4_INT_RESET   SPU_REG(0x28bc)
#define REG_SPU_INT_REQUEST     SPU_REG(0x2d00)
#define REG_SPU_INT_CLEAR       SPU_REG(0x2d04)

/* Register fields below */

#define SPU_PLAY_CTRL_KEY       SPU_GENMASK(15, 14)
#define SPU_PLAY_CTRL_LOOP      SPU_BIT(9)
#define SPU_PLAY_CTRL_FORMAT    SPU_GENMASK(8, 7)
#define SPU_PLAY_CTRL_ADDR_H    SPU_GENMASK(6, 0)

#define SPU_AMP_ENV1_DECAY2     SPU_GENMASK(15, 11)
#define SPU_AMP_ENV1_DECAY1     SPU_GENMASK(10, 6)
#define SPU_AMP_ENV1_ATTACK     SPU_GENMASK(4, 0)

#define SPU_AMP_ENV2_LINK       SPU_BIT(14)
#define SPU_AMP_ENV2_KEY        SPU_GENMASK(13, 10)
#define SPU_AMP_ENV2_DECAY_LVL  SPU_GENMASK(9, 5)
#define SPU_AMP_ENV2_RELEASE    SPU_GENMASK(4, 0)

#define SPU_PITCH_OCT           SPU_GENMASK(15, 11)
#define SPU_PITCH_FNS           SPU_GENMASK(9, 0)

#define SPU_LFO_RESET           SPU_BIT(15)
#define SPU_LFO_FREQ            SPU_GENMASK(14, 10)
#define SPU_LFO_FORM1           SPU_GENMASK(9, 8)
#define SPU_LFO_DEPTH1          SPU_GENMASK(7, 5)
#define SPU_LFO_FORM2           SPU_GENMASK(4, 3)
#define SPU_LFO_DEPTH2          SPU_GENMASK(2, 0)

#define SPU_DSP_SEND            SPU_GENMASK(11, 8)
#define SPU_DSP_CHN             SPU_GENMASK(3, 0)

#define SPU_VOL_PAN_VOL         SPU_GENMASK(11, 8)
#define SPU_VOL_PAN_PAN         SPU_GENMASK(4, 0)

#define SPU_LPF1_VOL            SPU_GENMASK(15, 8)
#define SPU_LPF1_OFF            SPU_BIT(5)
#define SPU_LPF1_Q              SPU_GENMASK(4, 0)

#define SPU_LPF2_VAL            SPU_GENMASK(12, 0)

#define SPU_LPF3_VAL            SPU_GENMASK(12, 0)

#define SPU_LPF4_VAL            SPU_GENMASK(12, 0)

#define SPU_LPF5_VAL            SPU_GENMASK(12, 0)

#define SPU_LPF6_VAL            SPU_GENMASK(12, 0)

#define SPU_LPF7_ATTACK         SPU_GENMASK(12, 8)
#define SPU_LPF7_DECAY          SPU_GENMASK(7, 0)

#define SPU_LPF8_DECAY          SPU_GENMASK(12, 8)
#define SPU_LPF8_RELEASE        SPU_GENMASK(7, 0)

#define SPU_DSP_MIXER_VOL       SPU_GENMASK(11, 8)
#define SPU_DSP_MIXER_PAN       SPU_GENMASK(4, 0)

#define SPU_MASTER_VOL_MODE     SPU_GENMASK(15, 8)
#define SPU_MASTER_VOL_VOL      SPU_GENMASK(3, 0)

#define SPU_INFO_REQUEST_REQ    SPU_GENMASK(13, 8)

#define SPU_INT_REQUEST_CODE    SPU_GENMASK(2, 0)

enum spu_timer_ctrl_div {
    SPU_TIMER_CTRL_DIV_1,
    SPU_TIMER_CTRL_DIV_2,
    SPU_TIMER_CTRL_DIV_4,
    SPU_TIMER_CTRL_DIV_8,
    SPU_TIMER_CTRL_DIV_16,
    SPU_TIMER_CTRL_DIV_32,
    SPU_TIMER_CTRL_DIV_64,
    SPU_TIMER_CTRL_DIV_128,
};

#define SPU_TIMER_CTRL_START    SPU_GENMASK(7, 0)
#define SPU_TIMER_CTRL_DIV      SPU_GENMASK(10, 8)

enum spu_int_codes {
    SPU_INT_TIMER               = 2,
    SPU_INT_SH4                 = 4,
    SPU_INT_BUS                 = 5,
};

/* These bits are used in REG_SPU_INT_ENABLE and other registers. */
#define SPU_INT_ENABLE_SH4      SPU_BIT(5)
#define SPU_INT_ENABLE_TIMER0   SPU_BIT(6)
#define SPU_INT_ENABLE_TIMER1   SPU_BIT(7)
#define SPU_INT_ENABLE_TIMER2   SPU_BIT(8)
#define SPU_INT_ENABLE_BUS      SPU_BIT(8)

#endif /* __ARM_AICA_REGISTERS_H */
