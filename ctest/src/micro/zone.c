/*
 * Copyright (c) 2008 Open Kernel Labs, Inc. (Copyright Holder).
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

#include <ctest/ctest.h>

#include <stdio.h>

#include <okl4/types.h>
#include <okl4/memsec.h>
#include <okl4/zone.h>
#include <okl4/static_memsec.h>
#include <okl4/physmem_item.h>
#include <okl4/pd.h>
#include <okl4/kspace.h>
#include <okl4/bitmap.h>
#include <okl4/init.h>

/* FIXME: Needed for L4_Yield(), this should go away because libokl4
 * clients should not directly reference libl4 symbols. */
#include <l4/schedule.h>

#include "../helpers.h"

#define N_PD    10
#define K_ZONE  10

#define STACK_SIZE ((0x1000 / sizeof(okl4_word_t)))
#define MAX(a, b) (((b)>(a))?(b):(a))
#define MIN(a, b) (((b)<(a))?(b):(a))

/* Stacks used by the zone tests. */
ALIGNED(32) static okl4_word_t zone_thread_stack[2][STACK_SIZE];
#define ZONE_THREAD_STACK(n) ((okl4_word_t)&zone_thread_stack[n][STACK_SIZE])

/*
 * Create a virtual pool of the given size with the given alignment,
 * derived from the root virtual pool.
 */

static okl4_virtmem_pool_t *
create_derived_virt_pool(okl4_word_t size, okl4_word_t alignment)
{
    int error;
    okl4_word_t base;
    okl4_virtmem_pool_t *pool;

    /* Setup attributes. */
    okl4_virtmem_pool_attr_t attr;
    okl4_virtmem_pool_attr_init(&attr);
    okl4_virtmem_pool_attr_setparent(&attr, root_virtmem_pool);
    /* Allocate twice as much as we need to begin with. */
    okl4_virtmem_pool_attr_setsize(&attr, 2 * size);
    pool = malloc(OKL4_VIRTMEM_POOL_SIZE_ATTR(attr));
    assert(pool);

    /* Perform the create. */
    error = okl4_virtmem_pool_alloc_pool(pool, &attr);
    fail_if(error != OKL4_OK, "failed to allocate new pool");

    /* Calculate the closest power of 2 aligned address. */
    base = (pool->virt.base + (alignment - 1)) & ~(alignment - 1);

    /* Free the allocation. */
    okl4_virtmem_pool_destroy(pool);
    free(pool);

    /* Make a new allocation with the aligned base. */
    okl4_virtmem_pool_attr_setrange(&attr, base, size);
    pool = malloc(OKL4_VIRTMEM_POOL_SIZE_ATTR(attr));
    assert(pool);
    error = okl4_virtmem_pool_alloc_pool(pool, &attr);
    fail_if(error != OKL4_OK, "failed to allocate new pool");

    return pool;
}

/*
 * Delete a virtual pool created by 'create_derived_virt_pool'.
 */
static void
delete_derived_virt_pool(okl4_virtmem_pool_t *pool)
{
    okl4_virtmem_pool_destroy(pool);
    free(pool);
}

/*
 * Create a zone covering all base memsecs.
 */
static okl4_zone_t *
create_elf_segments_zone(void)
{
    okl4_word_t i;
    okl4_word_t max_vaddr;
    okl4_word_t min_vaddr;
    okl4_word_t zone_size;
    okl4_zone_t *zone;

    /* Find the min and max vaddrs of the segment memsections - also the size
     * of the memsection with maximum vaddr. */
    max_vaddr = 0;
    min_vaddr = (okl4_word_t)-1;
    for (i = 0; ctest_segments[i] != NULL; i++) {
        okl4_word_t base, size;
        okl4_range_item_t range;

        range = okl4_memsec_getrange(ctest_segments[i]);
        base = okl4_range_item_getbase(&range);
        size = okl4_range_item_getsize(&range);

        max_vaddr = MAX(max_vaddr, base + size);
        min_vaddr = MIN(min_vaddr, base);
    }

    /* Create a zone to cover all the above memsections. */
    zone_size = max_vaddr - min_vaddr;

#if defined(ARM_SHARED_DOMAINS)
    /* Round up size to the nearest multiple of window size. */
    zone_size = (zone_size
            + (OKL4_ZONE_ALIGNMENT - 1)) & ~(OKL4_ZONE_ALIGNMENT - 1);
    /* Round down min_vaddr so that it is aligned to window size. */
    min_vaddr = min_vaddr & ~(OKL4_ZONE_ALIGNMENT - 1);
#endif

    zone = create_zone(min_vaddr, zone_size);
    assert(zone);

    /* Attach all segment memsections to this zone. */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        int error = okl4_zone_memsec_attach(zone, ctest_segments[i]);
        fail_unless(error == OKL4_OK, "Could not attach a memsec to a zone");
    }

    return zone;
}

/*
 * Delete a zone created by 'create_elf_segments_zone'.
 */
static void
delete_elf_segments_zone(okl4_zone_t *zone)
{
    int i;

    /* Detach all segment memsections from the zone. */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        okl4_zone_memsec_detach(zone, ctest_segments[i]);
    }

    /* Free the memory. */
    delete_zone(zone);
}

/*
 * Test creating several memsections, attaching them to a zone, and then
 * attaching the zone to a pd.
 */
