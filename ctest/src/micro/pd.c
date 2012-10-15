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

#include <atomic_ops/atomic_ops.h>

#include <okl4/types.h>
#include <okl4/memsec.h>
#include <okl4/zone.h>
#include <okl4/static_memsec.h>
#include <okl4/physmem_item.h>
#include <okl4/physmem_segpool.h>
#include <okl4/pd.h>
#include <okl4/pd_dict.h>
#include <okl4/kspace.h>
#include <okl4/bitmap.h>
#include <okl4/kclist.h>
#include <okl4/init.h>

#include "../helpers.h"

#define THREAD_FAULT_ADDRESS   ((char *)17)

/*
 * Create a custom memsec, and ensure callbacks are correctly used.
 */
START_TEST(PD0100)
{
    ms_t *m1, *m2, *m3;
    okl4_pd_attr_t attr;
    okl4_pd_t *pd;
    int ret;

    /* Create a kspace. */
    ks_t *ks = create_kspace();

    /* Create a PD. */
    okl4_pd_attr_init(&attr);
    okl4_pd_attr_setkspace(&attr, ks->kspace);
    pd = malloc(OKL4_PD_SIZE_ATTR(&attr));
    assert(pd);
    ret = okl4_pd_create(pd, &attr);
    fail_unless(ret == OKL4_OK, "PD Create failed.");

    /* Create three memsecs. */
    m1 = create_memsec(ANONYMOUS_BASE, 1 * OKL4_DEFAULT_PAGESIZE, NULL);
    m2 = create_memsec(ANONYMOUS_BASE, 2 * OKL4_DEFAULT_PAGESIZE, NULL);
    m3 = create_memsec(ANONYMOUS_BASE, 3 * OKL4_DEFAULT_PAGESIZE, NULL);

    /* Attach the three memsecs to the PD. */
    ret  = okl4_pd_memsec_attach(pd, m1->memsec);
    ret |= okl4_pd_memsec_attach(pd, m2->memsec);
    ret |= okl4_pd_memsec_attach(pd, m3->memsec);
    fail_unless(ret == 0, "Could not attach memsections.");

    /* Detach one of the memsections. */
    okl4_pd_memsec_detach(pd, m1->memsec);
    okl4_pd_memsec_detach(pd, m2->memsec);
    okl4_pd_memsec_detach(pd, m3->memsec);

    /* Destroy the PD. */
    okl4_pd_delete(pd);

    /* Clean up. */
    delete_memsec(m1, NULL);
    delete_memsec(m2, NULL);
    delete_memsec(m3, NULL);
    delete_kspace(ks);
    free(pd);
}
END_TEST

#define PD0200_SPACES 5

/*
 * Test PD maps.
 */
START_TEST(PD0200)
{
    ks_t *ks[PD0200_SPACES];
    okl4_pd_t *pds[PD0200_SPACES];
    okl4_pd_dict_attr_t map_attr;
    okl4_pd_dict_t map;
    int i;
    int res;
    okl4_pd_t *dummy;

    /* Create kspaces and PDs. */
    for (i = 0; i < PD0200_SPACES; i++) {
        okl4_pd_attr_t attr;

        ks[i] = create_kspace();

        okl4_pd_attr_init(&attr);
        okl4_pd_attr_setkspace(&attr, ks[i]->kspace);
        pds[i] = malloc(OKL4_PD_SIZE_ATTR(&attr));
        assert(pds[i]);
        res = okl4_pd_create(pds[i], &attr);
        fail_unless(res == OKL4_OK, "Failed to create PD.");
    }

    /* Create a map. */
    okl4_pd_dict_attr_init(&map_attr);
    okl4_pd_dict_init(&map, &map_attr);

    /* Add the spaces. */
    for (i = 0; i < PD0200_SPACES; i++) {
        res = okl4_pd_dict_add(&map, pds[i]);
        fail_unless(res == 0, "Could not add PD to map.");
    }

    /* Search for each of the spaces. */
    for (i = 0; i < PD0200_SPACES; i++) {
        okl4_pd_t *result;
        res = okl4_pd_dict_lookup(&map, ks[i]->kspace_id, &result);
        fail_unless(res == 0, "Could not not find PD.");
        fail_unless(result == pds[i], "Found incorrect PD.");
    }

    /* Search for invalid spaces. */
    res = okl4_pd_dict_remove(&map, pds[0]);
    fail_unless(res == 0, "Could not remove PD.");
    res = okl4_pd_dict_lookup(&map, ks[0]->kspace_id, &dummy);
    fail_unless(res == OKL4_OUT_OF_RANGE, "Found a PD when not such PD exists.");

    /* Release the other PDs. */
    for (i = 1; i < PD0200_SPACES; i++) {
        res = okl4_pd_dict_remove(&map, pds[i]);
        fail_unless(res == 0, "Could not remove PD.");
    }

    /* Done. */
    for (i = 0; i < PD0200_SPACES; i++) {
        delete_kspace(ks[i]);
        free(pds[i]);
    }
}
END_TEST

