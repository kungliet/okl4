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

#include <stdlib.h>
#include <string.h>
#include <bench/bench.h>
#include <l4/ipc.h>
#include <l4/space.h>
#include <l4/config.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <l4/kdebug.h>
#include <l4/schedule.h>
#include <okl4/kspaceid_pool.h>

extern struct bench_test bench_ipc_inter;
extern struct bench_test bench_ipc_intra;
extern struct bench_test bench_ipc_fass;
extern struct bench_test bench_ipc_fass_buffer;
extern struct bench_test bench_ipc_fass_buffer_vspace;
extern struct bench_test bench_ipc_pagemap;
extern struct bench_test bench_ipc_page_faults;
extern struct bench_test bench_ipc_pagemap_simulated;
extern struct bench_test bench_ipc_intra_open;
extern struct bench_test bench_ipc_intra_close;
extern struct bench_test bench_ipc_intra_rpc;
extern struct bench_test bench_ipc_intra_ovh;
extern struct bench_test bench_ipc_intra_async;
extern struct bench_test bench_ipc_intra_async_ovh;

#define UTCB_ADDRESS    (0x80000000UL)
#define FASS_UTCB_ADDRESS (0x81000000UL)
#define getTagE(tag) ((tag.raw & (1 << 15)) >> 15)
#define START_LABEL     0xbe9

uintptr_t server_stack[0x1000];

L4_ThreadId_t master_tid, pager_tid, ping_tid, pong_tid;
L4_ThreadId_t ping_th, pong_th;
L4_SpaceId_t ping_space, pong_space;

bool fass;
bool fass_buffer;
bool inter;
bool pagertimer;
bool pagertimer_simulated;
bool fault_test;
bool no_utcb_alloc;
bool intra_close;
bool intra_open;
bool intra_rpc;
bool intra_ovh;
bool intra_async;
bool intra_async_ovh;

L4_Word_t ping_stack[2048] __attribute__ ((aligned (16)));
L4_Word_t pong_stack[2048] __attribute__ ((aligned (16)));
L4_Word_t pager_stack[2048] __attribute__ ((aligned (16)));

L4_Word_t fault_area[1] __attribute__ ((aligned (16)));

#define UTCB(x) (L4_Address(utcb_area) + (x) * utcb_size)
#define PONGUTCB(x) (L4_Address(utcb_area) + (x) * utcb_size)
#define NOUTCB  ((void*)-1)
#define START_ADDR(func)    ((L4_Word_t) func)

static int num_iterations = 0;
static int num_mrs = 0;

static inline L4_Word_t
pingpong_ipc (L4_ThreadId_t dest, L4_Word_t untyped)
{
    L4_MsgTag_t tag;
    tag.raw = 0;
    tag.X.u = untyped;
    tag.X.flags = (4 | 8);
   
    L4_ThreadId_t from;
    tag = L4_Ipc(dest, dest, tag, &from);
    return tag.raw;
}

static inline L4_Word_t
pingpong_ipc_close (L4_ThreadId_t dest, L4_Word_t untyped)
{
    L4_MsgTag_t tag;
    tag.raw = 0;
    tag.X.u = untyped;      //Untyped item number
    tag.X.flags = (4 | 8);  //SRBLOCK
   
    L4_ThreadId_t from;
    tag = L4_Ipc(dest, dest, tag, &from);
    return tag.raw;
}

static inline L4_Word_t
pingpong_ipc_open (L4_ThreadId_t dest, L4_Word_t untyped)
{
    L4_MsgTag_t tag;
    tag.raw = 0;
    tag.X.u = untyped;      //Untyped item number
    tag.X.flags = (4 | 8);  //SRBLOCK
   
    L4_ThreadId_t from;
    tag = L4_Ipc(dest, L4_anythread, tag, &from);
    return tag.raw;
}

static inline L4_Word_t
pingpong_ipc_rpc_server(L4_ThreadId_t dest, L4_Word_t untyped,
        L4_ThreadId_t *cap)
{
    L4_MsgTag_t tag;
    tag.raw = 0;
    tag.X.u = untyped;      //Untyped item number
    tag.X.flags = (4);      //RBLOCK

    tag = L4_Ipc(dest, L4_anythread, tag, cap);
    return tag.raw;
}

