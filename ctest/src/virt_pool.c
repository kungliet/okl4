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
#include <okl4/errno.h>
#include <okl4/range.h>
#include <okl4/virtmem_pool.h>

#include <stdlib.h>

#define ALLOC_SIZE 4096
#define ALLOC_BASE(pool) okl4_virtmem_pool_getbase(pool)

#define POOL_SIZE (4*4096)
#define POOL_BASE okl4_virtmem_pool_getbase(root_virtmem_pool)

extern okl4_virtmem_pool_t *root_virtmem_pool;
okl4_virtmem_pool_t dyn_root_pool;

/*
 * Weaved root virtual memory pool lookup
 */
START_TEST(VIRTPOOL0100)
{
    int rv;
    okl4_word_t base, size;
    struct okl4_virtmem_pool *rootvpool;
    okl4_virtmem_item_t range;

    // lookup root pool
    rootvpool = root_virtmem_pool;

    // allocate range
    okl4_range_item_setsize(&range, ALLOC_SIZE);
    rv = okl4_virtmem_pool_alloc(rootvpool, &range);
    fail_unless(rv == OKL4_OK, "failed to allocate range by length from root vpool");

    base = okl4_range_item_getbase(&range);
    size = okl4_range_item_getsize(&range);
    fail_unless(size == ALLOC_SIZE, "range allocated by length from root vpool is the wrong size");
    fail_unless(base + size > base, "(base + size) overflows.");

    // free range
    okl4_virtmem_pool_free(rootvpool, &range);
}
END_TEST

/*
 * From weaved root pool, dynamically initialize pool by length, and destroy it
 */
START_TEST(VIRTPOOL0101)
{
    int rv;
    struct okl4_virtmem_pool    *vpool, *rootvpool;
    okl4_virtmem_pool_attr_t    attr;

    // lookup root pool
    rootvpool = root_virtmem_pool;

    okl4_virtmem_pool_attr_init(&attr);
    okl4_virtmem_pool_attr_setparent(&attr, rootvpool);
    okl4_virtmem_pool_attr_setsize(&attr, POOL_SIZE);

    vpool = malloc(OKL4_VIRTMEM_POOL_SIZE_ATTR(attr));
    fail_if(vpool == NULL, "failed to malloc vpool");

    rv = okl4_virtmem_pool_alloc_pool(vpool, &attr);
    fail_if(rv != OKL4_OK, "failed to allocate new pool");

    okl4_virtmem_pool_destroy(vpool);
    free(vpool);
}
END_TEST

/*
 * From weaved root pool, dynamically initialize pool by interval, and destroy it
 */
START_TEST(VIRTPOOL0102)
{
    int rv;
    struct okl4_virtmem_pool    *vpool, *rootvpool;
    okl4_virtmem_pool_attr_t    attr;

    // lookup root pool
    rootvpool = root_virtmem_pool;
    vpool = &dyn_root_pool;

    okl4_virtmem_pool_attr_init(&attr);
    okl4_virtmem_pool_attr_setparent(&attr, rootvpool);
    okl4_virtmem_pool_attr_setrange(&attr, POOL_BASE, POOL_SIZE);

    vpool = malloc(OKL4_VIRTMEM_POOL_SIZE_ATTR(attr));
    fail_if(vpool == NULL, "failed to malloc vpool");

    rv = okl4_virtmem_pool_alloc_pool(vpool, &attr);
    fail_if(rv != OKL4_OK, "failed to allocate new pool");

    okl4_virtmem_pool_destroy(vpool);
    free(vpool);
}
END_TEST

/*
 * Dynamically create root virtual memory pool
 */
