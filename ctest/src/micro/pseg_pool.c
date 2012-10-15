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
#include <okl4/physmem_item.h>
#include <okl4/physmem_pagepool.h>
#include <okl4/physmem_segpool.h>

#define POOL_START 0x10000000
#define POOL_SIZE  0x01000000
#define PAGE_SIZE 4096

//#define BAD_ASSERT_CHECKS

okl4_physmem_segpool_t *test_pool;

/*
 * Weaved root physical segment pool lookup
 */
START_TEST(PSEGPOOL0100)
{
    int rv;
    okl4_word_t size;
    okl4_physmem_item_t item;

    // allocate item
    okl4_physmem_item_setsize(&item, POOL_SIZE);
    rv = okl4_physmem_segpool_alloc(test_pool, &item);
    fail_unless(rv == OKL4_OK,
                "failed to allocate physmem item by length from root pool");

    size = okl4_physmem_item_getsize(&item);
    fail_unless(size == POOL_SIZE,
                "physmem item allocated by length from root pool is the wrong size");

    // free item
    okl4_physmem_segpool_free(test_pool, &item);
}
END_TEST

/*
 * From weaved root pool, dynamically initialize pool by size, and destroy it
 */
START_TEST(PSEGPOOL0101)
{
    int rv;
    okl4_word_t size;
    okl4_physmem_segpool_t *pool;
    okl4_physmem_item_t item;
    okl4_physmem_pool_attr_t attr;

    // dynamically create a pool from it
    okl4_physmem_pool_attr_init(&attr);
    okl4_physmem_pool_attr_setsize(&attr, POOL_SIZE);
    okl4_physmem_pool_attr_setparent(&attr, test_pool);
    okl4_physmem_pool_attr_setpagesize(&attr, PAGE_SIZE);

    pool = malloc(OKL4_PHYSICAL_POOL_SIZE_ATTR(attr));
    fail_if(pool == NULL, "failed to malloc psegpool");

    rv = okl4_physmem_segpool_alloc_pool(pool, &attr);
    fail_unless(rv == OKL4_OK, "failed to allocate new pool by size");

    // allocate item
    okl4_physmem_item_setsize(&item, POOL_SIZE);
    rv = okl4_physmem_segpool_alloc(pool, &item);
    fail_unless(rv == OKL4_OK,
                "failed to allocate physmem item by length from new pool");

    size = okl4_physmem_item_getsize(&item);
    fail_unless(size == POOL_SIZE,
                "physmem item allocated by length from new pool is the wrong size");

    // cleanup
    okl4_physmem_segpool_free(pool, &item);
    okl4_physmem_segpool_destroy(pool);
    free(pool);
}
END_TEST


/*
 * From weaved root pool, dynamically initialize pool by range, and destroy it
 */
START_TEST(PSEGPOOL0102)
{
    int rv;
    okl4_word_t size;
    okl4_physmem_segpool_t *pool;
    okl4_physmem_item_t item;
    okl4_physmem_pool_attr_t attr;

    // dynamically create a pool from it
    okl4_physmem_pool_attr_init(&attr);
    okl4_physmem_pool_attr_setrange(&attr, POOL_START, POOL_SIZE);
    okl4_physmem_pool_attr_setparent(&attr, test_pool);
    okl4_physmem_pool_attr_setpagesize(&attr, PAGE_SIZE);

    pool = malloc(OKL4_PHYSICAL_POOL_SIZE_ATTR(attr));
    fail_if(pool == NULL, "failed to malloc psegpool");

    rv = okl4_physmem_segpool_alloc_pool(pool, &attr);
    fail_unless(rv == OKL4_OK, "failed to allocate new pool by range");

    // allocate item
    okl4_physmem_item_setsize(&item, POOL_SIZE);
    rv = okl4_physmem_segpool_alloc(pool, &item);
    fail_unless(rv == OKL4_OK,
                "failed to allocate physmem item by length from new pool");

    size = okl4_physmem_item_getsize(&item);
    fail_unless(size == POOL_SIZE,
                "physmem item allocated by length from new pool is the wrong size");

    // cleanup
    okl4_physmem_segpool_free(pool, &item);
    okl4_physmem_segpool_destroy(pool);
    free(pool);
}
END_TEST

#ifdef BAD_ASSERT_CHECKS
/*
 * Attempt to dynamically initialize pool with no parent
 * (NB: Tests appropriate triggering of an assertion error)
 */
