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
 * Author: Changhua Chen Created: Aug 2 2004 
 */
#include <stdlib.h>
#include <string.h>
#include <bench/bench.h>
#include <l4/ipc.h>
#include <l4/config.h>
#include <l4/space.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <l4/kdebug.h>
#include <l4/schedule.h>
#include <cycles/cycles.h>


#define getTagE(tag) ((tag.raw & (1 << 15)) >> 15)
#define START_ADDR(func)    ((L4_Word_t) func)

#if defined(PLATFORM_PXA)
#define PMU_IRQ 12
#elif defined(CONFIG_PLAT_IMX31)
#define PMU_IRQ 23
#else
#error "Platform do not support Performance Moniter!"
#endif

#define SPINNER 1

static L4_SpaceId_t handler_space;

static L4_ThreadId_t master_tid, pager_tid, handler_tid;
#if SPINNER
static L4_ThreadId_t spinner_tid;
#endif
static L4_Word_t interrupt;
static int num_iterations = 0;
static volatile uint64_t * results;


static L4_Word_t handler_stack[2048] __attribute__ ((aligned (16)));
static L4_Word_t pager_stack[2048] __attribute__ ((aligned (16)));
#if SPINNER
static L4_Word_t spinner_stack[1024] __attribute__ ((aligned (16)));
#endif
extern struct new_bench_test new_bench_irq_ipc;

static int
get_num_counters(struct counter *myself)
{
    return 1;
}

static const char *
get_name(struct counter *myself, int counter)
{
    return "Cycle counter";
}

static const char *
get_unit(struct counter *myself, int counter)
{
    return "cycles";
}

static void
init(struct counter *myself)
{
}

static void
setup(struct counter *myself)
{
}

static void
start(void)
{
}

static void
stop(void)
{
}

static uint64_t
get_count(int counter)
{
    return 0;
}

static struct counter dummy_cycle_counter = {
    .get_num_counters = get_num_counters,
    .get_name = get_name,
    .get_unit = get_unit,
    .init = init,
    .setup = setup,
    .start = start,
    .stop = stop,
    .get_count = get_count
};

static struct counter *irq_ipc_counters[] = {
    &dummy_cycle_counter,
    NULL
};


static void
handler(void)
{
    int i;
    L4_Word_t irq, control;
    irq = interrupt;

    //L4_MsgTag_t tag;
    L4_Call(master_tid);

    /* Accept interrupts */
    L4_Accept(L4_NotifyMsgAcceptor);
    L4_Set_NotifyMask(~0UL);

    /* Handle interrupts */
    for (i=0; i < num_iterations + 2; i++)
    {
        int k,j;
        struct counter **count;
        uint64_t cnt = 0;

        L4_Word_t mask;
#if defined(CONFIG_CPU_ARM_XSCALE) || defined(CONFIG_CPU_ARM_ARM1136JS) || defined(CONFIG_CPU_ARM_ARM1176JZS)
        //Setup pmu irq
        L4_KDB_PMN_Write(REG_CCNT, 0xFF800000); //At least leave 0x100000 cycles
        L4_Word_t PMNC = L4_KDB_PMN_Read(REG_PMNC) & ~PMNC_CCNT_64DIV;
        L4_KDB_PMN_Write(REG_PMNC, (PMNC | PMNC_CCNT_OFL | PMNC_CCNT_ENABLE) & ~PMNC_CCNT_RESET);
        L4_KDB_PMN_Ofl_Write(REG_CCNT, ~0UL);
#endif
        // ack/wait IRQ
        control = 0 | (3 << 6);
        L4_LoadMR(0, irq);
        (void)L4_InterruptControl(L4_nilthread, control);
        L4_StoreMR(1, &mask);

        irq = __L4_TCR_PlatformReserved(0);

        //tag.raw = 0x0000C000;
        //tag = L4_Ipc(from, thread_tid, tag, &from);
        //if (getTagE(tag))
        //    printf("ipc error %lx\n", L4_ErrorCode());
        //printf("Receive irq ipc from %lx\n", from.raw);
#if defined(CONFIG_CPU_ARM_XSCALE) || defined(CONFIG_CPU_ARM_ARM1136JS) || defined(CONFIG_CPU_ARM_ARM1176JZS)
        //Get CCNT count
        cnt = L4_KDB_PMN_Read(REG_CCNT);
        PMNC = L4_KDB_PMN_Read(REG_PMNC);
        assert(PMNC & ~PMNC_CCNT_OFL);
        //Since on ARM11, PMNC IRQ can only be deasserted when PMU is enabled,
        //need to clear overflow flag and disable IRQ before disable PMU.
        L4_KDB_PMN_Write(REG_PMNC, (PMNC | PMNC_CCNT_OFL) & ~PMNC_CCNT_ENIRQ);
        //Stop CCNT.
        L4_KDB_PMN_Write(REG_PMNC, PMNC & ~PMNC_CCNT_ENABLE);
        //printf("CNT is %016lld\n", cnt);
#endif

        //Write result back.
        for (k = 0, count = irq_ipc_counters; *count != NULL; count++, k++)
        {
            for (j = 0; j < (*count)->get_num_counters(*count); j++)
            {
                results[k] += cnt;
                if ( i == 0 || i == 1) //we don't count the first 2 run, since they contain page fault and cache miss latency.
                    results[k] -= cnt;
            }
        }
    }

    /* Tell master that we're finished */
    L4_Set_MsgTag (L4_Niltag);
    L4_Send (master_tid);

    for (;;)
        L4_WaitForever();

    /* NOTREACHED */
}