START_TEST(ZONE0100)
{
    int error;
    okl4_word_t i;
    ms_t *ms_array[3];
    okl4_zone_t *zone;
    okl4_pd_t *pd;
    okl4_word_t memsec_base, memsec_top, size;

    /* Create a pd. */
    pd = create_pd();

    memsec_base = ~0UL;
    memsec_top = 0;
    for (i = 0; i < 3; i++) {
        ms_array[i] = create_memsec(ANONYMOUS_BASE,
                (i + 1) * OKL4_DEFAULT_PAGESIZE, NULL);
        memsec_base = MIN(memsec_base, ms_array[i]->virt.base);
        memsec_top = MAX(memsec_top, ms_array[i]->virt.base +
                ms_array[i]->virt.size);
    }
    size = memsec_top - memsec_base;

#if defined(ARM_SHARED_DOMAINS)
    /* Round up size to the nearest multiple of window size. */
    size = (size + (OKL4_ZONE_ALIGNMENT - 1)) & ~(OKL4_ZONE_ALIGNMENT - 1);
    /* Round down memsec_base so that it is aligned to window size. */
    memsec_base = memsec_base & ~(OKL4_ZONE_ALIGNMENT - 1);
#endif

    /* Create a zone which can accomadate the above memsections. */
    zone = create_zone(memsec_base, size);

    /* Attach the memsections to the zone. */
    for (i = 0; i < 3; i++) {
        error = okl4_zone_memsec_attach(zone, ms_array[i]->memsec);
        fail_unless(error == OKL4_OK, "Could not attach a memsec to a zone");
    }

    /* Attach the zone to a pd. */
    attach_zone(pd, zone);

    /* Clean up. */
    okl4_pd_zone_detach(pd, zone);
    delete_pd(pd);
    for (i = 0; i < 3; i++) {
        okl4_zone_memsec_detach(zone, ms_array[i]->memsec);
        delete_memsec(ms_array[i], NULL);
    }
    delete_zone(zone);
}
END_TEST

static volatile okl4_virtmem_item_t zone0105_memsec_addr;
static volatile okl4_word_t zone0105_child_done;

static void
zone0105_thread_routine(void)
{
    okl4_word_t *p;

    okl4_init_thread();

    p = (okl4_word_t *)zone0105_memsec_addr.base;
    *p = 0xdeadbee;

    zone0105_child_done = 1;
    while (1) {
        L4_Yield();
    }
}

/*
 * Attach a memsection to a zone already attached to a PD.
 */
START_TEST(ZONE0105)
{
    int error;
    okl4_virtmem_pool_t *zone_pool;
    okl4_word_t i;
    ms_t *ms_array[4];
    okl4_zone_t *zone, *zone_code;
    okl4_pd_t *pd;
    okl4_kthread_t *thread;

    /* Create a zone containing text/data. */
    zone_code = create_elf_segments_zone();

    /* Create a pd. */
    pd = create_pd();

    /* Allocate a derived virtmem pool which will be covered by our zone. */
    zone_pool = create_derived_virt_pool(128 * 1024 * 1024,
            OKL4_ZONE_ALIGNMENT);

    /* Create some memsecs using the above pool */
    for (i = 0; i < 4; i++) {
        ms_array[i] = create_memsec(ANONYMOUS_BASE,
                (i + 1) * OKL4_DEFAULT_PAGESIZE, zone_pool);
    }

    /* Create a zone which can accomadate the above memsections. */
    zone = create_zone(zone_pool->virt.base, zone_pool->virt.size);

    /* Attach the memsections to the zone. */
    for (i = 0; i < 3; i++) {
        error = okl4_zone_memsec_attach(zone, ms_array[i]->memsec);
        fail_unless(error == OKL4_OK, "Could not attach a memsec to a zone");
    }

    /* Attach the zone to a pd. */
    attach_zone(pd, zone);
    attach_zone(pd, zone_code);

    /* Attach a memsection to the zone */
    error = okl4_zone_memsec_attach(zone, ms_array[3]->memsec);
    fail_unless(error == OKL4_OK, "Could not attach a memsec to a zone");

    zone0105_memsec_addr = ms_array[3]->virt;
    zone0105_child_done = 0;

    thread = run_thread_in_pd(pd, ZONE_THREAD_STACK(0),
            (okl4_word_t)zone0105_thread_routine);

    while (!zone0105_child_done) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread));
    }

    /* Clean up. */
    okl4_pd_zone_detach(pd, zone);
    okl4_pd_zone_detach(pd, zone_code);
    delete_pd(pd);
    for (i = 0; i < 4; i++) {
        okl4_zone_memsec_detach(zone, ms_array[i]->memsec);
        delete_memsec(ms_array[i], zone_pool);
    }
    delete_zone(zone);
    delete_elf_segments_zone(zone_code);
    delete_derived_virt_pool(zone_pool);
}
END_TEST

/*
 * Try to attach a zone of size 0 to a PD.
 */
START_TEST(ZONE0110)
{
    okl4_zone_t *zone;
    okl4_pd_t *pd;

    /* Create a pd. */
    pd = create_pd();

    /* Create a zone which can accomadate the above memsections. */
    zone = create_zone(0, 0);

    /* Attach the zone to a pd. 'attach_zone' checks for errors. */
    attach_zone(pd, zone);

    /* Clean up. */
    okl4_pd_zone_detach(pd, zone);
    delete_zone(zone);
    delete_pd(pd);
}
END_TEST