static inline L4_Word_t
pingpong_ipc_ovh (L4_ThreadId_t dest, L4_Word_t untyped)
{
    L4_MsgTag_t tag;
    tag.raw = 0;
    tag.X.u = untyped;      //Untyped item number
    tag.X.flags = (4 | 8);  //SRBLOCK
    return tag.raw;
}

static inline L4_Word_t
pingpong_ipc_async (void)
{
    /* Wait for ping thread to send async ipc */
    //L4_ThreadId_t from;
    
    L4_Set_NotifyBits(0x55550000UL);
    //L4_Set_NotifyMask(0xaaaaffffUL);
    L4_Accept(L4_NotifyMsgAcceptor);

    L4_MsgTag_t tag;
    /*
    tag.raw = 0;
    tag.X.flags = (4 | 8);  //SRBLOCK

    tag = L4_Ipc(L4_nilthread, L4_anythread, tag, &from);
    */
    tag = L4_WaitNotify(0x0L);

    //if (getTagE(tag))
    //    printf("Open L4_call error %lx\n", L4_ErrorCode());
    //L4_Word_t data = 0;
    //L4_StoreMR(1, &data);
    //printf("Get message %lx, notifybits %lx\n", data, L4_Get_NotifyBits());

    //L4_Receive(pong_tid);

    return tag.raw;
}

static inline L4_Word_t
pingpong_ipc_async_ovh (void)
{
    L4_Set_NotifyBits(0x55550000UL);
    L4_Accept(L4_NotifyMsgAcceptor);
    L4_MsgTag_t tag;

    tag = L4_WaitNotify(0x0L);

    return tag.raw;
}
/* PONG STUFF */

static inline L4_Word_t pingpong_ipc_fass (L4_ThreadId_t dest, L4_Word_t untyped) /*__attribute__((section(".pongcode")))*/;

L4_Word_t pong_stack_fass[2048] __attribute__ ((aligned (16))) /*__attribute__((section(".pongdata")))*/;

static inline L4_Word_t
pingpong_ipc_fass (L4_ThreadId_t dest, L4_Word_t untyped)
{
    L4_MsgTag_t tag;
    tag.raw =  0;
    tag.X.u = untyped;
    tag.X.flags = (4 | 8);

    L4_ThreadId_t from;
    tag = L4_Ipc(dest, dest, tag, &from);
    return tag.raw;
}

void pong_thread_fass (void) /*__attribute__ ((section(".pongcode")))*/;

void
pong_thread_fass (void)
{
    L4_Word_t untyped = 0;

    for (;;) {
        untyped = pingpong_ipc_fass(ping_tid, untyped);
    }
}

static void pong_thread_faulter (void) /*__attribute__ ((section(".pongcode")))*/;

static void
pong_thread_faulter (void)
{
    volatile L4_Word_t x;
    x = fault_area[0];

    for (;;)
        L4_WaitForever();

    /* NOTREACHED */
}

static void
pong_thread (void)
{
    L4_Word_t untyped = 0;

    for (;;) {
        untyped = pingpong_ipc(ping_tid, untyped);
    }
}

static void
pong_thread_close (void)
{
    L4_Word_t untyped = 0;

    for (;;) {
        /*untyped = */pingpong_ipc_close(ping_tid, untyped);
    }
}

static void
pong_thread_open (void)
{
    L4_Word_t untyped = 0;

    for (;;) {
        /*untyped = */pingpong_ipc_open(ping_tid, untyped);
    }
}

static void
pong_thread_ovh (void)
{
    L4_Word_t untyped = 0;

    for (int i = 0; i < num_iterations; i++) {
        pingpong_ipc_ovh(ping_tid, untyped);
    }

    L4_Send(ping_tid);
    for (;;)
            L4_WaitForever();
    /* NOTREACHED */
}

static void
pong_thread_async (void)
{
    for (;;) {
        pingpong_ipc_async();
    }
}

static void
pong_thread_async_ovh (void)
{
    for (;;) {
        pingpong_ipc_async_ovh();
    }
}

static void
pong_thread_buffer (void)
{
    L4_Word_t untyped = 0;
    volatile L4_Word_t x;

    for (;;) {
        untyped = pingpong_ipc(ping_tid, untyped);
        x = fault_area[0];
    }
}

/* END PONG STUFF */

