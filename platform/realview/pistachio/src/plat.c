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
 * Description:   ARM versatile platform specfic functions
 */

#include <soc/soc.h>
#include <soc/interface.h>
#include <soc.h>
#include <reg.h>
#include <interrupt.h>
#include <simplesoc.h>

addr_t versatile_io_vbase;
addr_t versatile_uart0_vbase;

/* SDRAM region */
#define SDRAM_START             0x04000000
#define SDRAM_END               0x08000000

#define VERSATILE_UART0_PEND    (VERSATILE_UART0_PBASE + 0x1000)

/*---------------------------------------------------------------------------
  I/O Virtual to Physical mapping
---------------------------------------------------------------------------*/
#define MAP_ENTRY(base,n) { base + n * ARM_SECTION_SIZE, base##_PHYS + n * ARM_SECTION_SIZE}

#define SIZE_4K 4096

word_t soc_api_version = SOC_API_VERSION;

/*
 * Initialize the platform specific mappings needed
 * to start the kernel.
 * Add other hardware initialization here as well
 */
void SECTION(".init")
soc_init(void)
{
    volatile vic_t *sic = NULL;
    /* Map peripherals and control registers */

    versatile_io_vbase = kernel_add_mapping(SIZE_4K,
                                            (addr_t)VERSATILE_IO_PBASE,
                                            SOC_CACHE_DEVICE);
    SOC_ASSERT(ALWAYS, versatile_io_vbase);
    versatile_sctl_vbase = kernel_add_mapping(SIZE_4K,
                                              (addr_t)VERSATILE_SCTL_PBASE,
                                              SOC_CACHE_DEVICE);
    SOC_ASSERT(ALWAYS, versatile_sctl_vbase);
    versatile_timer0_vbase = kernel_add_mapping(SIZE_4K,
                                                (addr_t)VERSATILE_TIMER0_PBASE,
                                                SOC_CACHE_DEVICE);
    SOC_ASSERT(ALWAYS, versatile_timer0_vbase);
    versatile_vic_vbase = kernel_add_mapping(SIZE_4K,
                                             (addr_t)VERSATILE_VIC_PBASE,
                                             SOC_CACHE_DEVICE);
    SOC_ASSERT(ALWAYS, versatile_vic_vbase);
    versatile_sic_vbase = kernel_add_mapping(SIZE_4K,
                                             (addr_t)VERSATILE_SIC_PBASE,
                                             SOC_CACHE_DEVICE);
    SOC_ASSERT(ALWAYS, versatile_sic_vbase);
    versatile_uart0_vbase = kernel_add_mapping(SIZE_4K,
                                               (addr_t)VERSATILE_UART0_PBASE,
                                               SOC_CACHE_DEVICE);
    SOC_ASSERT(ALWAYS, versatile_uart0_vbase);

    SOC_TRACEF("SCTL_PBASE:0x%08x SCTL_VBASE:0x%08x\n",
               VERSATILE_SCTL_PBASE,
               versatile_sctl_vbase);
    SOC_TRACEF("TIMER0_PBASE:0x%08x TIMER0_VBASE:0x%08x\n",
               VERSATILE_TIMER0_PBASE,
               versatile_timer0_vbase);
    SOC_TRACEF("VIC_PBASE:0x%08x VIC_VBASE:0x%08x\n",
               VERSATILE_VIC_PBASE,
               versatile_vic_vbase);
    SOC_TRACEF("SIC_PBASE:0x%08x SIC_VBASE:0x%08x\n",
               VERSATILE_SIC_PBASE,
               versatile_sic_vbase);
    SOC_TRACEF("UART0_PBASE:0x%08x UART0_VBASE:0x%08x\n",
               VERSATILE_UART0_PBASE,
               versatile_uart0_vbase);

    /* Clear all SIC interrupt forwards */
    sic = (vic_t *)versatile_sic_vbase;
    sic->protect = 0UL;

    simplesoc_init();
    init_clocks();
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

