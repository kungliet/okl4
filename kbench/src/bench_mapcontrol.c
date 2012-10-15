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
 * Last Modified: Chang Hua Chen  Aug 2 2004
 */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <l4/config.h>
#include <l4/map.h>
#include <l4/space.h>
#include <l4/thread.h>
#include <l4/misc.h>

#include <okl4/env.h>
#include <okl4/kspaceid_pool.h>

#include <bench/bench.h>

#define UTCB_AREA_BASE  (16 * 1024 * 1024)


/*
 * Setup:
 * 1. create thread in new address space
 * 2. create enough fpage objects
 * 3. check memory descriptors for available pmem
 * 3. create enough ppage objects
 *
 *
 * Test:
 * 1. insert|delete|query many mappings into one ppage
 * 2. insert|delete|query many mappings into many ppages
 * 3. insert|delete|query many mappings into one ppage (2|4 maps / syscall)
 * 4. insert|delete|query many mappings into many ppages (2|4 maps / syscall)
 * 5. delete by killing thread
 *
 */

static  L4_MapItem_t    *map_item_list;
static  L4_MapItem_t    *unmap_item_list;

static  L4_ThreadId_t   test_tid;
static  L4_SpaceId_t    test_space;
L4_Fpage_t kip_area, utcb_area;


extern word_t kernel_test_segment_size;
extern word_t kernel_test_segment_vbase;
extern word_t get_seg(L4_SpaceId_t spaceid, word_t vaddr, word_t *offset, word_t *cache, word_t *rwx);

static int init_mem(int num_pages)
{
    L4_Word_t min_pgbits = L4_GetMinPageBits();
    L4_Word_t min_pgsize = 1 << min_pgbits;
    L4_Word_t min_size = min_pgsize * num_pages;
    int i;

    if (kernel_test_segment_size < min_size)
        return 1;

    for (i = 0; i < num_pages; i++) {
        L4_Word_t seg, offset, vaddr, cache, rwx;
        vaddr = kernel_test_segment_vbase + i * min_pgsize;
        seg = get_seg(test_space, vaddr, &offset, &cache, &rwx);
        assert(seg != ~0UL);
        L4_MapItem_Map(&map_item_list[i], seg, offset, vaddr, min_pgbits,
                       L4_DefaultMemory,
                       L4_Readable);
        L4_MapItem_Unmap(&unmap_item_list[i], vaddr, min_pgbits);
    }
    return 0;
}

extern okl4_kspaceid_pool_t *spaceid_pool;

static void
mapcontrol_setup(struct bench_test *test, int args[])
{
    int i;
    int rv;

    /*printf("Running %s test with %d iterations\n", test->name, args[0]); */
    /* Create new address space */
    test_tid = L4_GlobalId(KBENCH_SERVER.X.index + 2, 2);
    rv           = okl4_kspaceid_allocany(spaceid_pool, &test_space);
    assert(rv == OKL4_OK);

#ifdef NO_UTCB_RELOCATE
    utcb_area = L4_Nilpage;
#else
    /* Utcb_area is 0x10000000, size 0x200 * 1000*/
    utcb_area = L4_Fpage (16 * 1024 * 1024,
                  L4_GetUtcbSize() * 1000 );
#endif

    //printf("new space id is 0x%lx, test_tid = 0x%lx, utch_area = 0x%lx, kip_area = 0x%lx\n", test_space.raw, test_tid.raw, utcb_area.raw, kip_area.raw);
    rv = L4_SpaceControl(test_space, L4_SpaceCtrl_new, KBENCH_CLIST, utcb_area, 0, NULL);
    if (rv == 0) {
        printf("space control(%d) Error: %"PRIxPTR"\n", args[0], L4_ErrorCode());
    }
    assert(rv == 1);

    /* Create thread in new address space */
    L4_Word_t utcb;
#ifdef NO_UTCB_RELOCATE
    utcb = -1UL;
#else
    utcb = 16 * 1024 * 1024 + L4_GetUtcbSize() * (args[0]/1000);
#endif

    rv = L4_ThreadControl(test_tid, test_space, KBENCH_SERVER, L4_nilthread, L4_nilthread, 0, (void *) utcb);
    if (rv == 0) {
        printf("thread control(%d) Error: %"PRIxPTR"\n", args[0], L4_ErrorCode());
    }
    /* make sure thread control succeeded */
    assert(rv == 1);
    /* Allocate list of fpages and ppages*/
    map_item_list  = (L4_MapItem_t *)malloc(sizeof(*map_item_list) * args[0]);
    unmap_item_list  = (L4_MapItem_t *)malloc(sizeof(*unmap_item_list) * args[0]);

    /* make sure allocation succeeded */
    assert(map_item_list != NULL);
    assert(unmap_item_list != NULL);

    i = init_mem(args[0]);
    assert(i==0);
}


static void
mapcontrol_test_i1m2m(struct bench_test *test, int args[])
{
    int i;
    int rv;

    for (i = 0; i < args[0]; i++) {
        /*
        L4_LoadMR(0, ppage_list[i].raw);
        L4_LoadMR(1, fpage_list[i].raw);
        rv  = L4_MapControl(test_space, L4_MapCtrl_Modify);
        */
        rv = L4_ProcessMapItem(test_space, map_item_list[i]);
        assert(rv == 1);
    }
}