#define TEST0300_NUM_PDS     5
#define TEST0300_ITERATIONS  5

START_TEST(PD0300)
{
    int error;
    int i;

    okl4_pd_attr_t pd_attr;
    okl4_pd_t *pd[TEST0300_NUM_PDS];

    /* Setup the PD object with our root object pools. */
    okl4_pd_attr_init(&pd_attr);
    okl4_pd_attr_setkspaceidpool(&pd_attr, root_kspace_pool);
    okl4_pd_attr_setkclistidpool(&pd_attr, root_kclist_pool);
    okl4_pd_attr_setutcbmempool(&pd_attr, root_virtmem_pool);
    okl4_pd_attr_setkclistsize(&pd_attr, 42);
    okl4_pd_attr_setmaxthreads(&pd_attr, 12);

    /* Allocate memory for the PDs. */
    for (i = 0; i < TEST0300_NUM_PDS; i++) {
        pd[i] = malloc(OKL4_PD_SIZE_ATTR(&pd_attr));
        assert(pd[i]);
    }

    /* Create the PD objects. */
    for (i = 0; i < TEST0300_ITERATIONS; i++) {
        int j;
        for (j = 0; j < TEST0300_NUM_PDS; j++) {
            error = okl4_pd_create(pd[j], &pd_attr);
            fail_unless(error == OKL4_OK, "Could not create PD");
        }
        for (j = 0; j < TEST0300_NUM_PDS; j++) {
            okl4_pd_delete(pd[j]);
        }
    }

    /* Free memory used by the PDs. */
    for (i = 0; i < TEST0300_NUM_PDS; i++) {
        free(pd[i]);
    }
}
END_TEST

/* Test pd thread creation. */
START_TEST(PD0400)
{
    int error;
    okl4_pd_t *pd;
    okl4_kthread_t *thread;
    okl4_pd_thread_create_attr_t thread_attr;

    pd = create_pd();

    /* Create a thread within the PD */
    okl4_pd_thread_create_attr_init(&thread_attr);
    okl4_pd_thread_create_attr_setspip(&thread_attr, 0, 0);
    okl4_pd_thread_create_attr_setpriority(&thread_attr, 101);
    error = okl4_pd_thread_create(pd, &thread_attr, &thread);
    fail_unless(error == OKL4_OK, "Could not create thread");

    /* Delete thread. */
    okl4_pd_thread_delete(pd, thread);

    /* Delete pd */
    okl4_pd_delete(pd);
    free(pd);
}
END_TEST

/* Create maximum number of threads, then try to create one more. */
START_TEST(PD0410)
{
    int error;
    okl4_word_t i;
    okl4_pd_t *pd;
    okl4_kthread_t *threads[MAX_THREADS + 1];
    okl4_pd_thread_create_attr_t thread_attr;

    pd = create_pd();

    okl4_pd_thread_create_attr_init(&thread_attr);
    okl4_pd_thread_create_attr_setspip(&thread_attr, 0, 0);
    okl4_pd_thread_create_attr_setpriority(&thread_attr, 101);

    for (i = 0; i < MAX_THREADS; i++) {
        error = okl4_pd_thread_create(pd, &thread_attr, &threads[i]);
        fail_unless(error == OKL4_OK, "Could not create thread");
    }

    error = okl4_pd_thread_create(pd, &thread_attr, &threads[i]);
    fail_unless(error != OKL4_OK, "Made too many threads");

    for (i = 0; i < MAX_THREADS; i++) {
        okl4_pd_thread_delete(pd, threads[i]);
    }

    okl4_pd_delete(pd);
    free(pd);
}
END_TEST