static void
ping_thread (void)
{
    /* Wait for pong thread to come up */
    L4_Receive (pong_tid);

    for (int i=0; i < num_iterations; i++) {
        pingpong_ipc (pong_tid, num_mrs);
    }
    /* Tell master that we're finished */
    L4_Set_MsgTag (L4_Niltag);
    L4_Send (master_tid);

    for (;;)
        L4_WaitForever();

    /* NOTREACHED */
}

static void
ping_thread_close (void)
{
    /* Wait for pong thread to come up */
    L4_Receive (pong_tid);

    for (int i=0; i < num_iterations; i++) {
        pingpong_ipc_close (pong_tid, num_mrs);
    }
    /* Tell master that we're finished */
    L4_Set_MsgTag (L4_Niltag);
    L4_Send (master_tid);

    for (;;)
        L4_WaitForever();

    /* NOTREACHED */
}

static void
ping_thread_open (void)
{
    /* Wait for pong thread to come up */
    L4_Receive (pong_tid);

    for (int i=0; i < num_iterations; i++) {
        pingpong_ipc_open (pong_tid, num_mrs);
    }
    /* Tell master that we're finished */
    L4_Set_MsgTag (L4_Niltag);
    L4_Send (master_tid);

    for (;;)
        L4_WaitForever();

    /* NOTREACHED */
}

static void
ping_thread_rpc_server(void)
{
    int i;
    L4_ThreadId_t cap;

    /* Wait for pong thread to come up */
    L4_Wait(&cap);

    for (i = 0; i < num_iterations; i++) {
        pingpong_ipc_rpc_server(cap, num_mrs, &cap);
    }
    /* Tell master that we're finished */
    L4_Set_MsgTag(L4_Niltag);
    L4_Send(master_tid);

    L4_WaitForever();
    /* NOTREACHED */
}

static void
ping_thread_ovh (void)
{
    /* Wait for pong thread to come up */
    L4_Receive (pong_tid);
    for (int i=0; i < num_iterations; i++) {
        pingpong_ipc_ovh (pong_tid, num_mrs);
    }
    /* Tell master that we're finished */
    L4_Set_MsgTag (L4_Niltag);
    L4_Send (master_tid);

    for (;;)
        L4_WaitForever();

    /* NOTREACHED */
}

static void
ping_thread_async (void)
{
    L4_MsgTag_t tag;
    //tag.raw = 0;
    //tag.X.u = 1;      //Untyped item number
    //tag.X.flags = (4 | 8 | 2);  //SRBLOCK, notify

    //L4_ThreadId_t from;

    for (int i=0; i < num_iterations; i++)
    {
        //L4_LoadMR(1, 0xffff1234UL);
        //tag = L4_Ipc(dest, L4_nilthread, tag, &from);
        tag = L4_Notify(pong_tid, 0xffff1234UL);
        //if (getTagE(tag))
        //    printf("Open L4_call error %lx\n", L4_ErrorCode());
        /*
        tag.raw = 0;
        tag.X.flags = (4 | 8);
        tag = L4_Ipc(dest, L4_nilthread, tag, &from);
        */
        //if (getTagE(tag))
        //    printf("Open L4_call error %lx\n", L4_ErrorCode());

        //return tag.raw;
    }
    
    /* Tell master that we're finished */
    L4_Set_MsgTag (L4_Niltag);
    L4_Send (master_tid);

    for (;;)
        L4_WaitForever();

    /* NOTREACHED */
}

static void
ping_thread_async_ovh (void)
{
    for (int i=0; i < num_iterations; i++)
    { ; }
    
    /* Tell master that we're finished */
    L4_Set_MsgTag (L4_Niltag);
    L4_Send (master_tid);

    for (;;)
        L4_WaitForever();

    /* NOTREACHED */
}

static void
ping_thread_buffer (void)
{
    volatile L4_Word_t x;

    /* Wait for pong thread to come up */
    L4_Receive (pong_tid);

    for (int i=0; i < num_iterations; i++) {
        pingpong_ipc (pong_tid, num_mrs);
        x = fault_area[0];
    }

    /* Tell master that we're finished */
    L4_Set_MsgTag (L4_Niltag);
    L4_Send (master_tid);

    for (;;)
        L4_WaitForever();

    /* NOTREACHED */
}

static void
ping_thread_pager (void)
{
    volatile L4_Word_t x;
    /* Wait for pong thread to come up */
    for (int i=0; i < num_iterations; i++) {
        x = *  (L4_Word_t*) ( ((uintptr_t) &fault_area[i*1024]) + (30 * 1024 * 1024));
    }

    /* Tell master that we're finished */
    L4_Set_MsgTag (L4_Niltag);
    L4_Send (master_tid);

    for (;;)
        L4_WaitForever();

    /* NOTREACHED */
}

