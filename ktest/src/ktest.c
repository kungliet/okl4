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

#include <stdio.h>
#include <stdlib.h>
#include <l4/kdebug.h>
#include <l4/profile.h>
#include <l4/schedule.h>
#include <l4/ipc.h>
#include <l4/config.h>
#include <compat/c.h>
#include <ktest/ktest.h>
#include <ktest/utility.h>

#include <okl4/bitmap.h>
#include <okl4/env.h>
#include <okl4/kspaceid_pool.h>
#include <okl4/kclistid_pool.h>
#include <okl4/kmutexid_pool.h>

extern char * arch_excludes [];

L4_SpaceId_t KTEST_SPACE;
L4_ClistId_t KTEST_CLIST;

/* Put any tests that crash the kernel in here to avoid running them
 * in regression */
/* to run these tests either remove them from this list or comment out
 * the line that passes the list to lib check below */
char * hanging_tests [] = {
    /*"kmem02",*/
//    "FUZZ0100",
//    "FUZZ0201",
//    "FUZZ0500",
//    "FUZZ0701",
//    "FUZZ0801",
//    "FUZZ0802",
    NULL};

#define TCASE(x) suite_add_tcase(suite, make_ ## x ## _tcase())

static Suite *
make_suite(void)
{
    Suite * suite;

    suite = suite_create("L4 Test suite");
    TCASE(heap_exhaustion);
    TCASE(kdb);
    TCASE(mapcontrol);
    TCASE(spacecontrol);
    TCASE(ipc);
    TCASE(xas_ipc);
    TCASE(aipc);
    TCASE(xas_aipc);
    TCASE(ipc_cpx);
    TCASE(thread);
    TCASE(cache);
    TCASE(caps);
    TCASE(virtual_register);
    TCASE(thread_id);
    TCASE(cap_id);
    TCASE(threadcontrol);
    TCASE(exchange_registers);
    TCASE(spaceswitch);
    TCASE(kmem);
    TCASE(fuzz);
    TCASE(schedule);
    TCASE(preempt);
    TCASE(cust);
    TCASE(mutex);
    TCASE(arch);
    TCASE(chk_breakin_test);
    TCASE(platform_control);
#if defined(CONFIG_SCHEDULE_INHERITANCE)
    TCASE(ipc_schedule_inheritance);
    TCASE(mutex_schedule_inheritance);
    TCASE(schedule_inheritance_graph);
#endif
    TCASE(interrupt_control);
#if defined(CONFIG_REMOTE_MEMORY_COPY)
    TCASE(remote_memcpy);
#endif
    return suite;
}

/* Global config information to get out of the environment. */
word_t kernel_spaceid_base;
word_t kernel_spaceid_entries;
word_t kernel_mutexid_base;
word_t kernel_mutexid_entries;
word_t kernel_max_threads;
word_t kernel_phys_base;
word_t kernel_phys_end;
word_t kernel_max_root_caps;
word_t kernel_test_segment_id;
word_t kernel_test_segment_size;
word_t kernel_test_segment_vbase;
L4_Word_t VALID_IRQ1;
L4_Word_t VALID_IRQ2;

struct okl4_bitmap_allocator *spaceid_pool, *mutexid_pool, *clistid_pool;