#define PD0420_STACK_SIZE ((0x1000 / sizeof(okl4_word_t)))
ALIGNED(32) static okl4_word_t pd0420_stack[MAX_THREADS][PD0420_STACK_SIZE];
static okl4_atomic_word_t pd0420_child_thread_run;

static void
pd0420_thread_routine(void)
{
    okl4_init_thread();

    okl4_atomic_inc(&pd0420_child_thread_run);
    L4_WaitForever();
}

/*
 * Create a PD whose kclist and parent kclist are the same, then run some
 * threads.
 */
START_TEST(PD0420)
{
    int error;
    okl4_word_t i;
    okl4_pd_t *pd;
    okl4_pd_attr_t pd_attr;
    okl4_kthread_t *kthreads[MAX_THREADS];

    /* Create a PD. */
    okl4_pd_attr_init(&pd_attr);
    okl4_pd_attr_setkspaceidpool(&pd_attr, root_kspace_pool);
    okl4_pd_attr_setkclist(&pd_attr, root_kclist);
    okl4_pd_attr_setutcbmempool(&pd_attr, root_virtmem_pool);
    okl4_pd_attr_setrootkclist(&pd_attr, root_kclist);
    okl4_pd_attr_setmaxthreads(&pd_attr, MAX_THREADS);
    okl4_pd_attr_setprivileged(&pd_attr, 1);

    pd = malloc(OKL4_PD_SIZE_ATTR(&pd_attr));
    assert(pd);

    error = okl4_pd_create(pd, &pd_attr);
    fail_unless(error == OKL4_OK, "Could not create PD");

    /* Attach program memsecs to the PD. */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        error  = okl4_pd_memsec_attach(pd, ctest_segments[i]);
        fail_unless(error == 0, "Could not attach memsections.");
    }

    okl4_atomic_set(&pd0420_child_thread_run, 0);

    /* Run some threads in the pd. */
    for (i = 0; i < MAX_THREADS; i++) {
        kthreads[i] = run_thread_in_pd(pd,
                             (okl4_word_t)&pd0420_stack[i][PD0420_STACK_SIZE],
                            (okl4_word_t)pd0420_thread_routine);
        assert(kthreads[i]);
    }

    while (okl4_atomic_read(&pd0420_child_thread_run) < MAX_THREADS) {
        for (i = 0; i < MAX_THREADS; i++) {
            L4_ThreadSwitch(okl4_kthread_getkcap(kthreads[i]));
        }
    }


    /* Clean up time. */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        okl4_pd_memsec_detach(pd, ctest_segments[i]);
    }

    okl4_pd_delete(pd);
    free(pd);
}
END_TEST

#define PD0430_STACK_SIZE ((0x100 / sizeof(okl4_word_t)))
ALIGNED(32) static okl4_word_t pd0430_stack[2][PD0430_STACK_SIZE];

static volatile int pd0430_child_thread_run;
static volatile int pd0430_children_can_run;

static okl4_kcap_t thread0_cap;
static okl4_kcap_t thread1_cap;

static void
pd0430_thread0_routine(void)
{
    L4_MsgTag_t result;
    L4_ThreadId_t thread1_tid;

    okl4_init_thread();

    while (pd0430_children_can_run == 0) {
        /* spin */
    }

    thread1_tid.raw = thread1_cap.raw;

    result = L4_Call(thread1_tid);
    fail_unless(L4_IpcSucceeded(result) != 0, "thread0 L4_Call() failed.");

    result = L4_Receive(thread1_tid);
    fail_unless(L4_IpcSucceeded(result) != 0, "thread0 L4_Receive() failed.");
    result = L4_Send(thread1_tid);
    fail_unless(L4_IpcSucceeded(result) != 0, "thread0 L4_Send() failed.");

    pd0430_child_thread_run++;

    L4_WaitForever();
}

