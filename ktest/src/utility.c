/*
 * Copyright (c) 2005, National ICT Australia
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

#include <ktest/ktest.h>
#include <ktest/utility.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <l4/config.h>
#include <l4/space.h>
#include <l4/ipc.h>
#include <l4/misc.h>
#include <l4/schedule.h>
#include <okl4/env.h>
#include <l4e/map.h>
#include <compat/c.h>
#include <okl4/kspaceid_pool.h>

/*---------------------------------------------------------------------------*/
/* Multithreaded Test Support. */

/*
 * Structure for recording the per-thread failures during a
 * multithreaded test.
 */
 typedef struct threaded_results {
     int line;  /* Line the failure occured. */
     char message[BUFSIZ]; /* Details of the failure. */
 } threaded_results_t;

/* Per-thread error message buffers. */
threaded_results_t threaded_messages[NUM_TEST_THREADS];

/* Thread driving the multithreaded tests. */
L4_ThreadId_t rmt_master_thread;

/* Table of running multithreaded tests. */
static L4_ThreadId_t threaded_threads[NUM_TEST_THREADS];

/* Filename of the test source. */
static const char* threaded_test_file;

typedef void (*thread_entry) (L4_ThreadId_t);

struct thread_t {
    L4_ThreadId_t tid;
    L4_ThreadId_t handle;
    L4_SpaceId_t space;
    thread_entry entry;
};

ALIGNED(32) static L4_Word_t * stacks[NUM_THREADS];
static struct thread_t threads[NUM_THREADS];

/*
 * utcb area hardcoded address
 */
L4_Word_t utcb_addr = 0x58000000ULL;

word_t get_seg(L4_SpaceId_t spaceid, word_t vaddr, word_t *offset, word_t *cache, word_t *rwx, word_t *size)
{
    int i;
    static okl4_env_segments_t *segs = NULL;
    if (segs == NULL) {
        segs = OKL4_ENV_GET_SEGMENTS("SEGMENTS");
    }

    for (i = 0; i < segs->num_segments; ++i) {
        okl4_env_segment_t *seg;
        seg = &segs->segments[i];
        if (seg->virt_addr <= vaddr && vaddr < seg->virt_addr + seg->size) {
            *offset = seg->offset + (vaddr - seg->virt_addr);
            *cache = seg->cache_policy;
            *rwx = seg->rights;
            *size = seg->size;
            return seg->segment;
        }
    }
    return ~0UL;
}


/*
 * L4test test thread and address space pager / exception handler
 */

static void ktest_thread_handler(void)
{
    ARCH_THREAD_INIT
    L4_MsgTag_t tag;
    L4_Msg_t msg;
    L4_ThreadId_t client;
    L4_Word_t label;

    while(1)
    {
        tag = L4_Wait(&client);

        L4_MsgStore(tag, &msg);

        if(!L4_IpcSucceeded(tag)) {
            printf("Was receiving an IPC which was cancelled?\n");
            continue;
        }

        label = L4_MsgLabel(&msg);

        {
            L4_MsgTag_t tag = L4_Niltag;
            long fault;
            char type;

            /* get sign-extended (label>>4) */
            fault = (((long)(label<<16)) >> 20);
            type = label & 0xf;

            switch(fault)
            {
                case -2: {
                    L4_Word_t seg, offset, fault_addr, ip, cache, rwx, size, page_size;
                    L4_MapItem_t map;
                    L4_SpaceId_t spaceid;
                    int r;

                    fault_addr = L4_MsgWord(&msg, 0);
                    ip = L4_MsgWord(&msg, 1);

                    seg = get_seg(KTEST_SPACE, fault_addr, &offset, &cache, &rwx, &size);
                    if (seg == ~0UL)
                    {
                        printf("invalid pagefault addr: %p ip: %p\n", (void *)fault_addr, (void *)ip);
                        tag = L4_Niltag;
                        tag = L4_MsgTagAddLabel(tag, (-2UL)>>4<<4);    // indicate fail as pagefault
                        L4_Set_MsgTag(tag);
                        L4_Send(libcheck);
                        break;
                    }

                    //printf("pagefault msg type= %x  from %lx  [%p] @ %lx:%lx\n", type,
                    //       client.raw, (void*)ip, fault_addr, phys);

                    //printf("map space, thread %lx, %lx\n", client.raw, __L4_TCR_SenderSpace());

                    page_size = L4_GetMinPageBits();
                    fault_addr &= ~((1ul << page_size)-1);
                    offset &= ~((1ul << page_size)-1);

                    spaceid.raw = __L4_TCR_SenderSpace();

                    fault_addr -= offset;
                    offset = 0;
                    while ((int)size > 0) {

                        L4_MapItem_Map(&map, seg, offset, fault_addr, page_size,
                                cache, rwx);

                        r = L4_ProcessMapItem(spaceid, map);
                        assert(r == 1);

                        offset += (1UL << page_size);
                        fault_addr += (1UL << page_size);
                        size -= (1UL << page_size);
                    }

                    L4_MsgClear(&msg);
                    L4_MsgLoad(&msg);
                    L4_Reply(client);
                    break;
                }
                default:
                    if (label == 1) { // _fail_unless()
                        tag = L4_Niltag;
                        tag = L4_MsgTagAddLabel(tag, label);    // indicate fail as exception
                        L4_Set_MsgTag(tag);
                        L4_Send(libcheck);
                        break;
                    } else {
                        printf("default_thread_handler: msg label = %ld  from %lx\n", label, client.raw);
                        tag = L4_Niltag;
                        tag = L4_MsgTagAddLabel(tag, (-4UL)>>4<<4);    // indicate fail as exception
                        L4_Set_MsgTag(tag);
                        L4_Send(libcheck);
                        break;
                    }
            }
        }
    }
}

