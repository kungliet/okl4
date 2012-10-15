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
 * Last Modified: ChangHua Chen  Sep 19 2007 
 */
#include <l4/types.h>
#include <l4/config.h>
#include <l4/thread.h>
#include <l4/space.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include <l4/kdebug.h>
#include <bench/bench.h>
#include <l4/ipc.h>
#include <l4/schedule.h>

#if defined(ARM_SHARED_DOMAINS)
#include <l4/arch/ver/space_resources.h>
#endif

extern struct counter cycle_counter;

static struct counter *spaceswitch_counters[] = {
    &cycle_counter,
    NULL
};

#define TEST_SPACE_START    10
#define STACK_SIZE          0x400

static L4_SpaceId_t space_id[2];
static L4_Word_t test_stack[STACK_SIZE];
static L4_Word_t test2_stack[STACK_SIZE];
static L4_Word_t test3_stack[STACK_SIZE];
static L4_Word_t pager_stack[STACK_SIZE];
static L4_ThreadId_t main_tid, pager_tid, test_tid, test2_tid, test3_tid;
static L4_ThreadId_t test_th, test2_th, test3_th;
static L4_Word_t utcb_size;

static void dummy_thread(void)
{
    while (1)
    {
        L4_Call(main_tid);
    }
}

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
                assert ((from.raw == test_th.raw) || (from.raw == test2_th.raw) || (from.raw == test3_th.raw));

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

static void
spaceswitch_setup(struct new_bench_test *test, int args[])
{
    int r,i;
    pager_tid.raw = KBENCH_SERVER.raw + 1;
    test_tid.raw = KBENCH_SERVER.raw + 16;
    test2_tid.raw = KBENCH_SERVER.raw + 17;
    test3_tid.raw = KBENCH_SERVER.raw + 18;
    main_tid = KBENCH_SERVER;


    L4_Fpage_t utcb_area;
    L4_Word_t  utcb;
    L4_Word_t  dummy;

    utcb_size = L4_GetUtcbSize();

#ifdef NO_UTCB_RELOCATE
    utcb_area = L4_Nilpage;
    utcb = -1UL;
#else
    utcb_area = L4_Fpage(16 * 1024 * 1024,
                         L4_GetUtcbSize() * (1024));
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

    /* create test space */
    for (i = 0; i< 2; i++)
    {
        space_id[i] = L4_SpaceId(TEST_SPACE_START + i);
        r = L4_SpaceControl(space_id[i], L4_SpaceCtrl_new | L4_SpaceCtrl_kresources_accessible, KBENCH_CLIST, utcb_area, 0, &dummy);
        if (r == 0) 
        {
            printf("Create Space(arg = %d) Error: %"PRIxPTR"\n", args[0], L4_ErrorCode());
        }
        assert(r == 1);
    }


    /* create test threads */
#ifdef NO_UTCB_RELOCATE
    utcb = -1UL;
#else
    utcb = (L4_Word_t) ((L4_PtrSize_t) 16 * 1024 * 1024 + utcb_size * (16));
#endif

    r = L4_ThreadControl(test_tid, space_id[0], pager_tid, pager_tid, pager_tid, 0, (void *)(L4_PtrSize_t)utcb);
    L4_StoreMR(0, &test_th.raw);
    assert(r == 1);

    L4_Schedule(test_tid, -1, 1, -1, -1, 0, &dummy);
    
    L4_Start_SpIp(test_tid, (L4_Word_t)(L4_PtrSize_t)(&test_stack[STACK_SIZE]), (L4_Word_t) (L4_PtrSize_t) dummy_thread);

    L4_Receive(test_tid);
    /* test2 */
#ifdef NO_UTCB_RELOCATE
    utcb = -1UL;
#else
    utcb = (L4_Word_t) ((L4_PtrSize_t) 16 * 1024 * 1024 + utcb_size * (17));
#endif
    r = L4_ThreadControl(test2_tid, space_id[0], pager_tid, pager_tid, pager_tid, 0, (void *)(L4_PtrSize_t)utcb);
    L4_StoreMR(0, &test2_th.raw);
    assert(r == 1);

    L4_Schedule(test2_tid, -1, 1, -1, -1, 0, &dummy);
    
    L4_Start_SpIp(test2_tid, (L4_Word_t)(L4_PtrSize_t)(&test2_stack[STACK_SIZE]), (L4_Word_t) (L4_PtrSize_t) dummy_thread);

    L4_Receive(test2_tid);

    /* test3 */
#ifdef NO_UTCB_RELOCATE
    utcb = -1UL;
#else
    utcb = (L4_Word_t) ((L4_PtrSize_t) 16 * 1024 * 1024 + utcb_size * (17));
#endif
    r = L4_ThreadControl(test3_tid, space_id[1], pager_tid, pager_tid, pager_tid, 0, (void *)(L4_PtrSize_t)utcb);
    L4_StoreMR(0, &test3_th.raw);
    assert(r == 1);

    L4_Schedule(test3_tid, -1, 1, -1, -1, 0, &dummy);
    
    L4_Start_SpIp(test3_tid, (L4_Word_t)(L4_PtrSize_t)(&test3_stack[STACK_SIZE]), (L4_Word_t) (L4_PtrSize_t) dummy_thread);

    L4_Receive(test3_tid);
#if 0
    /* make test space sharedomain with root space */
    L4_Word_t vaddr = 0x80000000;
    vaddr &= ~((1UL << 31)-1); 
    L4_Fpage_t fpg = L4_FpageLog2(vaddr, 31);;
    
    L4_Set_Rights(&fpg, 7);

    L4_LoadMR(0, space_id[0].raw);
    L4_LoadMR(1, fpg.raw);

    r = L4_MapControl(KBENCH_SPACE, L4_MapCtrl_ShareDomain);
    assert(r == 1);
#endif
}

