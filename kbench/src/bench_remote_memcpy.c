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
 * Last Modified: ChangHua Chen  Aug 2 2004
 */
#include <stdlib.h>
#include <string.h>
#include <bench/bench.h>
#include <l4/ipc.h>
#include <l4/config.h>
#include <stdio.h>
#include <assert.h>
#include <l4/kdebug.h>

#include <l4/schedule.h>
#include <l4/space.h>
#include <l4/thread.h>
#include <okl4/kspaceid_pool.h>
#include <okl4/bitmap.h>

#define UTCB_ADDRESS    16 * 1024 * 1024
#define START_ADDR(func)    ((L4_Word_t) func)

/* Basic tags specifying send and receive blocking */
#define TAG_NOBLOCK 0x00000000
#define TAG_RBLOCK  0x00004000
#define TAG_SBLOCK  0x00008000
#define TAG_SRBLOCK 0x0000C000

#define COPYING_THREAD_BUFSIZE 0x8000
#define COPYING_THREAD_PATTERN 0x10


static char copying_thread_buffer[0x100000] __attribute__ ((aligned (16)));
char *walrus;

static L4_Word_t dummy_stack[2*1024] __attribute__ ((aligned (16)));
static L4_Word_t pager_stack[2*1024] __attribute__ ((aligned (16)));

L4_Word_t utcb;
L4_Word_t utcb_size;
L4_Fpage_t utcb_area;

L4_SpaceId_t space_id;
L4_ThreadId_t pager_tid;
L4_ThreadId_t test_tid;

word_t copy_size;
word_t iteration;

L4_MsgTag_t tag;
L4_ThreadId_t from;


static void dummy_thread(void)
{
        L4_MsgTag_t tag;

        ARCH_THREAD_INIT

        memset(copying_thread_buffer, COPYING_THREAD_PATTERN,
                COPYING_THREAD_BUFSIZE);

        tag = L4_Niltag;
        L4_Set_MemoryCopy(&tag);

        L4_LoadMR(0, tag.raw);
        L4_LoadMR(1, (word_t)copying_thread_buffer);
        L4_LoadMR(2, COPYING_THREAD_BUFSIZE);
        L4_LoadMR(3, L4_MemoryCopyBoth);

        tag = L4_Call(KBENCH_SERVER);
        assert(L4_IpcSucceeded(tag));
}

extern word_t get_seg(L4_SpaceId_t spaceid, word_t vaddr, word_t *offset, word_t *cache, word_t *rwx);