ALIGNED(32) static L4_Word_t handler_stack[STACK_SIZE];
/*
 * Difference between the thread's poisition in the stacks array below
 * and the threaded_threads array above.  Thread Ids are based on the
 * index into <stacks>.
 */
static int thread_offset;

/*---------------------------------------------------------------------------*/
/* Space creation deletion code
 * There are two operations, spaceCreate and spaceDelete
 */

static int space_init = 0;

L4_SpaceId_t createSpaceSimple(L4_ClistId_t clist, L4_Word_t max_prio, int isResourcedSpace)
{
    L4_SpaceId_t space;
    int i, r;
    L4_Fpage_t utcb_area;
    okl4_allocator_attr_t attr;
    L4_Word_t end;

    okl4_kspaceid_getattribute(spaceid_pool, &attr);
    end = attr.base = attr.size;
    if (!space_init)
    {
        for (i = attr.base; i < end; i++) {
            okl4_kspaceid_free(spaceid_pool, L4_SpaceId(i));
        }
        space_init = 1;
    }
    r = okl4_kspaceid_allocany(spaceid_pool, &space);
    if (r != OKL4_OK) {
        L4_KDB_Enter("too many spaces");
        while (1);
    }

    // Check that our assumptions are sane
    assert((1UL << L4_GetUtcbBits())
           <= UTCB_ALIGNMENT);
    assert(L4_GetUtcbSize()
           <= UTCB_SIZE);
// FIXME - Not handled after KIP removal
#if 0
    assert(L4_UtcbAreaSize( L4_GetKernelInterface())
           <= NUM_THREADS * UTCB_SIZE);
#endif
    // very hacky and evil
#ifndef NO_UTCB_RELOCATE
    utcb_area = L4_Fpage(utcb_addr, NUM_THREADS * UTCB_SIZE);
#else
    utcb_area = L4_Nilpage;
#endif

    if (isResourcedSpace)
    {
        r = L4_SpaceControl(space, L4_SpaceCtrl_prio(max_prio) | L4_SpaceCtrl_new | L4_SpaceCtrl_kresources_accessible, clist, utcb_area, 0, NULL);
    }
    else
    {
        r = L4_SpaceControl(space, L4_SpaceCtrl_prio(max_prio) | L4_SpaceCtrl_new, clist, utcb_area, 0, NULL);
    }
    if (r == 0)
        printf("space no = %lu, clistno = %lu\n", space.space_no, clist.clist_no);
    assert(r == 1);

    return space;
}

L4_SpaceId_t createSpace()
{
    return createSpaceSimple(KTEST_CLIST, DEFAULT_MAX_PRIO, 0);
}

L4_SpaceId_t createResourcedSpace()
{
    return createSpaceSimple(KTEST_CLIST, DEFAULT_MAX_PRIO, 1);
}

