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

addr_t versatile_gic_cpu_vbase;
addr_t versatile_gic_dist_vbase;

bool soc_timer_disabled = false;

/* Called back on IRQ register. */
void
soc_register_irq_callback(word_t irq)
{
    soc_unmask_irq(irq);
}

/* Called back on IRQ deregister. */
void
soc_deregister_irq_callback(word_t irq)
{
    soc_mask_irq(irq);
}

/* Mask an IRQ. */
void
soc_mask_irq(word_t irq)
{
    volatile gic_dist_t  *gic_dist_base = (gic_dist_t *)versatile_gic_dist_vbase;
    SOC_ASSERT(DEBUG, irq < (word_t) IRQS);

    gic_dist_base->clr_enable1 = 1 << irq;
}

/* Unmask an IRQ. */
void
soc_unmask_irq(word_t irq)
{
	volatile gic_cpu_t *gic_cpu_base = (gic_cpu_t *)versatile_gic_cpu_vbase;
    volatile gic_dist_t  *gic_dist_base = (gic_dist_t *)versatile_gic_dist_vbase;
    SOC_ASSERT(DEBUG, irq < (word_t) IRQS);

    gic_cpu_base->int_ack = VERSATILE_GIC_OFFSET + irq;
    gic_dist_base->set_enable1 |= 1 << irq;
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

    volatile gic_cpu_t *gic_cpu_base = (gic_cpu_t *)versatile_gic_cpu_vbase;

    /* Get the IRQ that fired. */
    SOC_ASSERT(ALWAYS, gic_cpu_base->int_ack);
    irq = gic_cpu_base->int_ack;

    /* Was it a timer IRQ? */
    if (irq == VERSATILE_TIMER0_IRQ) {
        handle_timer_interrupt(false, cont);
        ACTIVATE_CONTINUATION(cont);
    }

    /* Enable the timer if it happens to be disabled. */
    if (EXPECT_FALSE(soc_timer_disabled)) {
        soc_enable_timer();
    }

    /* Handle the IRQ. */
    simplesoc_handle_irq_reschedule(irq, cont);
    NOTREACHED();
}

void soc_init_interrupts(void)
{
    volatile gic_cpu_t *gic_cpu_base = (gic_cpu_t *)versatile_gic_cpu_vbase;
    volatile gic_dist_t  *gic_dist_base = (gic_dist_t *)versatile_gic_dist_vbase;
    int i;
	
	gic_cpu_base->pri_mask = 0xf0;    /* priority setting */
	gic_cpu_base->cpu_control = 1;    /* enable gic0 */
    
    gic_dist_base->dist_ctrl = 0x0;
    gic_dist_base->configuration[2] = 0x55555555;
    gic_dist_base->configuration[3] = 0x55555555;
    gic_dist_base->configuration[4] = 0x55555555;
    gic_dist_base->configuration[5] = 0x55555555;
    
    for(i=0; i<16; i++)
    {
		gic_dist_base->priority[8+i] = 0xa0a0a0a0;
	}
	/* disable all interrupts */
	gic_dist_base->clr_enable1 = 0xffffffff;
	gic_dist_base->clr_enable2 = 0xffffffff;
	gic_dist_base->dist_ctrl = 0x1;
	
	simplesoc_init();
}