static void
pager (void)
{
    L4_ThreadId_t tid;
    L4_MsgTag_t tag;
    L4_Msg_t msg;

    //L4_Send(KBENCH_SERVER);
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
                //printf("Fault at %lx faultIp %lx\n", faddr, fip);
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

extern okl4_kspaceid_pool_t *spaceid_pool;

static void
init(struct bench_test *test, int args[])
{
    int r;

    copy_size = args[1];
    iteration = args[0];

    /* Size for one UTCB */
    utcb_size = L4_GetUtcbSize();

    /* We need a maximum of two threads per task */
#ifdef NO_UTCB_RELOCATE
    utcb_area = L4_Nilpage;
#else
    utcb_area = L4_Fpage((L4_Word_t) UTCB_ADDRESS,
                         L4_GetUtcbAreaSize());
#endif
    /* Create pager */
    pager_tid = L4_GlobalId (KBENCH_SERVER.X.index + 1, 2);

#ifdef NO_UTCB_RELOCATE
    utcb = -1UL;
#else
    utcb = (L4_Word_t) (L4_PtrSize_t)L4_GetUtcbBase() + (2)*utcb_size;
#endif
    r = L4_ThreadControl (pager_tid, KBENCH_SPACE, KBENCH_SERVER, KBENCH_SERVER,
                          KBENCH_SERVER, 0, (void*)utcb);
    if (r == 0) {
        printf("pager thread_setup Error: 0x%lx\n", L4_ErrorCode());
    }
    assert(r == 1);

    //L4_KDB_SetThreadName(pager_tid, "pager");
    L4_Set_Priority(pager_tid, 254);
    L4_Start_SpIp (pager_tid, (L4_Word_t) pager_stack + sizeof(pager_stack) - 32, START_ADDR (pager));

    /* Create space */
    r = okl4_kspaceid_allocany(spaceid_pool, &space_id);
    assert(r == OKL4_OK);

    r = L4_SpaceControl(space_id, L4_SpaceCtrl_new, KBENCH_CLIST, utcb_area, 0, NULL);
    if (r != 1)
        printf("Couldn't create space - error is %ld\n", L4_ErrorCode());

    assert(r == 1);

    test_tid = L4_GlobalId (KBENCH_SERVER.X.index + 2, 2);

    walrus = malloc(COPYING_THREAD_BUFSIZE);

    memset(walrus, 0x0, COPYING_THREAD_BUFSIZE);

    /* Create test thread for remote memcopy */
#ifdef NO_UTCB_RELOCATE
    utcb = -1UL;
#else
    utcb = (L4_Word_t) ((L4_PtrSize_t) UTCB_ADDRESS + utcb_size);
#endif
#ifdef NO_UTCB_RELOCATE
    r = L4_ThreadControl(test_tid, space_id, KBENCH_SERVER,
                         pager_tid, pager_tid, 0, (void *) ~0UL);
#else
    r = L4_ThreadControl(test_tid, space_id, KBENCH_SERVER,
                         pager_tid, pager_tid, 0, (void *)utcb);
#endif
    if (r == 0)
    {
        printf("Create thread Error: 0x%lx\n", L4_ErrorCode());
    }
    assert(r == 1);

    L4_Start_SpIp(test_tid, (L4_Word_t) dummy_stack + sizeof(dummy_stack) - 32, START_ADDR(dummy_thread));

    L4_ThreadSwitch(test_tid);

    tag = L4_Wait(&from);
}

static void
remote_memcpy_test(struct bench_test *test, int args[])
{
        int i, r = 1;

        for(i = 0; i < iteration; i++) {
                r &= L4_MemoryCopy(from, (word_t)walrus, &copy_size, L4_MemoryCopyTo);
        }
        assert(r == 1);
        assert(copy_size == args[1]);
}

static void
teardown(struct bench_test *test, int args[])
{
        int i, r = 0;
        int diff = 0;

        /* check the final result */
        for(i = 0; i < copy_size; i++) {
                if (walrus[i] != COPYING_THREAD_PATTERN) {
                        diff = 1;
                }
        }
        if (diff) {
                printf("INCORRECT PATTERN %3d: walrus[%d] = 0x%x !!!!\n", r, i, walrus[i]);
        }

        free(walrus);

        /* Delete pager */
        r = L4_ThreadControl(pager_tid, L4_nilspace, L4_nilthread,
                                L4_nilthread, L4_nilthread, 0, (void *) 0);
        assert(r == 1);

        //Delete test thread
        r = L4_ThreadControl(test_tid, L4_nilspace, L4_nilthread, L4_nilthread,
                                        L4_nilthread, 0, (void *)0);
        assert(r == 1);

        //Delete space
        r = L4_SpaceControl(space_id, L4_SpaceCtrl_delete, KBENCH_CLIST, L4_Nilpage, 0, NULL);
        assert(r == 1);
        okl4_kspaceid_free(spaceid_pool, space_id);
}

struct bench_test bench_remote_memcpy = {
    .name = "remote_memcpy",
    .init = init,
    .test = remote_memcpy_test,
    .teardown = teardown,
    .indices = {
        { &iterations, 1000, 1000, 500, add_fn },
        { &mem_size, 4, 32*1024, 2, mul_fn },
        { NULL, 0, 0, 0 }
    }
};