/* Try to attach an incompatible memsection to a zone. */
START_TEST(ZONE0120)
{
    ms_t *m[4];
    int error;
    int i;
    okl4_zone_t *zone;
    okl4_word_t chunksize = OKL4_ZONE_ALIGNMENT;

    /* Create a small zone. */
    zone = create_zone(2 * chunksize, 3 * chunksize);

    /* Create memsections that do not fit into the zone. */
    m[0] = create_memsec(chunksize, 2 * chunksize, NO_POOL);
    m[1] = create_memsec(1 * chunksize, 3 * chunksize, NO_POOL);
    m[2] = create_memsec(2 * chunksize, 4 * chunksize, NO_POOL);
    m[3] = create_memsec(3 * chunksize, 4 * chunksize, NO_POOL);

    /* Ensure that we can not attach any of the memsections. */
    for (i = 0; i < 4; i++) {
        error = okl4_zone_memsec_attach(zone, m[i]->memsec);
        fail_unless(error != OKL4_OK, "Incompatible memsection attached to the pd");
    }

    /* Clean up. */
    delete_zone(zone);
    for (i = 0; i < 4; i++) {
        delete_memsec(m[i], NO_POOL);
    }
}
END_TEST

/*
 * Test to ensure that two memsections with overlapping ranges can not be
 * attached to a zone
 */
START_TEST(ZONE0125)
{
    int error;
    okl4_zone_t *zone;
    ms_t *ms1, *ms2;
    okl4_word_t chunksize = OKL4_ZONE_ALIGNMENT;

    /* Create two over-lapping memsections */
    ms1 = create_memsec(1 * chunksize, 3 * chunksize, NO_POOL);
    ms2 = create_memsec(2 * chunksize, 4 * chunksize, NO_POOL);

    /* Create a zone using the entire memsec pool so that the memsections fall
     * under the zone. */
    zone = create_zone(1 * chunksize, 4 * chunksize);

    /* Attach the memsections to the zone. This should not succeed since the
     * memsections are over-lapping */
    error = okl4_zone_memsec_attach(zone, ms1->memsec);
    fail_unless(error == OKL4_OK, "Could not attach a memsec to a zone");

    error = okl4_zone_memsec_attach(zone, ms2->memsec);
    fail_unless(error != OKL4_OK, "Attached an over lapping memsection");

    /* Clean Up */
    okl4_zone_memsec_detach(zone, ms1->memsec);
    delete_memsec(ms1, NO_POOL);
    delete_memsec(ms2, NO_POOL);
    delete_zone(zone);
}
END_TEST

static volatile okl4_virtmem_item_t zone0140_shared_zone_addr;
static volatile okl4_word_t zone0140_child_a_done;
static volatile okl4_word_t zone0140_child_b_done;

/* Write data to the shared zone. */
static void
zone0140_thread0_routine(void)
{
    okl4_word_t i;
    okl4_word_t *p;

    okl4_init_thread();

    for (i = 0, p = (okl4_word_t *)zone0140_shared_zone_addr.base;
            i < zone0140_shared_zone_addr.size / sizeof(okl4_word_t); i++, p++) {
        *p = 0xdeadbee;
    }

    zone0140_child_a_done = 1;
    while (1) {
        L4_Yield();
    }
}

/* Read data from the shared zone. */
static void
zone0140_thread1_routine(void)
{
    okl4_word_t i;
    okl4_word_t *p;

    okl4_init_thread();

    for (i = 0, p = (okl4_word_t *)zone0140_shared_zone_addr.base;
            i < zone0140_shared_zone_addr.size / sizeof(okl4_word_t); i++, p++) {
        while (*p != 0xdeadbee) {
            L4_Yield();
        }
    }

    zone0140_child_b_done = 1;
    while (1) {
        L4_Yield();
    }
}

/*
 * Attach a zone to two pd's and test that they are able to share data. 
 */
START_TEST(ZONE0140)
{
    int error;
    okl4_kthread_t *thread0, *thread1;
    okl4_pd_t *pd0, *pd1;
    okl4_zone_t *zone_code, *zone_shared;
    ms_t *memsec_shared;
#if defined(ARM_SHARED_DOMAINS)
    okl4_word_t memsec_base;
#endif

    /* Create a zone containing our text/data. */
    zone_code = create_elf_segments_zone();

    /* Create another memsection and zone for sharing data. */
#if defined(ARM_SHARED_DOMAINS)
    /* FIXME: The following is a hack that should go away when we can
     * do aligned allocations. */
    memsec_shared = create_memsec(ANONYMOUS_BASE,
            OKL4_ZONE_ALIGNMENT * 2, NULL);
    memsec_base = memsec_shared->virt.base;
    /* Round up base to the nearest multiple of window size. */
    memsec_base = (memsec_base + (OKL4_ZONE_ALIGNMENT - 1)) & ~(OKL4_ZONE_ALIGNMENT - 1);

    /* Delete the memsec and re-allocate with known base and size. */
    delete_memsec(memsec_shared, NULL);
    memsec_shared = create_memsec(memsec_base, OKL4_ZONE_ALIGNMENT, NULL);
#else
    memsec_shared = create_memsec(ANONYMOUS_BASE, OKL4_DEFAULT_PAGESIZE, NULL);
#endif

    zone_shared = create_zone(memsec_shared->virt.base,
            memsec_shared->virt.size);
    error = okl4_zone_memsec_attach(zone_shared, memsec_shared->memsec);
    fail_unless(error == OKL4_OK, "Could not attach a memsec to shared zone");

    /* Create a pd and attach code zone to it. */
    pd0 = create_pd();
    attach_zone(pd0, zone_code);
    attach_zone(pd0, zone_shared);

    /* Create another pd and attach the code zone to it. */
    pd1 = create_pd();
    attach_zone(pd1, zone_code);
    attach_zone(pd1, zone_shared);

    /* Tell the threads the address and size of the shared zone. */
    zone0140_shared_zone_addr = memsec_shared->virt;
    zone0140_child_a_done = 0;
    zone0140_child_b_done = 0;

    /* Run a thread in each pd. */
    thread0 = run_thread_in_pd(pd0, ZONE_THREAD_STACK(0),
            (okl4_word_t)zone0140_thread0_routine);
    thread1 = run_thread_in_pd(pd1, ZONE_THREAD_STACK(1),
            (okl4_word_t)zone0140_thread1_routine);

    /* We spin until they have shared data via the zone. */
    while (!zone0140_child_a_done || !zone0140_child_b_done) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread0));
        L4_ThreadSwitch(okl4_kthread_getkcap(thread1));
    }

    /* Clean up. */
    okl4_pd_thread_delete(pd0, thread0);
    okl4_pd_thread_delete(pd1, thread1);
    okl4_pd_zone_detach(pd0, zone_shared);
    okl4_pd_zone_detach(pd1, zone_shared);
    okl4_pd_zone_detach(pd0, zone_code);
    okl4_pd_zone_detach(pd1, zone_code);
    okl4_zone_memsec_detach(zone_shared, memsec_shared->memsec);
    delete_memsec(memsec_shared, NULL);
    delete_elf_segments_zone(zone_code);
    delete_zone(zone_shared);
    delete_pd(pd0);
    delete_pd(pd1);
}
END_TEST

