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
#include <l4/types.h>
#include <l4/config.h>
#include <l4/thread.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include <l4/kdebug.h>
#include <okl4/kspaceid_pool.h>
#include <bench/bench.h>

struct thread_desc {
    L4_ThreadId_t tid;
    void *utcb;
};

L4_Fpage_t utcb_area;

struct thread_desc desc[10000];
L4_SpaceId_t space_id;

extern okl4_kspaceid_pool_t *spaceid_pool;

static void
threadcontrol_setup(struct bench_test *test, int args[])
{
    int r;
    L4_Word_t old_control;

    L4_ThreadId_t new_tid;
    new_tid = L4_CapId(TYPE_THREAD_HANDLE, KBENCH_SERVER.raw + 1);
    r           = okl4_kspaceid_allocany(spaceid_pool, &space_id);

    assert(r == OKL4_OK);

#ifdef NO_UTCB_RELOCATE
    utcb_area = L4_Nilpage;
#else
    utcb_area = L4_Fpage (16 * 1024 * 1024,
                  L4_GetUtcbSize() * (args[0]+ 1));
#endif

    /* Create space */
    r = L4_SpaceControl(space_id, L4_SpaceCtrl_new, KBENCH_CLIST, utcb_area, 0, &old_control);
    if (r == 0) {
        printf("thread_setup(%d) Error: %"PRIxPTR"\n", args[0], L4_ErrorCode());
    }
    assert(r == 1);
#if 0
    r = L4_ThreadControl(new_tid, space_id, L4_Myself(),
                 L4_nilthread, L4_anythread, 0,
                 no_utcb_allc ? ~0UL : (void*)(0));
    assert(r == 1);
#endif
    for (int i=1; i < args[0] + 1; i++) {
        desc[i].tid = L4_GlobalId(KBENCH_SERVER.raw + i + 1, 2);
        desc[i].utcb = (void*) (16 * 1024 * 1024 + (L4_GetUtcbSize() * i));
    }

}


static void
threadcontrol_test(struct bench_test *test, int args[])
{
    int r = 0;
    for(int i=1; i < args[0] + 1; i++) {
#ifdef NO_UTCB_RELOCATE
        r = L4_ThreadControl(desc[i].tid, space_id, KBENCH_SERVER,
                     L4_nilthread, L4_nilthread, 0, (void *) ~0UL);
#else
        r = L4_ThreadControl(desc[i].tid, space_id, KBENCH_SERVER,
                     L4_nilthread, L4_nilthread, 0, desc[i].utcb);
#endif
        if (r == 0)
        {
            printf("Create thread %d Error: %"PRIxPTR"\n", i, L4_ErrorCode());
            printf("utcb is %p\n", desc[i].utcb);
        }
        assert(r == 1);
    }
}
static void
threadcontrol_ovh(struct bench_test *test, int args[])
{
    int r = 1;
    for(int i=1; i < args[0] + 1; i++) {
        if (r == 0)
        {
        }
        assert(r == 1);
    }

}

static void
threadcontrol_teardown(struct bench_test *test, int args[])
{
    int r;
    for(int i=1; i < args[0] + 1; i++) {
        //Delete threads
        r = L4_ThreadControl(desc[i].tid, L4_nilspace, L4_nilthread, L4_nilthread,
                 L4_nilthread, 0, (void *)0);
        assert(r== 1);
    }
    //Delete space
    r = L4_SpaceControl(space_id, L4_SpaceCtrl_delete, KBENCH_CLIST, L4_Nilpage, 0, NULL);
    assert(r == 1);
    okl4_kspaceid_free(spaceid_pool, space_id);
}

static void
threadcontrol_teardown_dummy(struct bench_test *test, int args[])
{
    int r;
    for(int i=1; i < args[0] + 1; i++) {
    }
    //Delete space
    r = L4_SpaceControl(space_id, L4_SpaceCtrl_delete, KBENCH_CLIST, L4_Nilpage, 0, NULL);
    assert(r == 1);
    okl4_kspaceid_free(spaceid_pool, space_id);
}

/*
 * From bench_threadcontrol_*, we can get:
 * syscall threadcontrol latency = (bench of threadcontrol -
 * bench of threadcontrol_ovh) / iterations
 */
struct bench_test bench_threadcontrol = {
    "threadcontrol",
    threadcontrol_setup,
    threadcontrol_test,
    threadcontrol_teardown,
    {
#if defined(CONFIG_CPU_ARM_XSCALE)
        { &iterations, 500, 500, 100, add_fn },
#else
        // XXX : need to change back to 2000 after fixing SConstruct
        { &iterations, 2000, 2000, 100, add_fn },
#endif
        { NULL }
    }

};

struct bench_test bench_threadcontrol_ovh = {
    "threadcontrol overhead", 
    threadcontrol_setup, 
    threadcontrol_ovh, 
    threadcontrol_teardown_dummy, 
    {
#if defined(CONFIG_CPU_ARM_XSCALE)
        { &iterations, 500, 500, 100, add_fn },
#else
        // XXX : need to change back to 2000 after fixing SConstruct
        { &iterations, 2000, 2000, 100, add_fn },
#endif
        { NULL }
    }

};
