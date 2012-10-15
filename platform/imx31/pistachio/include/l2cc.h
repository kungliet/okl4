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
 * Description:   i.MX31 Platform L2 Cache Controler
 */
#ifndef __PLATFORM__IMX31__L2CC_H_
#define __PLATFORM__IMX31__L2CC_H_

#include <soc/soc_types.h>
#include <io.h>

#ifdef CONFIG_ARM_L2_INSTRUCTIONS
    #error "Unimplemented"
#else

#define L2CC_PHYS_BASE              IO_AREA3_PADDR
#define L2CC_VIRT_BASE              ((word_t)io_area3_vaddr)

#define L2EVTMON_PHYS_BASE          IO_AREA4_PADDR
#define L2EVTMON_VIRT_BASE          ((word_t)io_area4_vaddr)

#define L2CC_CACHE_ID               (*(volatile word_t *)(L2CC_VIRT_BASE + 0x000))
#define L2CC_CACHE_TYPE             (*(volatile word_t *)(L2CC_VIRT_BASE + 0x004))
#define L2CC_CONTROL                (*(volatile word_t *)(L2CC_VIRT_BASE + 0x100))
#define L2CC_AUX_CONTROL            (*(volatile word_t *)(L2CC_VIRT_BASE + 0x104))
#define L2CC_CACHE_SYNC             (*(volatile word_t *)(L2CC_VIRT_BASE + 0x730))
#define L2CC_IVLD_BY_WAY            (*(volatile word_t *)(L2CC_VIRT_BASE + 0x77C))
#define L2CC_IVLD_BY_PA             (*(volatile word_t *)(L2CC_VIRT_BASE + 0x770))
#define L2CC_CLN_BY_IDX_WAY         (*(volatile word_t *)(L2CC_VIRT_BASE + 0x7B8))
#define L2CC_CLN_BY_WAY             (*(volatile word_t *)(L2CC_VIRT_BASE + 0x7BC))
#define L2CC_CLN_BY_PA              (*(volatile word_t *)(L2CC_VIRT_BASE + 0x7B0))
#define L2CC_IVLD_CLN_BY_IDX_WAY    (*(volatile word_t *)(L2CC_VIRT_BASE + 0x7F8))
#define L2CC_IVLD_CLN_BY_WAY        (*(volatile word_t *)(L2CC_VIRT_BASE + 0x7FC))
#define L2CC_IVLD_CLN_BY_PA         (*(volatile word_t *)(L2CC_VIRT_BASE + 0x7F0))
#define L2CC_LOCK_BY_WAY_DATA       (*(volatile word_t *)(L2CC_VIRT_BASE + 0x900))
#define L2CC_LOCK_BY_WAY_INST       (*(volatile word_t *)(L2CC_VIRT_BASE + 0x904))
#define L2CC_TEST_OP                (*(volatile word_t *)(L2CC_VIRT_BASE + 0xF00))
#define L2CC_LINE_DATA_ARRAY          (volatile word_t *)(L2CC_VIRT_BASE + 0xF10)
#define L2CC_LINE_TAG               (*(volatile word_t *)(L2CC_VIRT_BASE + 0xF30))
#define L2CC_DEBUG_CONTROL          (*(volatile word_t *)(L2CC_VIRT_BASE + 0xF40))

#define L2EVTMON_EMMC               (*(volatile word_t *)(L2EVTMON_VIRT_BASE + 0x00))
#define L2EVTMON_EMCS               (*(volatile word_t *)(L2EVTMON_VIRT_BASE + 0x04))
#define L2EVTMON_EMCC_BASE          (L2EVTMON_VIRT_BASE + 0x8)
#define L2EVTMON_EMCC(x)            (*(volatile word_t *)(L2EVTMON_EMCC_BASE + (x) * sizeof(word_t)))
#define L2EVTMON_EMC_BASE           (L2EVTMON_VIRT_BASE + 0x18)
#define L2EVTMON_EMC(x)             (*(volatile word_t *)(L2EVTMON_EMC_BASE + (x) * sizeof(word_t)))


/* i.MX31 has non-configurable l2 cache size of 128k, 8 ways */
//#define L2_WAY_LOG2                 3
//#define L2_INDEX_LOG2               9
//#define L2_TAG_LOG2                 18
#define RESET_VALUE                 0xE4020FFF
#ifdef CONFIG_HAS_L2_EVTMON
#define INIT_VALUE                  0x0013001B
#else
#define INIT_VALUE                  0x0003001B
#endif