void deleteSpace(L4_SpaceId_t space)
{
    int r;
    r = L4_SpaceControl(space, L4_SpaceCtrl_delete, KTEST_CLIST, L4_Nilpage, 0, NULL);
    if (r != 1) {
        printf("cleanup non existing/not empty space: %ld, error: %ld\n", space.raw, L4_ErrorCode());
    }
    okl4_kspaceid_free(spaceid_pool, space);
}

/**
 * @brief Create spaces to exhaustion
 *
 * Keep creating spaces till failure, then delete the ones that 
 * created and return the error code.
 * 
 * The aim here is to exercise the failure path, but pass the error
 * code back to be checked by arch-specific code (as the expected result
 * is different on each arch, depending on which resource gets exhausted 
 * first.
 */

L4_Word_t exhaustSpaces(void)
{
    L4_Word_t    result;
    L4_SpaceId_t space;
    L4_Word_t    i;
    L4_Word_t    failure_count;
    L4_Fpage_t   utcb_area;
    L4_Word_t    error_code;
    okl4_allocator_attr_t attr;

    okl4_kspaceid_getattribute(spaceid_pool, &attr);

#ifndef NO_UTCB_RELOCATE
    utcb_area = L4_Fpage(utcb_addr, NUM_THREADS * UTCB_SIZE);
#else
    utcb_area = L4_Nilpage;
#endif

    for (i = attr.base; ; i++) {
        space = L4_SpaceId(i);
        result = L4_SpaceControl(space,
                                 L4_SpaceCtrl_new,
                                 KTEST_CLIST, 
                                 utcb_area, 
                                 0, 
                                 NULL);
        if (result == 0) {
            error_code = L4_ErrorCode();
            failure_count = i;
            break;
        }
    }
    // Delete all created spaces
    for (i-- ; i >= attr.base; i--) {
        space = L4_SpaceId(i);
        result = L4_SpaceControl(space,
                                 L4_SpaceCtrl_delete,
                                 KTEST_CLIST,
                                 L4_Nilpage,
                                 0,
                                 NULL);

        fail_unless(result == 1, "Space control delete failed");
    }

    /* If the tests using this fail, checking how many actually got
       created (and confirming it matches your expectations) is of help */
/*  printf("Failed creating space number %ld, code %ld\n", 
           failure_count,
           error_code); */
    return error_code;
}

/* Thread creation deletion code
 * There are two operations, threadCreate and threadDelete
 * For the time being you can only create and delete a single thread
 */
static int own_as = 0;
static int inited = 0;
L4_ThreadId_t default_thread_handler;

void initThreads(int own_address_spaces)
{
    int i;
    L4_ThreadId_t tid;
    okl4_allocator_attr_t attr;
    L4_Word_t end;

    // if this is first init initialise array
    if(!inited) {
        int r;
        L4_Word_t utcb, dummy;

        for(i = 0; i < NUM_THREADS; i++)
            stacks[i] = NULL;

        /* create thread pager / exception handler */
        tid = KTEST_SERVER;
        tid.X.index += 2;
#ifndef NO_UTCB_RELOCATE
        utcb = (L4_Word_t) (L4_PtrSize_t)L4_GetUtcbBase() + (2)*UTCB_SIZE;
#else
            utcb = -1UL;
#endif
        r = L4_ThreadControl(tid, KTEST_SPACE, libcheck, libcheck,
                libcheck, 0, (void *)(L4_PtrSize_t)utcb);
        assert(r == 1);

        L4_Schedule(tid, -1, 0x00000001, -1, -1, 0, &dummy);
        L4_Set_Priority(tid, 253);
        L4_Start_SpIp(tid, (L4_Word_t)(L4_PtrSize_t)(&handler_stack[STACK_SIZE-1]) & (~7UL),
                (L4_Word_t) (L4_PtrSize_t)ktest_thread_handler);

        default_thread_handler = tid;
        L4_KDB_SetThreadName(default_thread_handler, "test_handler");
    }
    else {
        /*
         * On subsequent calls to this function, we delete any threads
         * that may still be lying around from previous test cases. We
         * only delete test threads (i.e., upto NUM_TEST_THREADS) and not
         * system threads (i.e., anything between NUM_TEST_THREADS and
         * (NUM_TEST_THREADS + SYSTEM_THREADS == NUM_THREADS).
         */
        for (i = 0; i < NUM_TEST_THREADS; i++) {
            if (stacks[i]) {
                tid = threads[i].tid;
                deleteThread(tid);
            }
        }

        okl4_kspaceid_getattribute(spaceid_pool, &attr);
        end = attr.base + attr.size;
        /* free any spaces as well */
        for (i = attr.base; i < end; i++)
        {
            L4_SpaceId_t id = L4_SpaceId(i);
            if (okl4_kspaceid_isallocated(spaceid_pool, id) && 
                (id.space_no != KTEST_SPACE.space_no))
            {
                deleteSpace(id);
            }
        }
    }
    inited = 1;
    own_as = own_address_spaces;
}