#if SPINNER
static void
spinner (void)
{
    for (;;) {
        //L4_ThreadSwitch(pager_tid);
    }
}
#endif

extern word_t get_seg(L4_SpaceId_t spaceid, word_t vaddr, word_t *offset, word_t *cache, word_t *rwx);

static void
pager (void)
{
    L4_ThreadId_t tid;
    L4_MsgTag_t tag;
    L4_Msg_t msg;

    L4_Send(master_tid);
    for (;;) {
        tag = L4_Wait(&tid);

        for (;;) {
            L4_Word_t faddr, fip;
            L4_MsgStore(tag, &msg);

            if (L4_UntypedWords (tag) != 2 ||
                !L4_IpcSucceeded (tag)) {
                printf ("Malformed pagefault IPC from %p (tag=%p)\n",
                        (void *) tid.raw, (void *) tag.raw);
                L4_KDB_Enter ("malformed pf");
                break;
            }

            faddr = L4_MsgWord(&msg, 0);
            fip   = L4_MsgWord (&msg, 1);
            L4_MsgClear(&msg);
            {
                L4_MapItem_t map;
                L4_SpaceId_t space;
                L4_Word_t seg, offset, cache, rwx, size;
                int r;

                seg = get_seg(KBENCH_SPACE, faddr, &offset, &cache, &rwx);
                assert(seg != ~0UL);

                size = L4_GetMinPageBits();
                faddr &= ~((1ul << size)-1);
                offset &= ~((1ul << size)-1);

                space.raw = __L4_TCR_SenderSpace();

                L4_MapItem_Map(&map, seg, offset, faddr, size,
                               cache, rwx);
                r = L4_ProcessMapItem(space, map);
                assert(r == 1);
            }
            L4_MsgLoad(&msg);
//            tag = L4_ReplyWait (tid, &tid);
            tag = L4_MsgTag();
            L4_Set_SendBlock(&tag);
            L4_Set_ReceiveBlock(&tag);
            tag = L4_Ipc(tid, L4_anythread, tag, &tid);
        }
    }
}




static void
ipc_irq_test(struct new_bench_test *test, int args[], volatile uint64_t *count)
{
    results = count;
    /* Send empty message to notify pager to startup test thread */
    L4_LoadMR (0, 0);
    L4_Send (handler_tid);
    L4_Receive (handler_tid);
}

static void
ipc_irq_teardown(struct new_bench_test *test, int args[])
{
    int r = 0;
    L4_Word_t control;

    //L4_DeassociateInterrupt(thread_tid);
    control = 0 | (1 << 6) | (31<<27);
    L4_LoadMR(0, interrupt);
    r = L4_InterruptControl(handler_tid, control);
    if (r == 0) {
        printf("Cannot unregister interrupt\n");
    }

#if SPINNER
    /* Delete spinner */
    r = L4_ThreadControl (spinner_tid, L4_nilspace, L4_nilthread, L4_nilthread, L4_nilthread, 0, (void *) 0);
    assert (r == 1);
#endif
    /* Delete handler thread */
    r = L4_ThreadControl (handler_tid, L4_nilspace, L4_nilthread,
        L4_nilthread, L4_nilthread, 0, (void *) 0);
    assert (r == 1);

    /* Delete pager */
    r = L4_ThreadControl (pager_tid, L4_nilspace, L4_nilthread,
            L4_nilthread, L4_nilthread, 0, (void *) 0);
    assert (r == 1);
}