static void
pd0430_thread1_routine(void)
{
    L4_MsgTag_t result;

    L4_ThreadId_t thread0_tid;

    okl4_init_thread();

    while (pd0430_children_can_run == 0) {
        /* spin */
    }

    thread0_tid.raw = thread0_cap.raw;

    result = L4_Receive(thread0_tid);
    fail_unless(L4_IpcSucceeded(result) != 0, "thread1 L4_Receive() failed.");
    result = L4_Send(thread0_tid);
    fail_unless(L4_IpcSucceeded(result) != 0, "thread1 L4_Send() failed.");

    result = L4_Call(thread0_tid);
    fail_unless(L4_IpcSucceeded(result) != 0, "thread1 L4_Call() failed.");

    pd0430_child_thread_run++;

    L4_WaitForever();
}

/*
 * Create two threads within the same PD and make sure that they cap IPC each
 * other.
 */
START_TEST(PD0430)
{
    int i, error;
    okl4_pd_t *pd;
    okl4_kthread_t *thread0;
    okl4_kthread_t *thread1;

    pd = create_pd();

    /* Attach program memsecs to the PD. */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        error  = okl4_pd_memsec_attach(pd, ctest_segments[i]);
        fail_unless(error == OKL4_OK, "Could not attach memsections.");
    }

    pd0430_child_thread_run = 0;
    pd0430_children_can_run = 0;

    /* Run some threads in the pd and make sure that they can ipc each other. */
    thread0 = run_thread_in_pd(pd,
            (okl4_word_t)&pd0430_stack[0][PD0430_STACK_SIZE],
                            (okl4_word_t)pd0430_thread0_routine);
    thread1 = run_thread_in_pd(pd,
            (okl4_word_t)&pd0430_stack[1][PD0430_STACK_SIZE],
                            (okl4_word_t)pd0430_thread1_routine);

    error = okl4_pd_createcap(pd, thread0, &thread0_cap);
    fail_unless(error == OKL4_OK, "Could not give pd threads capability to IPC thread0.");
    error = okl4_pd_createcap(pd, thread1, &thread1_cap);
    fail_unless(error == OKL4_OK, "Could not give pd threads capability to IPC thread1.");

    pd0430_children_can_run = 1;

    while (pd0430_child_thread_run < 2) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread0));
        L4_ThreadSwitch(okl4_kthread_getkcap(thread1));
    }

    /* Clean up time. */
    okl4_pd_deletecap(pd, thread1_cap);
    okl4_pd_deletecap(pd, thread0_cap);

    for (i = 0; ctest_segments[i] != NULL; i++) {
        okl4_pd_memsec_detach(pd, ctest_segments[i]);
    }

    okl4_pd_delete(pd);
    free(pd);
}
END_TEST

static volatile int pd0500_child_thread_run;

static void
pd0500_thread_routine(void)
{
    okl4_init_thread();

    pd0500_child_thread_run = 1;

    /* Write to magic address to trigger a pagefault. */
    *((volatile char *)THREAD_FAULT_ADDRESS) = 1;

    /* Let parent know we survived the ordeal. */
    pd0500_child_thread_run = 2;

    L4_WaitForever();
}

static int
pd0500_lookup(okl4_memsec_t *memsec, okl4_word_t vaddr,
        okl4_physmem_item_t *map_item, okl4_word_t *dest_vaddr)
{
    /* Don't perform any eager mapping. */
    return 1;
}

static int
pd0500_map(okl4_memsec_t *memsec, okl4_word_t vaddr,
        okl4_physmem_item_t *map_item, okl4_word_t *dest_vaddr)
{
    int error;
    okl4_physmem_item_t *phys;

    /* Allocate a page. */
    phys = malloc(sizeof(okl4_physmem_item_t));
    assert(phys);
    okl4_physmem_item_setsize(phys, OKL4_DEFAULT_PAGESIZE);
    error = okl4_physmem_segpool_alloc(root_physseg_pool, phys);
    assert(!error);

    /* Give that back to the user. */
    *dest_vaddr = vaddr & ~(OKL4_DEFAULT_PAGESIZE - 1);
    *map_item = *phys;

    return 0;
}

static void
pd0500_destroy(okl4_memsec_t *memsec)
{
}

