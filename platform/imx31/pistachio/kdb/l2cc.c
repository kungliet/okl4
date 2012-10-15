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
 * Description:   imx31 L2 Cache KDB interface implement
 */
#include <soc/soc.h>
#include <soc/arch/soc.h>
#include <l2cc.h>

#if defined(CONFIG_KDB_CONS)
#if defined(CONFIG_HAS_SOC_CACHE)
static void print_index(word_t index)
{
    (void)kernel_printf("-----------------------------------\n");
    (void)kernel_printf("Index = %d (address[%d:%d] = 0x%x) :\n", 
            index, L2_LINE_SIZE_LOG2 + arm_l2_cache_get_index_size_log2() - 1, L2_LINE_SIZE_LOG2, index);
    (void)kernel_printf("-----------------------------------\n");
}

static void print_cache_line(word_t index, word_t way)
{
    word_t tag = 0;
    word_t line[(1 << L2_LINE_SIZE_LOG2) / sizeof(word_t)];
    word_t tag_shift = L2_LINE_SIZE_LOG2 + arm_l2_cache_get_index_size_log2();

    arm_l2_cache_debug_read_line_tag(way, index, line, &tag);

    if (((tag >> TAG_REG_VALID_SHIFT) & 1) != 0)
    {
        (void)kernel_printf("Way %d: (0x%x/0x%x)\n",
                way, ((tag >> tag_shift) << tag_shift) | (index << L2_LINE_SIZE_LOG2),
                tag >> tag_shift);

        if (((tag >> TAG_REG_DIRTY_L_SHIFT) & 1) != 0)
            (void)kernel_printf("[DIRTY]\t");
        else
            (void)kernel_printf("[CLEAN]\t");
        (void)kernel_printf("%08x %08x %08x %08x\n",
                line[0], line[1], line[2], line[3]);
        if (((tag >> TAG_REG_DIRTY_H_SHIFT) & 1) != 0)
            (void)kernel_printf("[DIRTY]\t");
        else
            (void)kernel_printf("[CLEAN]\t");
        (void)kernel_printf("%08x %08x %08x %08x\n",
                line[4], line[5], line[6], line[7]);
    }
    else /* invalid line */
    {
        (void)kernel_printf("Way %d: (---/---)\n",
                way);
        (void)kernel_printf("[INVAL]\t******** ******** ******** ********\n[INVAL]\t******** ******** ******** ********\n"); 
    }
}

static void print_cache_index(word_t index)
{
    word_t way;

    print_index(index);
    for (way = 0; way < arm_l2_cache_get_associativity(); way++)
    {
        print_cache_line(index, way);
    }
}

static void print_format(word_t all)
{
    if (all)
        (void)kernel_printf("\n\n========== Dumping Whole L2 Cache ==========\n");
    else
        (void)kernel_printf("\n\n========== Dumping L2 Cache line ===========\n");
    (void)kernel_printf("Way X: (Phys Addr/Tag)\n[1st half line dirty flag]\tword0 word1 word2 word3\n[2nd half line dirty flag]\tword4 word5 word6 word7\n");
}

void soc_kdb_dump_whole_l2cache(void)
{
    word_t index;
    print_format(1);
    for (index = 0; index < (1UL << arm_l2_cache_get_index_size_log2()); index++)
    {
        print_cache_index(index);
    }
}

void soc_kdb_dump_l2cache(word_t index)
{
    print_format(0);
    print_cache_index(index);
}
#endif

#ifdef CONFIG_HAS_L2_EVTMON
struct string_l2_event {
    char * string;
    word_t event;
};

static struct string_l2_event events[17] = {
    {"No event", 0},
    {"L2 bufferd write abort", L2EVTMON_EVT_BWABT},
    {"L2 half-line eviction", L2EVTMON_EVT_CO},
    {"L2 data read hit", L2EVTMON_EVT_DRHIT},
    {"L2 data read request", L2EVTMON_EVT_DRREQ},
    {"L2 data write hit", L2EVTMON_EVT_DWHIT},
    {"L2 data write request", L2EVTMON_EVT_DWREQ},
    {"L2 data write through request", L2EVTMON_EVT_DWTREQ},
    {"L2 instruction read hit", L2EVTMON_EVT_IRHIT},
    {"L2 instruction read request", L2EVTMON_EVT_IRREQ},
    {"L2 write allocate", L2EVTMON_EVT_WA},
    {"Counter 3 overflow", L2EVTMON_EVT_EMC3OFL},
    {"Counter 2 overflow", L2EVTMON_EVT_EMC2OFL},
    {"Counter 1 overflow", L2EVTMON_EVT_EMC1OFL},
    {"Counter 0 overflow", L2EVTMON_EVT_EMC0OFL},
    {"L2 Clock", L2EVTMON_EVT_CLK},
    {"Invalid event", 16},
};

static char * get_string(word_t event)
{
    for (int i = 0; i< 17; i++)
    {
        if (events[i].event == event)
            return events[i].string;
    }
    return "Invalid event";
}

static void print_l2_events(void)
{
    (void)kernel_printf("\t%2d - %s\n", 0, "Display this list");
    for (int i = 1; i< 16; i++)
    {
        (void)kernel_printf("\t%2d - %s\n", events[i].event, events[i].string);
    }
    (void)kernel_printf("\t%2d - %s\n", 16, "Disable this event register");
}

static void print_evtmon_registers(void)
{
    word_t count0, count1, count2;

    (void)kernel_printf("\t           count           event\n");
    if (arm_read_EMC(0, &count0))
        (void)kernel_printf("count 0%16lu        %s\n", count0, get_string(arm_read_EMCC(0) >> 2));
    if (arm_read_EMC(1, &count1))
        (void)kernel_printf("count 1%16lu        %s\n", count1, get_string(arm_read_EMCC(1) >> 2));
    if (arm_read_EMC(2, &count2))
        (void)kernel_printf("count 2%16lu        %s\n", count2, get_string(arm_read_EMCC(2) >> 2));
#if 0 /* print raw value of each regs for debug purpose. */
    kernel_printf("EMMC  0x%08lx, EMCS  0x%08lx\n"
           "EMCC0 0x%08lx, EMCC1 0x%08lx, EMCC2 0x%08lx, EMCC3 0x%08lx\n"
           "EMC0  0x%08lx, EMC1  0x%08lx, EMC2  0x%08lx, EMC3  0x%08lx\n",
           L2EVTMON_EMMC, L2EVTMON_EMCS,
           L2EVTMON_EMCC(0), L2EVTMON_EMCC(1), L2EVTMON_EMCC(2), L2EVTMON_EMCC(3),
           L2EVTMON_EMC(0), L2EVTMON_EMC(1), L2EVTMON_EMC(2), L2EVTMON_EMC(3));
#endif
}

void soc_kdb_read_event_monitor(void)
{
    /* disable l2 event monitor to prevent changing counter because of print */
    arm_EMC_disable();

    print_evtmon_registers();

    arm_EMC_enable();
}

void soc_kdb_set_event_monitor(word_t counter, word_t event)
{
    /* disable l2 event monitor to prevent changing counter because of print */
    arm_EMC_disable();

    if (event == 0)
        print_l2_events();
    else if (event < 16)
    {
        arm_set_EMCC(counter, event, L2EVTMON_TRIGGER_METHOD_OVERFLOW, L2EVTMON_IRQ_DISABLED);
    }
    else if (event >= 16)
    {
        arm_set_EMCC(counter, 0, L2EVTMON_TRIGGER_METHOD_OVERFLOW, L2EVTMON_IRQ_DISABLED);
    }

    arm_EMC_enable();
}

#endif
#endif