#define  L2_LINE_SIZE_LOG2      5

#define  TAG_REG_VALID_SHIFT    13
#define  TAG_REG_DIRTY_L_SHIFT  12
#define  TAG_REG_DIRTY_H_SHIFT  11

#define  L2EVTMON_TRIGGER_METHOD_OVERFLOW    0
#define  L2EVTMON_TRIGGER_METHOD_INCREAMENT  2
#define  L2EVTMON_IRQ_DISABLED               0
#define  L2EVTMON_IRQ_ENABLED                1
#define  L2EVTMON_REGISTER_NUMBER            4
#define  L2EVTMON_EMMC_INIT_VALUE            0x00000F01
#define  L2EVTMON_EMMC_COUNTER_RESET         0x00000F00
#define  L2EVTMON_EMMC_ENABLE                0x00000001

#define  L2EVTMON_EVT_BWABT                  0x01UL
#define  L2EVTMON_EVT_CO                     0x02UL
#define  L2EVTMON_EVT_DRHIT                  0x03UL
#define  L2EVTMON_EVT_DRREQ                  0x04UL
#define  L2EVTMON_EVT_DWHIT                  0x05UL
#define  L2EVTMON_EVT_DWREQ                  0x06UL
#define  L2EVTMON_EVT_DWTREQ                 0x07UL
#define  L2EVTMON_EVT_IRHIT                  0x08UL
#define  L2EVTMON_EVT_IRREQ                  0x09UL
#define  L2EVTMON_EVT_WA                     0x0AUL
#define  L2EVTMON_EVT_EMC3OFL                0x0BUL
#define  L2EVTMON_EVT_EMC2OFL                0x0CUL
#define  L2EVTMON_EVT_EMC1OFL                0x0DUL
#define  L2EVTMON_EVT_EMC0OFL                0x0EUL
#define  L2EVTMON_EVT_CLK                    0x0FUL

#define  L2EVTMON_IRQ                        23

#endif /* !CONFIG_ARM_L2_INSTRUCTIONS */

typedef struct
{
    union {
        word_t raw;
        struct {
            BITFIELD12 (word_t,
                    lat_data_read       : 3,
                    lat_data_write      : 3,
                    lat_tag             : 3,
                    lat_dirty           : 3,
                    wrap_disable        : 1,
                    associativity       : 4,
                    way_size            : 3,
                    evt_bus_enable      : 1,
                    parity_enable       : 1,
                    wa_override         : 1,
                    ex_abt_disable      : 1,
                    reserved            : 7
                    );
        } reg;
    };
}  l210_aux_ctrl_t;


INLINE void l210_reset(l210_aux_ctrl_t *c);
INLINE void l210_init(l210_aux_ctrl_t *c);
INLINE word_t l210_read(l210_aux_ctrl_t *c);
INLINE void l210_write(l210_aux_ctrl_t *c, word_t value);
INLINE word_t l210_get_way_log2(l210_aux_ctrl_t *c);

word_t arm_l2_cache_get_associativity(void);
word_t arm_l2_cache_get_index_size_log2(void);
word_t arm_l2_cache_get_way_log2(void);

INLINE void l210_reset(l210_aux_ctrl_t *c)
{
    c->raw = RESET_VALUE;
}

INLINE void l210_init(l210_aux_ctrl_t *c)
{
    c->raw = INIT_VALUE;
}

INLINE word_t l210_read(l210_aux_ctrl_t *c)
{
    c->raw = L2CC_AUX_CONTROL;
    return c->raw;
}

/** 
 * i.MX31 has non-configurable l2 cache, can not change associativity
 * and way_size.
 */
INLINE void l210_write(l210_aux_ctrl_t *c, word_t value)
{
    value &= ~0xFE000UL;
    value |= 0x30000UL;
    c->raw = value;
    L2CC_AUX_CONTROL = value;
}

INLINE word_t l210_get_way_log2(l210_aux_ctrl_t *c)
{
    return 3;
}

INLINE void arm_l2_cache_clean_by_pa(addr_t pa)
{
    L2CC_CLN_BY_PA = (word_t)pa & ~((1UL << L2_LINE_SIZE_LOG2) - 1);
}

/**
 * @brief Full flush of l2 cache.
 */
void arm_l2_cache_flush(void);

/**
 * @brief Flush a cache line by a single physical address.
 *
 * @param addr The physical address to flush.
 *
 * @post If the physical address is in the cache, the line which contains
 *       it is flushed.
 */
