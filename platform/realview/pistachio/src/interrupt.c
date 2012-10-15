/*
 * Copyright (c) 2003-2004, National ICT Australia (NICTA)
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
 * Description:  Versatile Platform Interrupt Handling
 */

#include <soc/interface.h>
#include <soc/soc.h>
#include <soc/arch/soc.h>
#include <kernel/arch/special.h>
#include <interrupt.h>
#include <reg.h>
#include <soc.h>
#include <kernel/arch/continuation.h>
#include <simplesoc.h>

addr_t versatile_vic_vbase;
addr_t versatile_sic_vbase;

static int num_sic_irqs = 0;
bool soc_timer_disabled = false;

/* Enable passthrough of the given IRQ. */
static void
register_passthrough_irq(word_t irq)
{
    volatile sic_t *sic = (sic_t *)versatile_sic_vbase;

    SOC_ASSERT(DEBUG, irq < (word_t) IRQS);
    sic->irq_passthrough = (1UL << irq);
}

/* Disable passthrough of the given IRQ. */
static void
unregister_passthrough_irq(word_t irq)
{
    volatile sic_t *sic = (sic_t *)versatile_sic_vbase;

    SOC_ASSERT(DEBUG, irq < (word_t) IRQS);
    sic->irq_passthrough_clear = (1UL << irq);
}

/* Mask a SIC irq. */
static void
mask_sic_irq(word_t irq)
{
    volatile sic_t *sic = (sic_t *)versatile_sic_vbase;
    sic->irq_enable_clear = (1UL << irq);
}

/* Unmask a SIC irq. */
static void
unmask_sic_irq(word_t irq)
{
    volatile sic_t *sic = (sic_t *)versatile_sic_vbase;
    sic->irq_enable = (1UL << irq);
}

/* Called back on IRQ register. */
void
soc_register_irq_callback(word_t irq)
{
    if (irq >= SIC_FORWARD_START && irq <= SIC_FORWARD_END) {
        register_passthrough_irq(irq);
    }
    if (irq > VERSATILE_SIC_IRQ) {
        num_sic_irqs++;
        soc_unmask_irq(VERSATILE_SIC_IRQ);
        register_passthrough_irq(VERSATILE_SIC_IRQ);
    }
}

/* Called back on IRQ deregister. */
void
soc_deregister_irq_callback(word_t irq)
{
    if (irq >= SIC_FORWARD_START && irq <= SIC_FORWARD_END) {
        unregister_passthrough_irq(irq);
    }

    if (irq > VERSATILE_SIC_IRQ) {
        num_sic_irqs--;
        SOC_ASSERT(ALWAYS, num_sic_irqs >= 0);
        if (num_sic_irqs == 0) {
            soc_mask_irq(VERSATILE_SIC_IRQ);
            unregister_passthrough_irq(VERSATILE_SIC_IRQ);
        }
    }
}

/* Mask an IRQ. */
void
soc_mask_irq(word_t irq)
{
    volatile vic_t  *vic = (vic_t *)versatile_vic_vbase;
    SOC_ASSERT(DEBUG, irq < (word_t) IRQS);

    if (irq <= VERSATILE_SIC_IRQ) {
        vic->irq_enable_clear = (1UL << irq);
    } else {
        mask_sic_irq(irq - VERSATILE_SIC_OFFSET);
    }
}

/* Unmask an IRQ. */
void
soc_unmask_irq(word_t irq)
{
    volatile vic_t  *vic = (vic_t *)versatile_vic_vbase;
    SOC_ASSERT(DEBUG, irq < (word_t) IRQS);

    if (irq <= VERSATILE_SIC_IRQ) {
        vic->irq_enable |= (1UL << irq);
    } else {
        unmask_sic_irq(irq - VERSATILE_SIC_OFFSET);
    }
}

/* Disable the system timer. */
void
soc_disable_timer(void)
{
    soc_timer_disabled = 1;
    soc_mask_irq(VERSATILE_TIMER0_IRQ);
}

/* Enable the system timer. */
void
soc_enable_timer(void)
{
    soc_timer_disabled = 0;
    soc_unmask_irq(VERSATILE_TIMER0_IRQ);
}

/*
 * Handle an incoming interrupt.
 */
void
soc_handle_interrupt(word_t ctx, word_t fiq)
{
    continuation_t cont = ASM_CONTINUATION;
    word_t irq;

    volatile vic_t *vic = (vic_t *)versatile_vic_vbase;
    volatile sic_t *sic = (sic_t *)versatile_sic_vbase;

    /* Get the IRQ that fired. */
    SOC_ASSERT(ALWAYS, vic->irq_status);
    irq = msb(vic->irq_status);

    /* Was it a timer IRQ? */
    if (irq == VERSATILE_TIMER0_IRQ) {
        handle_timer_interrupt(false, cont);
        ACTIVATE_CONTINUATION(cont);
    }

    /* Enable the timer if it happens to be disabled. */
    if (EXPECT_FALSE(soc_timer_disabled)) {
        soc_enable_timer();
    }

    /* If it is a SIC IRQ, calculate the real IRQ number. */
    if (irq == VERSATILE_SIC_IRQ) {
        irq = msb(sic->irq_status);
        irq += VERSATILE_SIC_OFFSET;
    }

    /* Handle the IRQ. */
    simplesoc_handle_irq_reschedule(irq, cont);
    NOTREACHED();
}