#define ZONE0150_STACK_SIZE ((0x100 / sizeof(okl4_word_t)))
ALIGNED(32) static okl4_word_t zone0150_stack[2][ZONE0150_STACK_SIZE];

static volatile int zone0150_child_thread_run;
static volatile int zone0150_children_can_run;

static okl4_kcap_t thread0_cap;
static okl4_kcap_t thread1_cap;

static void
zone0150_thread0_routine(void)
{
    L4_MsgTag_t result;

    L4_ThreadId_t thread1_tid;

    okl4_init_thread();

    while (zone0150_children_can_run == 0) {
        /* spin */
    }

    thread1_tid.raw = thread1_cap.raw;

    result = L4_Call(thread1_tid);
    fail_unless(L4_IpcSucceeded(result) != 0, "thread0 L4_Call() failed.");

    result = L4_Receive(thread1_tid);
    fail_unless(L4_IpcSucceeded(result) != 0, "thread0 L4_Receive() failed.");
    result = L4_Send(thread1_tid);
    fail_unless(L4_IpcSucceeded(result) != 0, "thread0 L4_Send() failed.");

    zone0150_child_thread_run++;

    L4_WaitForever();
}

static void
zone0150_thread1_routine(void)
{
    L4_MsgTag_t result;

    L4_ThreadId_t thread0_tid;

    okl4_init_thread();

    while (zone0150_children_can_run == 0) {
        /* spin */
    }

    thread0_tid.raw = thread0_cap.raw;

    result = L4_Receive(thread0_tid);
    fail_unless(L4_IpcSucceeded(result) != 0, "thread1 L4_Receive() failed.");
    result = L4_Send(thread0_tid);
    fail_unless(L4_IpcSucceeded(result) != 0, "thread1 L4_Send() failed.");

    result = L4_Call(thread0_tid);
    fail_unless(L4_IpcSucceeded(result) != 0, "thread1 L4_Call() failed.");

    zone0150_child_thread_run++;

    L4_WaitForever();
}

/*
 * Create two threads in in different PDs and check that they can IPC each
 * other. They will need to share the program zone.
 */
START_TEST(ZONE0150)
{
    int error;
    okl4_pd_t *pd0, *pd1;
    okl4_zone_t *program_zone;
    okl4_pd_zone_attach_attr_t zone_attr;
    okl4_kthread_t *thread0;
    okl4_kthread_t *thread1;
    okl4_word_t i;
    okl4_word_t max_vaddr;
    okl4_word_t min_vaddr;
    okl4_word_t zone_size;

    /* Find the min and max vaddrs of the segment memsections - also the size
     * of the memsection with maximum vaddr. */
    max_vaddr = 0;
    min_vaddr = ~0UL;
    for (i = 0; ctest_segments[i] != NULL; i++) {
        okl4_word_t base, size;
        okl4_range_item_t range;

        range = okl4_memsec_getrange(ctest_segments[i]);
        base = okl4_range_item_getbase(&range);
        size = okl4_range_item_getsize(&range);

        max_vaddr = MAX(max_vaddr, base + size);
        min_vaddr = MIN(min_vaddr, base);
    }

    /* Create a zone to cover all the above memsections. */
    zone_size = max_vaddr - min_vaddr;

    /* Create a zone to cover all the above memsections. */
#if defined(ARM_SHARED_DOMAINS)
    /* Round up size to the nearest multiple of window size. */
    zone_size = (zone_size + (OKL4_ZONE_ALIGNMENT - 1))
            & ~(OKL4_ZONE_ALIGNMENT - 1);
    /* Round down min_vaddr so that it is aligned to window size. */
    min_vaddr = min_vaddr & ~(OKL4_ZONE_ALIGNMENT - 1);
#endif
    program_zone = create_zone(min_vaddr, zone_size);

    /* Attach all code memsections to this zone. */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        error = okl4_zone_memsec_attach(program_zone, ctest_segments[i]);
        fail_unless(error == OKL4_OK, "Could not attach a memsec to a zone");
    }

    /* Create two pds and attach program zone to them. */
    pd0 = create_pd();
    pd1 = create_pd();
    okl4_pd_zone_attach_attr_init(&zone_attr);
    error = okl4_pd_zone_attach(pd0, program_zone, &zone_attr);
    fail_unless(error == OKL4_OK, "Could not attach program zone to pd0");
    error = okl4_pd_zone_attach(pd1, program_zone, &zone_attr);
    fail_unless(error == OKL4_OK, "Could not attach program zone to pd1");

    zone0150_child_thread_run = 0;
    zone0150_children_can_run = 0;

    /* Run a thread in each of the pds. */
    thread0 = run_thread_in_pd(pd0,
            (okl4_word_t)&zone0150_stack[0][ZONE0150_STACK_SIZE],
                            (okl4_word_t)zone0150_thread0_routine);
    thread1 = run_thread_in_pd(pd1,
            (okl4_word_t)&zone0150_stack[1][ZONE0150_STACK_SIZE],
                            (okl4_word_t)zone0150_thread1_routine);

    error = okl4_pd_createcap(pd0, thread1, &thread1_cap);
    fail_unless(error == OKL4_OK, "Could not give pd0 threads capability to thread1.");
    error = okl4_pd_createcap(pd1, thread0, &thread0_cap);
    fail_unless(error == OKL4_OK, "Could not give pd1 threads capability to thread0.");

    zone0150_children_can_run = 1;

    while (zone0150_child_thread_run < 2) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread0));
        L4_ThreadSwitch(okl4_kthread_getkcap(thread1));
    }

    /* Clean up time. */
    okl4_pd_deletecap(pd0, thread1_cap);
    okl4_pd_deletecap(pd1, thread0_cap);

    for (i = 0; ctest_segments[i] != NULL; i++) {
        okl4_zone_memsec_detach(program_zone, ctest_segments[i]);
    }

    okl4_pd_zone_detach(pd0, program_zone);
    okl4_pd_zone_detach(pd1, program_zone);

    okl4_zone_delete(program_zone);
    free(program_zone);

    okl4_pd_delete(pd0);
    okl4_pd_delete(pd1);
    free(pd0);
    free(pd1);
}
END_TEST


