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
#include <okl4/static_memsec.h>
#include <okl4/pd.h>
#include <okl4/kspace.h>
#include <okl4/bitmap.h>
#include <okl4/kclist.h>
#include <okl4/init.h>
#include <okl4/extension.h>

#include "../helpers.h"

#define STACK_SIZE ((0x1000 / sizeof(okl4_word_t)))
ALIGNED(32) static okl4_word_t ext0100_stack[STACK_SIZE + sizeof(okl4_word_t)];
volatile okl4_extension_token_t ext0100_token;
static volatile int ext0100_progress;

#define EXT0100_ITERATIONS 3

/* Child thread. */
static void
ext0100_child(void)
{
    volatile char *addr;
    int i;

    okl4_init_thread();

    switch (ext0100_progress) {
        case 0:
        case 1:
        case 2:
            /* Attempt to touch extended memory. Should fault
             * and restart. */
            addr = (volatile char *)(EXTENSION_BASE_ADDR
                    + (ext0100_progress * OKL4_DEFAULT_PAGESIZE));
            ext0100_progress++;
            *addr = 1;
            break;

        case 3:
            /* Activate the extension. */
            OKL4_EXTENSION_ACTIVATE(&ext0100_token);
            ext0100_progress = 4;
            for (i = 0; i < EXT0100_ITERATIONS; i++) {
                *((volatile char *)EXTENSION_BASE_ADDR
                        + (i * OKL4_DEFAULT_PAGESIZE)) = 1;
            }
            OKL4_EXTENSION_DEACTIVATE(&ext0100_token);

            /* Try touching the memory again. Should fault. */
            ext0100_progress = 5;
            *((volatile char *)EXTENSION_BASE_ADDR) = 1;
            break;

        default:
            break;
    }

    fail("Shouldn't reach here.");
}

static void
wait_for_pagefault_ipc(L4_ThreadId_t *from, okl4_word_t *addr)
{
    L4_MsgTag_t tag;
    L4_Msg_t msg;

    /* Wait for child to IPC to us. */
    tag = L4_Wait(from);
    L4_MsgStore(tag, &msg);
    fail_unless(L4_IpcSucceeded(tag) != 0, "Did not get pagefault IPC.");
    L4_MsgGetWord(&msg, 0, addr);
}

static void
resume_pagefaulted_thread(L4_CapId_t dest, okl4_word_t start_addr,
        okl4_word_t stack)
{
    L4_Start_SpIp(dest, stack, start_addr);
}

/* Create an empty extension, and ensure it can be activated/deactivated. */
START_TEST(EXT0100)
{
    okl4_pd_t *pd;
    okl4_extension_t *ext;
    ms_t *ms[EXT0100_ITERATIONS];
    int i;
    int error;
    okl4_kthread_t *thread;
    L4_ThreadId_t from;
    okl4_word_t addr;

    /* Create a PD / Extension / Memsec. */
    pd = create_pd();
    ext = create_extension(pd);

    /* Attach the extension. */
    error = okl4_pd_extension_attach(pd, ext);
    fail_unless(error == 0, "Could not attach extension.");

    /* Fetch a token for our user to use activate/deactivate. */
    ext0100_token = okl4_extension_gettoken(ext);

    /* Setup progress. */
    ext0100_progress = 0;

    /* Attach memsecs to the PD. These contain program segments. */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        error  = okl4_pd_memsec_attach(pd, ctest_segments[i]);
        fail_unless(error == 0, "Could not attach memsections.");
    }

    /* Attach a few memsecs to the extension space. */
    for (i = 0; i < EXT0100_ITERATIONS; i++) {
        ms[i] = create_memsec(EXTENSION_BASE_ADDR + OKL4_DEFAULT_PAGESIZE * i,
                OKL4_DEFAULT_PAGESIZE, NO_POOL);
        error = okl4_extension_memsec_attach(ext, ms[i]->memsec);
        fail_unless(error == 0, "Could not attach memsec to extension.");
    }

    /* Run the thread in the PD */
    thread = run_thread_in_pd(pd,
            (okl4_word_t)&ext0100_stack[STACK_SIZE],
            (okl4_word_t)ext0100_child);

    /* Wait for it to pagefault in the non-extended space. */
    for (i = 0; i < EXT0100_ITERATIONS; i++) {
        wait_for_pagefault_ipc(&from, &addr);
        fail_unless(addr == EXTENSION_BASE_ADDR + OKL4_DEFAULT_PAGESIZE * i,
                "Thread faulted on wrong address.");
        fail_unless(ext0100_progress == 1 + i,
                "Thread did not make correct progress.");
        resume_pagefaulted_thread(okl4_kthread_getkcap(thread),
                (okl4_word_t)ext0100_child,
                (okl4_word_t)&ext0100_stack[STACK_SIZE]);
    }

    /* Wait for it to pagefault in the non-extended space again. */
    wait_for_pagefault_ipc(&from, &addr);
    fail_unless(addr == EXTENSION_BASE_ADDR, "Thread faulted on wrong address.");
    fail_unless(ext0100_progress == 5, "Thread did not make correct progress.");

    /* Delete the thread. */
    okl4_pd_thread_delete(pd, thread);

    /* Detach the extension. */
    okl4_pd_extension_detach(pd, ext);

    /* Detach memsections */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        okl4_pd_memsec_detach(pd, ctest_segments[i]);
    }
    for (i = 0; i < EXT0100_ITERATIONS; i++) {
        okl4_extension_memsec_detach(ext, ms[i]->memsec);
        delete_memsec(ms[i], NO_POOL);
    }

    /* Delete extension. */
    okl4_extension_delete(ext);
    free(ext);

    /* Delete pd */
    okl4_pd_delete(pd);
    free(pd);
}
END_TEST





