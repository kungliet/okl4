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

#include <okl4/init.h>
#include <ctest/ctest.h>

#ifdef OKL4_KERNEL_MICRO
#include <stdio.h>
#endif

#if defined(CONFIG_SMT)
#include <okl4/pd.h>
#endif

#include "clock_driver.h"

#define TCASE(x) suite_add_tcase(suite, make_ ## x ## _tcase())

#if defined(CONFIG_SMT)

#if !defined(CONFIG_NUM_UNITS)
#define CONFIG_NUM_UNITS 6
#endif

/* True if the additional SMT hardware threads are enabled. Used to
 * guard agains disabling hw threads more than once. */
static int additional_hw_threads_enabled = 1;

/* This is useful for creating threads in the root space. */
extern okl4_pd_t *root_pd;

/* Array of pointers to spinner threads. */
static okl4_kthread_t *spinning_threads[CONFIG_NUM_UNITS - 1];

/* Function for busy-idle threads. */
static void
loop_forever(void)
{
    for (;;);
}

/* This function effectively disables all but a single hw thread on
 * SMT systems by making additional threads wait in a busy loop. */
static void
disable_additional_hw_threads(void)
{
    okl4_word_t i;

    if (!additional_hw_threads_enabled) {
        return;
    }

    /* Enable us to run on any hardware thread. */
    L4_Schedule(L4_Myself(), -1UL, 0xffffffff, -1UL, -1UL, 0, NULL);

    /* Create the spinning threads. */
    for (i = 0; i < CONFIG_NUM_UNITS - 1; i++) {
        int error;
        char thread_name[16];
        okl4_pd_thread_create_attr_t thread_attr;

        /* Create and start a thread in the root_pd. */
        okl4_pd_thread_create_attr_init(&thread_attr);
        okl4_pd_thread_create_attr_setspip(&thread_attr, 0xdeadbeef,
                (okl4_word_t)loop_forever);
        error = okl4_pd_thread_create(root_pd, &thread_attr, &spinning_threads[i]);
        assert(error == OKL4_OK);
        okl4_pd_thread_start(root_pd, spinning_threads[i]);

        /* Give thread the maximum priority, and only let it run on
         * the latter (CONFIG_NUM_UNITS - 1) hardware units. */
        error = L4_Schedule(spinning_threads[i]->cap, 0, 0xfffffffe, -1UL,
                255, 0, NULL);
        assert(error != 0);

        /* Provide the thread with a name. */
        sprintf(thread_name, "spinner%lu", i);
        L4_KDB_SetThreadName(spinning_threads[i]->cap, thread_name);
    }

    /* Constrain us back to the first hardware thread. */
    L4_Schedule(L4_Myself(), -1UL, 0x00000001, -1UL, -1UL, 0, NULL);

    additional_hw_threads_enabled = 0;
}
#endif

static Suite *
make_suite(void)
{
    Suite * suite;

    suite = suite_create("Cell test suite");

    TCASE(bitmap_allocator);
    TCASE(range_allocator);
    TCASE(kthread);
    TCASE(notify);
    TCASE(message);
    TCASE(mutex);
    TCASE(memsec);
    TCASE(pd);

#ifdef OKL4_KERNEL_NANO
    TCASE(semaphore);
    TCASE(barrier);
#endif /* OKL4_KERNEL_NANO */

#ifdef OKL4_KERNEL_MICRO
    TCASE(kmutex_id);
    TCASE(kclist_id);
    TCASE(kspace_id);
    TCASE(virt_pool);
    TCASE(pseg_pool);
    TCASE(page_pool);
    TCASE(env);
    TCASE(utcb);
    TCASE(kclist);
    TCASE(kspace);
    TCASE(zone);
    TCASE(extension);
    TCASE(interrupt);
#endif /* OKL4_KERNEL_MICRO */

    return suite;
}

int
main(int argc, char *argv[])
{
    int i;
    int error;
    SRunner * sr;

    init_root_pools();

#ifdef OKL4_KERNEL_MICRO
    init_micro_root_pools();
#endif
    okl4_init_thread();

    /* Setup clock for interrupt tests. */
    error = init_clock();
    if (error) {
        printf("\n");
        printf("*** WARNING: Clock could not be initialised.\n");
        printf("             Interrupt tests will not be performed.\n");
        printf("\n");
    }

    /* Check and print the command line arguments. */
    if (argv[argc] != NULL) {
        printf("ERROR: Argv array is not NULL terminated.\n");

        /* Not reached. */
        for (;;) {}
    }

    printf("argc    = %d\n", argc);
    for (i = 0; i < argc; i++)
    {
        printf("argv[%d] = %s\n", i, argv[i]);
    }

#if defined(CONFIG_SMT)
    /* Ctest is not thread safe so on SMT CPUs we disable all but one
     * of the hardware threads. */
    disable_additional_hw_threads();
#endif

    sr = srunner_create(make_suite());
    srunner_run_all(sr, CK_VERBOSE);
    srunner_free(sr);

    printf("\n");
    printf("CTest Done\n");
    printf("\n");

    for (;;) {}
}