/* lookup reply handle of thread 
 * Should not be used in any except IPC tests to check the reply handle is correct
 */
L4_ThreadId_t lookupReplyHandle(L4_ThreadId_t thread)
{
    int i;

    for (i = 0; i < NUM_THREADS; i++)
        if (threads[i].tid.raw == thread.raw)
            return threads[i].handle;
    return L4_nilthread;

}

/* lookup space of thread  */
L4_SpaceId_t lookupSpace(L4_ThreadId_t thread)
{
    int i;

    for (i = 0; i < NUM_THREADS; i++) {
        if (threads[i].tid.raw == thread.raw) {
            return threads[i].space;
        }
    }
    return L4_nilspace;
}

/* update space of thread */
void updateSpace(L4_ThreadId_t thread, L4_SpaceId_t space)
{
    int i;
    for (i = 0; i < NUM_THREADS; i++) {
        if (threads[i].tid.raw == thread.raw) {
            threads[i].space = space;
        }
    }
}

/* create a thread that starts execution at the given function */
L4_ThreadId_t createThread(void (*ip)(void))
{
    if(own_as)
        return createThreadInSpace(L4_nilspace, ip);
    else
        return createThreadInSpace(KTEST_SPACE, ip);
}

static void thread_entry_func(void)
{
    int i;
    L4_Word_t stack_var = 0, stack;
    ARCH_THREAD_INIT

    stack = (L4_Word_t)&stack_var;
    for (i = 0; i < NUM_THREADS; i++){
        if ((((L4_Word_t)stacks[i]) < stack) && ((L4_Word_t)(stacks[i] + STACK_SIZE) >= stack)) {
            threads[i].entry(threads[i].tid);
            L4_WaitForever();
        }
    }
    assert(!"Should not get here\n");
    L4_WaitForever();

}

static L4_ThreadId_t
createSysThreadInSpace(L4_SpaceId_t space, void (*ip)(void), int system_thread)
{
    //L4_Word_t old_control = 0;
    int i;
    L4_ThreadId_t tid;
    int r;
    L4_Word_t utcb, dummy, stack;

    /* Find a slot for the thread. */
    if (system_thread) {
        /* Find a system thread. System threads are between NUM_TEST_THREADS
         * and (NUM_TEST_THREADS + NUM_SYSTEM_THREADS) */
        for (i = 0; i < NUM_SYSTEM_THREADS; i++) {
            if (!stacks[i + NUM_TEST_THREADS]) {
                i = i + NUM_TEST_THREADS;
                break;
            }
        }
        assert(i < NUM_THREADS);
    } else {
        /* Find a spare thread. */
        for (i = 0; i < NUM_TEST_THREADS; i++) {
            if (!stacks[i]) {
                break;
            }
        }
        assert(i < NUM_TEST_THREADS);
    }

    // Make sure we found an unused thread
    tid = KTEST_SERVER;
    tid.X.index += (i+16);

    // Check that our assumptions are sane
    assert((1ul << L4_GetUtcbBits()) <= UTCB_ALIGNMENT);
    assert(L4_GetUtcbSize() <= UTCB_SIZE);
// FIXME - with KIP going, need to work out how to do some of this.
#if 0
    assert(L4_UtcbAreaSize(L4_GetKernelInterface()) <= NTHREADS*UTCB_SIZE);
#endif

#ifndef NO_UTCB_RELOCATE
    if(space.raw == KTEST_SPACE.raw)
    {
        utcb = (L4_Word_t) (L4_PtrSize_t)L4_GetUtcbBase() + (i+16)*UTCB_SIZE;
    }
    else
    {
        utcb = utcb_addr + i*UTCB_SIZE;
    }
#else
    utcb = -1UL;
#endif

    stacks[i] = malloc(sizeof(L4_Word_t)*STACK_SIZE);
    assert(stacks[i]);
    if (space.raw == L4_nilspace.raw)
    {
        space = createSpace();
    }
    r = L4_ThreadControl(tid, space, default_thread_handler, default_thread_handler, default_thread_handler,
                         0, (void *)(L4_PtrSize_t)utcb);

    L4_StoreMR(0, &threads[i].handle.raw);
    threads[i].tid = tid;
    threads[i].space = space;
    threads[i].entry = (thread_entry)ip;

    //printf("create thread %d, %lx, %ld, %p %x\n", i, tid.raw, space.raw, ip, (int)threads[i].handle.raw);

    if(r != 1)
    {
        printf("\nThread Create failed, ErrorCode = %d\n", (int)L4_ErrorCode());
    }
    assert(r == 1);

    /* Only let the thread run on the first hardware thread / hardware core. */
    L4_Schedule(tid, -1, 0x00000001, -1, -1, 0, &dummy);
    L4_Set_Priority(tid, 100);

    // Push thread-id
    stack = ((L4_Word_t)stacks[i] + sizeof(L4_Word_t)*(STACK_SIZE-1)) & (~7UL);
    *(L4_Word_t*)(stack) = tid.raw;

    L4_Start_SpIp(tid, stack,
                  (L4_Word_t) (L4_PtrSize_t)thread_entry_func);
    return tid;
}