INLINE void arm_l2_cache_flush_by_pa(addr_t pa)
{
    L2CC_IVLD_CLN_BY_PA = (word_t)pa & ~((1UL << L2_LINE_SIZE_LOG2) - 1);
}


/**
 * @brief Flush cache by "way" rather than lines.
 *
 * @param wb Has the bit set which indicates the "way" to flush.
 *
 * @post The "way" indicated in #wb has been flushed.
 */
void arm_l2_cache_flush_by_way(word_t wb);

/**
 * @brief Mark whole l2 cache as invalid.
 */
void arm_l2_cache_invalidate(void);

/**
 * @brief Mark whole l2 cache as cleaned.
 */
void arm_l2_cache_clean(void);

/**
 * @brief Invalidate a cache line by a single physical address.
 *
 * @param addr The physical address to mark.
 *
 * @post If the physical address is in the cache, the line which contains
 *       it is marked as invalid.
 */
INLINE void arm_l2_cache_invalidate_by_pa(addr_t pa)
{
    L2CC_IVLD_BY_PA = (word_t)pa & ~((1UL << L2_LINE_SIZE_LOG2) - 1);
}


/**
 * @brief Invalidate cache by "way" rather than lines.
 *
 * @param wb Has the bit set which indicates the "way" to invalidate.
 *
 * @post All lines in the "way" indicated in #wb have been marked as invalid.
 */
void arm_l2_cache_invalidate_by_way(word_t wb);

/**
 * @brief Clean cache by "way" rather than lines.
 *
 * @param wb Has the bit set which indicates the "way" to clean.
 *
 * @post All lines in the "way" indicated in #wb have been marked as cleaned.
 */
void arm_l2_cache_clean_by_way(word_t wb);

/**
 * @brief Flush cache lines by physical address range.
 *
 * @param addr  The physical address to start the flush at.
 * @param range The size (in bytes) of the range to flush.
 *
 * @post All cache lines which contain part of the specified physical
 *       address range are flushed.
 */

INLINE void arm_l2_cache_flush_by_pa_range_log2(addr_t pa, word_t sizelog2)
{
    if (sizelog2 <= L2_LINE_SIZE_LOG2)
        arm_l2_cache_flush_by_pa(pa);
    else if (sizelog2 >= (L2_LINE_SIZE_LOG2 + arm_l2_cache_get_index_size_log2()))
        arm_l2_cache_flush();
    else
    {
        word_t index;
        for (index = 0; index < (1UL << (sizelog2 - L2_LINE_SIZE_LOG2)); index++)
            arm_l2_cache_flush_by_pa((addr_t)((word_t)pa + index * (1UL << L2_LINE_SIZE_LOG2)));
    }
}

INLINE void arm_l2_cache_flush_by_pa_range(addr_t pa, word_t size)
{
    if (size <= (1UL << L2_LINE_SIZE_LOG2))
        arm_l2_cache_flush_by_pa(pa);
    else if (size >= (1UL << (L2_LINE_SIZE_LOG2 + arm_l2_cache_get_index_size_log2())))
        arm_l2_cache_flush();
    else
    {
        word_t index;
        for (index = 0; index < size; index += 1UL << L2_LINE_SIZE_LOG2)
            arm_l2_cache_flush_by_pa((addr_t)((word_t)pa + index));
    }
}

/**
 * @brief Invalidate cache lines by physical address range.
 *
 * @param addr  The physical address to start the invalidation at.
 * @param range The size (in bytes) of the range to invalidate.
 *
 * @post All cache lines which contain part of the specified physical
 *       address range are invalidated.
 */

INLINE void arm_l2_cache_invalidate_by_pa_range_log2(addr_t pa, word_t sizelog2)
{
    if (sizelog2 <= L2_LINE_SIZE_LOG2)
        arm_l2_cache_invalidate_by_pa(pa);
    else if (sizelog2 >= (L2_LINE_SIZE_LOG2 + arm_l2_cache_get_index_size_log2()))
        arm_l2_cache_invalidate();
    else
    {
        word_t index;
        for (index = 0; index < (1UL << (sizelog2 - L2_LINE_SIZE_LOG2)); index++)
            arm_l2_cache_invalidate_by_pa((addr_t)((word_t)pa + index * (1UL << L2_LINE_SIZE_LOG2)));
    }
}

