/*
 * Copyright (c) 2004, National ICT Australia
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
 * Author: Changhua Chen Created: Aug 2 2007
 */

#include <l4/time.h>
#include <l4/kdebug.h>
#include <l4/caps.h>
#include <okl4/env.h>
#include <okl4/kmutexid_pool.h>
#include <okl4/kclistid_pool.h>
#include <okl4/kspaceid_pool.h>
#include <okl4/bitmap.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <bench/bench.h>

/* Misc benchs */
extern struct bench_test bench_zerosleep;
extern struct bench_test bench_memcpy;
extern struct bench_test bench_remote_memcpy;
extern struct bench_test bench_exreg;
//extern struct bench_test bench_myself;
extern struct bench_test bench_switch_myself;

/* IPC benchs */
extern struct bench_test bench_ipc_intra;
extern struct bench_test bench_ipc_pagemap;
extern struct bench_test bench_ipc_pagemap_simulated;
extern struct bench_test bench_ipc_fass;
extern struct bench_test bench_ipc_fass_buffer_vspace;
extern struct bench_test bench_ipc_fass_buffer;
extern struct bench_test bench_ipc_inter;
extern struct bench_test bench_ipc_page_faults;
extern struct bench_test bench_ipc_intra_close;
extern struct bench_test bench_ipc_intra_open;
extern struct bench_test bench_ipc_intra_rpc;
extern struct bench_test bench_ipc_intra_ovh;
extern struct bench_test bench_ipc_intra_async;
extern struct bench_test bench_ipc_intra_async_ovh;

/* Map Control benchs */
extern struct bench_test bench_mapcontrol_insert_m2m;
extern struct bench_test bench_mapcontrol_insert2_m2m;
extern struct bench_test bench_mapcontrol_delete_m2m;
extern struct bench_test bench_mapcontrol_delete2_m2m;
extern struct bench_test bench_mapcontrol_ovh1;
extern struct bench_test bench_mapcontrol_ovh2;

/* Thread Control benchs */
extern struct bench_test bench_threadcontrol;
extern struct bench_test bench_threadcontrol_ovh;

/* New interface benchs */
extern struct new_bench_test new_bench_irq_ipc;
extern struct new_bench_test new_bench_spaceswitch_priv;
extern struct new_bench_test new_bench_spaceswitch_sd;
extern struct new_bench_test new_bench_spaceswitch_no_sd;
extern struct new_bench_test new_bench_no_contention_mutex_lock;
extern struct new_bench_test new_bench_no_contention_mutex_unlock;
extern struct new_bench_test new_bench_contention_mutex_trylock;
extern struct new_bench_test new_bench_contention_mutex_lock;
extern struct new_bench_test new_bench_contention_mutex_unlock;

/* common counters */
struct counter cycle_counter;
struct counter null_counter;

#if defined(ARCH_ARM)
/* ARM PMN counters */
struct counter arm_PMN_branch_prediction;
struct counter arm_PMN_stall;
struct counter arm_PMN_cache_miss;
struct counter arm_PMN_d_cache_detail;
struct counter arm_PMN_PC;
struct counter arm_PMN_i_cache_efficiency;
struct counter arm_PMN_d_cache_efficiency;
struct counter arm_PMN_i_fetch_latency;
struct counter arm_PMN_d_stall;
#if defined(CONFIG_CPU_ARM_XSCALE)
struct counter arm_xscale_PMN_tlb;
struct counter arm_xscale_PMN_d_buffer_stall;
#endif
#if (defined(CONFIG_CPU_ARM_ARM1136JS) || defined(CONFIG_CPU_ARM_ARM1176JZS))
struct counter arm_V6_PMN_i_tlb;
struct counter arm_V6_PMN_d_tlb;
struct counter arm_V6_PMN_d_L2;
struct counter arm_V6_PMN_d_buffer;
#if defined(CONFIG_CPU_ARM_ARM1176JZS)
struct counter arm_V6_PMN_RS;
struct counter arm_v6_PMN_RS_Prediction;
#endif
#endif
#endif

extern uintptr_t _end, _start;

static struct counter *counters[][10] = { 
    {&cycle_counter},
    //{&null_counter}, 

#if defined(ARCH_ARM)
    /*!!! Important !!!
     * Only one of the following counters can be 
     * activatived at a time. 
     */
    //{&arm_PMN_branch_prediction}
    //{&arm_PMN_stall}
    //{&arm_PMN_cache_miss}
    //{&arm_PMN_d_cache_detail}
    //{&arm_PMN_PC}
    //{&arm_PMN_i_cache_efficiency}
    //{&arm_PMN_d_cache_efficiency}
    //{&arm_PMN_i_fetch_latency}
    //{&arm_PMN_d_stall}
#if defined(CONFIG_CPU_ARM_XSCALE)
    //{&arm_xscale_PMN_tlb}
    //{&arm_xscale_PMN_d_buffer_stall}
#endif
#if (defined(CONFIG_CPU_ARM_ARM1136JS) || defined(CONFIG_CPU_ARM_ARM1176JZS))
    //{&arm_V6_PMN_i_tlb}
    //{&arm_V6_PMN_d_tlb}
    //{&arm_V6_PMN_d_L2}
    //{&arm_V6_PMN_d_buffer}
#if defined(CONFIG_CPU_ARM_ARM1176JZS)
    //{&arm_V6_PMN_RS}
    //{&arm_v6_PMN_RS_Prediction}
#endif
#endif
#endif
};

