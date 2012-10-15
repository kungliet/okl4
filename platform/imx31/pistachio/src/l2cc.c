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
 * Description:   imx31 L2 Cache Controlor implement
 */

#include <soc/soc.h>
#include <soc/arch/soc.h>
#include <l2cc.h>
#include <kernel/cache.h>

#ifdef CONFIG_CP15_SUPPORT_L2CACHE
    #error "Unimplemented yet"
#else


#if defined (CONFIG_HAS_SOC_CACHE)

static l210_aux_ctrl_t aux;
static word_t enabled;

void init_enable_l2cache(void)
{
#if defined(CONFIG_HAS_L2_EVTMON)
    arm_eventmon_init();
#endif
    arm_l2_cache_init();
    arm_l2_cache_enable();
    arm_l2_cache_flush();
}

void arm_l2_cache_init(void)
{
    /* Aux control reg should only be changed when l2cc is disabled. */
    arm_l2_cache_disable();
    l210_init(&aux);
    L2CC_AUX_CONTROL = aux.raw;
    arm_l2_cache_invalidate();
}

void arm_l2_cache_enable(void)
{
        enabled = 0x1;
        L2CC_CONTROL = enabled;
}

void arm_l2_cache_disable(void)
{
        enabled = 0x0;
        L2CC_CONTROL = enabled;
}

int arm_l2_cache_is_enabled(void)
{
    return (int)enabled;
}

void arm_l2_cache_flush_by_way(word_t way_bit)
{
    L2CC_IVLD_CLN_BY_WAY = way_bit;
    while (L2CC_IVLD_CLN_BY_WAY != 0);
}

void arm_l2_cache_flush(void)
{
    word_t way_bit = (1UL << arm_l2_cache_get_associativity()) - 1;
    arm_l2_cache_flush_by_way(way_bit);
}

void arm_l2_cache_invalidate_by_way(word_t way_bit)
{
    L2CC_IVLD_BY_WAY = way_bit;
    while (L2CC_IVLD_BY_WAY != 0);
}

void arm_l2_cache_clean_by_way(word_t way_bit)
{
    L2CC_CLN_BY_WAY = way_bit;
    while (L2CC_CLN_BY_WAY != 0);
}

void arm_l2_cache_invalidate(void)
{
    word_t way_bit = (1UL << arm_l2_cache_get_associativity()) - 1;
    arm_l2_cache_invalidate_by_way(way_bit);
}

void arm_l2_cache_clean(void)
{
    word_t way_bit = (1UL << arm_l2_cache_get_associativity()) - 1;
    arm_l2_cache_clean_by_way(way_bit);
}

void arm_l2_cache_debug_read_line_tag(word_t way,
                                      word_t index,
                                      word_t *data,
                                      word_t *tag)
{
    word_t word_bit = BITS_WORD;
    word_t way_log2 = arm_l2_cache_get_way_log2();
    word_t index_log2 = arm_l2_cache_get_index_size_log2();
    word_t op_word =
            (way << (word_bit - way_log2))
            | ((index << (word_bit - index_log2))
                    >> (word_bit - index_log2 - L2_LINE_SIZE_LOG2));
    word_t i;
    volatile word_t *line = L2CC_LINE_DATA_ARRAY;
    L2CC_DEBUG_CONTROL = 0x3; //Disable WB and linefile.
    L2CC_TEST_OP = op_word;
    /* Important!!!
     * Have to wait for a while until line register to be filled. */
    for (i = 0; i < 100; i++);
    *tag = L2CC_LINE_TAG;
    for (i = 0; i < ((1UL << L2_LINE_SIZE_LOG2)/sizeof(word_t)); i++)
        data[i] = line[i];
    L2CC_DEBUG_CONTROL = 0x0; //Enable WB and linefile.
}
void arm_l2_cache_lockway_i(word_t way_bit)
{
    L2CC_LOCK_BY_WAY_INST = way_bit;
}

void arm_l2_cache_lockway_d(word_t way_bit)
{
    L2CC_LOCK_BY_WAY_DATA = way_bit;
}

word_t arm_l2_cache_get_associativity(void)
{
    return (word_t)aux.reg.associativity;
}