static volatile okl4_virtmem_item_t zone0400_shared_zone_addr;
static volatile okl4_word_t zone0400_child_thread_a_done;
static volatile okl4_word_t zone0400_child_thread_b_done;

static void
zone0400_thread0_routine(void)
{
    okl4_init_thread();

    *(volatile okl4_word_t *)zone0400_shared_zone_addr.base = 1;

    zone0400_child_thread_a_done = 1;
    L4_WaitForever();
}

static void
zone0400_thread1_routine(void)
{
    okl4_init_thread();

    *(volatile okl4_word_t *)zone0400_shared_zone_addr.base = 1;

    zone0400_child_thread_b_done = 1;
    L4_WaitForever();
}

/**
 * Two protection domains with a zone attached (containing a single memsec).
 * First Detach the zone from PD1, the thread in PD should fail to access the
 * memory. Re-attach the zone to PD1 , the memory access should work fine. Now
 * delete the memsection from the zone. Now neither pd can access the memory
 * any longer.
 */
START_TEST(ZONE0400)
{
    int error;
    okl4_kthread_t *thread0, *thread1;
    okl4_pd_t *pd0, *pd1;
    okl4_zone_t *zone_program, *zone_shared;
    ms_t *memsec_shared;
    L4_MsgTag_t tag;
    L4_ThreadId_t from;
    L4_Msg_t msg;
    okl4_word_t pagefault_addr;
#if defined(ARM_SHARED_DOMAINS)
    okl4_word_t memsec_base;
#endif

    /* Create a zone containing our text/data. */
    zone_program = create_elf_segments_zone();

    /* Create a pd and attach the zone containing the program segments to it. */
    pd0 = create_pd();
    attach_zone(pd0, zone_program);
    pd1 = create_pd();
    attach_zone(pd1, zone_program);

    /* Create a memsec to share between the two. */
#if defined(ARM_SHARED_DOMAINS)
    /* FIXME: The following is a hack that should go away when we can
     * do aligned allocations. */
    memsec_shared = create_memsec(ANONYMOUS_BASE, OKL4_ZONE_ALIGNMENT * 2, NULL);
    memsec_base = memsec_shared->virt.base;
    /* Round up base to the nearest multiple of window size. */
    memsec_base = (memsec_base + (OKL4_ZONE_ALIGNMENT - 1)) & ~(OKL4_ZONE_ALIGNMENT - 1);

    /* Delete the memsec and re-allocate with known base and size. */
    delete_memsec(memsec_shared, NULL);
    memsec_shared = create_memsec(memsec_base, OKL4_ZONE_ALIGNMENT, NULL);
#else
    memsec_shared = create_memsec(ANONYMOUS_BASE, OKL4_DEFAULT_PAGESIZE, NULL);
#endif
    zone_shared = create_zone(memsec_shared->virt.base, memsec_shared->virt.size);

    /* Attach the memsection to the zone and the zone to both pd's */
    error = okl4_zone_memsec_attach(zone_shared, memsec_shared->memsec);
    fail_unless(error == OKL4_OK, "Could not attach a memsec to shared zone");
    attach_zone(pd0, zone_shared);
    attach_zone(pd1, zone_shared);

    /* Tell the threads the address and size of the shared zone. */
    zone0400_shared_zone_addr = memsec_shared->virt;
    zone0400_child_thread_a_done = 0;
    zone0400_child_thread_b_done = 0;

    /* Run a thread in pd. */
    thread0 = run_thread_in_pd(pd0, ZONE_THREAD_STACK(0),
            (okl4_word_t)zone0400_thread0_routine);
    thread1 = run_thread_in_pd(pd1, ZONE_THREAD_STACK(1),
            (okl4_word_t)zone0400_thread1_routine);

    /* Wait for both to finish. */
    while (!zone0400_child_thread_a_done || !zone0400_child_thread_b_done) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread0));
        L4_ThreadSwitch(okl4_kthread_getkcap(thread1));
    }

    /* Detach the zone_shared from PD0 and restart the threads. The access
     * should fail for thread0 */
    okl4_pd_zone_detach(pd0, zone_shared);
    okl4_pd_thread_start(pd0, thread0);

     /* Wait for the child to IPC us */
    tag = L4_Wait(&from);
    L4_MsgStore(tag, &msg);
    fail_unless(L4_IpcSucceeded(tag) != 0, "Did not get pagefault IPC.");
    L4_MsgGetWord(&msg, 0, &pagefault_addr);
    fail_unless((okl4_word_t *)pagefault_addr ==
            (okl4_word_t *)zone0400_shared_zone_addr.base,
            "Thread0 faulted on the wrong address.");

    /* Attach the zone back to PD0 */
    attach_zone(pd0, zone_shared);
    fail_unless(error == OKL4_OK, "Could not attach shared zone to pd0");

    /* Clear the flag */
    zone0400_child_thread_a_done = 0;

    /* Start the thread again. */
    L4_MsgClear(&msg);
    L4_MsgLoad(&msg);
    tag = L4_Reply(from);
    fail_unless(L4_IpcSucceeded(tag) != 0, "IPC Failed.");

    /* Wait for the thread to run */
    while (!zone0400_child_thread_a_done) {
         L4_ThreadSwitch(okl4_kthread_getkcap(thread0));
    }

    /* Detach the memsection from the shared zone and restart both thread0 */
    okl4_zone_memsec_detach(zone_shared, memsec_shared->memsec);

    /* Re-start the thread */
    okl4_pd_thread_start(pd1, thread1);
    tag = L4_Wait(&from);
    L4_MsgStore(tag, &msg);
    fail_unless(L4_IpcSucceeded(tag) != 0, "Did not get pagefault IPC.");
    L4_MsgGetWord(&msg, 0, &pagefault_addr);
    fail_unless(pagefault_addr == zone0400_shared_zone_addr.base,
            "Thread0 faulted on the wrong address.");

    /* Clean up. */
    okl4_pd_thread_delete(pd0, thread0);
    okl4_pd_thread_delete(pd1, thread1);
    okl4_pd_zone_detach(pd0, zone_shared);
    okl4_pd_zone_detach(pd0, zone_program);
    okl4_pd_zone_detach(pd1, zone_shared);
    okl4_pd_zone_detach(pd1, zone_program);
    delete_zone(zone_shared);
    delete_elf_segments_zone(zone_program);
    delete_pd(pd0);
    delete_pd(pd1);
}
END_TEST