static void
spaceswitch_test(struct new_bench_test *test, int args[], volatile uint64_t *count)
{
    int i, r = 1;
    L4_Word_t utcb = 
#ifdef NO_UTCB_RELOCATE
        -1UL;
#else
        (L4_Word_t) ((L4_PtrSize_t) 16 * 1024 * 1024 + utcb_size * (16));
#endif

    L4_Word_t ovh = 0;
    for (i = 0; i < 1000; i++)
    {
        cycle_counter.start();
        cycle_counter.stop();
        ovh += cycle_counter.get_count(0);
    }
    ovh /= 1000;

    for (i =0; i < args[0]; i++)
    {    

        if (!(i & 1))
        {
            cycle_counter.start();
            r = L4_SpaceSwitch(test_tid, space_id[1], (void *)utcb);
            cycle_counter.stop();
            if (r == 0)
                printf("Space switch Error: %"PRIxPTR"\n", L4_ErrorCode());
            assert(r == 1);
            count[0] += (cycle_counter.get_count(0) - ovh);
        }
        else
        {
            cycle_counter.start();
            r = L4_SpaceSwitch(test_tid, space_id[0], (void *)utcb);
            cycle_counter.stop();
            if (r == 0)
                printf("Space switch Error: %"PRIxPTR"\n", L4_ErrorCode());
            assert(r == 1);
            count[0] += (cycle_counter.get_count(0) - ovh);
        }
    }

}

static void
spaceswitch_teardown(struct new_bench_test *test, int args[])
{
    int i,r;

    /* Delete test thread and pager */
    /* test_tid: */
    r = L4_ThreadControl(test_tid, L4_nilspace, L4_nilthread, L4_nilthread, L4_nilthread, 0, (void *)0);
    if (r == 0)
        printf("Thread Delete failed, ErrorCode = %d\n", (int)L4_ErrorCode());
    assert(r == 1);
    /* test2_tid: */
    r = L4_ThreadControl(test2_tid, L4_nilspace, L4_nilthread, L4_nilthread, L4_nilthread, 0, (void *)0);
    if (r == 0)
        printf("Thread Delete failed, ErrorCode = %d\n", (int)L4_ErrorCode());
    assert(r == 1);
    /* test3_tid: */
    r = L4_ThreadControl(test3_tid, L4_nilspace, L4_nilthread, L4_nilthread, L4_nilthread, 0, (void *)0);
    if (r == 0)
        printf("Thread Delete failed, ErrorCode = %d\n", (int)L4_ErrorCode());
    assert(r == 1);
    /* pager: */
    r = L4_ThreadControl(pager_tid, L4_nilspace, L4_nilthread, L4_nilthread, L4_nilthread, 0, (void *)0);
    if (r == 0)
        printf("Thread Delete failed, ErrorCode = %d\n", (int)L4_ErrorCode());
    assert(r == 1);


    /* Delete test space */
    for (i = 0; i < 2; i++)
    {
        r = L4_SpaceControl(space_id[i], L4_SpaceCtrl_delete, KBENCH_CLIST, L4_Nilpage, 0, NULL);
        if (r == 0)
        {
            printf("Delete Space Error: %"PRIxPTR"\n",  L4_ErrorCode());
        }
        assert(r == 1);
    }
}

struct new_bench_test new_bench_spaceswitch_priv = {
    "space switch - by privileged thread", 
    spaceswitch_counters,
    spaceswitch_setup,
    spaceswitch_test,
    spaceswitch_teardown,
    {
        {&iterations, 1000, 1000, 5, add_fn },
        {NULL, 0, 0, 0}
    }
};