L4_ThreadId_t createThreadInSpace(L4_SpaceId_t space, void (*ip)(void))
{
    return createSysThreadInSpace(space, ip, 0);
}

L4_ThreadId_t createSystemThread(void (*ip)(void))
{
    return createSysThreadInSpace(KTEST_SPACE, ip, 1);
}

int isSystemThread(L4_ThreadId_t tid)
{
    int i;
    for (i = NUM_TEST_THREADS; i < NUM_THREADS; i++) {
        if (threads[i].tid.raw == tid.raw) {
            return 1;
        }
    }
    return 0;
}

/* 
   Update a thread's ThreadID. This is used when a test externally changes
   an allocated thread's ThreadId. (E.g: to test version bits are working
   correctly).
*/
void
update_thread_id(L4_ThreadId_t old_thread, L4_ThreadId_t new_thread)
{
    int i;

    for (i = 0; i < NUM_THREADS; i++)
        if (threads[i].tid.raw == old_thread.raw)
            break;
    assert(i != NUM_THREADS);
    threads[i].tid.raw = new_thread.raw;
}


L4_ThreadId_t allocateThread(L4_SpaceId_t space)
{
    int i;
    L4_ThreadId_t tid;

    for (i = 0; i < NUM_THREADS; i++) {
        if (!stacks[i]) {
            break;
        }
    }

    // Make sure we found an unused thread
    assert(i < NUM_THREADS);

    tid = KTEST_SERVER;
    tid.X.index += (i+16);

    threads[i].tid = tid;
    threads[i].space = space;
    stacks[i] = (void*)0xdeadbeef;

    return tid;
}


/* delete the given thread */
void deleteThread(L4_ThreadId_t thread)
{
    int r;
    int i;

    for (i = 0; i < NUM_THREADS; i++) {
        if (threads[i].tid.raw == thread.raw) {
            break;
        }
    }
    assert(i != NUM_THREADS);
    assert(stacks[i] != NULL);

    r = L4_ThreadControl(thread, L4_nilspace, L4_nilthread, L4_nilthread, L4_nilthread, 0, (void *)0);
//printf("delete thread %d, %lx, %ld\n", i, thread.raw, threads[i].space.raw);

    /* handle allocateThread threads */
    if ((word_t)stacks[i] == 0xdeadbeef) {
        stacks[i] = NULL;

        threads[i].tid = L4_nilthread;
        threads[i].space = L4_nilspace;
        return;
    }

    if (r != 1)
    {
        printf("\nThread Delete failed, ErrorCode = %d\n", (int)L4_ErrorCode());
    }
    assert(r == 1);
    free(stacks[i]);
    stacks[i] = NULL;

    threads[i].tid = L4_nilthread;
    threads[i].space = L4_nilspace;
}