ALIGNED(32) static okl4_word_t ext0200_stack[STACK_SIZE];
volatile okl4_extension_token_t ext0200_token;
volatile int ext0200_progress;
okl4_word_t ext0200_base;

static void
ext0200_child(void)
{
    L4_ThreadId_t from;
    L4_MsgTag_t tag;

    okl4_init_thread();

    OKL4_EXTENSION_ACTIVATE(&ext0200_token);
    ext0200_progress = 1;
    tag = L4_Wait(&from);
    fail_unless(L4_IpcSucceeded(tag) != 0, "IPC Failed");

    *(volatile int*)ext0200_base = 1;
    OKL4_EXTENSION_DEACTIVATE(&ext0200_token);

    ext0200_progress = 2;
    *(volatile int*)ext0200_base = 1;

    L4_WaitForever();
}


/**
 *  Protection domain with an extension space. Activate the extension. Map a
 *  new memsec into the base pd. Ensure that the activated thread can access
 *  the new mapping.
 */
START_TEST(EXT0200)
{
    int i, error;
    okl4_pd_t *pd;
    okl4_extension_t *ext;
    ms_t *ms;

    okl4_kthread_t *thread;
    okl4_pd_thread_create_attr_t thread_attr;


    /* Create a PD / Extension / Memsec. */
    pd = create_pd();
    ext = create_extension(pd);

    /* Attach the extension. */
    error = okl4_pd_extension_attach(pd, ext);
    fail_unless(error == 0, "Could not attach extension.");

    /* Fetch a token for our user to use activate/deactivate. */
    ext0200_token = okl4_extension_gettoken(ext);

    /* Attach memsecs to the PD. These contain program segments. */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        error  = okl4_pd_memsec_attach(pd, ctest_segments[i]);
        fail_unless(error == 0, "Could not attach memsections.");
    }

    /* Setup progress */
    ext0200_progress = 0;

    /* Start a thread in the PD */
    okl4_pd_thread_create_attr_init(&thread_attr);
    okl4_pd_thread_create_attr_setspip(&thread_attr,
            (okl4_word_t)&ext0200_stack[STACK_SIZE],
            (okl4_word_t)ext0200_child);
    //okl4_pd_thread_create_attr_setpager(&thread_attr,
    //        okl4_kthread_getkcap(root_thread));
    error = okl4_pd_thread_create(pd, &thread_attr, &thread);
    fail_unless(error == OKL4_OK, "Could not create thread");
    okl4_pd_thread_start(pd, thread);

    while (ext0200_progress != 1) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread));
    }

    /* Create a memsection and attach to PD */
    ms = create_memsec(ANONYMOUS_BASE, OKL4_DEFAULT_PAGESIZE, NULL);
    error = okl4_pd_memsec_attach(pd, ms->memsec);
    fail_unless(error == 0, "Could not attach memsec to PD.");
    ext0200_base = ms->virt.base;

    /* Reply to thread */
    send_ipc(okl4_kthread_getkcap(thread));

    while (ext0200_progress != 2) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread));
    }

    /* Delete the thread. */
    okl4_pd_thread_delete(pd, thread);

    /* Detach the extension. */
    okl4_pd_extension_detach(pd, ext);

    /* Detach memsections */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        okl4_pd_memsec_detach(pd, ctest_segments[i]);
    }
    okl4_pd_memsec_detach(pd, ms->memsec);

    /* Delete extension. */
    okl4_extension_delete(ext);
    free(ext);

    /* Delete the PD */
    okl4_pd_delete(pd);
    free(pd);

    /* Delete the memsec */
    delete_memsec(ms, NULL);
}
END_TEST

