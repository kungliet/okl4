/*
 * Copyright (c) 2004-2006, National ICT Australia (NICTA)
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
 * Description:
 */

#include <soc/interface.h>
#include <soc/soc.h>
#include <soc.h>
#include <intctrl.h>
#include <console.h>
#include <timer.h>
#include <interrupt.h>

static const long XSCALE_DEV_PHYS = 0x40000000;

word_t soc_api_version = SOC_API_VERSION;

#define SIZE_4K 4096

#if defined(CONFIG_PERF)
word_t soc_perf_counter_irq;
#endif

void simplesoc_init(int num_irqs);

/*
 * Initialize the platform specific mappings needed
 * to start the kernel.
 * Add other hardware initialization here as well
 */
void SECTION(".init")
soc_init(void)
{
#if defined(CONFIG_PERF)
    soc_perf_counter_irq = CCNT_IRQ_ENABLE | PMN1_IRQ_ENABLE | PMN0_IRQ_ENABLE ;
#endif

    /* Map in the control registers */
    xscale_int_va = kernel_add_mapping(
        SIZE_4K, (addr_t)(XSCALE_DEV_PHYS + INTERRUPT_POFFSET),
         SOC_CACHE_DEVICE);
    SOC_ASSERT(ALWAYS, xscale_int_va);

    xscale_timers_va = kernel_add_mapping(
        SIZE_4K, (addr_t)(XSCALE_DEV_PHYS + TIMER_POFFSET), SOC_CACHE_DEVICE);
    SOC_ASSERT(ALWAYS, xscale_timers_va);

    xscale_clocks_va = kernel_add_mapping(
        SIZE_4K, (addr_t)(XSCALE_DEV_PHYS + CLOCKS_POFFSET),
            SOC_CACHE_DEVICE);
    SOC_ASSERT(ALWAYS, xscale_clocks_va);

    xscale_gpio_va = kernel_add_mapping(
        SIZE_4K, (addr_t)(XSCALE_DEV_PHYS + GPIO_POFFSET), SOC_CACHE_DEVICE);
    SOC_ASSERT(ALWAYS, xscale_gpio_va);

    /* Initialize PXA interrupt handling */
    simplesoc_init(IRQS);

    /*Disbale_fiq*/
    XSCALE_INT_ICLR = 0x00; /* No FIQs for now */
#if defined(CONFIG_SUBPLAT_PXA270)
    XSCALE_INT_ICLR2 = 0x00;
#endif

    init_clocks();

    return;
}