/* Test to ensure that two zones with overlapping ranges can not be attached
 * to a PD.
 */

START_TEST(ZONE0500)
{
    int error;
    okl4_virtmem_pool_t *zone_pool;
    okl4_zone_t *zone1, *zone2;
    okl4_pd_t *pd1, *pd2;
    okl4_pd_zone_attach_attr_t zone1_attr, zone2_attr;

#if defined(ARM_SHARED_DOMAINS)
    zone_pool = create_derived_virt_pool(3 * OKL4_ZONE_ALIGNMENT, OKL4_ZONE_ALIGNMENT);
    zone1 = create_zone(zone_pool->virt.base, 2 * OKL4_ZONE_ALIGNMENT);
    zone2 = create_zone(zone_pool->virt.base, 4 * OKL4_ZONE_ALIGNMENT);
#else
    zone_pool = create_derived_virt_pool(3 * OKL4_DEFAULT_PAGESIZE, 1);
    zone1 = create_zone(zone_pool->virt.base, 2 * OKL4_DEFAULT_PAGESIZE);
    zone2 = create_zone(zone_pool->virt.base, 4 * OKL4_DEFAULT_PAGESIZE);
#endif

    /* Create pd1 */
    pd1 = create_pd();

    /* Create pd2 */
    pd2 = create_pd();

    /* Attach these zones to pd1 */
    okl4_pd_zone_attach_attr_init(&zone1_attr);
    okl4_pd_zone_attach_attr_init(&zone2_attr);

    error = okl4_pd_zone_attach(pd1, zone1, &zone1_attr);
    fail_unless(error == OKL4_OK, "Could not attach a zone to a pd1");

    error = okl4_pd_zone_attach(pd1, zone2, &zone2_attr);
    fail_unless(error != OKL4_OK, "Over Lapping zones got attached to pd1");

    /* Attach zone2 to pd2 */
    error = okl4_pd_zone_attach(pd2, zone2, &zone2_attr);
    fail_unless(error == OKL4_OK, "Could not attach a zone2 to pd2");

    /* CleanUp */
    delete_derived_virt_pool(zone_pool);
    okl4_pd_zone_detach(pd1, zone1);
    okl4_pd_zone_detach(pd2, zone2);

    okl4_pd_delete(pd1);
    free(pd1);

    okl4_pd_delete(pd2);
    free(pd2);
}
END_TEST

volatile uintptr_t base;
volatile int read_write_done;

static void
zone0600_thread(void)
{
    int tmp;

    okl4_init_thread();

    tmp = *(volatile int *)base;
    read_write_done = 1;

    *(volatile int *)base = 1;
    read_write_done = 2;

    *(volatile int *)base = tmp;

    L4_WaitForever();
}

/**
 * Two protection domains. A single zone. The zone is attached read/write to
 * one protection domain, read-only to the other. Ensure permissions are
 * obeyed.
 */