static void
ipc_irq_setup(struct new_bench_test *test, int args[])
{
    int r;
    L4_Word_t utcb;
    L4_Word_t utcb_size;
//    L4_Word_t dummy;

    num_iterations = args[0];
    handler_space = L4_nilspace;


    /* We need a maximum of two threads per task */
    utcb_size = L4_GetUtcbSize();
#ifdef NO_UTCB_RELOCATE
    utcb = ~0UL;
#else
    utcb =(L4_Word_t)L4_GetUtcbBase() + utcb_size;
#endif
    /* Create pager */
    master_tid = KBENCH_SERVER;
    pager_tid.raw = KBENCH_SERVER.raw + 1;
    handler_tid.raw = KBENCH_SERVER.raw + 2;
    interrupt = PMU_IRQ;
#if SPINNER
    spinner_tid = L4_GlobalId (L4_ThreadNo (master_tid) + 3, 2);
#endif

    r = L4_ThreadControl (pager_tid, KBENCH_SPACE, master_tid, master_tid, 
                master_tid, 0, (void*)utcb);
    if (r == 0 && (L4_ErrorCode() == 2))
    {
        r = L4_ThreadControl (pager_tid, L4_nilspace, L4_nilthread,
            L4_nilthread, L4_nilthread, 0, (void *) 0);
        assert(r == 1);
        r = L4_ThreadControl (pager_tid, KBENCH_SPACE, master_tid, master_tid, 
                master_tid, 0, (void*)utcb);
        assert(r == 1);
    }
    L4_KDB_SetThreadName(pager_tid, "pager");
    //L4_Schedule(pager_tid, -1, -1, 1, -1, -1, 0, &dummy, &dummy);
    L4_Set_Priority(pager_tid, 254);
    L4_Start_SpIp (pager_tid, (L4_Word_t) pager_stack + sizeof(pager_stack) - 32, START_ADDR (pager));

    L4_Receive(pager_tid);

#ifdef NO_UTCB_RELOCATE
    utcb = ~0UL;
#else
    utcb += utcb_size;
#endif

    r = L4_ThreadControl(handler_tid, KBENCH_SPACE, master_tid, pager_tid, pager_tid, 0, (void *) utcb);
    assert(r == 1);
    L4_KDB_SetThreadName(handler_tid, "handler");
    L4_Set_Priority(handler_tid, 100);

    // Startup notification, start handler thread
    //printf("register irq %ld, to %lx\n", interrupt, handler_tid.raw);

    L4_Word_t control = 0 | (0 << 6) | (31<<27);
    L4_LoadMR(0, interrupt);
    r = L4_InterruptControl(handler_tid, control);
    if (r == 0) {
        printf("Cannot register interrupt %lu\n", interrupt);
    }

    L4_Start_SpIp (handler_tid, (L4_Word_t) handler_stack + sizeof(handler_stack) - 32, START_ADDR(handler));

    L4_Receive(handler_tid);

#if SPINNER
    //Create spinner thread
#ifdef NO_UTCB_RELOCATE
    utcb = ~0UL;
#else
    utcb += utcb_size;
#endif
    r = L4_ThreadControl (spinner_tid, KBENCH_SPACE, master_tid, pager_tid, pager_tid, 0, (void*) utcb);
    if (r == 0) printf("create spinner failed %ld\n", L4_ErrorCode());
    assert(r == 1);
    L4_KDB_SetThreadName(spinner_tid, "spinner");
    //L4_Schedule(spinner_tid, -1, -1, 1, -1, -1, 0, &dummy, &dummy);
    //Set priority to the lowest.
    L4_Set_Priority(spinner_tid, 1);    
    L4_Start_SpIp (spinner_tid, (L4_Word_t) spinner_stack + sizeof(spinner_stack) - 32, START_ADDR (spinner));
#endif
}

/*
 * From new_bench_irq_ipc, we can get:
 * Latancy of IRQ IPC = bench of new_bench_irq_ipc / 500
 */
struct new_bench_test new_bench_irq_ipc = {
    "ipc - irq ipc latency", irq_ipc_counters, ipc_irq_setup, ipc_irq_test, ipc_irq_teardown,
    {
        {&iterations, 100, 100, 5, add_fn },
        {NULL, 0, 0, 0}
    }
};
