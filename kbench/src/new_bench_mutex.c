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

#include <bench/bench.h>
#include <mutex/mutex.h>
#include <l4/thread.h>
#include <l4/types.h>
#include <l4/config.h>
#include <l4/mutex.h>
#include <l4/ipc.h>
#include <l4/space.h>
#include <l4/map.h>
#include <l4/schedule.h>

#include <assert.h>
#include <stdio.h>
#include <inttypes.h>

#define TEST_SPACE_START    10
#define STACK_SIZE          0x400

extern struct counter cycle_counter;

static struct counter *mutex_counters[] = {
    &cycle_counter,
    NULL
};

static L4_Word_t thread_stack[STACK_SIZE];
static L4_Word_t thread2_stack[STACK_SIZE];
static L4_Word_t pager_stack[STACK_SIZE];
static L4_ThreadId_t main_tid, pager_tid, tid, tid2;
static L4_ThreadId_t thandle, thandle2;
static L4_Word_t utcb_size;

extern struct new_bench_test new_bench_no_contention_mutex_lock;
extern struct new_bench_test new_bench_no_contention_mutex_unlock;
extern struct new_bench_test new_bench_contention_mutex_trylock;
extern struct new_bench_test new_bench_contention_mutex_lock;
extern struct new_bench_test new_bench_contention_mutex_unlock;

extern word_t get_seg(L4_SpaceId_t spaceid, word_t vaddr, word_t *offset, word_t *cache, word_t *rwx);

static void pager(void)
{
    L4_ThreadId_t from;
    L4_MsgTag_t tag;
    L4_Msg_t msg;
    L4_Word_t label;

    L4_Send(main_tid);

    while(1)
    {
        tag = L4_Wait(&from);

        L4_MsgStore(tag, &msg);
        if (!L4_IpcSucceeded(tag))
        {
            printf("Was receiving an IPC which was cancelled?\n");
            continue;
        }

        label = L4_MsgLabel(&msg);

        tag = L4_Niltag;
        long fault;
        char type;
        fault = (((long)(label<<16)) >> 20);
        type = label & 0xf;

        switch(fault)
        {
            case -2: 
            {
                L4_Word_t fault_addr, ip;
                L4_SpaceId_t space;
                L4_Word_t seg, offset, cache, rwx, size;
                L4_MapItem_t map;
                int r;

                fault_addr = L4_MsgWord(&msg, 0);
                ip = L4_MsgWord(&msg, 1);
//printf(">>>>>>>>>Got pagefault addr: %p ip: %p from:0x%lx, space: 0x%lx\n", (void *)fault_addr, (void *)ip, from.raw, __L4_TCR_SenderSpace());
                seg = get_seg(KBENCH_SPACE, fault_addr, &offset, &cache, &rwx);
                if (seg == ~0UL)
                {
                    printf("invalid pagefault addr: %p ip: %p from:0x%lx\n", (void *)fault_addr, (void *)ip, from.raw);
                    tag = L4_MsgTagAddLabel(tag, (-2UL)>>4<<4); //indicate fail as pagefault
                    L4_Set_MsgTag(tag);
                    L4_Send(KBENCH_SERVER);
                    break;
                }
                assert ((from.raw == thandle.raw) || (from.raw == thandle2.raw));

                size = L4_GetMinPageBits();
                fault_addr &= ~((1ul << size)-1);
                offset &= ~((1ul << size)-1);

                space.raw = __L4_TCR_SenderSpace();
                L4_MapItem_Map(&map, seg, offset, fault_addr, size,
                               cache, rwx);
                r = L4_ProcessMapItem(space, map);
                assert(r == 1);

                L4_MsgClear(&msg);
                L4_MsgLoad(&msg);
                L4_Reply(from);
                break;
            }
            default:
                assert(0);
                break;
        }
    }
}

static int iteration;
static volatile uint64_t *result;
static okl4_libmutex_t mutex;

static void
no_contention_mutex_lock_thread(void)
{
    int i;

    L4_Call(main_tid);
    L4_Word_t ovh = 0;
    for (i = 0; i < 1005; i++)
    {
        cycle_counter.start();
        cycle_counter.stop();
        /* Ignore the first 5 runs to eliminate creation and line fill overhead */
        if (i >= 5)
            ovh += cycle_counter.get_count(0);
    }
    ovh /= 1000;

    for (i = 0; i < iteration; i++)
    {
        cycle_counter.start();
        okl4_libmutex_lock(mutex);
        cycle_counter.stop();
        /* Ignore the first 5 runs to eliminate creation and line fill overhead */
        if (i > 5)
            *result += (cycle_counter.get_count(0) - ovh);
        okl4_libmutex_unlock(mutex);
    }
    L4_Send(main_tid);
}