#define L4_PAGEFAULT        (-2UL << 20)

static void
ping_thread_simulated (void)
{
    volatile L4_Word_t x;
    /* Wait for pong thread to come up */
    L4_Msg_t msg;
    L4_MsgTag_t tag;
    for (int i=0; i < num_iterations; i++) {
        L4_Fpage_t ppage = L4_Fpage((L4_Word_t)&fault_area[i*1024], 4096);

        L4_Set_Rights(&ppage, L4_FullyAccessible);
        /* accept fpages */
        L4_Accept(L4_UntypedWordsAcceptor);

        /* send it to our pager */
        L4_MsgClear(&msg);
        L4_MsgAppendWord(&msg, (L4_Word_t) ppage.raw);
        L4_MsgAppendWord(&msg, (L4_Word_t) 0);
        L4_Set_Label(&msg.tag, L4_PAGEFAULT);
        L4_MsgLoad(&msg);

        /* make the call */
        tag = L4_Call(pager_tid);
        x = fault_area[i*1024];
    }

    /* Tell master that we're finished */
    L4_Set_MsgTag (L4_Niltag);
    L4_Send (master_tid);

    for (;;)
        L4_WaitForever();

    /* NOTREACHED */
}

static void
send_startup_ipc (L4_ThreadId_t tid, L4_Word_t ip, L4_Word_t sp)
{
    L4_Msg_t msg;
    L4_MsgClear(&msg);
    L4_MsgAppendWord(&msg, ip);
    L4_MsgAppendWord(&msg, sp);
    L4_MsgLoad(&msg);
    L4_Send(tid);
}

extern word_t get_seg(L4_SpaceId_t spaceid, word_t vaddr, word_t *offset, word_t *cache, word_t *rwx);