START_TEST(VIRTPOOL0200)
{
    int rv;
    okl4_word_t base, size;
    struct okl4_virtmem_pool *rootvpool;
    okl4_virtmem_item_t range;
    okl4_virtmem_pool_attr_t attr;

    rootvpool = &dyn_root_pool;

    // Create root virtual memory pool
    okl4_virtmem_pool_attr_init(&attr);
    okl4_virtmem_pool_attr_setparent(&attr, NULL);
    okl4_virtmem_pool_attr_setrange(&attr, 0xa0000000U, POOL_SIZE);

    okl4_virtmem_pool_init(rootvpool, &attr);

    // allocate range
    okl4_range_item_setsize(&range, ALLOC_SIZE);
    rv = okl4_virtmem_pool_alloc(rootvpool, &range);
    fail_unless(rv == OKL4_OK, "failed to allocate range by length from root vpool");

    base = okl4_range_item_getbase(&range);
    size = okl4_range_item_getsize(&range);
    fail_unless(size == ALLOC_SIZE, "range allocated by length from root vpool is the wrong size");
    fail_unless(base + size > base, "(base + size) overflows");

    // free range
    okl4_virtmem_pool_free(rootvpool, &range);
}
END_TEST

/*
 * From dynamically created root pool, dynamically initialize pool by length,
 * and destroy it
 */
START_TEST(VIRTPOOL0201)
{
    int rv;
    struct okl4_virtmem_pool    *vpool, *rootvpool;
    okl4_virtmem_pool_attr_t    attr;

    // lookup root pool
    rootvpool = &dyn_root_pool;

    okl4_virtmem_pool_attr_init(&attr);
    okl4_virtmem_pool_attr_setparent(&attr, rootvpool);
    okl4_virtmem_pool_attr_setsize(&attr, POOL_SIZE);

    vpool = malloc(OKL4_VIRTMEM_POOL_SIZE_ATTR(attr));
    fail_if(vpool == NULL, "failed to malloc vpool");

    rv = okl4_virtmem_pool_alloc_pool(vpool, &attr);
    fail_if(rv != OKL4_OK, "failed to allocate new pool");

    okl4_virtmem_pool_destroy(vpool);
    free(vpool);
}
END_TEST

/*
 * From dynamically created root pool, dynamically initialize pool by interval,
 * and destroy it
 */
START_TEST(VIRTPOOL0202)
{
    int rv;
    struct okl4_virtmem_pool    *vpool, *rootvpool;
    okl4_virtmem_pool_attr_t    attr;

    // lookup root pool
    rootvpool = &dyn_root_pool;

    okl4_virtmem_pool_attr_init(&attr);
    okl4_virtmem_pool_attr_setparent(&attr, rootvpool);
    okl4_virtmem_pool_attr_setrange(&attr,
            okl4_range_item_getbase(&rootvpool->virt), POOL_SIZE);

    vpool = malloc(OKL4_VIRTMEM_POOL_SIZE_ATTR(attr));
    fail_if(vpool == NULL, "failed to malloc vpool");

    rv = okl4_virtmem_pool_alloc_pool(vpool, &attr);
    fail_if(rv != OKL4_OK, "failed to allocate new pool");

    okl4_virtmem_pool_destroy(vpool);
    free(vpool);

    okl4_virtmem_pool_destroy(rootvpool);
}
END_TEST

static struct okl4_virtmem_pool *
vpool_dyn_init_interval(struct okl4_virtmem_pool *parent, okl4_word_t start, okl4_word_t size)
{
    int rv;
    struct okl4_virtmem_pool *vpool;
    okl4_virtmem_pool_attr_t attr;

    okl4_virtmem_pool_attr_init(&attr);
    okl4_virtmem_pool_attr_setparent(&attr, parent);
    okl4_virtmem_pool_attr_setrange(&attr, start, size);

    vpool = malloc(OKL4_VIRTMEM_POOL_SIZE_ATTR(&attr));

    if (vpool == NULL) {
        return NULL;
    }

    rv = okl4_virtmem_pool_alloc_pool(vpool, &attr);
    if (rv != OKL4_OK) {
        free(vpool);
        return NULL;
    }

    return vpool;
}

/*
 * Allocate and free an item from a pool
 */
