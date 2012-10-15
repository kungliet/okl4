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
 * Description:   imx31 platform setup
 */
#include <io.h>
#include <soc/soc.h>
#include <soc/arch/soc.h>
#include <soc.h>
#include <interrupt.h>
#include <intctrl.h>
#include <l2cc.h>

word_t soc_api_version = SOC_API_VERSION;

#define SIZE_4K     (4*1024)
#define SIZE_1M     (1024*1024)

addr_t io_area0_vaddr;
addr_t io_area1_vaddr;
addr_t io_area2_vaddr;
addr_t io_area3_vaddr;
addr_t io_area4_vaddr;

#if defined(CONFIG_PERF)
word_t soc_perf_counter_irq;
#endif

void soc_panic(void)
{
    for(;;);
}

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

    /* Map in the IMX31 L4 IO registers */
    io_area0_vaddr = kernel_add_mapping(SIZE_1M, (addr_t)(IO_AREA0_PADDR),
                                        SOC_CACHE_DEVICE);
    SOC_ASSERT(ALWAYS, io_area0_vaddr);
    io_area1_vaddr = kernel_add_mapping(SIZE_1M, (addr_t)(IO_AREA1_PADDR),
                                        SOC_CACHE_DEVICE);
    SOC_ASSERT(ALWAYS, io_area1_vaddr);
    io_area2_vaddr = kernel_add_mapping(SIZE_1M, (addr_t)(IO_AREA2_PADDR),
                                        SOC_CACHE_DEVICE);
    SOC_ASSERT(ALWAYS, io_area2_vaddr);

    io_area3_vaddr = kernel_add_mapping(SIZE_4K, (addr_t)(IO_AREA3_PADDR),
                                        SOC_CACHE_DEVICE);
    SOC_ASSERT(ALWAYS, io_area3_vaddr);

    io_area4_vaddr = kernel_add_mapping(SIZE_4K, (addr_t)(IO_AREA4_PADDR),
                                        SOC_CACHE_DEVICE);
    SOC_ASSERT(ALWAYS, io_area4_vaddr);

    soc_init_interrupts();

    /* Disable fiq */
    imx31_avic->intcntl |= FIDIS;

    init_clocks();

#if defined(CONFIG_HAS_SOC_CACHE)
    init_enable_l2cache();
#endif
}

word_t soc_do_platform_control(tcb_h current,
                               plat_control_t control,
                               word_t param1,
                               word_t param2,
                               word_t param3,
                               continuation_t cont)
{
    utcb_t *current_utcb = kernel_get_utcb(current);
    current_utcb->error_code = ENOT_IMPLEMENTED;
    return 0;
}