static struct bench_test *benchmarks[] = {
    /* misc benchs */
    &bench_empty,
    //&bench_zerosleep,
    //&bench_memcpy,
    &bench_remote_memcpy,
    &bench_switch_myself,
    /* map control benchs */
    &bench_mapcontrol_insert_m2m,
    &bench_mapcontrol_insert2_m2m,
    &bench_mapcontrol_delete_m2m,
    &bench_mapcontrol_delete2_m2m,
    &bench_mapcontrol_ovh1,
    &bench_mapcontrol_ovh2,
    /* exchange register benchs */
    &bench_exreg,
    /* thread control benchs */
    &bench_threadcontrol,
    &bench_threadcontrol_ovh,
    /* ipc tests */
    &bench_ipc_intra_close,
    &bench_ipc_intra_open,
    &bench_ipc_intra_rpc,
    &bench_ipc_intra_ovh,
    &bench_ipc_intra_async,
    &bench_ipc_intra_async_ovh,
    /* deprecated ipc tests */
    //&bench_ipc_page_faults,
    //&bench_ipc_pagemap,
    //&bench_ipc_pagemap_simulated,
    //&bench_ipc_inter,
    //&bench_ipc_intra,
#if (defined(ARCH_ARM) && (ARCH_VER == 5))
    //&bench_ipc_fass,
    //&bench_ipc_fass_buffer,
    //&bench_ipc_fass_buffer_vspace,
#endif
    NULL
};

static struct new_bench_test *new_benchmarks[] = {
    /* !!! important !!!
     * new_bench_irq_ipc has to be the 1st bench test here,
     * since it will cleanup pager thread that is left from
     * old interface ipc bench tests above.
     */
    /* irq ipc latency benchs */
    &new_bench_irq_ipc,
    /* space switch benchs */
    //&new_bench_spaceswitch_priv,
#if defined(ARM_SHARED_DOMAINS)
    &new_bench_spaceswitch_sd,
#endif
    &new_bench_spaceswitch_no_sd,
    &new_bench_no_contention_mutex_lock,
    &new_bench_no_contention_mutex_unlock,
    &new_bench_contention_mutex_trylock,
    &new_bench_contention_mutex_lock,
    &new_bench_contention_mutex_unlock,
    NULL
};

word_t get_seg(L4_SpaceId_t spaceid, word_t vaddr, word_t *offset, word_t *cache, word_t *rwx);

word_t kernel_test_segment_id;
word_t kernel_test_segment_size;
word_t kernel_test_segment_vbase;

word_t get_seg(L4_SpaceId_t spaceid, word_t vaddr, word_t *offset, word_t *cache, word_t *rwx)
{
    static okl4_env_segments_t *segs = NULL;
    if (segs == NULL) {
        segs = OKL4_ENV_GET_SEGMENTS("SEGMENTS");
    }

    for (int i = 0; i < segs->num_segments; ++i) {
        okl4_env_segment_t *seg;
        seg = &segs->segments[i];
        if (seg->virt_addr <= vaddr && vaddr < seg->virt_addr + seg->size) {
            *offset = seg->offset + (vaddr - seg->virt_addr);
            *cache = seg->cache_policy;
            *rwx = seg->rights;
            return seg->segment;
        }
    }
    return ~0UL;
}


struct okl4_bitmap_allocator *spaceid_pool;
struct okl4_bitmap_allocator *mutexid_pool;
struct okl4_bitmap_allocator *clistid_pool;

int
main(void)
{
    int num_counters = sizeof(counters) / sizeof(counters[0]);

    printf("L4 Bench\n");
    for(int i = 0; i < num_counters; i++) {
        for(struct counter **counter = counters[i]; *counter != NULL; counter++) {
            printf("Init Counter... %p\n", (*counter));
            (*counter)->init(*counter);
        }
    }

    spaceid_pool = okl4_env_get("MAIN_SPACE_ID_POOL");
    clistid_pool = okl4_env_get("MAIN_CLIST_ID_POOL");
    mutexid_pool = okl4_env_get("MAIN_MUTEX_ID_POOL");
    if (!spaceid_pool || !mutexid_pool || !clistid_pool) {
        printf("Couldn't get all range id pools\n");
        exit(-1);
    }

    okl4_env_segment_t *seg;
    seg = okl4_env_get_segment("MAIN_KTEST_SEGMENT");
    assert(seg != NULL);

    kernel_test_segment_id = seg->segment;
    kernel_test_segment_size = seg->size;
    kernel_test_segment_vbase = seg->virt_addr;

    printf("<kbench>\n");
    for(struct bench_test **benchmark = benchmarks; *benchmark != NULL; benchmark++) {
        for(int i = 0; i < num_counters; i++) {
            run_test(counters[i], *benchmark);
        }
    }
    for(struct new_bench_test **benchmark = new_benchmarks; *benchmark != NULL; benchmark++)
    {
        run_new_test(*benchmark);
    }
    printf("</kbench>\n");
    printf("L4 Bench completed\n");

    L4_KDB_Enter("bench_end");
}