START_TEST(VIRTPOOL0300)
{
    int rv;
    okl4_word_t                      base, size;
    struct okl4_virtmem_pool    *vpool, *rootvpool;
    okl4_virtmem_item_t         range;

    // lookup root pool
    rootvpool = root_virtmem_pool;

    // create new pool
    assert(rootvpool != NULL);
    vpool = vpool_dyn_init_interval(rootvpool,
            okl4_virtmem_pool_getbase(rootvpool), 4096);
    fail_if(vpool == NULL, "failed to dynamically create virtual pool by interval");

    // allocate range from new pool
    okl4_range_item_setrange(&range, ALLOC_BASE(vpool), ALLOC_SIZE);
    rv = okl4_virtmem_pool_alloc(vpool, &range);
    fail_unless(rv == OKL4_OK, "failed to allocate range by interval from new vpool");

    // check validity
    base = okl4_range_item_getbase(&range);
    size = okl4_range_item_getsize(&range);
    fail_unless((base == ALLOC_BASE(vpool)), "range allocated by interval from new vpool has the wrong start value");
    fail_unless(size == ALLOC_SIZE, "range allocated by interval from new vpool has the wrong size value");

    // cleanup
    okl4_virtmem_pool_free(vpool, &range);
    okl4_virtmem_pool_destroy(vpool);
    free(vpool);
}
END_TEST


static void
recursive_child_vpool_init_test(struct okl4_virtmem_pool *vpool)
{
    int rv;
    okl4_word_t range_size;
    okl4_word_t vpool_base, vpool_size, base, size, child_base, child_size;
    struct okl4_range_item      range;
    struct okl4_virtmem_pool    *child;

    vpool_base = okl4_range_item_getbase(&vpool->virt);
    vpool_size   = okl4_range_item_getsize(&vpool->virt);

    range_size = vpool_size / 2;
    child_size = vpool_size - range_size;

    if (range_size == 0) {
        return;
    }

    // allocate range
    okl4_range_item_setrange(&range, vpool_base, range_size);
    rv = okl4_virtmem_pool_alloc(vpool, &range);
    fail_unless(rv == OKL4_OK, "failed to allocate range");

    size = okl4_range_item_getsize(&range);
    base = okl4_range_item_getbase(&range);

    fail_unless(base == vpool_base, "range allocated has wrong start value");
    fail_unless(size == range_size, "range allocated is the wrong size");

    // create child pool from remaining space and test it
    child_base = base + size;

    child = vpool_dyn_init_interval(vpool, child_base, child_size);
    fail_if(child == NULL, "failed to create child pool");

    size = okl4_range_item_getsize(&child->virt);
    base = okl4_range_item_getbase(&child->virt);
    fail_unless(base == child_base, "child allocated has wrong start value");
    fail_unless(size == child_size, "child allocated is the wrong size");

    recursive_child_vpool_init_test(child);

    // free child
    okl4_virtmem_pool_destroy(child);
    free(child);

    // free allocated range
    okl4_virtmem_pool_free(vpool, &range);
}

/*
 * Child pool initialization stress test
 */
START_TEST(VIRTPOOL0700)
{
    struct okl4_virtmem_pool *rootvpool;

    // lookup root pool
    rootvpool = root_virtmem_pool;

    recursive_child_vpool_init_test(rootvpool);

}
END_TEST

TCase *
make_virt_pool_tcase(void)
{
    TCase *tc;

    tc = tcase_create("Virtual Pool");
    tcase_add_test(tc, VIRTPOOL0100);
    tcase_add_test(tc, VIRTPOOL0101);
    tcase_add_test(tc, VIRTPOOL0102);
    tcase_add_test(tc, VIRTPOOL0200);
    tcase_add_test(tc, VIRTPOOL0201);
    tcase_add_test(tc, VIRTPOOL0202);
    tcase_add_test(tc, VIRTPOOL0300);
    tcase_add_test(tc, VIRTPOOL0700);

    return tc;
}