static void
pager (void)
{
    L4_ThreadId_t tid;
    L4_MsgTag_t tag;
    L4_Msg_t msg;
    int count = 0;

    for (;;) {
        tag = L4_Wait(&tid);

        for (;;) { 
            L4_Word_t faddr, fip;
            L4_MsgStore(tag, &msg);

            if (L4_Label(tag) == START_LABEL) {
                // Startup notification, start ping and pong thread
                void (*start_addr)(void);
                void (*pong_start_addr)(void);
                L4_Word_t *pong_stack_addr = pong_stack;
                if (pagertimer) {
                    start_addr = ping_thread_pager;
                } else if (pagertimer_simulated) {
                    start_addr = ping_thread_simulated;
                } else if (fass_buffer) {
                    start_addr = ping_thread_buffer;
                } else if (fault_test) {
                    count = 0;
                    start_addr = NULL;
                } else if (intra_close) {
                    start_addr = ping_thread_close;
                } else if (intra_open) {
                    start_addr = ping_thread_open;
                } else if (intra_rpc) {
                    start_addr = ping_thread_rpc_server;
                } else if (intra_ovh) {
                    start_addr = ping_thread_ovh;
                } else if (intra_async) {
                    start_addr = ping_thread_async;
                } else if (intra_async_ovh) {
                    start_addr = ping_thread_async_ovh;
                } else {
                    start_addr = ping_thread;
                }

                if (start_addr != NULL) {
                    /*printf("ping_start_addr: %lx ping_stack_addr: %lx\n",
                           START_ADDR (start_addr), (L4_Word_t) ping_stack);*/
                    send_startup_ipc (ping_tid,
                              START_ADDR(start_addr),
                              (L4_Word_t) ping_stack +
                              sizeof (ping_stack) - 32);
                    L4_ThreadSwitch(ping_tid);
                }

                if (fass_buffer) {
                    pong_start_addr = pong_thread_buffer;
                    pong_stack_addr = pong_stack_fass;
                } else if (fass) {
                    pong_start_addr = pong_thread_fass;
                    pong_stack_addr = pong_stack_fass;
                } else if (fault_test) {
                    pong_stack_addr = pong_stack_fass;
                    pong_start_addr = pong_thread_faulter;
                } else if (intra_close) {
                    pong_start_addr = pong_thread_close;
                } else if (intra_open) {
                    pong_start_addr = pong_thread_open;
                } else if (intra_rpc) {
                    pong_start_addr = pong_thread_close;
                } else if (intra_ovh) {
                    pong_start_addr = pong_thread_ovh;
                } else if (intra_async) {
                    pong_start_addr = pong_thread_async;
                } else if (intra_async_ovh) {
                    pong_start_addr = pong_thread_async_ovh;
                } else {
                    pong_start_addr = pong_thread;
                }

                if (!pagertimer) {
                    /*printf("pong_start_addr: %lx pong_stack_addr: %lx\n",
                           START_ADDR (pong_start_addr), (L4_Word_t) pong_stack_addr);*/
                    L4_Set_Priority(ping_tid, 100);
                    L4_Set_Priority(pong_tid, 99);
                    send_startup_ipc (pong_tid, 
                              START_ADDR (pong_start_addr),
                              (L4_Word_t) pong_stack_addr + sizeof (ping_stack) - 32);
                }
                break;
            }


            if (L4_UntypedWords (tag) != 2 ||
                !L4_IpcSucceeded (tag)) {
                printf ("pingpong: malformed pagefault IPC from %p (tag=%p)\n",
                    (void *) tid.raw, (void *) tag.raw);
                L4_KDB_Enter ("malformed pf");
                break;
            }

            faddr = L4_MsgWord(&msg, 0);
            fip   = L4_MsgWord (&msg, 1);
            L4_MsgClear(&msg);

            if (fault_test && (faddr == (uintptr_t) fault_area)) {
                if (count < num_iterations) {
                    count++;
                } else {
                    /* Tell master that we're finished */
                    L4_Set_MsgTag (L4_Niltag);
                    L4_Send (master_tid);
                    break;
                }
            } else {
                L4_MapItem_t map;
                L4_SpaceId_t space;
                L4_Word_t seg, offset, cache, rwx, size;
                int r;

                seg = get_seg(KBENCH_SPACE, faddr, &offset, &cache, &rwx);
                //if can not find mapping, must be page fault test,
                //just map any valid address, since fault address is dummy.
                if (seg == ~0UL)
                    seg = get_seg(KBENCH_SPACE, (L4_Word_t) fault_area,
                                  &offset, &cache, &rwx);

                if (tid.raw == ping_th.raw)
                    space = ping_space;
                else if (tid.raw == pong_th.raw)
                {
                    if (pong_space.raw != L4_nilspace.raw)
                        space = pong_space;
                    else //pong_space is not created, only ping_space is used.
                        space = ping_space;
                }
                else
                    space = KBENCH_SPACE;

                size = L4_GetMinPageBits();
                faddr &= ~((1ul << size)-1);
                offset &= ~((1ul << size)-1);

                L4_MapItem_Map(&map, seg, offset, faddr, size,
                               cache, rwx);
                r = L4_ProcessMapItem(space, map);
                assert(r == 1);
            }
            L4_MsgLoad(&msg);
            tag = L4_ReplyWait (tid, &tid);
        }
    }
}

L4_Fpage_t pong_utcb_area, utcb_area;
L4_Word_t utcb_size;

extern okl4_kspaceid_pool_t *spaceid_pool;