static okl4_memsec_t *
pd0500_create_paged_memsec(void)
{
    okl4_memsec_attr_t attr;
    okl4_virtmem_item_t range;
    okl4_memsec_t *memsec;

    /* Setup the memsec attributes. */
    okl4_range_item_setrange(&range, 0, OKL4_DEFAULT_PAGESIZE);
    okl4_memsec_attr_init(&attr);
    okl4_memsec_attr_setrange(&attr, range);
    okl4_memsec_attr_setpremapcallback(&attr, pd0500_lookup);
    okl4_memsec_attr_setaccesscallback(&attr, pd0500_map);
    okl4_memsec_attr_setdestroycallback(&attr, pd0500_destroy);

    /* Create the memsec. */
    memsec = malloc(OKL4_MEMSEC_SIZE_ATTR(&attr));
    assert(memsec);
    okl4_memsec_init(memsec, &attr);

    return memsec;
}

#define STACK_SIZE ((0x1000 / sizeof(okl4_word_t)))
ALIGNED(32) static okl4_word_t pd0500_stack[STACK_SIZE];

/* Test running thread. */
START_TEST(PD0500)
{
    int error, i;
    okl4_pd_t *pd;
    okl4_kthread_t *thread;
    okl4_pd_thread_create_attr_t thread_attr;
    L4_MsgTag_t tag;
    L4_ThreadId_t from;
    L4_Msg_t msg;
    okl4_word_t pagefault_addr;
    okl4_memsec_t *paged_memsec;

    /* Make pd. */
    pd = create_pd();

    /* Attach memsecs to the PD. These contain program segments. */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        error  = okl4_pd_memsec_attach(pd, ctest_segments[i]);
        fail_unless(error == 0, "Could not attach memsections.");
    }

    /* Attach a demand-paged memsec, attached to 0x00000000 -> 0x00001000. */
    paged_memsec = pd0500_create_paged_memsec();
    error = okl4_pd_memsec_attach(pd, paged_memsec);
    fail_unless(error == 0, "Could not attach demand paged memsection.");

    /* Clear the flag we use to determine if the child has run. */
    pd0500_child_thread_run = 0;

    /* Create a thread within the PD */
    okl4_pd_thread_create_attr_init(&thread_attr);
    okl4_pd_thread_create_attr_setspip(&thread_attr,
            (okl4_word_t)&pd0500_stack[STACK_SIZE],
            (okl4_word_t)pd0500_thread_routine);
    okl4_pd_thread_create_attr_setpriority(&thread_attr, 101);
    okl4_pd_thread_create_attr_setpager(&thread_attr, okl4_kthread_getkcap(root_thread));
    error = okl4_pd_thread_create(pd, &thread_attr, &thread);
    fail_unless(error == OKL4_OK, "Could not create thread");
    okl4_pd_thread_start(pd, thread);

    /* Wait for child to IPC to us. */
    tag = L4_Wait(&from);
    L4_MsgStore(tag, &msg);
    fail_unless(L4_IpcSucceeded(tag) != 0, "Did not get pagefault IPC.");
    L4_MsgGetWord(&msg, 0, &pagefault_addr);
    fail_unless((char *)pagefault_addr == THREAD_FAULT_ADDRESS,
            "Thread faulted on the wrong address.");
    fail_unless(pd0500_child_thread_run == 1, "Child did not run.");

    /* Map in the page. */
    error = okl4_pd_handle_pagefault(pd, L4_SenderSpace(), pagefault_addr, 0);
    fail_unless(error == 0, "Mapping failed.");

    /* Start the thread again. */
    L4_MsgClear(&msg);
    L4_MsgLoad(&msg);
    tag = L4_Reply(from);
    fail_unless(L4_IpcSucceeded(tag) != 0, "IPC Failed.");

    /* Wait for the thread to run. */
    while (pd0500_child_thread_run != 2) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread));
    }

    /* Delete the thread. */
    okl4_pd_thread_delete(pd, thread);

    /* Detach memsections. */
    okl4_pd_memsec_detach(pd, paged_memsec);
    for (i = 0; ctest_segments[i] != NULL; i++) {
        okl4_pd_memsec_detach(pd, ctest_segments[i]);
    }

    /* Clean up. */
    okl4_pd_delete(pd);
    free(pd);

}
END_TEST

