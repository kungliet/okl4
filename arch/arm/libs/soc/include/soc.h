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

/**
 * @file
 * @brief Contains the interface for functionality provided on a per-platform
 *        basis, specific to ARM-based platforms.
 *
 */

#ifndef __L4__ARCH_PLATFORM_H__
#define __L4__ARCH_PLATFORM_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_PERF)
/**
 * @brief This word contains the flags which indicate that performance
 *        counters are enabled.
 */
extern word_t soc_perf_counter_irq;

void soc_perf_counter_unmask(void);
#endif

/**
 * @brief Interrupt dispatch routine.
 *
 * This is normally called from assembly code common to the
 * architecture.
 *
 * @param ctx A pointer to the context at interrupt time.
 * @param fiq If set to 0 indicates a normal interrupt.  !0 indicates
 *            an FIQ.
 */
void soc_handle_interrupt(word_t ctx,
                          word_t fiq);

/*
 * SOC platform cache manipulation functions
 *
 * These functions provide a "common" API for all caches that are
 * not integrated in CPU but are implemented on platform.
 */

#if defined(CONFIG_HAS_SOC_CACHE)
    /*
     * @brief Perform outer cache range operation.
     *
     * This is normally called from cache control system call.
     *
     * @param pa physical address that the cache line contains.
     * @param sizelog2 log base 2 of the size of the cache range.
     * @param attrib Instuction or Data side of the cache.
     */
    void soc_cache_range_op_by_pa(addr_t pa, word_t sizelog2, word_t attrib);
    /*
     * @brief Perform outer cache full operation.
     *
     * This is normally called from cache control system call.
     *
     * @param attrib Instuction or Data side of the cache.
     */
    void soc_cache_full_op(word_t attrib);
    /*
     * @brief Drain all write buffers on the outer cache (Outer Memory Barrier).
     *
     * This is normally called from cache control system call.
     *
     */
    void soc_cache_drain_write_buffer(void);
    /* Will add more functions for cache control system call and kdb in the future. */
#endif

/*
 * SOC platform KDB cache and evtmon functions
 *
 * These functions prints debug information of L2 cache and event monitor.
 */
#if defined(CONFIG_KDB_CONS)
#if defined(CONFIG_HAS_SOC_CACHE)
/*
 * @brief Perform dumping all cache lines in level 2 cache.
 */
void soc_kdb_dump_whole_l2cache(void);
/*
 * @brief Dump cache lines of all the ways that shares the same index.
 *
 * @param index the cache index.
 */
void soc_kdb_dump_l2cache(word_t index);
#endif
#if defined(CONFIG_HAS_L2_EVTMON)
/*
 * @brief Dump level 2 event monitor counters.
 */
void soc_kdb_read_event_monitor(void);
/*
 * @brief Set level 2 event monitor counter and start to count a event.
 *
 * @param counter which counter to use.
 * @param event which level 2 event to be counted.
 */
void soc_kdb_set_event_monitor(word_t counter, word_t event);
#endif
#endif


#ifdef __cplusplus
}
#endif

#endif /* ! __L4__ARCH_PLATFORM_H__ */