START_TEST(ZONE0600)
{
    int error;
    ms_t *ms;

    okl4_kthread_t *thread1, *thread2;
    okl4_zone_t *zone, *zone_program;
    okl4_pd_t *pd1, *pd2;

    okl4_word_t pagefault_addr;
    L4_MsgTag_t tag;
    L4_ThreadId_t from;
    L4_Msg_t msg;

    /* Create PDs. */
    pd1 = create_pd();
    pd2 = create_pd();

    /* Create a zone to hold the program segments */
    zone_program = create_elf_segments_zone();

    /* Attach zone_program to PDs. */
    attach_zone(pd1, zone_program);
    attach_zone(pd2, zone_program);

    /* Create shared memsec/zone pair. */
    ms = create_memsec(ANONYMOUS_BASE, 1 * OKL4_ZONE_ALIGNMENT, NULL);
    base = ms->virt.base;
    zone = create_zone(ms->virt.base, ms->virt.size);
    error = okl4_zone_memsec_attach(zone, ms->memsec);
    fail_unless(error == OKL4_OK, "Could not attach a memsec to a zone");

    /* Attach the zone to PDs. */
    attach_zone_perms(pd1, zone, L4_Readable | L4_Writable);
    attach_zone_perms(pd2, zone, L4_Readable);

    /* Run a thread in PD1. */
    read_write_done = 0;
    thread1 = run_thread_in_pd(pd1, ZONE_THREAD_STACK(0),
            (okl4_word_t)zone0600_thread);

    /* Wait for thread to read and write. */
    while (read_write_done != 2) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread1));
    }
    fail_unless(read_write_done == 2, "Child did to get to where we expected.");

    /* Run a thread in PD2. */
    read_write_done = 0;
    thread2 = run_thread_in_pd(pd2, ZONE_THREAD_STACK(1),
            (okl4_word_t)zone0600_thread);

    /* Wait for thread to read. */
    while (read_write_done != 1) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread2));
    }

    /* Wait for child to IPC to us. */
    tag = L4_Wait(&from);
    L4_MsgStore(tag, &msg);
    fail_unless(L4_IpcSucceeded(tag) != 0, "Did not get pagefault IPC.");
    L4_MsgGetWord(&msg, 0, &pagefault_addr);
    fail_unless((char *)pagefault_addr == (char *)base,
            "Thread faulted on the wrong address.");
    fail_unless(read_write_done == 1, "Child did to get to where we expected.");

    /* Clean up. */
    okl4_pd_thread_delete(pd1, thread1);
    okl4_pd_thread_delete(pd2, thread2);

    okl4_pd_zone_detach(pd1, zone);
    okl4_pd_zone_detach(pd1, zone_program);
    delete_pd(pd1);

    okl4_pd_zone_detach(pd2, zone);
    okl4_pd_zone_detach(pd2, zone_program);
    delete_pd(pd2);

    delete_elf_segments_zone(zone_program);
    okl4_zone_memsec_detach(zone, ms->memsec);
    delete_memsec(ms, NULL);
    delete_zone(zone);
}
END_TEST


volatile okl4_word_t zone0700_base[K_ZONE];
volatile int myself;

static void
zone0700_write_thread(void)
{
    okl4_init_thread();

    *(volatile okl4_word_t *)zone0700_base[myself] = 0xdeadbeefU;
    read_write_done = 1;

    L4_WaitForever();
}

static void
zone0700_read_thread(void)
{
    okl4_word_t tmp;
    int i;

    okl4_init_thread();

    for(i=0; i<K_ZONE; i++) {
        tmp = *(volatile okl4_word_t *)zone0700_base[i];
        fail_unless(tmp == 0xdeadbeefU, "Unexpected value read!\n");
    }
    read_write_done = 1;

    L4_WaitForever();
}

START_TEST(ZONE0700)
{
    int i, j, error;
    okl4_pd_t *pd[N_PD];
    ms_t *ms[K_ZONE];
    okl4_zone_t *zone[K_ZONE];
    okl4_zone_t *zone_program;
    okl4_kthread_t *thread;

    /* Create a zone to hold the program segments, and attach it to our pds. */
    zone_program = create_elf_segments_zone();

    /* Create PDs. */
    for(i = 0; i < N_PD; i++) {
        pd[i] = create_pd();
        attach_zone(pd[i], zone_program);
    }

    /* Create memsecs using default pool. */
    for (i = 0; i < K_ZONE; i++) {
        ms[i] = create_memsec(ANONYMOUS_BASE, OKL4_ZONE_ALIGNMENT, NULL);
        zone[i] = create_zone(ms[i]->virt.base, ms[i]->virt.size);
        error = okl4_zone_memsec_attach(zone[i], ms[i]->memsec);
        fail_unless(error == OKL4_OK, "Could not attach a memsec to a zone");
    }

    /* Attach the zone to PDs. */
    for (i = 0; i < N_PD; i++) {
        for (j = 0; j < K_ZONE; j++) {
            attach_zone(pd[i], zone[j]);
        }
    }

    /* Run a thread in PDs to write to a memsection */
    for (i = 0; i < N_PD; i++) {
        myself = i;
        read_write_done = 0;
        zone0700_base[i] = ms[0]->virt.base;
        thread = run_thread_in_pd(pd[i], ZONE_THREAD_STACK(0),
                      (okl4_word_t)zone0700_write_thread);

        while (read_write_done != 1) {
                L4_ThreadSwitch(okl4_kthread_getkcap(thread));
        }

        okl4_pd_thread_delete(pd[i], thread);
    }

    /* Run threads to check that the memsections have been written */
    for (i = 0; i < N_PD; i++) {
        read_write_done = 0;

        thread = run_thread_in_pd(pd[i], ZONE_THREAD_STACK(0),
                (okl4_word_t)zone0700_read_thread);

        while (read_write_done != 1) {
            L4_ThreadSwitch(okl4_kthread_getkcap(thread));
        }
        okl4_pd_thread_delete(pd[i], thread);
    }

    /* Clean up. */
    for (i = 0; i < N_PD; i++) {
        for (j = 0; j < K_ZONE; j++) {
            okl4_pd_zone_detach(pd[i], zone[j]);
        }
        okl4_pd_zone_detach(pd[i], zone_program);
        delete_pd(pd[i]);
    }
    for (i = 0; i < K_ZONE; i++) {
        okl4_zone_memsec_detach(zone[i], ms[i]->memsec);
        delete_memsec(ms[i], NULL);
        delete_zone(zone[i]);
    }
    delete_elf_segments_zone(zone_program);
}
END_TEST

