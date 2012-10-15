/*
 * Copyright (c) 2006, National ICT Australia (NICTA)
 */
/*
 * Copyright (c) 2007 Open Kernel Labs, Inc. (Copyright Holder).
 * All rights reserved.
 *
 * 1. Redistribution and use of OKL4 (Software) in source and binary
 * forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 *     (a) Redistributions of source code must retain this clause 1
 *         (including paragraphs (a), (b) and (c)), clause 2 and clause 3
 *         (Licence Terms) and the above copyright notice.
 *
 *     (b) Redistributions in binary form must reproduce the above
 *         copyright notice and the Licence Terms in the documentation and/or
 *         other materials provided with the distribution.
 *
 *     (c) Redistributions in any form must be accompanied by information on
 *         how to obtain complete source code for:
 *        (i) the Software; and
 *        (ii) all accompanying software that uses (or is intended to
 *        use) the Software whether directly or indirectly.  Such source
 *        code must:
 *        (iii) either be included in the distribution or be available
 *        for no more than the cost of distribution plus a nominal fee;
 *        and
 *        (iv) be licensed by each relevant holder of copyright under
 *        either the Licence Terms (with an appropriate copyright notice)
 *        or the terms of a licence which is approved by the Open Source
 *        Initative.  For an executable file, "complete source code"
 *        means the source code for all modules it contains and includes
 *        associated build and other files reasonably required to produce
 *        the executable.
 *
 * 2. THIS SOFTWARE IS PROVIDED ``AS IS'' AND, TO THE EXTENT PERMITTED BY
 * LAW, ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, OR NON-INFRINGEMENT, ARE DISCLAIMED.  WHERE ANY WARRANTY IS
 * IMPLIED AND IS PREVENTED BY LAW FROM BEING DISCLAIMED THEN TO THE
 * EXTENT PERMISSIBLE BY LAW: (A) THE WARRANTY IS READ DOWN IN FAVOUR OF
 * THE COPYRIGHT HOLDER (AND, IN THE CASE OF A PARTICIPANT, THAT
 * PARTICIPANT) AND (B) ANY LIMITATIONS PERMITTED BY LAW (INCLUDING AS TO
 * THE EXTENT OF THE WARRANTY AND THE REMEDIES AVAILABLE IN THE EVENT OF
 * BREACH) ARE DEEMED PART OF THIS LICENCE IN A FORM MOST FAVOURABLE TO
 * THE COPYRIGHT HOLDER (AND, IN THE CASE OF A PARTICIPANT, THAT
 * PARTICIPANT). IN THE LICENCE TERMS, "PARTICIPANT" INCLUDES EVERY
 * PERSON WHO HAS CONTRIBUTED TO THE SOFTWARE OR WHO HAS BEEN INVOLVED IN
 * THE DISTRIBUTION OR DISSEMINATION OF THE SOFTWARE.
 *
 * 3. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ANY OTHER PARTICIPANT BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Description:   ARM versatile platform constants
 */

#ifndef __PLATFORM__VERSATILE__REGS_H__
#define __PLATFORM__VERSATILE__REGS_H__

#include <soc/soc_types.h>


#define VERSATILE_IO_PBASE      0x10000000

typedef struct Timer {
    word_t load;
    word_t val;
    word_t ctrl;
    word_t clear;
} Timer_t;


typedef struct SysCtl {
    word_t ctrl;
    /* add more */
} SysCtl_t;

typedef struct vic {
    word_t irq_status;
    word_t fiq_status;
    word_t irq_raw_status;
    word_t int_select;
    word_t irq_enable;
    word_t irq_enable_clear;
    word_t irq_soft;
    word_t irq_soft_clear;
    word_t protect; /* 0x20 */
    word_t pad[3];  /* 0x24 - 0x30 */
    word_t vect_addr; /* 0x30 */
    word_t def_vect_addr;
    /* add more */
} vic_t;

typedef struct sic {
    word_t irq_status;
    word_t irq_raw_status;
    word_t irq_enable;
    word_t irq_enable_clear;
    word_t irq_soft;            /* 0x10 */
    word_t irq_soft_clear;
    word_t pad[2];
    word_t irq_passthrough;     /* 0x20 */
    word_t irq_passthrough_clear;
} sic_t;

/* Versatile Timer */
static const int TIMER_TICK_LENGTH = 5000;

#define VERSATILE_TIMER0_IRQ     4
#define VERSATILE_REG_IRQ_PBASE  0x10140000     /* Vectored interrupt controller */

#define VERSATILE_VIC_VECT_CNTL_ENABLE    (1UL << 5)
#define VERSATILE_VIC_TIMER0_INT_BIT      (1UL << 4)
#define VERSATILE_VIC_TIMER2_INT_BIT      (1UL << 5)
#define VERSATILE_VIC_UART0_TX_INT_BIT (1UL << 12)

#define WAIT                                    \
    asm volatile (                              \
            "mov        r0, #0x1800     \n"     \
            "0:                         \n"     \
            "subs       r0, r0, #0x1    \n"     \
            "bne        0b              \n"     \
    );


#define VERSATILE_SYS_RESETCTL_OFFSET           0x40