static void
no_contention_mutex_unlock_thread(void)
{
    int i;

    L4_Call(main_tid);
    L4_Word_t ovh = 0;
    for (i = 0; i < 1005; i++)
    {
        cycle_counter.start();
        cycle_counter.stop();
        /* Ignore the first 5 runs to eliminate creation and line fill overhead */
        if (i >= 5)
            ovh += cycle_counter.get_count(0);
    }
    ovh /= 1000;

    for (i = 0; i < iteration; i++)
    {
        okl4_libmutex_lock(mutex);
        cycle_counter.start();
        okl4_libmutex_unlock(mutex);
        cycle_counter.stop();
        /* Ignore the first 5 runs to eliminate creation and line fill overhead */
        if (i > 5)
            *result += (cycle_counter.get_count(0) - ovh);
    }
    L4_Send(main_tid);
}

static void
locking_thread(void)
{
    L4_Send(main_tid);
    L4_Receive(tid);
    okl4_libmutex_lock(mutex);
    L4_Send(tid);

    L4_Receive(tid);
    okl4_libmutex_unlock(mutex);
    L4_Send(tid);

    L4_WaitForever();
}

static void
contention_mutex_trylock_thread(void)
{
    int i;

    L4_Call(main_tid);
    L4_Call(tid2);
    L4_Word_t ovh = 0;
    for (i = 0; i < 1005; i++)
    {
        cycle_counter.start();
        cycle_counter.stop();
        /* Ignore the first 5 runs to eliminate creation and line fill overhead */
        if (i >= 5)
            ovh += cycle_counter.get_count(0);
    }
    ovh /= 1000;

    for (i = 0; i < iteration + 5; i++)
    {
        cycle_counter.start();
        okl4_libmutex_trylock(mutex);
        cycle_counter.stop();
        /* Ignore the first 5 runs to eliminate creation and line fill overhead */
        if (i > 5)
            *result += (cycle_counter.get_count(0) - ovh);
    }
    L4_Call(tid2);
    L4_Send(main_tid);
}

static void
low_prio_locking_thread(void)
{
    L4_Send(main_tid);
    while (1) {
        okl4_libmutex_lock(mutex);
        L4_Receive(tid);
        cycle_counter.stop();
        okl4_libmutex_unlock(mutex);
    }

    L4_WaitForever();
}

static void
contention_mutex_lock_thread(void)
{
    int i;

    L4_Call(main_tid);
    L4_Word_t ovh = 0;
    for (i = 0; i < 1005; i++)
    {
        cycle_counter.start();
        cycle_counter.stop();
        /* Ignore the first 5 runs to eliminate creation and line fill overhead */
        if (i >= 5)
            ovh += cycle_counter.get_count(0);
    }
    ovh /= 1000;

    for (i = 0; i < iteration + 5; i++)
    {
        L4_Send(tid2);
        cycle_counter.start();
        okl4_libmutex_lock(mutex);
        /* Ignore the first 5 runs to eliminate creation and line fill overhead */
        if (i > 5)
            *result += (cycle_counter.get_count(0) - ovh);
        okl4_libmutex_unlock(mutex);
    }
    L4_Send(main_tid);
}

static void
low_prio_unlocking_thread(void)
{
    L4_Send(main_tid);
    while (1) {
        okl4_libmutex_lock(mutex);
        L4_Receive(tid);
        cycle_counter.start();
        okl4_libmutex_unlock(mutex);
    }

    L4_WaitForever();
}

static void
contention_mutex_unlock_thread(void)
{
    int i;

    L4_Call(main_tid);
    L4_Word_t ovh = 0;
    for (i = 0; i < 1005; i++)
    {
        cycle_counter.start();
        cycle_counter.stop();
        /* Ignore the first 5 runs to eliminate creation and line fill overhead */
        if (i >= 5)
            ovh += cycle_counter.get_count(0);
    }
    ovh /= 1000;

    for (i = 0; i < iteration + 5; i++)
    {
        L4_Send(tid2);
        okl4_libmutex_lock(mutex);
        cycle_counter.stop();
        /* Ignore the first 5 runs to eliminate creation and line fill overhead */
        if (i > 5)
            *result += (cycle_counter.get_count(0) - ovh);
        okl4_libmutex_unlock(mutex);
    }
    L4_Send(main_tid);
}