ALIGNED(32) static okl4_word_t ext0210_stack[STACK_SIZE];
volatile okl4_extension_token_t ext0210_token;
volatile int ext0210_progress;
okl4_word_t ext0210_base;

static void
ext0210_child(void)
{
    L4_ThreadId_t from;
    L4_MsgTag_t tag;

    okl4_init_thread();

    OKL4_EXTENSION_ACTIVATE(&ext0210_token);
    ext0210_progress = 1;
    tag = L4_Wait(&from);
    fail_unless(L4_IpcSucceeded(tag) != 0, "IPC Failed");

    *(volatile int*)ext0210_base = 1;
    OKL4_EXTENSION_DEACTIVATE(&ext0210_token);

    ext0210_progress = 2;
    tag = L4_Wait(&from);
    *(volatile int*)ext0210_base = 1;
    ext0210_progress = 3;

    L4_WaitForever();
}

/**
 *  Protection domain with an extension space. Activate the extension. Map a
 *  new demand-pagedmemsec into the base pd. Ensure that the activated thread 
 *  can access the new mapping.
 */
START_TEST(EXT0210)
{
    int i, error;
    okl4_pd_t *pd;
    okl4_extension_t *ext;
    okl4_memsec_t *ms;
    okl4_range_item_t *virt;

    okl4_kthread_t *thread;
    okl4_pd_thread_create_attr_t thread_attr;

    L4_ThreadId_t from;
    L4_Msg_t msg;
    L4_MsgTag_t tag;
    okl4_word_t pagefault_addr;

    /* Create a PD / Extension / Memsec. */
    pd = create_pd();
    ext = create_extension(pd);

    /* Attach the extension. */
    error = okl4_pd_extension_attach(pd, ext);
    fail_unless(error == 0, "Could not attach extension.");

    /* Fetch a token for our user to use activate/deactivate. */
    ext0210_token = okl4_extension_gettoken(ext);

    /* Attach memsecs to the PD. These contain program segments. */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        error  = okl4_pd_memsec_attach(pd, ctest_segments[i]);
        fail_unless(error == 0, "Could not attach memsections.");
    }

    /* Setup progress */
    ext0210_progress = 0;

    /* Start a thread in the PD */
    okl4_pd_thread_create_attr_init(&thread_attr);
    okl4_pd_thread_create_attr_setspip(&thread_attr,
            (okl4_word_t)&ext0210_stack[STACK_SIZE],
            (okl4_word_t)ext0210_child);
    okl4_pd_thread_create_attr_setpager(&thread_attr,
            okl4_kthread_getkcap(root_thread));
    error = okl4_pd_thread_create(pd, &thread_attr, &thread);
    fail_unless(error == OKL4_OK, "Could not create thread");
    okl4_pd_thread_start(pd, thread);

    /* Wait for the thread to run. */
    while (ext0210_progress != 1) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread));
    }

    virt = get_virtual_address_range(2 * OKL4_DEFAULT_PAGESIZE);
    ms = create_paged_memsec(*virt);
    error = okl4_pd_memsec_attach(pd, ms);
    fail_unless(error == 0, "Could not attach memsec to PD.");
    ext0210_base = ms->super.range.base;

    /* Reply to thread */
    send_ipc(okl4_kthread_getkcap(thread));

    /* Wait for child to IPC to us. */
    wait_for_pagefault_ipc(&from, &pagefault_addr);
    fail_unless(pagefault_addr == ext0210_base,
            "Thread faulted on the wrong address.");

    /* Map in the page. */
    error = okl4_pd_handle_pagefault(pd, L4_SenderSpace(), pagefault_addr, 0);
    fail_unless(error == 0, "Mapping failed.");

    /* Start the thread again. */
    L4_MsgClear(&msg);
    L4_MsgLoad(&msg);
    tag = L4_Reply(from);
    fail_unless(L4_IpcSucceeded(tag) != 0, "IPC Failed");

    while (ext0210_progress != 2) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread));
    }

    /* Reply to thread */
    send_ipc(okl4_kthread_getkcap(thread));

    while (ext0210_progress != 3) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread));
    }

    /* Delete the thread. */
    okl4_pd_thread_delete(pd, thread);

    /* Detach the extension. */
    okl4_pd_extension_detach(pd, ext);

    /* Detach memsections */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        okl4_pd_memsec_detach(pd, ctest_segments[i]);
    }
    okl4_pd_memsec_detach(pd, ms);
    free(ms);

    /* Delete extension. */
    okl4_extension_delete(ext);
    free(ext);

    /* Delete the PD */
    delete_pd(pd);
    free_virtual_address_range(virt);
}
END_TEST