#define VERSATILE_SYS_RESETCTL_VADDR *((volatile word_t *)      \
        ((word_t)versatile_io_vbase + VERSATILE_SYS_RESETCTL_OFFSET))
#define VERSATILE_SYS_CTRL_RESET_CONFIGCLR      0x1

#define VERSATILE_SCTL_PBASE                    0x101E0000
#define VERSATILE_SCTL_VBASE                    ((word_t)versatile_sctl_vbase)
#define VERSATILE_VIC_PBASE                     0x10140000      /* Vectored interrupt controller */
#define VERSATILE_SIC_PBASE                     0x10003000      /* Secondary interrupt controler */
#define VERSATILE_UART0_PBASE                   0x101F1000      /* UART 0 */
#define VERSATILE_VIC_VECT_ADDR0_PBASE          (VERSATILE_VIC_PBASE + 0x100)
#define VERSATILE_VIC_CNTL0_PBASE               (VERSATILE_VIC_PBASE + 0x200)
#define VERSATILE_VIC_ITCR_PBASE                (VERSATILE_VIC_PBASE + 0x300)

#define VERSATILE_REG_VIC_ENABLE_CLEAR_PADDR    (VERSATILE_VIC_PBASE + 0x14)
#define VERSATILE_REG_VIC_ENABLE_PADDR          (VERSATILE_VIC_PBASE + 0x10)

#define VERSATILE_BIT_TIMER_EN0_SEL             15
#define VERSATILE_BIT_TIMER_EN1_SEL             17
#define VERSATILE_BIT_TIMER_EN2_SEL             19
#define VERSATILE_BIT_TIMER_EN3_SEL             21

#define VERSATILE_TIMER0_PBASE                  0x101E2000
#define VERSATILE_TIMER0_VBASE                  (word_t)versatile_timer0_vbase
#define VERSATILE_TIMER1_VBASE                  ((word_t)versatile_timer0_vbase + 0x20)
#define VERSATILE_TIMER2_VBASE                  ((word_t)versatile_timer0_vbase + 0x1000)
#define VERSATILE_TIMER3_VBASE                  ((word_t)versatile_timer0_vbase + 0x1020)

#define TICK_PER_uSEC                           1
#define VERSATILE_TIMER_ENABLE                  (1 << 7)        /* Timer Enable        */
#define VERSATILE_TIMER_MODE                    (1 << 6)        /* Timer Enable        */
#define VERSATILE_TIMER_IE                      (1 << 5)        /* Interrupt Enable    */
#define VERSATILE_TIMER_DIV1                    (0 << 2)        /* Divided by 1        */
#define VERSATILE_TIMER_DIV16                   (1 << 2)        /* Divided by 16       */
#define VERSATILE_TIMER_DIV256                  (2 << 2)        /* Divided by 256      */
#define VERSATILE_TIMER_32BIT                   (1 << 1)        /* 32 bit counter      */

#if 0
#if TIMER_TICK_LENGTH >= 0x100000
#   define VERSATILE_TIMER_RELOAD               (TIMER_TICK_LENGTH >> 8)        /* Divide by 256 */
#   define VERSATILE_TIMER_CTRL                 (VERSATILE_TIMER_ENABLE | VERSATILE_TIMER_DIV256)
#   define TICKS2USECS(x)                       (256 * (x) / TICKS_PER_uSEC)
#elif TIMER_TICK_LENGTH >= 0x10000
#   define VERSATILE_TIMER_RELOAD               (TIMER_TICK_LENGTH >> 4)        /* Divide by 16 */
#   define VERSATILE_TIMER_CTRL                 (VERSATILE_TIMER_ENABLE | VERSATILE_TIMER_DIV14)
#   define VERSATILE_TICKS2USECS(x)             (16 * (x) / TICKS_PER_uSEC)
#else
#   define VERSATILE_TIMER_RELOAD               (TIMER_TICK_LENGTH)             /* Divide by 1 */
#   define VERSATILE_TIMER_CTRL                 (VERSATILE_TIMER_ENABLE)
#   define VERSATILE_TICKS2USECS(x)             ((x) / TICKS_PER_uSEC)
#endif
#endif
/* XXX: Here while TIMER_TICK_LENGTH is a const int. */
#   define VERSATILE_TIMER_RELOAD               (TIMER_TICK_LENGTH)             /* Divide by 1 */
#   define VERSATILE_TIMER_CTRL                 (VERSATILE_TIMER_ENABLE)
#   define VERSATILE_TICKS2USECS(x)             ((x) / TICKS_PER_uSEC)

#define VERSATILE_REG_TIMER_LOAD_OFF            0x0
#define VERSATILE_REG_TIMER_READ_OFF            0x4
#define VERSATILE_REG_TIMER_CRTL_OFF            0x8
#define VERSATILE_REG_TIMER_INTCRTL_OFF         0xC
#define VERSATILE_REG_TIMER_RIS_OFF             0x10
#define VERSATILE_REG_TIMER_MIS_OFF             0x14
#define VERSATILE_REG_TIMER_BGLOAD_OFF          0x18

#define VERSATILE_CLOCK_MODE_NORMAL             0x40

#endif /* __PLATFORM__VERSATILE__REGS_H__ */