static void
mutex_setup(struct new_bench_test *test, int args[])
{
    int r;

    iteration = args[0];
    pager_tid.raw = KBENCH_SERVER.raw + 1;
    tid.raw = KBENCH_SERVER.raw + 16;
    tid2.raw = KBENCH_SERVER.raw + 17;
    main_tid = KBENCH_SERVER;

    L4_Word_t  utcb;
    L4_Word_t  dummy;

    utcb_size = L4_GetUtcbSize();

#ifdef NO_UTCB_RELOCATE
    utcb = -1UL;
#else
    utcb = (L4_Word_t) (L4_PtrSize_t)L4_GetUtcbBase() + (2)*utcb_size;
#endif

    /* create pager */
    r = L4_ThreadControl(pager_tid, KBENCH_SPACE, KBENCH_SERVER, KBENCH_SERVER, KBENCH_SERVER, 0, (void *)(L4_PtrSize_t)utcb);
    assert(r == 1);
    L4_Schedule(pager_tid, -1, 1, -1, -1, 0, &dummy);
    L4_Set_Priority(pager_tid, 254);
    L4_Start_SpIp(pager_tid, (L4_Word_t)(L4_PtrSize_t)(&pager_stack[STACK_SIZE]), (L4_Word_t) (L4_PtrSize_t) pager);
    L4_KDB_SetThreadName(pager_tid, "pager");

    L4_Receive(pager_tid);

    /* create test threads */
#ifdef NO_UTCB_RELOCATE
    utcb = -1UL;
#else
    utcb = (L4_Word_t) (L4_PtrSize_t)L4_GetUtcbBase() + (3)*utcb_size;
#endif
    r = L4_ThreadControl(tid, KBENCH_SPACE, pager_tid, pager_tid, pager_tid, 0, (void *)(L4_PtrSize_t)utcb);
    assert(r == 1);
    L4_StoreMR(0, &thandle.raw);

    L4_Schedule(tid, -1, 1, -1, -1, 0, &dummy);
    
    if (test == &new_bench_no_contention_mutex_lock)
        L4_Start_SpIp(tid, (L4_Word_t)(L4_PtrSize_t)(&thread_stack[STACK_SIZE]), (L4_Word_t) (L4_PtrSize_t) no_contention_mutex_lock_thread);
    if (test == &new_bench_no_contention_mutex_unlock)
        L4_Start_SpIp(tid, (L4_Word_t)(L4_PtrSize_t)(&thread_stack[STACK_SIZE]), (L4_Word_t) (L4_PtrSize_t) no_contention_mutex_unlock_thread);
    if (test == &new_bench_contention_mutex_trylock)
        L4_Start_SpIp(tid, (L4_Word_t)(L4_PtrSize_t)(&thread_stack[STACK_SIZE]), (L4_Word_t) (L4_PtrSize_t) contention_mutex_trylock_thread);
    if (test == &new_bench_contention_mutex_lock)
        L4_Start_SpIp(tid, (L4_Word_t)(L4_PtrSize_t)(&thread_stack[STACK_SIZE]), (L4_Word_t) (L4_PtrSize_t) contention_mutex_lock_thread);
    if (test == &new_bench_contention_mutex_unlock)
        L4_Start_SpIp(tid, (L4_Word_t)(L4_PtrSize_t)(&thread_stack[STACK_SIZE]), (L4_Word_t) (L4_PtrSize_t) contention_mutex_unlock_thread);

    L4_Receive(tid);
    
    if ((test == &new_bench_contention_mutex_trylock) || (test == &new_bench_contention_mutex_lock) ||
        (test == &new_bench_contention_mutex_unlock)) {
#ifdef NO_UTCB_RELOCATE
        utcb = -1UL;
#else
        utcb = (L4_Word_t) (L4_PtrSize_t)L4_GetUtcbBase() + (4)*utcb_size;
#endif
        r = L4_ThreadControl(tid2, KBENCH_SPACE, pager_tid, pager_tid, pager_tid, 0, (void *)(L4_PtrSize_t)utcb);
        assert(r == 1);
        L4_StoreMR(0, &thandle2.raw);

        L4_Schedule(tid2, -1, 1, -1, -1, 0, &dummy);
        
        if (test == &new_bench_contention_mutex_trylock)
            L4_Start_SpIp(tid2, (L4_Word_t)(L4_PtrSize_t)(&thread2_stack[STACK_SIZE]), (L4_Word_t) (L4_PtrSize_t) locking_thread);
        if (test == &new_bench_contention_mutex_lock) {
            L4_Set_Priority(tid2, 90);
            L4_Start_SpIp(tid2, (L4_Word_t)(L4_PtrSize_t)(&thread2_stack[STACK_SIZE]), (L4_Word_t) (L4_PtrSize_t) low_prio_locking_thread);
        }
        if (test == &new_bench_contention_mutex_unlock) {
            L4_Set_Priority(tid2, 90);
            L4_Start_SpIp(tid2, (L4_Word_t)(L4_PtrSize_t)(&thread2_stack[STACK_SIZE]), (L4_Word_t) (L4_PtrSize_t) low_prio_unlocking_thread);
        }

        L4_Receive(tid2);
    }
}