/*------------------------------------------------------------------------------*/
static int iteration;
static volatile uint64_t *result;

static void switch_self_thread(void)
{
    int i,r;
    L4_Word_t utcb = 
#ifdef NO_UTCB_RELOCATE
        -1UL;
#else
        (L4_Word_t) ((L4_PtrSize_t) 16 * 1024 * 1024 + utcb_size * (16));
#endif

    L4_Call(main_tid);

    L4_Word_t ovh = 0;
    for (i=0; i< 1005; i++)
    {
        cycle_counter.start();
        cycle_counter.stop();
        /* Ignore the first 5 runs to eliminate creation and line fill overhead */
        if (i >= 5)
            ovh += cycle_counter.get_count(0);
    }
    ovh /= 1000;

    //now test function send us a ipc indicate test start.
    for (i =0; i < iteration + 5; i++)
    {
        if (!(i & 1))
        {
            cycle_counter.start();
            r = L4_SpaceSwitch(test_tid, space_id[1], (void *)utcb);
            cycle_counter.stop();
            if (r == 0)
                printf("Space switch Error: %"PRIxPTR"\n", L4_ErrorCode());
            assert(r == 1);
            /* Ignore the first 5 runs to eliminate creation and line fill overhead */
            if (i > 5)
                *result += (cycle_counter.get_count(0) - ovh);
        }
        else
        {
            cycle_counter.start();
            r = L4_SpaceSwitch(test_tid, space_id[0], (void *)utcb);
            cycle_counter.stop();
            if (r == 0)
                printf("Space switch Error: %"PRIxPTR"\n", L4_ErrorCode());
            assert(r == 1);
            /* Ignore the first 5 runs to eliminate creation and line fill overhead */
            if (i >= 5)
                *result += (cycle_counter.get_count(0) - ovh);
        }
    }
    //send ipc to main thread indicate test finished.
    L4_Send(main_tid);
}


static void
spaceswitch_self_test(struct new_bench_test *test, int args[], volatile uint64_t *count)
{
    result = count;
    L4_Call(test_tid);
}

static void
spaceswitch_self_teardown(struct new_bench_test *test, int args[])
{
    int i,r;
    /* Delete test thread and pager */
    /* test_tid: */
    r = L4_ThreadControl(test_tid, L4_nilspace, L4_nilthread, L4_nilthread, L4_nilthread, 0, (void *)0);
    if (r == 0)
        printf("Thread Delete failed, ErrorCode = %d\n", (int)L4_ErrorCode());
    assert(r == 1);
    /* test2_tid: */
    r = L4_ThreadControl(test2_tid, L4_nilspace, L4_nilthread, L4_nilthread, L4_nilthread, 0, (void *)0);
    if (r == 0)
        printf("Thread Delete failed, ErrorCode = %d\n", (int)L4_ErrorCode());
    assert(r == 1);
    /* test3_tid: */
    r = L4_ThreadControl(test3_tid, L4_nilspace, L4_nilthread, L4_nilthread, L4_nilthread, 0, (void *)0);
    if (r == 0)
        printf("Thread Delete failed, ErrorCode = %d\n", (int)L4_ErrorCode());
    assert(r == 1);
    /* pager: */
    r = L4_ThreadControl(pager_tid, L4_nilspace, L4_nilthread, L4_nilthread, L4_nilthread, 0, (void *)0);
    if (r == 0)
        printf("Thread Delete failed, ErrorCode = %d\n", (int)L4_ErrorCode());
    assert(r == 1);

    /* Delete test space */
    for (i = 0; i < 2; i++)
    {
        r = L4_SpaceControl(space_id[i], L4_SpaceCtrl_delete, KBENCH_CLIST, L4_Nilpage, 0, NULL);
        if (r == 0)
        {
            printf("Delete Space Error: %"PRIxPTR"\n",  L4_ErrorCode());
        }
        assert(r == 1);
    }
}

static void
spaceswitch_self_setup(struct new_bench_test *test, int args[]);

struct new_bench_test new_bench_spaceswitch_sd = {
    "space switch - using shared domain",
    spaceswitch_counters,
    spaceswitch_self_setup,
    spaceswitch_self_test,
    spaceswitch_self_teardown,
    {
        {&iterations, 1000, 1000, 5, add_fn },
        {NULL, 0, 0, 0}
    }
};

struct new_bench_test new_bench_spaceswitch_no_sd = {
    "space switch - not using shared domain",
    spaceswitch_counters,
    spaceswitch_self_setup,
    spaceswitch_self_test,
    spaceswitch_self_teardown,
    {
        {&iterations, 1000, 1000, 5, add_fn },
        {NULL, 0, 0, 0}
    }
};