ALIGNED(32) static okl4_word_t ext0300_stack[STACK_SIZE];
volatile okl4_extension_token_t ext0300_token;
volatile int ext0300_progress;

static void
ext0300_child(void)
{
    L4_ThreadId_t from;
    L4_MsgTag_t tag;

    okl4_init_thread();

    OKL4_EXTENSION_ACTIVATE(&ext0300_token);
    ext0300_progress = 1;
    tag = L4_Wait(&from);
    fail_unless(L4_IpcSucceeded(tag) != 0, "IPC Failed");

    *((volatile char *)EXTENSION_BASE_ADDR) = 1;
    ext0300_progress = 2;
    OKL4_EXTENSION_DEACTIVATE(&ext0300_token);

    ext0300_progress = 3;
    *((volatile char *)EXTENSION_BASE_ADDR) = 1;

    L4_WaitForever();
}


/**
 *  Protection domain with an extension space. Activate the 
 *  extension. Map a new memsec into the extension. Ensure that 
 *  the activated thread can access the new mapping, but not when 
 *  the extension has been deactivated.
 */
START_TEST(EXT0300)
{
    int i, error;
    okl4_pd_t *pd;
    okl4_extension_t *ext;
    ms_t *ms;

    okl4_kthread_t *thread;
    L4_ThreadId_t from;
    okl4_word_t addr;

    /* Create a PD / Extension / Memsec. */
    pd = create_pd();
    ext = create_extension(pd);

    /* Attach the extension. */
    error = okl4_pd_extension_attach(pd, ext);
    fail_unless(error == 0, "Could not attach extension.");

    /* Fetch a token for our user to use activate/deactivate. */
    ext0300_token = okl4_extension_gettoken(ext);

    /* Setup progress */
    ext0300_progress = 0;

    /* Attach memsecs to the PD. These contain program segments. */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        error  = okl4_pd_memsec_attach(pd, ctest_segments[i]);
        fail_unless(error == 0, "Could not attach memsections.");
    }

    /* Attach a single memsec to the extension space. */
    ms = create_memsec(EXTENSION_BASE_ADDR, OKL4_DEFAULT_PAGESIZE, NO_POOL);
    error = okl4_extension_memsec_attach(ext, ms->memsec);
    fail_unless(error == 0, "Could not attach memsec to extension.");

    /* Run the thread in the PD */
    thread = run_thread_in_pd(pd,
            (okl4_word_t)&ext0300_stack[STACK_SIZE],
            (okl4_word_t)ext0300_child);

    /* Reply to thread */
    send_ipc(okl4_kthread_getkcap(thread));

    /* Wait for it to pagefault in the non-extended space. */
    wait_for_pagefault_ipc(&from, &addr);
    fail_unless(addr == EXTENSION_BASE_ADDR, "Thread faulted on wrong address.");
    fail_unless(ext0300_progress == 3, "Thread did not make correct progress.");

    /* Delete the thread. */
    okl4_pd_thread_delete(pd, thread);

    /* Detach the extension. */
    okl4_pd_extension_detach(pd, ext);

    /* Detach memsections */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        okl4_pd_memsec_detach(pd, ctest_segments[i]);
    }
    okl4_extension_memsec_detach(ext, ms->memsec);

    /* Delete extension. */
    okl4_extension_delete(ext);
    free(ext);

    /* Delete the PD */
    okl4_pd_delete(pd);
    free(pd);
}
END_TEST