static void
ipc_setup(struct bench_test *test, int args[])
{
    static bool setup = false;
    int r;
    L4_Word_t utcb;
    fass = (test == &bench_ipc_fass);
    pagertimer = (test == &bench_ipc_pagemap);
    pagertimer_simulated = (test == &bench_ipc_pagemap_simulated);
    inter = (test == &bench_ipc_inter);
    fass_buffer = ((test == &bench_ipc_fass_buffer) || (test == &bench_ipc_fass_buffer_vspace));
    fault_test = (test == &bench_ipc_page_faults);
    intra_open = (test == &bench_ipc_intra_open);
    intra_close = (test == &bench_ipc_intra_close);
    intra_rpc = (test == &bench_ipc_intra_rpc);
    intra_ovh = (test == &bench_ipc_intra_ovh);
    intra_async = (test == &bench_ipc_intra_async);
    intra_async_ovh = (test == &bench_ipc_intra_async_ovh);

    num_iterations = args[0];
    num_mrs = args[1];

    ping_space = pong_space = L4_nilspace;

    if (! setup) {
        /* Find smallest supported page size. There's better at least one
         * bit set. */

        /* Size for one UTCB */
        utcb_size = L4_GetUtcbSize();

        /* We need a maximum of two threads per task */
#ifdef NO_UTCB_RELOCATE
        no_utcb_alloc = 1;
        utcb_area = L4_Nilpage;
        if (fass) {
            pong_utcb_area = L4_Nilpage;
        }
#else
        no_utcb_alloc = 0;
        utcb_area = L4_Fpage((L4_Word_t) UTCB_ADDRESS,
                             L4_GetUtcbAreaSize() + 1);
        if (fass) {
            pong_utcb_area = L4_Fpage ((L4_Word_t) UTCB_ADDRESS,
                                       L4_GetUtcbAreaSize() + 1);
        }
#endif

        /* Create pager */
        master_tid.raw = KBENCH_SERVER.raw;
        pager_tid.raw = KBENCH_SERVER.raw + 1;
        ping_tid.raw  = KBENCH_SERVER.raw + 2;
        pong_tid.raw  = KBENCH_SERVER.raw + 3;

        /* VU: calculate UTCB address -- this has to be revised */
        /** @todo FIXME: Should put into arch subdir - changhua. */
#if defined(ARCH_ARM)
        L4_Word_t pager_utcb = (L4_Word_t) __L4_ARM_Utcb();
#elif defined(ARCH_IA32)
        L4_Word_t pager_utcb = (L4_Word_t) __L4_X86_Utcb();
#else
        #error "Please define arch get_Utcb()"
#endif
        pager_utcb = no_utcb_alloc ? ~0UL : (pager_utcb & ~(utcb_size - 1)) + utcb_size;
        r = L4_ThreadControl (pager_tid, KBENCH_SPACE, master_tid, master_tid, 
                      master_tid, 0, (void*)pager_utcb);
        if (r == 0) printf("Thread create Error: %lx\n", L4_ErrorCode());
        assert(r == 1);
        L4_KDB_SetThreadName(pager_tid, "pager");
        L4_Start_SpIp (pager_tid, (L4_Word_t) pager_stack + sizeof(pager_stack) - 32, START_ADDR (pager));
        /* Find some area of memory to page to */
        setup = true;
    }

    if (pagertimer) {
        /* Only create ping space and ping thread. */
        r = okl4_kspaceid_allocany(spaceid_pool, &ping_space);
        assert(r == OKL4_OK);
        r = L4_SpaceControl(ping_space, L4_SpaceCtrl_new, KBENCH_CLIST, utcb_area, 0, NULL);
        assert(r == 1);

        utcb = no_utcb_alloc ? ~0UL : UTCB(0);

        r = L4_ThreadControl(ping_tid, ping_space, master_tid, pager_tid, pager_tid, 0, (void *) utcb);
        L4_StoreMR(0, &ping_th.raw);
        assert(r == 1);
    } else if (fault_test) {
        /* Only create pong space and pong thread. */
        r = okl4_kspaceid_allocany(spaceid_pool, &pong_space);
        assert(r == OKL4_OK);
        r = L4_SpaceControl(pong_space, L4_SpaceCtrl_new, KBENCH_CLIST, utcb_area, 0, NULL);
        assert(r == 1);

        utcb = no_utcb_alloc ? ~0UL : UTCB(0);

        r = L4_ThreadControl(pong_tid, pong_space, master_tid, pager_tid, pager_tid, 0, (void *) utcb);
        L4_StoreMR(0, &pong_th.raw);
        assert(r == 1);
    } else if (pagertimer_simulated || inter || fass || fass_buffer) {
        /* Create both ping, pong space, and create ping, pong thread in their own space */
        L4_Word_t ctrl = 0;
        if (test == &bench_ipc_fass_buffer_vspace) {
            ctrl = (1 << 16);
        }
        r = okl4_kspaceid_allocany(spaceid_pool, &ping_space);
        assert(r == OKL4_OK);

        r = L4_SpaceControl(ping_space, L4_SpaceCtrl_new, KBENCH_CLIST, utcb_area, ctrl, NULL);
        if (r == 0) printf("Space control Error: 0x%lx\n", L4_ErrorCode());
        assert( r == 1 );
        r = okl4_kspaceid_allocany(spaceid_pool, &pong_space);
        assert(r == OKL4_OK);
        r = L4_SpaceControl(pong_space, L4_SpaceCtrl_new, KBENCH_CLIST, (fass ? pong_utcb_area : utcb_area), ctrl, NULL);
        assert( r == 1);

        utcb = no_utcb_alloc ? ~0UL : UTCB(0);

        r = L4_ThreadControl(ping_tid, ping_space, master_tid, pager_tid, pager_tid, 0, (void *) utcb);
        L4_StoreMR(0, &ping_th.raw);
        assert( r == 1);

        utcb = no_utcb_alloc ? ~0UL : fass ? PONGUTCB(1) : UTCB(1);

        r = L4_ThreadControl(pong_tid, pong_space, master_tid, pager_tid, pager_tid, 0, (void *)utcb);
        L4_StoreMR(0, &pong_th.raw);
    } else {
        /* Only Create ping space, but create both ping, pong thread in that space. */
        r = okl4_kspaceid_allocany(spaceid_pool, &ping_space);
        assert(r == OKL4_OK);
        r = L4_SpaceControl(ping_space, L4_SpaceCtrl_new, KBENCH_CLIST, utcb_area, 0, NULL);
        assert( r == 1 );

        utcb = no_utcb_alloc ? ~0UL : UTCB(0);

        r = L4_ThreadControl(ping_tid, ping_space, master_tid, pager_tid, pager_tid, 0, (void *) utcb);
        L4_StoreMR(0, &ping_th.raw);
        assert( r == 1);

        utcb = no_utcb_alloc ? ~0UL : UTCB(1);

        r = L4_ThreadControl(pong_tid, ping_space, master_tid, pager_tid, pager_tid, 0, (void *) utcb);
        L4_StoreMR(0, &pong_th.raw);
        assert( r == 1);
    }

    L4_KDB_SetThreadName(ping_tid, "ping");
    if (test != &bench_ipc_pagemap) {
        L4_KDB_SetThreadName(pong_tid, "pong");
    }
}