/* Test use of root PD. */
START_TEST(PD1000)
{
    int error;
    ms_t *ms;
    okl4_range_item_t allocated_item;

    /* Create a memsec to attach to ourself. */
    ms = create_memsec(ANONYMOUS_BASE, OKL4_DEFAULT_PAGESIZE, root_virtmem_pool);
    assert(ms);

    /* Attach it. */
    error = okl4_pd_memsec_attach(root_pd, ms->memsec);
    fail_unless(error == 0, "Could not attach memsec.");

    /* Try touching the memory. */
    allocated_item = okl4_memsec_getrange(ms->memsec);
    *((volatile char *)okl4_range_item_getbase(&allocated_item)) = 42;

    /* Detach. */
    okl4_pd_memsec_detach(root_pd, ms->memsec);

    /* Clean up. */
    delete_memsec(ms, root_virtmem_pool);
}
END_TEST

ALIGNED(32) static okl4_word_t pd1100_stack[STACK_SIZE];
static okl4_atomic_word_t pd1100_thread_started;

static void
pd1100_test_thread(void)
{
    okl4_init_thread();

    okl4_atomic_add(&pd1100_thread_started, 1);
    L4_WaitForever();
}

/* Attempt to create a new thread in a root PD. */
START_TEST(PD1100)
{
    int error;
    okl4_pd_thread_create_attr_t thread_attr;
    okl4_kthread_t *thread;

    okl4_atomic_init(&pd1100_thread_started, 0);

    /* Create a new thread. */
    okl4_pd_thread_create_attr_init(&thread_attr);
    okl4_pd_thread_create_attr_setspip(&thread_attr,
            (okl4_word_t)&pd1100_stack[STACK_SIZE],
            (okl4_word_t)pd1100_test_thread);
    error = okl4_pd_thread_create(root_pd, &thread_attr, &thread);
    fail_unless(error == 0, "Could not create child thread.");
    okl4_pd_thread_start(root_pd, thread);

    /* Wait for the new thread to execute. */
    while (!okl4_atomic_read(&pd1100_thread_started)) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread));
    }

    /* Delete the thread. */
    okl4_pd_thread_delete(root_pd, thread);
}
END_TEST

/* Attempt to create as many threads as possible in a root PD. */
START_TEST(PD1101)
{
    int error;
    okl4_pd_thread_create_attr_t thread_attr;
    okl4_kthread_t **thread;
    okl4_word_t cur_size = MAX_THREADS;
    okl4_word_t j, i = 0;

    okl4_atomic_init(&pd1100_thread_started, 0);

    thread = malloc(sizeof(okl4_kthread_t) * cur_size);
    assert(thread != NULL);

    /* Create a new thread. */
    okl4_pd_thread_create_attr_init(&thread_attr);
    okl4_pd_thread_create_attr_setspip(&thread_attr,
            (okl4_word_t)&pd1100_stack[STACK_SIZE],
            (okl4_word_t)pd1100_test_thread);
    okl4_pd_thread_create_attr_setpriority(&thread_attr, 255);
    do {
        if (i >= cur_size) {
            cur_size += MAX_THREADS;
            thread = realloc(thread, sizeof(okl4_kthread_t) * cur_size);
            assert(thread != NULL);
        }
        error = okl4_pd_thread_create(root_pd, &thread_attr, &thread[i]);
        if (!error) {
            okl4_pd_thread_start(root_pd, thread[i++]);
        }
    }
    while (error == 0);

    /* Wait for the new threads to execute. */
    while (okl4_atomic_read(&pd1100_thread_started) < i) {
        L4_ThreadSwitch(L4_nilthread);
    }

    /* Delete the thread. */
    for (j = 0; j < i; j++) {
        okl4_pd_thread_delete(root_pd, thread[j]);
    }
    free(thread);
}
END_TEST


TCase *
make_pd_tcase(void)
{
    TCase * tc;

    tc = tcase_create("Protection domains");
    tcase_add_test(tc, PD0100);
    tcase_add_test(tc, PD0200);
    tcase_add_test(tc, PD0300);
    tcase_add_test(tc, PD0400);
    tcase_add_test(tc, PD0410);
    tcase_add_test(tc, PD0420);
    tcase_add_test(tc, PD0430);
    tcase_add_test(tc, PD0500);
    tcase_add_test(tc, PD1000);
    tcase_add_test(tc, PD1100);
    tcase_add_test(tc, PD1101);

    return tc;
}