/* delete allocated thread */
void deallocateThread(L4_ThreadId_t tid)
{
    int i;
    
    for (i = 0; i < NUM_THREADS; i++) {
        if (threads[i].tid.raw == tid.raw) {
            break;
        }
    }

    // Make sure we found a valid used thread
    assert(i < NUM_THREADS);
    
    threads[i].tid = L4_nilthread;
    threads[i].space = L4_nilspace;
    stacks[i] = NULL;
}

L4_Word_t* threadStack (L4_ThreadId_t t)
{
    int i = L4_ThreadNo(t) - L4_ThreadNo(KTEST_SERVER) - 1;

    if (i >= 0 && i < NUM_THREADS) {
        return stacks[i];
    } else {
        return NULL;
    }
}


int checkSending(L4_ThreadId_t thread)
{
    L4_Word_t control, dummy, r;

    r = L4_ExchangeRegisters(thread, L4_ExReg_Deliver, 0, 0, 0, 0, L4_nilthread, 
                             &control, &dummy, &dummy, &dummy, &dummy);

    if (r != 1) {
        printf("\nExregs failed, ErrorCode = %d\n", (int)L4_ErrorCode());
    }
    _fail_unless(r == 1, __FILE__, __LINE__, "exregs failed");

    return (control & 0x4) >> 2;
}

int checkReceiving(L4_ThreadId_t thread)
{
    L4_Word_t control, dummy, r;

    r = L4_ExchangeRegisters(thread, L4_ExReg_Deliver, 0, 0, 0, 0, L4_nilthread, &control, &dummy, &dummy, &dummy, &dummy);
    _fail_unless(r == 1, __FILE__, __LINE__, "exregs failed");

    return (control & 0x2) >> 1;
}

/*
 * wait for a thread to start sending
 * Do not use this function !
 */
void waitSending(L4_ThreadId_t thread)
{
    int i;
    /*
      Give up to 10 timeslices to start it's receive
      (Should take 0 due to priorities)
    */
    for(i = 0; i < 30; i++)
    {
        if(checkSending(thread)) {
            break;
        }
        L4_ThreadSwitch(thread);
    }
}
/*
 * wait for a thread to start receiving
 * Do not use this function !
 */
void waitReceiving(L4_ThreadId_t thread)
{
    int i;
    // Give up to 10 timeslices to start it's receive
    // (Should take 0 due to priorities)
    for(i = 0; i < 30; i++)
    {
        if(checkReceiving(thread))
            break;
        L4_ThreadSwitch(thread);
    }

}

/* wait for a thread to send IPC until timeout */
int waitThreadTimeout(L4_ThreadId_t thread)
{
    L4_MsgTag_t tag;
    L4_Word_t timeout;
    
    timeout = 0;
    while (timeout < 1000)
    {
        L4_ThreadSwitch(thread);
        timeout++;

        tag = L4_Receive_Nonblocking(thread);
        if (L4_IpcSucceeded(tag))
            break;
    }
    return (timeout < 1000);
}

/* Tag and acceptor permutation code
 * These functions support the permutation of
 * specified bits in the message tag and the acceptor
 *
 * the permutations are generated from the low order bits of an integer
 */
/* Set the message Tag and acceptor based on the given integer */
L4_MsgTag_t setMsgTagAndAcceptor(int i)
{
    L4_Word_t t = 0;
    L4_MsgTag_t tag;

    if(i & BIT_A)
        L4_Accept(L4_NotifyMsgAcceptor);
    else
        L4_Accept(L4_UntypedWordsAcceptor);

    // work out how many mrs
    t = (i & BIT_U)?10:0;

    // copy the srnp flags
    t |= (i & (0x3c)) << 10;

    tag.raw = t;
    return tag;
}

/* checks that i has the bits specified in the mask set as required by the value
 * Used to check that this is a permutation that we actually want to test
 */
int validTest(int i, int valid_mask, int valid_value)
{
    return (i & valid_mask) == (valid_value & valid_mask);
}

/* Generate a user friendly output of what the integer means for the tag and the acceptor */
char * tagIntToString(int i)
{
    static char str[10];

    snprintf(str, 9, "%c%c%c%c%c%c", (i & BIT_S)?'S':'s', (i & BIT_R)?'R':'r',
             (i & BIT_N)?'N':'n', (i & BIT_P)?'P':'p', (i & BIT_U)?'U':'u',
             (i & BIT_A)?'A':'a');
    return str;
}