START_TEST(PSEGPOOL0103)
{
    int rv;
    struct okl4_physmem_segpool pool;
    okl4_physmem_pool_attr_t attr;

    // attempt to dynamically create a root page pool from no parent
    okl4_physmem_pool_attr_init(&attr);
    okl4_physmem_pool_attr_setrange(&attr, POOL_START, POOL_SIZE);
    okl4_physmem_pool_attr_setparent(&attr, NULL);
    okl4_physmem_pool_attr_setpagesize(&attr, PAGE_SIZE);

    /* this should trigger an assertion error */
    rv = okl4_physmem_segpool_alloc_pool(&pool, &attr);
    fail_if(1, "assert was not triggered on attempt to alloc "
                    "page pool without parent. shouldn't be here!");
    fail_if(rv == OKL4_OK, "furthermore, the attempt returned OKL4_OK!");
}
END_TEST
#endif // BAD_ASSERT_CHECKS

/* Create a new pool by interval and allocate a range from it */
START_TEST(PSEGPOOL0200)
{
    int rv;
    okl4_physmem_pool_attr_t attr;
    okl4_physmem_item_t item;
    okl4_word_t size;
    okl4_physmem_segpool_t *new_pool;

    /* Setup a new derived pool. */
    okl4_physmem_pool_attr_init(&attr);
    okl4_physmem_pool_attr_setrange(&attr,
            POOL_START, POOL_SIZE - (2 * PAGE_SIZE));
    okl4_physmem_pool_attr_setparent(&attr, test_pool);
    okl4_physmem_pool_attr_setpagesize(&attr, PAGE_SIZE);
    new_pool = malloc(OKL4_PHYSICAL_POOL_SIZE_ATTR(attr));
    assert(new_pool != NULL);
    rv = okl4_physmem_segpool_alloc_pool(new_pool, &attr);
    fail_unless(rv == OKL4_OK, "failed to allocate new pool");

    /* Allocate an item out of the pool. */
    okl4_physmem_item_setsize(&item, (PAGE_SIZE * 2));
    rv = okl4_physmem_segpool_alloc(new_pool, &item);
    fail_unless(rv == OKL4_OK,
                "failed to allocate physmem item by length from new pool");

    /* Ensure that the size is correct. */
    size = okl4_physmem_item_getsize(&item);
    fail_unless(size == (PAGE_SIZE * 2),
                "physmem item allocated by range from new pool is the wrong size");

    /* Attempt to  allocate memory out of the parent pool which overlaps
     * the child. */
    okl4_physmem_item_setrange(&item, POOL_START, (PAGE_SIZE * 2));
    rv = okl4_physmem_segpool_alloc(test_pool, &item);
    fail_if(rv == OKL4_OK, "attempt to allocate range overlapping child pool succeeded");

    /* Ensure we can still allocate from the end of the pool, which was not
     * given to the child. */
    okl4_physmem_item_setsize(&item, (PAGE_SIZE * 2));
    rv = okl4_physmem_segpool_alloc(test_pool, &item);
    fail_unless(rv == OKL4_OK, "anonymous allocation failed");
    okl4_physmem_segpool_free(test_pool, &item);

    okl4_physmem_item_setrange(&item, POOL_START + POOL_SIZE - PAGE_SIZE, PAGE_SIZE);
    rv = okl4_physmem_segpool_alloc(test_pool, &item);
    fail_unless(rv == OKL4_OK, "attempt to allocate from end of root pool failed");
    okl4_physmem_segpool_free(test_pool, &item);

    /* Delete the pool. */
    okl4_physmem_segpool_destroy(new_pool);
    free(new_pool);

    /* Ensure we can now use the memory taken by the pool. */
    okl4_physmem_item_setrange(&item, POOL_START, PAGE_SIZE);
    rv = okl4_physmem_segpool_alloc(test_pool, &item);
    fail_unless(rv == OKL4_OK,
            "attempt to allocate range after destroying child pool failed");
}
END_TEST

/* Allocate a region the same size as a pool, then free it and try to
 * allocate a region larger than the pool. */
START_TEST(PSEGPOOL0300)
{
    int rv;
    okl4_physmem_item_t item;
    okl4_physmem_pool_attr_t attr;
    okl4_physmem_segpool_t *new_pool;

    okl4_physmem_pool_attr_init(&attr);
    okl4_physmem_pool_attr_setsize(&attr, POOL_SIZE);
    okl4_physmem_pool_attr_setparent(&attr, test_pool);
    okl4_physmem_pool_attr_setpagesize(&attr, PAGE_SIZE);

    new_pool = malloc(OKL4_PHYSICAL_POOL_SIZE_ATTR(attr));
    assert(new_pool != NULL);

    rv = okl4_physmem_segpool_alloc_pool(new_pool, &attr);
    fail_unless (rv == OKL4_OK, "could not create new pool");

    okl4_physmem_item_setsize(&item, POOL_SIZE);
    rv = okl4_physmem_segpool_alloc(new_pool, &item);
    fail_unless (rv == OKL4_OK, "could not allocate range equal to pool size");

    okl4_physmem_segpool_free(new_pool, &item);
    okl4_physmem_item_setsize(&item, POOL_SIZE + PAGE_SIZE);
    rv = okl4_physmem_segpool_alloc(new_pool, &item);
    fail_if (rv == OKL4_OK, "successfully allocated range larger than pool size");

    /* Attempt to allocate a region outside the range of the pool */
    okl4_physmem_item_setrange(&item, 0, PAGE_SIZE * 4);
    rv = okl4_physmem_segpool_alloc(new_pool, &item);
    fail_if(rv == OKL4_OK, "successfully allocated outside range of the pool");

    okl4_physmem_segpool_destroy(new_pool);
    free(new_pool);
}
END_TEST