static void
ipc_test(struct bench_test *test, int args[])
{
    L4_ThreadId_t  tid;

    /* Send "begin" message to notify pager to startup both threads */
    L4_MsgTag_t tag = L4_Niltag;
    L4_Set_Label(&tag, START_LABEL);
    L4_Set_MsgTag(tag);
    L4_Send(pager_tid);
    L4_Wait(&tid);
}

static void
ipc_test_fault(struct bench_test *test, int args[])
{
    /* Send empty message to notify pager to startup both threads */
    L4_LoadMR (0, 0);
    L4_Send (pager_tid);
    L4_Receive (pager_tid);
}

static void
ipc_teardown(struct bench_test *test, int args[])
{
    int r = 0;
    if (!fault_test)
    {
        /* Kill ping thread */
        r = L4_ThreadControl (ping_tid, L4_nilspace, L4_nilthread,
                  L4_nilthread, L4_nilthread, 0, (void *) 0);
        if (r == 0) printf("thread control Error: %lx\n", L4_ErrorCode());
        assert (r == 1);
    }

    if (!pagertimer)
    {
        /* Kill pong thread */
        r = L4_ThreadControl (pong_tid, L4_nilspace, L4_nilthread,
              L4_nilthread, L4_nilthread, 0, (void *) 0);
        assert (r == 1);
    }
    /* Delete ping space */
    if (ping_space.raw != L4_nilspace.raw)
    {
         r = L4_SpaceControl(ping_space, L4_SpaceCtrl_delete, KBENCH_CLIST, L4_Nilpage, 0, NULL);
         assert (r == 1);
         okl4_kspaceid_free(spaceid_pool, ping_space);
    }
    /* Delete pong space */
    if (pong_space.raw != L4_nilspace.raw)
    {
        r = L4_SpaceControl(pong_space, L4_SpaceCtrl_delete, KBENCH_CLIST, L4_Nilpage, 0, NULL);
        assert (r == 1);
         okl4_kspaceid_free(spaceid_pool, pong_space);
    }
}

struct index_type payload_size = { "Payload size", "registers" };

struct bench_test bench_ipc_page_faults = {
    "ipc - page faults", ipc_setup, ipc_test_fault, ipc_teardown, 
    { 
#if (defined(ARCH_ARM) && (ARCH_VER == 5))
        { &iterations, 10000, 10000, 500, add_fn },
#else
        { &iterations, 100000, 100000, 500, add_fn },
#endif
        { NULL, 0, 0, 0 }
    }
};