static void
spaceswitch_self_setup(struct new_bench_test *test, int args[])
{
    int r,i;
    pager_tid.raw = KBENCH_SERVER.raw + 1;
    test_tid.raw = KBENCH_SERVER.raw + 16;
    test2_tid.raw = KBENCH_SERVER.raw + 17;
    test3_tid.raw = KBENCH_SERVER.raw + 18;
    main_tid = KBENCH_SERVER;
    iteration = args[0];

    L4_Fpage_t utcb_area;
    L4_Word_t  utcb;
    L4_Word_t  dummy;

    utcb_size = L4_GetUtcbSize();

#ifdef NO_UTCB_RELOCATE
    utcb_area = L4_Nilpage;
    utcb = -1UL;
#else
    utcb_area = L4_Fpage(16 * 1024 * 1024,
                         L4_GetUtcbSize() * (1024));
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

    /* create test space */
    for (i = 0; i< 2; i++)
    {
        space_id[i] = L4_SpaceId(TEST_SPACE_START + i);
        r = L4_SpaceControl(space_id[i], L4_SpaceCtrl_new | L4_SpaceCtrl_kresources_accessible, KBENCH_CLIST, utcb_area, 0, &dummy);
        if (r == 0) 
        {
            printf("Create Space(arg = %d) Error: %"PRIxPTR"\n", args[0], L4_ErrorCode());
        }
        assert(r == 1);
    }

    /* create test threads */
#ifdef NO_UTCB_RELOCATE
    utcb = -1UL;
#else
    utcb = (L4_Word_t) ((L4_PtrSize_t) 16 * 1024 * 1024 + utcb_size * (16));
#endif
    r = L4_ThreadControl(test_tid, space_id[0], pager_tid, pager_tid, pager_tid, 0, (void *)(L4_PtrSize_t)utcb);
    L4_StoreMR(0, &test_th.raw);
    assert(r == 1);

    L4_Schedule(test_tid, -1, 1, -1, -1, 0, &dummy);
    
    L4_Start_SpIp(test_tid, (L4_Word_t)(L4_PtrSize_t)(&test_stack[STACK_SIZE]), (L4_Word_t) (L4_PtrSize_t) switch_self_thread);

    L4_Receive(test_tid);
    /* test2 */
#ifdef NO_UTCB_RELOCATE
    utcb = -1UL;
#else
    utcb = (L4_Word_t) ((L4_PtrSize_t) 16 * 1024 * 1024 + utcb_size * (17));
#endif
    r = L4_ThreadControl(test2_tid, space_id[0], pager_tid, pager_tid, pager_tid, 0, (void *)(L4_PtrSize_t)utcb);
    L4_StoreMR(0, &test2_th.raw);
    assert(r == 1);

    L4_Schedule(test2_tid, -1, 1, -1, -1, 0, &dummy);
    
    L4_Start_SpIp(test2_tid, (L4_Word_t)(L4_PtrSize_t)(&test2_stack[STACK_SIZE]), (L4_Word_t) (L4_PtrSize_t) dummy_thread);

    L4_Receive(test2_tid);

    /* test3 */
    r = L4_ThreadControl(test3_tid, space_id[1], pager_tid, pager_tid, pager_tid, 0, (void *)(L4_PtrSize_t)utcb);
    L4_StoreMR(0, &test3_th.raw);
    assert(r == 1);

    L4_Schedule(test3_tid, -1, 1, -1, -1, 0, &dummy);
    
    L4_Start_SpIp(test3_tid, (L4_Word_t)(L4_PtrSize_t)(&test3_stack[STACK_SIZE]), (L4_Word_t) (L4_PtrSize_t) dummy_thread);

    L4_Receive(test3_tid);

#if defined(ARM_SHARED_DOMAINS)
    if (test == &new_bench_spaceswitch_sd)
    {
        L4_LoadMR(0, space_id[1].raw);
        r = L4_SpaceControl(space_id[0], L4_SpaceCtrl_resources, KBENCH_CLIST, 
            L4_Nilpage, L4_SPACE_RESOURCES_WINDOW_GRANT, NULL);
        assert(r == 1);
        /* make test space sharedomain with root space */
        L4_Word_t vaddr = 0x00000000;
        vaddr = vaddr >> 20 << 20;
        L4_Fpage_t fpg = L4_FpageLog2(vaddr, 20);

        L4_Set_Rights(&fpg, 7);

        L4_LoadMR(0, space_id[1].raw);
        L4_LoadMR(1, fpg.raw);

        r = L4_MapControl(space_id[0], L4_MapCtrl_MapWindow);
        assert(r == 1);
    }
#endif
}