ALIGNED(32) static okl4_word_t ext0310_stack[STACK_SIZE];
volatile okl4_extension_token_t ext0310_token;
volatile int ext0310_progress;
okl4_word_t ext0310_base;

static void
ext0310_child(void)
{
    L4_ThreadId_t from;
    L4_MsgTag_t tag;

    okl4_init_thread();

    OKL4_EXTENSION_ACTIVATE(&ext0310_token);
    ext0310_progress = 1;
    tag = L4_Wait(&from);
    fail_unless(L4_IpcSucceeded(tag) != 0, "IPC Failed");

    *(volatile int*)ext0310_base = 1;
    OKL4_EXTENSION_DEACTIVATE(&ext0310_token);

    ext0310_progress = 2;

    L4_WaitForever();
}


/**
 *  Protection domain with an extension space. Activate the extension. Map a
 *  new demand-paged memsec into the extension pd. Ensure that the activated thread 
 *  can access the new mapping.
 */
START_TEST(EXT0310)
{
    int i, error;
    okl4_pd_t *pd;
    okl4_extension_t *ext;
    okl4_memsec_t *ms;
    okl4_range_item_t virt;

    okl4_kthread_t *thread;
    okl4_pd_thread_create_attr_t thread_attr;

    L4_ThreadId_t from;
    L4_Msg_t msg;
    L4_MsgTag_t tag;
    okl4_word_t pagefault_addr;

    /* Create a PD / Extension / Memsec. */
    pd = create_pd();
    ext = create_extension(pd);

    /* Attach the extension. */
    error = okl4_pd_extension_attach(pd, ext);
    fail_unless(error == 0, "Could not attach extension.");

    /* Fetch a token for our user to use activate/deactivate. */
    ext0310_token = okl4_extension_gettoken(ext);

    /* Attach memsecs to the PD. These contain program segments. */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        error  = okl4_pd_memsec_attach(pd, ctest_segments[i]);
        fail_unless(error == 0, "Could not attach memsections.");
    }

    /* Setup progress */
    ext0310_progress = 0;

    /* Start a thread in the PD */
    okl4_pd_thread_create_attr_init(&thread_attr);
    okl4_pd_thread_create_attr_setspip(&thread_attr,
            (okl4_word_t)&ext0310_stack[STACK_SIZE],
            (okl4_word_t)ext0310_child);
    okl4_pd_thread_create_attr_setpager(&thread_attr,
            okl4_kthread_getkcap(root_thread));
    error = okl4_pd_thread_create(pd, &thread_attr, &thread);
    fail_unless(error == OKL4_OK, "Could not create thread");
    okl4_pd_thread_start(pd, thread);

    /* Wait for the thread to run. */
    while (ext0310_progress != 1) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread));
    }

    okl4_range_item_setrange(&virt, EXTENSION_BASE_ADDR,
            2 * OKL4_DEFAULT_PAGESIZE);
    ms = create_paged_memsec(virt);
    error = okl4_extension_memsec_attach(ext, ms);
    fail_unless(error == 0, "Could not attach memsec to extension.");
    ext0310_base = ms->super.range.base;

    /* Reply to thread */
    send_ipc(okl4_kthread_getkcap(thread));

    /* Wait for child to IPC to us. */
    wait_for_pagefault_ipc(&from, &pagefault_addr);
    fail_unless(pagefault_addr == ext0310_base,
            "Thread faulted on the wrong address.");

    /* Map in the page. */
    error = okl4_pd_handle_pagefault(pd, L4_SenderSpace(), pagefault_addr, 0);
    fail_unless(error == 0, "Mapping failed.");

    /* Start the thread again. */
    L4_MsgClear(&msg);
    L4_MsgLoad(&msg);
    tag = L4_Reply(from);
    fail_unless(L4_IpcSucceeded(tag) != 0, "IPC Failed");

    while (ext0310_progress != 2) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread));
    }

    /* Delete the thread. */
    okl4_pd_thread_delete(pd, thread);

    /* Detach the extension. */
    okl4_pd_extension_detach(pd, ext);

    /* Detach memsections */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        okl4_pd_memsec_detach(pd, ctest_segments[i]);
    }
    okl4_extension_memsec_detach(ext, ms);
    free(ms);

    /* Delete extension. */
    okl4_extension_delete(ext);
    free(ext);

    /* Delete the PD */
    delete_pd(pd);
}
END_TEST