word_t arm_l2_cache_get_index_size_log2(void)
{
    return (word_t)aux.reg.way_size + 8;
}

word_t arm_l2_cache_get_way_log2(void)
{
    return l210_get_way_log2(&aux);
}

/* SoC functions. */

void soc_cache_range_op_by_pa(addr_t pa, word_t size, word_t attrib)
{
    switch((enum cacheattr_e)attrib)
    {
        case attr_invalidate_i:
        case attr_invalidate_d:
        case attr_invalidate_id:
            arm_l2_cache_invalidate_by_pa_range(pa, size);
            break;
        case attr_clean_i:
        case attr_clean_d:
        case attr_clean_id:
            arm_l2_cache_clean_by_pa_range(pa, size);
            break;
        case attr_clean_inval_i:
        case attr_clean_inval_d:
        case attr_clean_inval_id:
            arm_l2_cache_flush_by_pa_range(pa, size);
            break;
        default:
            break;
    }
}

void soc_cache_full_op(word_t attrib)
{
    switch((enum cacheattr_e)attrib)
    {
        case attr_invalidate_i:
        case attr_invalidate_d:
        case attr_invalidate_id:
            arm_l2_cache_invalidate();
            break;
        case attr_clean_i:
        case attr_clean_d:
        case attr_clean_id:
            arm_l2_cache_clean();
            break;
        case attr_clean_inval_i:
        case attr_clean_inval_d:
        case attr_clean_inval_id:
            arm_l2_cache_flush();
            break;
        default:
            break;
    }
}

void soc_cache_drain_write_buffer(void)
{
    /* 
     * Nothing needs to be done, on L210,
     * Cache Sync (drain write buffer) is
     * automatically performed on clean,
     * invalidate or flush cache operations.
     */
}

#endif

#ifdef CONFIG_HAS_L2_EVTMON
word_t arm_eventmon_counter_number;

void arm_eventmon_init(void)
{
    arm_eventmon_counter_number = L2EVTMON_REGISTER_NUMBER;
    /*Use the last counter to detect L2 Cache Write Buffer abort */
    L2EVTMON_EMCC(arm_eventmon_counter_number - 1) = (L2EVTMON_EVT_BWABT << 2) | L2EVTMON_TRIGGER_METHOD_INCREAMENT | L2EVTMON_IRQ_ENABLED;
    L2EVTMON_EMMC = L2EVTMON_EMMC_INIT_VALUE;
}

void arm_EMC_enable(void)
{
    volatile word_t old_value = L2EVTMON_EMMC;
    L2EVTMON_EMMC = old_value | L2EVTMON_EMMC_ENABLE;
}

void arm_EMC_disable(void)
{
    volatile word_t old_value = L2EVTMON_EMMC;
    L2EVTMON_EMMC = old_value & ~L2EVTMON_EMMC_ENABLE;
}

bool arm_is_EMC_event_occured(word_t number)
{
    if (number < arm_eventmon_counter_number)
        return (L2EVTMON_EMCS & (1UL << number));
    else
        return false;
}

void arm_clear_EMC_event(word_t number)
{
    if (number < arm_eventmon_counter_number)
    {
        volatile word_t old_value = L2EVTMON_EMCS;
        L2EVTMON_EMCS = old_value | (1UL << number);
    }
}

bool arm_read_EMC(word_t number, word_t * pvalue)
{
    if (number < arm_eventmon_counter_number)
    {
        *pvalue = (word_t)L2EVTMON_EMC(number);
        return 1;
    }
    else
        return false;
}

word_t arm_read_EMCC(word_t number)
{
    return L2EVTMON_EMCC(number);
}
void arm_set_EMCC(word_t number, word_t event, word_t trigger_method, word_t enable_irq)
{
    /**
     * !!!Important: the last counter is used to detect
     * L2 Buffer Write abort, and can only set in the
     * init function, not allowed to set here.
     */
    if (number < (arm_eventmon_counter_number - 1))
    {
        L2EVTMON_EMCC(number) = event << 2 | trigger_method | enable_irq;
    }
}
#endif
#endif