static int
init_boot_params(void)
{
    okl4_env_kernel_info_t *kernel_info;
    okl4_env_segment_t *seg;
    okl4_allocator_attr_t attr;
    okl4_env_device_irqs_t *test_irqs;

    kernel_info = OKL4_ENV_GET_KERNEL_INFO("OKL4_KERNEL_INFO");
    if (kernel_info == NULL) {
        return -1;
    }
    kernel_phys_end = kernel_info->end;
    kernel_phys_base = kernel_info->base;

    spaceid_pool = okl4_env_get("MAIN_SPACE_ID_POOL");
    clistid_pool = okl4_env_get("MAIN_CLIST_ID_POOL");
    mutexid_pool = okl4_env_get("MAIN_MUTEX_ID_POOL");
    if (!spaceid_pool || !mutexid_pool || !clistid_pool) {
        printf("Couldn't get all range id pools\n");
        exit(-1);
    }

    /* The following values should be provided by the environment */
    kernel_spaceid_base = *(OKL4_ENV_GET_MAIN_SPACEID("MAIN_SPACE_ID"));
    okl4_kspaceid_getattribute(spaceid_pool, &attr);
    printf("spaceid pool base %ld, size %ld\n", attr.base, attr.size);
    kernel_spaceid_entries = attr.size;
    okl4_kmutexid_getattribute(mutexid_pool, &attr);
    kernel_mutexid_base = attr.base;
    kernel_mutexid_entries = attr.size;

    kernel_max_threads = kernel_info->max_threads;

    printf("\nKernel info: max space = %lu, max mutex = %lu, max_thread = %lu, phys [0x%lx, 0x%lx]\n",
            kernel_info->max_spaces, kernel_info->max_mutexes, kernel_max_threads,
            kernel_phys_base, kernel_phys_end);

    seg = okl4_env_get_segment("MAIN_KTEST_SEGMENT");
    assert(seg != NULL);

    kernel_test_segment_id = seg->segment;
    kernel_test_segment_size = seg->size;
    kernel_test_segment_vbase = seg->virt_addr;

    test_irqs = okl4_env_get("TEST_DEV_IRQ_LIST");
    if (test_irqs == NULL) {
        test_irqs = okl4_env_get("NO_DEVICE_IRQ_LIST");
    }
    assert(test_irqs != NULL);
    assert(test_irqs->num_irqs >= 2);
    VALID_IRQ1 = test_irqs->irqs[0];
    VALID_IRQ2 = test_irqs->irqs[1];

    return 0;
}

int
main(int argc, char *argv[])
{
    int n, ht_size=0, ax_size=0;
    SRunner * sr;
    char ** excluded_tests;
    int result;

    KTEST_SPACE = L4_SpaceId(*(OKL4_ENV_GET_MAIN_SPACEID("MAIN_SPACE_ID")));
    KTEST_CLIST = L4_ClistId(*(OKL4_ENV_GET_MAIN_CLISTID("MAIN_CLIST_ID")));
    L4_Profiling_Reset();

    L4_KDB_SetSpaceName(KTEST_SPACE, "ktest");

    libcheck = KTEST_SERVER;

    result = init_boot_params();
    if (result != 0) {
        printf("Couldn't find boot parameters\n");
        exit(-1);
    }

    /* Lower our priority to 254, allowing system threads to override
     * us if necessary. */
    L4_Set_Priority(L4_Myself(), 254);

    /* Architecture-specific setup. */
    ktest_arch_setup();

    /* Construct a list of tests to be excluded */ 
    while (hanging_tests[ht_size++]);
    while (arch_excludes[ax_size++]);
    excluded_tests = malloc((ht_size + ax_size - 1) * sizeof(char *));
    for (n = 0; n < ht_size - 1; n++) {
        excluded_tests[n] = hanging_tests[n];
    }
    for (; n < ht_size + ax_size - 2; n++) {
        excluded_tests[n] = arch_excludes[n - ht_size + 1];
    }
    excluded_tests[n] = NULL;

    sr = srunner_create(make_suite());
    srunner_exceptions(sr, excluded_tests);
    srunner_set_fork_status(sr, CK_WITHPAGER);

    L4_Profiling_Start();

    srunner_run_all(sr, CK_VERBOSE);

    L4_Profiling_Stop();

    n = srunner_ntests_failed(sr);
    srunner_free(sr);
    free(excluded_tests);


    L4_Profiling_Print();
#if defined(CONFIG_DEBUG)
    L4_KDB_Breakin_Enable();
#endif
    L4_KDB_Enter("L4Test Done");

    for (;;) {}
}