void fillMessage(L4_Msg_t * msg, L4_MsgTag_t tag)
{
    L4_MsgClear(msg);
    L4_Set_MsgMsgTag(msg, tag);
    L4_Set_MsgLabel(msg, MAGIC_LABEL2);
    L4_MsgAppendWord(msg, MAGIC_MR0);
    L4_MsgAppendWord(msg, MAGIC_MR1);
    L4_MsgAppendWord(msg, MAGIC_MR2);
    L4_MsgAppendWord(msg, MAGIC_MR3);
    L4_MsgAppendWord(msg, MAGIC_MR4);
    L4_MsgAppendWord(msg, MAGIC_MR5);
    L4_MsgAppendWord(msg, MAGIC_MR6);
    L4_MsgAppendWord(msg, MAGIC_MR7);
    L4_MsgAppendWord(msg, MAGIC_MR8);
    L4_MsgAppendWord(msg, MAGIC_MR9);
}

void checkMessage(L4_Msg_t * msg, int i)
{
    _fail_unless(getTagU(L4_MsgMsgTag(msg)) == 10, __FILE__, __LINE__, "Tag.u has wrong value, input tag %s", tagIntToString(i));
    _fail_unless(L4_MsgLabel(msg) == MAGIC_LABEL2, __FILE__, __LINE__, "Tag has wrong label %s", tagIntToString(i));
    fail_unless(L4_MsgWord(msg, 0) == MAGIC_MR0,
                "Message registers set with incorrect values, input tag");
    fail_unless(L4_MsgWord(msg, 1) == MAGIC_MR1,
                "Message registers set with incorrect values, input tag");
    fail_unless(L4_MsgWord(msg, 2) == MAGIC_MR2,
                "Message registers set with incorrect values, input tag");
    fail_unless(L4_MsgWord(msg, 3) == MAGIC_MR3,
                "Message registers set with incorrect values, input tag");
    fail_unless(L4_MsgWord(msg, 4) == MAGIC_MR4,
                "Message registers set with incorrect values, input tag");
    fail_unless(L4_MsgWord(msg, 5) == MAGIC_MR5,
                "Message registers set with incorrect values, input tag");
    fail_unless(L4_MsgWord(msg, 6) == MAGIC_MR6,
                "Message registers set with incorrect values, input tag");
    fail_unless(L4_MsgWord(msg, 7) == MAGIC_MR7,
                "Message registers set with incorrect values, input tag");
    fail_unless(L4_MsgWord(msg, 8) == MAGIC_MR8,
                "Message registers set with incorrect values, input tag");
    fail_unless(L4_MsgWord(msg, 9) == MAGIC_MR9,
                "Message registers set with incorrect values, input tag");
}

/*
 * Switch to a destination thread until it is blocked.
 */
void
threadswitch_until_blocked(L4_ThreadId_t dest)
{
    int i = 100;
    L4_Word_t res;

    do {
        L4_Word_t dummy;
        L4_ThreadSwitch(dest);
        i--;
        res = L4_Schedule(dest, ~0UL, ~0UL, ~0UL, ~0UL, 0, &dummy);
    } while (res == L4_SCHEDRESULT_RUNNING && i > 0);
}

/*---------------------------------------------------------------------------*/

/*
 * Like checklib's fail_unless(), except that the message is stored in
 * a shared memory buffer and the master thread, not check's driver
 * thread, is informed of the failure.
 */
void multithreaded_fail_unless (int result, int line, const char* msg, ...)
{
    if (!result) {
        va_list ap;
        int thread_idx = L4_ThreadNo(KTEST_SERVER) -
            L4_ThreadNo(rmt_master_thread) - 1 - thread_offset;
        threaded_results_t* output = &threaded_messages[thread_idx];

        output->line = line;
        va_start(ap,msg);
        vsnprintf(output->message, sizeof(output->message), msg, ap);
        va_end(ap);

        L4_LoadMR(0, 1 << 16 | 0xC000);
        L4_Call(rmt_master_thread);
        /* Never returns.  The main thread will kill us off. */
    }
}

void multithreaded_pass (void)
{
    L4_LoadMR(0, 0xC000);
    L4_Call(rmt_master_thread);
    /* Never returns.  The main thread will kill us off. */
}

extern L4_ThreadId_t test_tid;
/*
 * Start the multithreaded test and then return the list of thread ids
 * so that the calling function can interact with the threads during
 * the test.
 */