struct bench_test bench_ipc_pagemap_simulated = {
    "ipc - pagemap (simulated)", ipc_setup, ipc_test, ipc_teardown, 
    { 
        { &iterations, 2500, 2500, 500, add_fn }, 
        /*        { &payload_size, 0, 64, 4, add_fn }, */
        { NULL, 0, 0, 0 }
    }
      
};

struct bench_test bench_ipc_pagemap = {
    "ipc - pagemap (faulting)", ipc_setup, ipc_test, ipc_teardown, 
    { 
        { &iterations, 2500, 2500, 500, add_fn }, 
        /*        { &payload_size, 0, 64, 4, add_fn }, */
        { NULL, 0, 0, 0 }
    }
      
};

struct bench_test bench_ipc_inter = {
    "ipc - inter", ipc_setup, ipc_test, ipc_teardown, 
    { 
        { &iterations, 10000, 10000, 1000, add_fn }, 
        /*        { &payload_size, 0, 64, 4, add_fn }, */
        { NULL, 0, 0, 0 }
    }
};

struct bench_test bench_ipc_intra = {
    "ipc - intra", ipc_setup, ipc_test, ipc_teardown, 
    { 
        { &iterations, 10000, 10000, 1000, add_fn }, 
        /*        { &payload_size, 0, 64, 4, add_fn }, */
        { NULL, 0, 0, 0 }
    }
};

struct bench_test bench_ipc_fass = {
    "ipc - fass", ipc_setup, ipc_test, ipc_teardown, 
    { 
        { &iterations, 10000, 10000, 1000, add_fn }, 
        /*        { &payload_size, 0, 64, 4, add_fn }, */
        { NULL, 0, 0, 0 }
    }
};

struct bench_test bench_ipc_fass_buffer = {
    "ipc - fass w/buffer", ipc_setup, ipc_test, ipc_teardown, 
    { 
        { &iterations, 10000, 10000, 1000, add_fn }, 
        /*        { &payload_size, 0, 64, 4, add_fn }, */
        { NULL, 0, 0, 0 }
    }
};

struct bench_test bench_ipc_fass_buffer_vspace = {
    "ipc - fass w/buffer vspace", ipc_setup, ipc_test, ipc_teardown, 
    { 
        { &iterations, 10000, 10000, 1000, add_fn }, 
        /*        { &payload_size, 0, 64, 4, add_fn }, */
        { NULL, 0, 0, 0 }
    }
};

/*
 * What we can get from bench_ipc_intra_*:
 * Open L4 call latency = (bench of ipc_intra_open - 
 * bench of ipc_intra_ovh) / 20000
 * Close L4 call latency = (bench of ipc_intra_close -
 * bench of ipc_intra_ovh) / 20000
 */
struct bench_test bench_ipc_intra_close = {
    "ipc - close call", ipc_setup, ipc_test, ipc_teardown,
    {
        { &iterations, 10000, 10000, 1000, add_fn },
        { NULL, 0, 0, 0 }
    }
};

struct bench_test bench_ipc_intra_open = {
    "ipc - open call", ipc_setup, ipc_test, ipc_teardown,
    {
        { &iterations, 10000, 10000, 1000, add_fn },
        { NULL, 0, 0, 0 }
    }
};

struct bench_test bench_ipc_intra_rpc = {
    "ipc - RPC", ipc_setup, ipc_test, ipc_teardown,
    {
        { &iterations, 10000, 10000, 1000, add_fn },
        { NULL, 0, 0, 0 }
    }
};

struct bench_test bench_ipc_intra_ovh = {
    "ipc - call overhead", ipc_setup, ipc_test, ipc_teardown,
    {
        { &iterations, 10000, 10000, 1000, add_fn },
        { NULL, 0, 0, 0 }
    }
};

/*
 * What we can get from bench_ipc_intra_asyc_*:
 * Async L4 IPC latency + thread switch latency = (bench of ipc_intra_asyn - 
 * bench of ipc_intra_async_ovh) /10000
 */
struct bench_test bench_ipc_intra_async = {
    "ipc - async send", ipc_setup, ipc_test, ipc_teardown,
    {
        { &iterations, 10000, 10000, 1000, add_fn },
        { NULL, 0, 0, 0 }
    }
};

struct bench_test bench_ipc_intra_async_ovh = {
    "ipc - async test overhead", ipc_setup, ipc_test, ipc_teardown,
    {
        { &iterations, 10000, 10000, 1000, add_fn },
        { NULL, 0, 0, 0 }
    }
};