static struct okl4_physmem_segpool *
pool_dyn_init_interval(struct okl4_physmem_segpool *parent, word_t start, word_t size)
{
    int rv;
    struct okl4_physmem_segpool *pool;
    okl4_physmem_pool_attr_t attr;

    okl4_physmem_pool_attr_init(&attr);
    okl4_physmem_pool_attr_setparent(&attr, parent);
    okl4_physmem_pool_attr_setrange(&attr, start, size);

    pool = malloc(OKL4_PHYSICAL_POOL_SIZE_ATTR(&attr));

    if (pool == NULL) {
        return NULL;
    }

    rv = okl4_physmem_segpool_alloc_pool(pool, &attr);
    if (rv != OKL4_OK) {
        free(pool);
        return NULL;
    }

    return pool;
}


static void
recursive_child_pool_init_test(struct okl4_physmem_segpool *pool)
{
    word_t child0_size, child1_size;
    word_t pool_base, pool_size, base, size, child_base;
    struct okl4_physmem_segpool    *child0, *child1;

    pool_base = okl4_range_item_getbase(&pool->phys.range);
    pool_size   = okl4_range_item_getsize(&pool->phys.range);

    child0_size = pool_size / 2;
    child1_size = pool_size - child0_size;

    if (child0_size == 0) {
        return;
    }

    // create child pool occupying about half of space
    child0 = pool_dyn_init_interval(pool, pool_base, child0_size);
    fail_if(child0 == NULL, "failed to create child pool");

    size = okl4_range_item_getsize(&child0->phys.range);
    base = okl4_range_item_getbase(&child0->phys.range);
    fail_unless(base == pool_base, "child allocated has wrong start value");
    fail_unless(size == child0_size, "child allocated is the wrong size");

    // create another child pool from remaining space and test it
    child_base = base + size;

    child1 = pool_dyn_init_interval(pool, child_base, child1_size);
    fail_if(child1 == NULL, "failed to create child pool");

    size = okl4_range_item_getsize(&child1->phys.range);
    base = okl4_range_item_getbase(&child1->phys.range);
    fail_unless(base == child_base, "child allocated has wrong start value");
    fail_unless(size == child1_size, "child allocated is the wrong size");

    recursive_child_pool_init_test(child1);

    // free children
    okl4_physmem_segpool_destroy(child0);
    okl4_physmem_segpool_destroy(child1);
    free(child0);
    free(child1);
}

/*
 * Child pool initialization stress test
 */
START_TEST(PSEGPOOL0700)
{
    recursive_child_pool_init_test(test_pool);

}
END_TEST


static void test_setup(void)
{
    okl4_physmem_pool_attr_t attr;
    okl4_physmem_pool_attr_init(&attr);
    okl4_physmem_pool_attr_setparent(&attr, NULL);
    okl4_physmem_pool_attr_setrange(&attr, POOL_START, POOL_SIZE);
    okl4_physmem_pool_attr_setpagesize(&attr, PAGE_SIZE);
    test_pool = malloc(OKL4_PHYSICAL_POOL_SIZE_ATTR(&attr));
    assert(test_pool);
    okl4_physmem_segpool_init(test_pool, &attr);
}

static void test_teardown(void)
{
    free(test_pool);
}

TCase *
make_pseg_pool_tcase(void)
{
    TCase *tc;

    tc = tcase_create("Physical Segment Pool");
    tcase_add_checked_fixture(tc, test_setup, test_teardown);
    tcase_add_test(tc, PSEGPOOL0100);
    tcase_add_test(tc, PSEGPOOL0101);
    tcase_add_test(tc, PSEGPOOL0102);
#ifdef BAD_ASSERT_CHECKS
    tcase_add_test(tc, PSEGPOOL0103);
#endif // BAD_ASSERT_CHECKS
    tcase_add_test(tc, PSEGPOOL0200);
    tcase_add_test(tc, PSEGPOOL0300);
    tcase_add_test(tc, PSEGPOOL0700);

    return tc;
}