static void
mapcontrol_test_i2m2m(struct bench_test *test, int args[])
{
    int i;
    int rv;

    for (i = 0; i < args[0] / 2; i++) {
        /*
        L4_LoadMR(0, ppage_list[2*i].raw);
        L4_LoadMR(1, fpage_list[2*i].raw);
        L4_LoadMR(2, ppage_list[2*i + 1].raw);
        L4_LoadMR(3, fpage_list[2*i + 1].raw);
        rv  = L4_MapControl(test_space, L4_MapCtrl_Modify | 1);
        */
        rv = L4_ProcessMapItems(test_space, 2, &map_item_list[2*i]);
        assert(rv == 1);
    }
}

static void
mapcontrol_test_d1m2m(struct bench_test *test, int args[])
{
    int i;
    int rv;

    for (i = 0; i < args[0]; i++) {
        rv  = L4_ProcessMapItem(test_space, unmap_item_list[i]);
        assert(rv == 1);
    }
}


static void
mapcontrol_test_d2m2m(struct bench_test *test, int args[])
{
    int i;
    int rv;

    for (i = 0; i < args[0] / 2; i++) {
        rv  = L4_ProcessMapItems(test_space, 2, &unmap_item_list[2*i]);
        assert(rv == 1);
    }
}

static void
mapcontrol_test_ovh_1(struct bench_test *test, int args[])
{
    int i;
    int rv = 1;
    
    for (i = 0; i < args[0]; i++) {
        asm("" : "+r" (rv));
        assert(rv == 1);
    }
}

static void
mapcontrol_test_ovh_2(struct bench_test *test, int args[])
{
    int i;
    int rv = 1;
    
    for (i = 0; i < args[0] / 2; i++) {
        asm("" : "+r" (rv));
        assert(rv == 1);
    }
}

static void
mapcontrol_setup_and_map(struct bench_test *test, int args[])
{
    mapcontrol_setup(test, args);
    mapcontrol_test_i1m2m(test, args);
}


static void
mapcontrol_teardown(struct bench_test *test, int args[])
{
    int rv;

    free(map_item_list);
    free(unmap_item_list);
    /* Delete Thread */
    rv = L4_ThreadControl(test_tid, L4_nilspace, L4_nilthread, L4_nilthread, L4_nilthread, 0, (void *) 0);
    if (rv == 0) {
        printf("Thread delete(%d) Error: %"PRIxPTR"\n", args[0], L4_ErrorCode());
    }
    assert(rv == 1);
    /* Delete Space */
    rv = L4_SpaceControl(test_space, L4_SpaceCtrl_delete, KBENCH_CLIST, L4_Nilpage, 0, NULL);
    if (rv == 0) {
        printf("Space delete(%d) Error: %"PRIxPTR"\n", args[0], L4_ErrorCode());
    }
    assert(rv == 1);
    okl4_kspaceid_free(spaceid_pool, test_space);
}

/*
 * From bench_mapcontrol_*, we can get:
 * Latency of mapcontrol of insert 1 fpage = (bench of bench_mapcontrol_insert_m2m -
 * bench of bench_mapcontrol_ovh1) / 10000
 * Latency of mapcontrol of delete 1 fpage = (bench of bench_mapcontrol_delete_m2m -
 * bench of bench_mapcontrol_ovh1) / 10000
 * Latency of mapcontrol of insert 2 fpage = (bench of bench_mapcontrol_insert2_m2m -
 * bench of bench_mapcontrol_ovh2) / 5000
 * Latency of mapcontrol of delete 2 fpage = (bench of bench_mapcontrol_delete2_m2m -
 * bench of bench_mapcontrol_ovh2) / 5000
 */

struct bench_test bench_mapcontrol_insert_m2m = {
    "mapcontrol insert many to many",
    mapcontrol_setup,
    mapcontrol_test_i1m2m,
    mapcontrol_teardown,
    { 
        { &iterations, 10000, 10000, 1000, add_fn },
        { NULL }
    }
};


struct bench_test bench_mapcontrol_insert2_m2m = {
    "mapcontrol insert (2 per syscall) many to many",
    mapcontrol_setup,
    mapcontrol_test_i2m2m,
    mapcontrol_teardown,
    { 
        { &iterations, 10000, 10000, 1000, add_fn },
        { NULL }
    }
};


struct bench_test bench_mapcontrol_delete_m2m = {
    "mapcontrol delete many to many",
    mapcontrol_setup_and_map,
    mapcontrol_test_d1m2m,
    mapcontrol_teardown,
    { 
        { &iterations, 10000, 10000, 1000, add_fn },
        { NULL }
    }
};

struct bench_test bench_mapcontrol_delete2_m2m = {
    "mapcontrol delete (2 per syscall) many to many",
    mapcontrol_setup_and_map,
    mapcontrol_test_d2m2m,
    mapcontrol_teardown,
    { 
        { &iterations, 10000, 10000, 1000, add_fn },
        { NULL }
    }
};

struct bench_test bench_mapcontrol_ovh1 = {
    "mapcontrol overhead",
    mapcontrol_setup_and_map,
    mapcontrol_test_ovh_1,
    mapcontrol_teardown,
    { 
        { &iterations, 10000, 10000, 1000, add_fn },
        { NULL }
    }
};

struct bench_test bench_mapcontrol_ovh2 = {
    "mapcontrol overhead (2 per syscall)",
    mapcontrol_setup_and_map,
    mapcontrol_test_ovh_2,
    mapcontrol_teardown,
    { 
        { &iterations, 10000, 10000, 1000, add_fn },
        { NULL }
    }
};