/**
 * Create a PD with a single zone attached to it.
 * Attach a demand paged memsection to the zone. Test the
 * paging on demand behaviour.
 *
 * Map the faulting page into PD0
 * and test that the thread running in PD0 can access the
 * mapped address.
 */

static volatile int zone0800_child_thread_run;
static okl4_word_t zone0800_thread_faulting_address;

static void
zone0800_thread_routine(void)
{
    okl4_init_thread();

    zone0800_child_thread_run = 1;

    /* Write to magic address to trigger a pagefault. */
    *((volatile okl4_word_t *)zone0800_thread_faulting_address) = 1;

    /* Let parent know we survived the ordeal. */
    zone0800_child_thread_run = 2;
    L4_WaitForever();
}

/* Test running thread. */
START_TEST(ZONE0800)
{
    int error;
    okl4_pd_t *pd;
    okl4_zone_t *zone, *zone_program;
    okl4_kthread_t *thread;
    L4_MsgTag_t tag;
    L4_ThreadId_t from;
    L4_Msg_t msg;
    okl4_word_t pagefault_addr;
    okl4_memsec_t *paged_memsec;
    okl4_range_item_t *virt;

    /* Create a PD */
    pd = create_pd();

    /* Create a zone to hold the program segments, and attach it to our pds. */
    zone_program = create_elf_segments_zone();
    attach_zone(pd, zone_program);

    /* Allocate a virtual address range for the tests to take place in. */
    virt = get_virtual_address_range(2 * OKL4_ZONE_ALIGNMENT);

    /* Create the zone using the above zone pool */
    zone = create_zone(virt->base, virt->size);

    /* Attach a demand-paged memsec */
    paged_memsec = create_paged_memsec(*virt);
    error = okl4_zone_memsec_attach(zone, paged_memsec);
    fail_unless(error == 0, "Attach failed.");

    /* Attach a zone to the PD. */
    attach_zone(pd, zone);

    /* Clear the flag we use to determine if the child has run. */
    zone0800_child_thread_run = 0;
    zone0800_thread_faulting_address = virt->base;

    /* Create a thread within the PD */
    thread = run_thread_in_pd(pd, ZONE_THREAD_STACK(0),
            (okl4_word_t)zone0800_thread_routine);

    /* Wait for child to IPC to us. */
    tag = L4_Wait(&from);
    L4_MsgStore(tag, &msg);
    fail_unless(L4_IpcSucceeded(tag) != 0, "Did not get pagefault IPC.");
    L4_MsgGetWord(&msg, 0, &pagefault_addr);
    fail_unless(pagefault_addr == zone0800_thread_faulting_address,
            "Thread faulted on the wrong address.");
    fail_unless(zone0800_child_thread_run == 1, "Child did not run.");

    /* Map in the page. */
    error = okl4_pd_handle_pagefault(pd, L4_SenderSpace(), pagefault_addr, 0);
    fail_unless(error == 0, "Mapping failed.");

    /* Start the thread again. */
    L4_MsgClear(&msg);
    L4_MsgLoad(&msg);
    tag = L4_Reply(from);
    fail_unless(L4_IpcSucceeded(tag) != 0, "IPC Failed.");

    /* Wait for the thread to run. */
    while (zone0800_child_thread_run != 1) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread));
    }

    /* Delete the thread. */
    okl4_pd_thread_delete(pd, thread);

    /* Delete the PD. */
    okl4_pd_zone_detach(pd, zone);
    okl4_pd_zone_detach(pd, zone_program);
    delete_pd(pd);

    /* Detach all the memsections. */
    okl4_zone_memsec_detach(zone, paged_memsec);
    delete_elf_segments_zone(zone_program);
    free_virtual_address_range(virt);
}
END_TEST

TCase *
make_zone_tcase(void)
{
    TCase * tc;

    tc = tcase_create("Zones");
    tcase_add_test(tc, ZONE0100);
    tcase_add_test(tc, ZONE0105);
    tcase_add_test(tc, ZONE0110);
    tcase_add_test(tc, ZONE0120);
    tcase_add_test(tc, ZONE0125);
    tcase_add_test(tc, ZONE0140);
    tcase_add_test(tc, ZONE0150);
    tcase_add_test(tc, ZONE0400);
    tcase_add_test(tc, ZONE0500);
#if defined(ARM_SHARED_DOMAINS)
    /* ARM shared domains does not support attaching one zone with
     * different permissions. */
    (void)ZONE0600;
#else
    tcase_add_test(tc, ZONE0600);
#endif
    tcase_add_test(tc, ZONE0700);
    tcase_add_test(tc, ZONE0800);

    return tc;
}