L4_ThreadId_t* start_multithreaded_test (const char* file, void (*multithread_test)(void),
                                         int nthreads)
{
    int i;

    if (nthreads > NUM_MULTITHREADED_TEST_THREADS)
        return NULL;

    /* Nilthread fill the array. */
    for (i = 0; i < NUM_MULTITHREADED_TEST_THREADS; i++)
        threaded_threads[i] = L4_nilthread;

    rmt_master_thread = test_tid;
    threaded_test_file = file;

    /* Run the test multiple times, all at once. */
    for (i = 0; i < nthreads; i++)
        threaded_threads[i] = createThread (multithread_test);

    thread_offset = L4_ThreadNo(threaded_threads[0]) - L4_ThreadNo(rmt_master_thread) - 1;

    return threaded_threads;
}

/*
 * Wait for the results, kill the test threads and then report the
 * results.
 */
void collect_multithreaded_results (void)
{
    int i;
    int failing_idx = -1;

    /*
     * Wait for each to report.  We don't care about the order in
     * which they report, and if there are any failures then just
     * note the last one to come in.
     */
    for (i = 0; i < NUM_MULTITHREADED_TEST_THREADS; i++) {
        L4_ThreadId_t from = threaded_threads[i];
        L4_MsgTag_t tag = L4_Receive(from);
        L4_Msg_t msg;

        L4_MsgStore(tag, &msg);

        if (L4_MsgLabel(&msg))
            failing_idx = i;

        /* Thank you very much for all your work.  Now DIE! */
        deleteThread (threaded_threads[i]);
    }

    /*
     * Report any failures.
     */
    if (failing_idx != -1)
        _fail_unless (0, threaded_test_file, threaded_messages[failing_idx].line,
                      "Thread %d: %s", failing_idx,
                      threaded_messages[failing_idx].message);
}

/* Combine the above functions in one easy form. */
void run_multithreaded_test(const char* file, void (*multithread_test)(void))
{
    start_multithreaded_test(file, multithread_test,
            NUM_MULTITHREADED_TEST_THREADS);
    collect_multithreaded_results ();
}


/**
 * Architecture-specific intialisation function.
 *
 * If no setup function exists for the architecture, we default to this dummy
 * implementation.
 */
WEAK void
ktest_arch_setup(void)
{
}

/* The priority of the hopper thread. */
volatile int hopper_prio = -1;

/* Worker thread that reduces its priority until it is preempted.
 * Used by get_current_threads_priority(). */
static void priority_hopper(void)
{
    /* Wait for our creator to start us. */
    L4_ThreadId_t dummy;
    L4_Wait(&dummy);

    hopper_prio = 255;
    while (hopper_prio >= 0) {
        int i;

        /* Adjust our priority. */
        L4_Set_Priority(L4_Myself(), hopper_prio);

        /* Yield to other threads at our current priority, to ensure
         * they get a chance to run. */
        for (i = 0; i < 3; i++) {
            L4_Yield();
        }
        hopper_prio--;
    }

    /* The other thread should have been executed by now. */
    fail("get_current_threads_priority() failed.");
}

/**
 * Get the calling thread's effective priority.
 *
 * This function must only be called when there are no other runnable
 * thread's in the system, otherwise it will return an undefined value.
 * The calling thread must also be privileged.
 *
 * The function determines the current thread's effective priority empirically,
 * by creating a new thread with a high priority, and slow reducing this new
 * thread's priority down, until the caller preempts it.
 */
int get_current_threads_priority(void)
{
    /* Create the priority-hopper thread. */
    L4_ThreadId_t hopper = createThread(priority_hopper);

    /* Adjust the hopper's priority. */
    L4_Set_Priority(hopper, 255);

    /* Allow the hopper to start. */
    L4_Send(hopper);

    /* We should only continue running when the hopper is on equal
     * priority to us. Delete it. */
    deleteThread(hopper);

    /* ...and return back our priority. */
    return hopper_prio;
}

/**
 * Architecture-specific fault instruction.
 *
 * By default, we generate a divide-by-zero, which on many (but not all!)
 * architectures, causes a CPU fault.
 */
WEAK void
trigger_cpu_fault(void)
{
    static volatile int global_zero = 0;
    int result = 1 / global_zero;
    printf("Warning: We managed to divide by zero: 1 / 0 = %d\n", result);
}