ALIGNED(32) static okl4_word_t ext0400_stack[STACK_SIZE];
volatile okl4_extension_token_t ext0400_token;
okl4_word_t ext0400_base;
int ext0400_progress;

static void
ext0400_child(void)
{
    L4_ThreadId_t from;
    L4_MsgTag_t tag;

    okl4_init_thread();

    OKL4_EXTENSION_ACTIVATE(&ext0400_token);

    ext0400_progress = 1;

    tag = L4_Wait(&from);
    fail_unless(L4_IpcSucceeded(tag) != 0, "IPC Failed");

    *(volatile int*)ext0400_base = 1;

    OKL4_EXTENSION_DEACTIVATE(&ext0400_token);

    ext0400_progress = 2;

    L4_WaitForever();
}

/**
 *  Protection domain with an extension space. Activate the 
 *  extension. Map a new zone into the base pd. Ensure that the 
 *  activated thread can access the new mapping.
 */
START_TEST(EXT0400)
{
    int i, error;
    okl4_pd_t *pd;
    okl4_extension_t *ext;
    okl4_zone_t *zone;
    ms_t *ms;
    okl4_pd_zone_attach_attr_t zone_attr;
    okl4_kthread_t *thread;

    /* Create a PD / Extension / Memsec. */
    pd = create_pd();
    ext = create_extension(pd);

    /* Attach the extension. */
    error = okl4_pd_extension_attach(pd, ext);
    fail_unless(error == 0, "Could not attach extension.");

    /* Fetch a token for our user to use activate/deactivate. */
    ext0400_token = okl4_extension_gettoken(ext);

    /* Attach memsecs to the PD. These contain program segments. */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        error  = okl4_pd_memsec_attach(pd, ctest_segments[i]);
        fail_unless(error == 0, "Could not attach memsections.");
    }

    /* Create memsecs using the default pool. */
    ms = create_memsec(ANONYMOUS_BASE, OKL4_DEFAULT_PAGESIZE, NULL);
    ext0400_base = ms->virt.base;

    /* Create a zone */
    zone = create_zone(ms->virt.base, OKL4_ZONE_ALIGNMENT);
    fail_unless(error == OKL4_OK, "Could not attach a memsec to a zone");

    /* Attach memsec to zone */
    error = okl4_zone_memsec_attach(zone, ms->memsec);

    /* Setup progress */
    ext0400_progress = 0;

    /* Run the thread in the PD */
    thread = run_thread_in_pd(pd,
            (okl4_word_t)&ext0400_stack[STACK_SIZE],
            (okl4_word_t)ext0400_child);

    while (ext0400_progress != 1) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread));
    }

    /* Attach zone to PD */
    okl4_pd_zone_attach_attr_init(&zone_attr);
    error = okl4_pd_zone_attach(pd, zone, &zone_attr);
    fail_unless(error == OKL4_OK, "Could not attach a zone to a pd");

    /* Reply to thread */
    send_ipc(okl4_kthread_getkcap(thread));
    while (ext0400_progress != 2) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread));
    }

    /* Delete the thread. */
    okl4_pd_thread_delete(pd, thread);

    /* Detach the extension. */
    okl4_pd_extension_detach(pd, ext);

    /* Detach memsections */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        okl4_pd_memsec_detach(pd, ctest_segments[i]);
    }
    okl4_zone_memsec_detach(zone, ms->memsec);

    /* Delete extension. */
    okl4_extension_delete(ext);
    free(ext);

    /* Delete the zone */
    okl4_pd_zone_detach(pd, zone);
    okl4_zone_delete(zone);
    free(zone);

    /* Delete the PD */
    okl4_pd_delete(pd);
    free(pd);

    /* Delete the memsec */
    delete_memsec(ms, NULL);
}
END_TEST


ALIGNED(32) static okl4_word_t ext0500_stack[STACK_SIZE];
volatile okl4_extension_token_t ext0500_token;
volatile int ext0500_progress;
volatile okl4_word_t ext0500_zone;