static void
mutex_teardown(struct new_bench_test *test, int args[])
{
    int r;

    /* Delete test threads and pager */
    /* tid: */
    r = L4_ThreadControl(tid, L4_nilspace, L4_nilthread, L4_nilthread, L4_nilthread, 0, (void *)0);
    if (r == 0)
        printf("Thread Delete failed, ErrorCode = %d\n", (int)L4_ErrorCode());
    assert(r == 1);
    /* tid2: */
    if ((test == &new_bench_contention_mutex_trylock) || (test == &new_bench_contention_mutex_lock) ||
        (test == &new_bench_contention_mutex_unlock)) {
        r = L4_ThreadControl(tid2, L4_nilspace, L4_nilthread, L4_nilthread, L4_nilthread, 0, (void *)0);
        if (r == 0)
            printf("Thread Delete failed, ErrorCode = %d\n", (int)L4_ErrorCode());
        assert(r == 1);
    }
    /* pager: */
    r = L4_ThreadControl(pager_tid, L4_nilspace, L4_nilthread, L4_nilthread, L4_nilthread, 0, (void *)0);
    if (r == 0)
        printf("Thread Delete failed, ErrorCode = %d\n", (int)L4_ErrorCode());
    assert(r == 1);
}


static void
mutex_test(struct new_bench_test *test, int args[], volatile uint64_t *count)
{
    struct okl4_libmutex m;

    result = count;
    mutex = &m;
    okl4_libmutex_init(mutex);
    L4_Call(tid);
    okl4_libmutex_free(mutex);
}

struct new_bench_test new_bench_no_contention_mutex_lock = {
    "mutex lock - no contention", 
    mutex_counters,
    mutex_setup,
    mutex_test,
    mutex_teardown,
    {
        {&iterations, 1000, 1000, 5, add_fn },
        {NULL, 0, 0, 0}
    }
};

struct new_bench_test new_bench_no_contention_mutex_unlock = {
    "mutex unlock - no contention", 
    mutex_counters,
    mutex_setup,
    mutex_test,
    mutex_teardown,
    {
        {&iterations, 1000, 1000, 5, add_fn },
        {NULL, 0, 0, 0}
    }
};

struct new_bench_test new_bench_contention_mutex_trylock = {
    "mutex trylock - contention", 
    mutex_counters,
    mutex_setup,
    mutex_test,
    mutex_teardown,
    {
        {&iterations, 1000, 1000, 5, add_fn },
        {NULL, 0, 0, 0}
    }
};

struct new_bench_test new_bench_contention_mutex_lock = {
    "mutex lock + switch to holder - contention", 
    mutex_counters,
    mutex_setup,
    mutex_test,
    mutex_teardown,
    {
        {&iterations, 1000, 1000, 5, add_fn },
        {NULL, 0, 0, 0}
    }
};

struct new_bench_test new_bench_contention_mutex_unlock = {
    "mutex unlock + switch to blocked thread - contention", 
    mutex_counters,
    mutex_setup,
    mutex_test,
    mutex_teardown,
    {
        {&iterations, 1000, 1000, 5, add_fn },
        {NULL, 0, 0, 0}
    }
};
