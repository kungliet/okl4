/*
 * Copyright (c) 2003-2004, National ICT Australia (NICTA)
 */
/*
 * Copyright (c) 2008 Open Kernel Labs, Inc. (Copyright Holder).
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
 * s3c2410 Platform Interrupt Handling
 */

#include <soc/interface.h>
#include <soc/soc.h>
#include <interrupt.h>
#include <intctrl.h>
#include <timer.h>
#include <kernel/arch/continuation.h>
#include <simplesoc.h>

addr_t s3c2410_int_va;

void
soc_mask_irq(word_t irq)
{
    SOC_ASSERT(DEBUG, irq < (word_t)IRQS);

    if (irq >= 32) {
        LN2410_INT_SUBMASK |= 1 << (irq-32);
    } else {
        word_t mask = LN2410_INT_MASK;
        mask |= (1 << irq);
        LN2410_INT_MASK = (mask & ~(1UL<<31) & ~(1UL<<28) & ~(1UL<<23) & ~(1UL<<15));
    }
}

void
soc_unmask_irq(word_t irq)
{
    SOC_ASSERT(DEBUG, irq < (word_t) IRQS);

    if (irq >= 32) {
        LN2410_INT_SUBMASK &= ~(1UL << (irq-32));
    } else {
        LN2410_INT_MASK &= ~(1UL << irq);
    }
}

void
soc_disable_timer()
{
    soc_timer_disabled = 1;
    LN2410_INT_MASK |= (1 << TIMER4_INT); /* Mask it */
}

void
soc_init_interrupts(void)
{
    LN2410_INT_MOD = 0; /* Set all to IRQ mode (not fiq mode) */
    LN2410_INT_SRCPND = LN2410_INT_SRCPND; /* Ack src pending */
    LN2410_INT_PND = LN2410_INT_PND; /* Ack int pending */
    LN2410_INT_SUBMASK = 0xffff;
    LN2410_INT_SUBSRCPND = 0;

    /* Mask off most all interrupts, except
     * those that are dealt with by sub controller */
    LN2410_INT_MASK = (0xffffffffUL & ~(1UL<<31) & ~(1UL<<28) & ~(1UL<<23) & ~(1UL<<15));

    soc_timer_disabled = 0;

    /* Setup libsimplesoc. */
    simplesoc_init();
}

/**
 * s3c2410 platform interrupt handler
 */
NORETURN
void soc_handle_interrupt(word_t ctx, word_t fiq)
{
    continuation_t cont = ASM_CONTINUATION;
    int i = LN2410_INT_OFFSET;
    word_t sub;
    int irq = -1;

    switch (i) {
        case 15:
        case 23:
        case 28:
        case 31:
            /* Look up submask top bits */
            sub = LN2410_INT_SUBSRCPND & (~LN2410_INT_SUBMASK);
            if (sub == 0) {
                LN2410_INT_SRCPND = 1 << i; /* Ack it */
                LN2410_INT_PND = 1 << i; /* Ack it again (IRQ) */
                ACTIVATE_CONTINUATION(cont);
            }
            irq = msb(sub);

            LN2410_INT_SUBMASK |= (1<<irq);
            LN2410_INT_SUBSRCPND = (1<<irq);

            irq += 32;
            break;

        case TIMER4_INT:
            LN2410_INT_SRCPND = 1 << i; /* Ack it */
            LN2410_INT_PND = 1 << i; /* Ack it again (IRQ) */

            handle_timer_interrupt(false, cont);
            ACTIVATE_CONTINUATION(cont);
            break;

        default:
            irq = i;
            LN2410_INT_MASK |= (1 << i); /* Mask it */
            break;
    }

    if (EXPECT_FALSE(soc_timer_disabled)) {
        soc_timer_disabled = 0;
        LN2410_INT_MASK &= ~(1 << TIMER4_INT); /* UnMask it */
    }

    LN2410_INT_SRCPND = 1 << i; /* Ack it */
    LN2410_INT_PND = 1 << i; /* Ack it again (IRQ) */

    /* Handle the interrupt and perform a reschedule. */
    simplesoc_handle_irq_reschedule(irq, cont);
}