static void
ext0500_child(void)
{
    L4_ThreadId_t from;
    L4_MsgTag_t tag;

    okl4_init_thread();

    OKL4_EXTENSION_ACTIVATE(&ext0500_token);

    /* Ensure we can touch the extension space and the zone. */
    *(volatile int*)ext0500_zone = 1;

    /* Wait for main thread to detach the zone. */
    ext0500_progress = 1;
    tag = L4_Wait(&from);
    fail_unless(L4_IpcSucceeded(tag) != 0, "IPC Failed");

    /* Ensure that we can not touch the zone any longer. */
    ext0500_progress = 2;
    *(volatile int*)ext0500_zone = 1;

    /* Should not reach. */
    while (1) {
        assert(!"Should not reach.");
    }
}


/**
 * Attach an extension after a zone has been attached.
 * Detach a zone from the base space with an extension attached.
 */
START_TEST(EXT0500)
{
    int i, error;
    okl4_pd_t *pd;
    okl4_extension_t *ext;
    okl4_zone_t *zone;
    ms_t *ms;
    L4_ThreadId_t from;
    okl4_word_t addr;

    okl4_kthread_t *thread;
    okl4_pd_thread_create_attr_t thread_attr;

    /* Create a PD / Extension / Memsec. */
    pd = create_pd();
    ext = create_extension(pd);

    /* Create a memsection */
    ms = create_memsec(ANONYMOUS_BASE, OKL4_DEFAULT_PAGESIZE, NULL);
    ext0500_zone = ms->virt.base;

    /* Create a zone, and attach the memsection */
    zone = create_zone(ms->virt.base, OKL4_ZONE_ALIGNMENT);
    error = okl4_zone_memsec_attach(zone, ms->memsec);
    fail_unless(error == 0, "Could not attach memsec to zone.");

    /* Attach zone to PD */
    attach_zone(pd, zone);

    /* Attach the extension. */
    error = okl4_pd_extension_attach(pd, ext);
    fail_unless(error == 0, "Could not attach extension.");

    /* Fetch a token for our user to use activate/deactivate. */
    ext0500_token = okl4_extension_gettoken(ext);

    /* Attach memsecs to the PD. These contain program segments. */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        error  = okl4_pd_memsec_attach(pd, ctest_segments[i]);
        fail_unless(error == 0, "Could not attach memsections.");
    }

    /* Setup progress */
    ext0500_progress = 0;

    /* Start a thread in the PD */
    okl4_pd_thread_create_attr_init(&thread_attr);
    okl4_pd_thread_create_attr_setspip(&thread_attr,
            (okl4_word_t)&ext0500_stack[STACK_SIZE],
            (okl4_word_t)ext0500_child);
    okl4_pd_thread_create_attr_setpager(&thread_attr,
            okl4_kthread_getkcap(root_thread));
    error = okl4_pd_thread_create(pd, &thread_attr, &thread);
    fail_unless(error == OKL4_OK, "Could not create thread");
    okl4_pd_thread_start(pd, thread);

    /* Wait for thread to startup and activate the extension. */
    while (ext0500_progress != 1) {
        L4_ThreadSwitch(okl4_kthread_getkcap(thread));
    }

    /* Detach and delete the zone */
    okl4_pd_zone_detach(pd, zone);
    okl4_zone_memsec_detach(zone, ms->memsec);

    /* Reply to thread */
    send_ipc(okl4_kthread_getkcap(thread));

    /* Ensure thread pagefaults. */
    wait_for_pagefault_ipc(&from, &addr);
    fail_unless(addr == ext0500_zone,
            "Thread faulted on wrong address.");
    fail_unless(ext0500_progress == 2,
            "Thread did not make correct progress.");

    /* Delete the thread. */
    okl4_pd_thread_delete(pd, thread);

    /* Detach the extension. */
    okl4_pd_extension_detach(pd, ext);

    /* Detach memsections */
    for (i = 0; ctest_segments[i] != NULL; i++) {
        okl4_pd_memsec_detach(pd, ctest_segments[i]);
    }

    /* Delete extension. */
    okl4_extension_delete(ext);
    free(ext);

    /* Delete the PD */
    okl4_pd_delete(pd);
    free(pd);

}
END_TEST

TCase *
make_extension_tcase(void)
{
    TCase * tc;

    tc = tcase_create("Extensions");
    tcase_add_test(tc, EXT0100);
    tcase_add_test(tc, EXT0200);
    tcase_add_test(tc, EXT0210);
    tcase_add_test(tc, EXT0300);
    tcase_add_test(tc, EXT0310);
    tcase_add_test(tc, EXT0400);
    tcase_add_test(tc, EXT0500);

    return tc;
}