INLINE void arm_l2_cache_invalidate_by_pa_range(addr_t pa, word_t size)
{
    if (size <= (1UL << L2_LINE_SIZE_LOG2))
        arm_l2_cache_invalidate_by_pa(pa);
    else if (size >= (1UL << (L2_LINE_SIZE_LOG2 + arm_l2_cache_get_index_size_log2())))
        arm_l2_cache_invalidate();
    else
    {
        word_t index;
        for (index = 0; index < size; index += 1UL << L2_LINE_SIZE_LOG2)
            arm_l2_cache_invalidate_by_pa((addr_t)((word_t)pa + index));
    }
}

/**
 * @brief Clean cache lines by physical address range.
 *
 * @param addr  The physical address to start the clean at.
 * @param range The size (in bytes) of the range to clean.
 *
 * @post All cache lines which contain part of the specified physical
 *       address range are cleaned.
 */

INLINE void arm_l2_cache_clean_by_pa_range_log2(addr_t pa, word_t sizelog2)
{
    if (sizelog2 <= L2_LINE_SIZE_LOG2)
        arm_l2_cache_clean_by_pa(pa);
    else if (sizelog2 >= (L2_LINE_SIZE_LOG2 + arm_l2_cache_get_index_size_log2()))
        arm_l2_cache_clean();
    else
    {
        word_t index;
        for (index = 0; index < (1UL << (sizelog2 - L2_LINE_SIZE_LOG2)); index++)
            arm_l2_cache_clean_by_pa((addr_t)((word_t)pa + index * (1UL << L2_LINE_SIZE_LOG2)));
    }
}

INLINE void arm_l2_cache_clean_by_pa_range(addr_t pa, word_t size)
{
    if (size <= (1UL << L2_LINE_SIZE_LOG2))
        arm_l2_cache_clean_by_pa(pa);
    else if (size >= (1UL << (L2_LINE_SIZE_LOG2 + arm_l2_cache_get_index_size_log2())))
        arm_l2_cache_clean();
    else
    {
        word_t index;
        for (index = 0; index < size; index += 1UL << L2_LINE_SIZE_LOG2)
            arm_l2_cache_clean_by_pa((addr_t)((word_t)pa + index));
    }
}
/**
 * @brief Read the data and tag in a cache line.
 *
 * @param      wb     The "way" to use.
 * @param      index  The line in the "way" to read.
 * @param[out] data   Storage for the current data in the cache line.
 * @param[out] tag    Storage for the cache line's tag.
 *
 * @post #data and #tag are loaded with the values from the specified
 *       cache line.
 */
void arm_l2_cache_debug_read_line_tag(word_t wb,
                                      word_t index,
                                      word_t *data,
                                      word_t *tag);
/**
 * @brief Lock one "way" in the instruction section of the cache.
 *
 * @param wb Indicates the "way" to lock.
 *
 */
void arm_l2_cache_lockway_i(word_t wb);

/**
 * @brief Lock one "way" in the data section of the cache.
 *
 * @param wb Indicates the "way" to lock.
 *
 */
void arm_l2_cache_lockway_d(word_t wb);

/**
 * @brief Indicate if the l2 cache is currently enabled or not.
 *
 * @retval 0 if cache is not enabled
 * @retval !0 if cache is enabled.
 */
int  arm_l2_cache_is_enabled(void);

/**
 * @brief Enable the l2 cache.
 *
 * @post Cache is enabled.
 */
void arm_l2_cache_enable(void);

/**
 * @brief Disable the l2 cache.
 *
 * @post Cache is disabled.
 */
void arm_l2_cache_disable(void);

/**
 * @brief Initialize the l2 cache.
 *
 * @post ?
 */
void arm_l2_cache_init(void);

/*
 * Cache event monitoring.
 */

#if defined(CONFIG_HAS_L2_EVTMON)

extern word_t arm_eventmon_counter_number;

/**
 * @brief initialise the event monitoring.
 *
 * @post All counters are cleared and monitoring is disabled.
 */

void arm_eventmon_init(void);

bool arm_is_EMC_event_occured(word_t number);
void arm_clear_EMC_event(word_t number);
bool arm_read_EMC(word_t number, word_t * pvalue);
word_t arm_read_EMCC(word_t number);
void arm_set_EMCC(word_t number,
                  word_t event,
                  word_t trigger_method,
                  word_t enable_irq);

/**
 * @brief Enable event monitoring.
 */
void arm_EMC_enable(void);

/**
 * @brief Disable event monitoring.
 *
 * @post current counter values are preserved.
 */
void arm_EMC_disable(void);

#endif /* CONFIG_HAS_L2_EVTMON */

void init_enable_l2cache(void);


#endif /* __PLATFORM__IMX31__CACHE_H_ */
