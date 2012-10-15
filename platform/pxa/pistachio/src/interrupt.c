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
 * Description:  PXA Platform Interrupt Handling
 */

#include <soc/interface.h>
#include <soc/soc.h>
#include <interrupt.h>
#include <intctrl.h>
#include <timer.h>
#include <kernel/arch/continuation.h>
#include <simplesoc.h>

/* performance counters */
#if defined(CONFIG_PERF)
extern word_t count_CCNT_overflow;
extern word_t count_PMN0_overflow;
extern word_t count_PMN1_overflow;
#endif

/* Addresses set in soc_init() */
addr_t xscale_int_va;
addr_t xscale_gpio_va;

#if !defined(CONFIG_SUBPLAT_PXA255)
#error "Unsupported subplatform."
#endif

void
soc_mask_irq(word_t irq)
{
    SOC_ASSERT(DEBUG, irq < (word_t) IRQS);
    if (irq < (word_t) PRIMARY_IRQS) {
        XSCALE_INT_ICMR &= ~(1UL << irq);
    } else {
        /* XXX This is a GPIO IRQ and we should mask GPIO set */
        XSCALE_INT_ICMR &= ~(1 << GPIO_IRQ);
    }
}

void
soc_unmask_irq(word_t irq)
{
    SOC_ASSERT(DEBUG, irq < (word_t) IRQS);

    if (irq < (word_t) PRIMARY_IRQS) {
        XSCALE_INT_ICMR |= (1UL << irq);
    } else {
        word_t gpio = (irq - PRIMARY_IRQS + 2) & 31;
        word_t idx = (irq - PRIMARY_IRQS + 2) >> 5;
        /* XXX This is a GPIO IRQ and we should unmask GPIO set */
        XSCALE_INT_ICMR |= (1 << GPIO_IRQ);

        if (idx == 0) {
            PXA_GRER0 = (1UL << gpio);
        } else if (idx == 1) {
            PXA_GRER1 = (1UL << gpio);
        } else {
            PXA_GRER2 = (1UL << gpio);
        }
    }
}


static int
pxa_gpio_irq(void)
{
    word_t mask;
    int irq;

    mask = PXA_GEDR0 & (~0x3);
    if (mask) {
        irq = msb(mask);
        PXA_GEDR0 = (1UL << irq);

        return irq - 2;
    }

    mask = PXA_GEDR1;
    if (mask) {
        irq = msb(mask);
        PXA_GEDR1 = (1UL << irq);

        return 32 + irq - 2;
    }

    mask = PXA_GEDR2;
    if (mask) {
        irq = msb(mask);
        PXA_GEDR2 = (1UL << irq);

        return 64 + irq - 2;
    }

    return -1;
}

#if defined(CONFIG_PERF)
void soc_perf_counter_unmask(void)
{
    soc_unmask_irq(PMU_IRQ);
}
#endif

/**
 * PXA platform interrupt handler
 */
NORETURN
void soc_handle_interrupt(word_t ctx, word_t fiq)
{
    continuation_t cont = ASM_CONTINUATION;

    word_t status = XSCALE_INT_ICIP;
    word_t timer_int = 0;
    int irq = -1;

#if defined(CONFIG_PERF)
    if (EXPECT_FALSE(status & (1UL << PMU_IRQ))) {
        int overflow = 0;
        unsigned long PMNC = 0;

        /* Read PMNC. */
        __asm__(
                "   mrc p14, 0, %0, c0, c0, 0   \n"
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
                    "   mcr p14, 0, %0, c0, c0, 0   \n"
                    :
                    :"r" (PMNC)
                    );
            ACTIVATE_CONTINUATION(cont);
        }
    }
#endif

    /* Handle timer interrupt */
    if (status & (1UL << XSCALE_IRQ_OS_TIMER)) {
        timer_int = 1;
        status &= ~(1UL << XSCALE_IRQ_OS_TIMER);
    }

    /* 0..6 are reserved */
    status = status & (~0x7fUL);

    /* Get the IRQ number that fired. */
    if (EXPECT_FALSE(status & (1UL << GPIO_IRQ))) {
        irq = pxa_gpio_irq() + PRIMARY_IRQS;
    } else {
        irq = msb(status);
    }

    /* Handle the IRQ. */
    if (irq >= 0) {
        if (timer_int) {
            int wakeup = simplesoc_handle_irq(irq);
            handle_timer_interrupt(wakeup, cont);
        } else {
            simplesoc_handle_irq_reschedule(irq, cont);
        }
    }

    /* Handle timer ticks. */
    if (EXPECT_TRUE(timer_int)) {
        handle_timer_interrupt(0, cont);
    }

    /* Spurious interrupt. */
    ACTIVATE_CONTINUATION(cont);
}
