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
 * Description:  imx31 Platform Interrupt Handling
 */

#include <soc/soc.h>
#include <soc/arch/soc.h>
#include <soc/interface.h>
#include <interrupt.h>
#include <intctrl.h>
#include <io.h>
#include <l2cc.h>
#include <timer.h>
#include <soc.h>
#include <kernel/arch/continuation.h>
#include <simplesoc.h>

#if defined(CONFIG_PERF)
extern unsigned long count_CCNT_overflow;
extern unsigned long count_PMN0_overflow;
extern unsigned long count_PMN1_overflow;
#endif

/* IMX31 Interrupt controller */
volatile struct IMX31_AVIC * imx31_avic;

word_t soc_timer_disabled;

void
soc_mask_irq(word_t irq)
{
    SOC_ASSERT(DEBUG, irq < (word_t)IRQS);
    imx31_avic->intdisnum = irq;
}

void
soc_unmask_irq(word_t irq)
{
    SOC_ASSERT(DEBUG, irq < (word_t)IRQS);
    imx31_avic->intennum = irq;
}

#if defined(CONFIG_PERF)
void
soc_perf_counter_unmask(void)
{
    soc_unmask_irq(PMU_IRQ);
}
#endif

void
soc_disable_timer(void)
{
    soc_timer_disabled = 1;
    soc_mask_irq(TIMER_IRQ);
}

void SECTION(".init")
soc_init_interrupts(void)
{
    /* Setup interrupt controller. */
    imx31_avic = (struct IMX31_AVIC *)AVIC_VADDR;
    imx31_avic->intcntl = 0;

    /* Disable all interrupts. */
    imx31_avic->intenableh = 0;
    imx31_avic->intenableh = 0;

    /* Set all interrupts to normal (i.e., not fast). */
    imx31_avic->inttypeh = 0;
    imx31_avic->inttypel = 0;

    soc_timer_disabled = 0;

    /* Setup interrupt routines. */
    simplesoc_init();
}

/**
 * IMX31 platform interrupt handler
 */
void
soc_handle_interrupt(word_t ctx, word_t fiq)
{
    continuation_t cont = ASM_CONTINUATION;
    word_t irq;

    /* Look for a pending interrupt in the controllers */
    irq = (imx31_avic->nivecsr & 0xFFFF0000) >> 16;

#if defined(CONFIG_HAS_SOC_CACHE) && defined(CONFIG_HAS_L2_EVTMON)
    if (EXPECT_FALSE(irq == L2EVTMON_IRQ)) {
        if (arm_is_EMC_event_occured(arm_eventmon_counter_number - 1)) {
            /**
             * L2 Cache Write Buffer occured, this is un-recoverable,
             * we can only clean and invalidate l2 cache to make
             * l2 cache to be consistent with l3, but the
             * write back could still be permanently lost.
             */
            arm_clear_EMC_event(arm_eventmon_counter_number - 1);
            arm_l2_cache_flush();
            kernel_fatal_error("Unrecoverable L2 cache write back abort occured!\n");
        }
    }
#endif

#if defined(CONFIG_PERF)
    if (EXPECT_FALSE(irq == PMU_IRQ)) {
        unsigned long PMNC = 0;
        int overflow = 0;

        /* Read PMNC. */
        __asm__ (
                "   mrc p15, 0, %0, c15, c12, 0 \n"
                :"=r" (PMNC)
                );

        if (PMNC & (1UL << 10)) {
            /* CCNT overflow. */
            if (count_CCNT_overflow == ~0UL) {
                overflow = 1;
            }
            count_CCNT_overflow++;
        }

        if (PMNC & (1UL << 9)) {
            /* PMN1 overflow. */
            if (count_PMN1_overflow == ~0UL) {
                overflow = 1;
            }
            count_PMN1_overflow++;
        }

        if (PMNC & (1UL << 8)) {
            /* PMN0 overflow. */
            if (count_PMN0_overflow == ~0UL) {
                overflow = 1;
            }
            count_PMN0_overflow++;
        }

        if (!overflow) {
            /* Clear interrupt and continue. */
            __asm__ __volatile__ (
                    "   mcr p15, 0, %0, c15, c12, 0 \n"
                    : :"r" (PMNC)
            );
            ACTIVATE_CONTINUATION(cont);
        } else {
            /* We deliver the interrupt to userspace, who can set the
             * overflow counters to 0xffffffff to override the kernel
             * counters. */
        }
    }
#endif

    /* If the timer is disabled, wake it up again. */
    if (EXPECT_TRUE(soc_timer_disabled)) {
        soc_timer_disabled = 0;
        soc_unmask_irq(TIMER_IRQ);
    }

    /* If we got a spurious IRQ, dump debugging info to console and continue. */
    if (EXPECT_FALSE(irq == 0xFFFF)) {
        SOC_TRACEF("spurious irq\n");
        SOC_TRACEF("nipndh  = %08lx\n", imx31_avic->nipndh);
        SOC_TRACEF("nipndl  = %08lx\n", imx31_avic->nipndl);
        SOC_TRACEF("fivecsr = %08lx\n", imx31_avic->fivecsr);
        SOC_TRACEF("nipndh  = %08lx\n", imx31_avic->fipndh);
        SOC_TRACEF("nipndl  = %08lx\n", imx31_avic->fipndl);
        ACTIVATE_CONTINUATION(cont);
    }

    /* Handle timer interrupt */
    if (EXPECT_FALSE(irq == TIMER_IRQ)) {
        handle_timer_interrupt(0, cont);
        NOTREACHED();
    }

    /* Deliver the IRQ. */
    simplesoc_handle_irq_reschedule(irq, cont);
    NOTREACHED();
}
