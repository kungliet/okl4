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

#define NUM_PAGES 7
#define PAGE_SIZE 4096

#define POOL_SIZE (PAGE_SIZE * NUM_PAGES)
#define POOL_START(pool) (okl4_physmem_item_getoffset(      \
            okl4_physmem_pagepool_getphys(pool)))
#define POOL_END(pool) (POOL_START(pool) + POOL_SIZE)

extern okl4_physmem_segpool_t *root_physseg_pool;

okl4_physmem_pagepool_t *new_page_pool;

/*
 * Dynamic page pool creation by size from weaved physseg pool
 */
START_TEST(PAGEPOOL0100)
{
    int rv;
    okl4_word_t size;
    struct okl4_physmem_pagepool *pool;
    okl4_physmem_segpool_t *rootpool;
    okl4_physmem_item_t item;
    okl4_physmem_pool_attr_t attr;

    // look up the root physseg pool
    rootpool = root_physseg_pool;

    // dynamically create a page pool from it, by size
    okl4_physmem_pool_attr_init(&attr);
    okl4_physmem_pool_attr_setsize(&attr, POOL_SIZE);
    okl4_physmem_pool_attr_setparent(&attr, rootpool);
    okl4_physmem_pool_attr_setpagesize(&attr, PAGE_SIZE);

    pool = malloc(OKL4_PHYSMEM_PAGEPOOL_SIZE_ATTR(&attr));
    fail_unless(pool != NULL, "failed to malloc physmem page pool");

    rv = okl4_physmem_pagepool_alloc_pool(pool, &attr);
    fail_unless(rv == OKL4_OK, "failed to allocate new pool");
    assert(pool->phys.range.base % pool->pagesize == 0);

    // allocate item
    okl4_physmem_item_setsize(&item, PAGE_SIZE);
    rv = okl4_physmem_pagepool_alloc(pool, &item);
    fail_unless(rv == OKL4_OK,
                "failed to allocate physmem item by length from page pool");

    size = okl4_physmem_item_getsize(&item);
    fail_unless(size == PAGE_SIZE,
                "physmem item allocated by length from page pool is the wrong size");

    // cleanup
    okl4_physmem_pagepool_free(pool, &item);
    okl4_physmem_pagepool_destroy(pool);
    free(pool);
}
END_TEST

/*
 * Dynamic page pool creation by range from weaved physseg pool
 */
START_TEST(PAGEPOOL0101)
{
    int rv;
    okl4_word_t size;
    struct okl4_physmem_pagepool *pool;
    okl4_physmem_segpool_t *rootpool;
    okl4_physmem_item_t item;
    okl4_physmem_pool_attr_t attr;

    // look up the root physseg pool
    rootpool = root_physseg_pool;

    // dynamically create a page pool from it, by range
    okl4_physmem_pool_attr_init(&attr);
    okl4_physmem_pool_attr_setrange(&attr, 0, POOL_SIZE);
    okl4_physmem_pool_attr_setparent(&attr, rootpool);
    okl4_physmem_pool_attr_setpagesize(&attr, PAGE_SIZE);

    pool = malloc(OKL4_PHYSMEM_PAGEPOOL_SIZE_ATTR(&attr));
    fail_if(pool == NULL, "failed to malloc physmem page pool");

    rv = okl4_physmem_pagepool_alloc_pool(pool, &attr);
    fail_unless(rv == OKL4_OK, "failed to allocate new pool by range");

    // allocate item
    okl4_physmem_item_setsize(&item, PAGE_SIZE);
    rv = okl4_physmem_pagepool_alloc(pool, &item);
    fail_unless(rv == OKL4_OK,
                "failed to allocate physmem item by length from page pool");

    size = okl4_physmem_item_getsize(&item);
    fail_unless(size == PAGE_SIZE,
                "physmem item allocated by length from page pool is the wrong size");

    // cleanup
    okl4_physmem_pagepool_free(pool, &item);
    okl4_physmem_pagepool_destroy(pool);
    free(pool);
}
END_TEST

#ifdef BAD_ASSERT_CHECKS
/*
 * Dynamic root pagepool initialization attempt
 * (NB: Tests appropriate triggering of an assertion error)
 */
START_TEST(PAGEPOOL0102)
{
    int rv;
    struct okl4_physmem_pagepool pool;
    okl4_physmem_pool_attr_t attr;

    // attempt to dynamically create a root page pool from no parent
    okl4_physmem_pool_attr_init(&attr);
    okl4_physmem_pool_attr_setrange(&attr, POOL_START(rootpool), POOL_SIZE);
    okl4_physmem_pool_attr_setparent(&attr, NULL);
    okl4_physmem_pool_attr_setpagesize(&attr, PAGE_SIZE);

    /* this should trigger an assertion error */
    rv = okl4_physmem_pagepool_alloc_pool(&pool, &attr);
    fail_if(1, "assert was not triggered on attempt to alloc "
                    "page pool without parent. shouldn't be here!");
    fail_if(rv == OKL4_OK, "furthermore, the attempt returned OKL4_OK!");
}
END_TEST
#endif // BAD_ASSERT_CHECKS

#if 1 // specific allocation bug

/*
 * Anonymously allocate as many pages as there are in the pool,
 * then check that a final attempt fails.
 */
START_TEST(PAGEPOOL0200)
{
    int rv, i;
    okl4_physmem_item_t items[NUM_PAGES], item_failer;
    okl4_physmem_pool_attr_t attr;

    okl4_physmem_pool_attr_init(&attr);
    okl4_physmem_pool_attr_setsize(&attr, POOL_SIZE);
    okl4_physmem_pool_attr_setparent(&attr, root_physseg_pool);
    okl4_physmem_pool_attr_setpagesize(&attr, PAGE_SIZE);

    new_page_pool = malloc(OKL4_PHYSICAL_POOL_SIZE_ATTR(attr));
    assert(new_page_pool != NULL);

    rv = okl4_physmem_pagepool_alloc_pool(new_page_pool, &attr);
    fail_unless (rv == OKL4_OK, "could not create new pool");

    for (i = 0; i < NUM_PAGES; i++)
    {
        // NB: For physmem page items, size is ignored - means anonymous.
        okl4_physmem_item_setsize(&items[i], PAGE_SIZE);
        rv = okl4_physmem_pagepool_alloc(new_page_pool, &items[i]);
        fail_unless (rv == OKL4_OK, "could not allocate page");
    }

    okl4_physmem_item_setsize(&item_failer, PAGE_SIZE);
    rv = okl4_physmem_pagepool_alloc(new_page_pool, &item_failer);
    fail_if (rv == OKL4_OK, "successfully allocated beyond pool capacity");

    for (i = 0; i < NUM_PAGES; i++)
    {
        okl4_physmem_pagepool_free(new_page_pool, &items[i]);
    }

    // Do it again.
    for (i = 0; i < NUM_PAGES; i++)
    {
        // NB: For physmem page items, size is ignored - means anonymous.
        okl4_physmem_item_setsize(&items[i], PAGE_SIZE);
        rv = okl4_physmem_pagepool_alloc(new_page_pool, &items[i]);
        fail_unless (rv == OKL4_OK, "could not allocate page");
    }

    okl4_physmem_item_setsize(&item_failer, PAGE_SIZE);
    rv = okl4_physmem_pagepool_alloc(new_page_pool, &item_failer);
    fail_if (rv == OKL4_OK, "successfully allocated beyond pool capacity");

    for (i = 0; i < NUM_PAGES; i++)
    {
        okl4_physmem_pagepool_free(new_page_pool, &items[i]);
    }
}
END_TEST

/*
 * Allocate a specific page within the pool. Repeat over entire pool.
 */
START_TEST(PAGEPOOL0300)
{
    int rv;
    okl4_word_t i;
    okl4_physmem_item_t items[NUM_PAGES], item_failer;

    for (i = 0; i < NUM_PAGES; i++) {
        // NB: Physmem page items must have a page-aligned base.
        okl4_physmem_item_setrange(&items[i], POOL_START(new_page_pool) + (i * PAGE_SIZE), PAGE_SIZE);
        rv = okl4_physmem_pagepool_alloc(new_page_pool, &items[i]);
        fail_unless (rv == OKL4_OK, "could not allocate page");
    }

    okl4_physmem_item_setsize(&item_failer, PAGE_SIZE);
    rv = okl4_physmem_pagepool_alloc(new_page_pool, &item_failer);
    fail_if (rv == OKL4_OK, "successfully allocated beyond pool capacity");

    for (i = 0; i < NUM_PAGES; i++) {
        okl4_physmem_pagepool_free(new_page_pool, &items[i]);
    }

    // Do it again.
    for (i = 0; i < NUM_PAGES; i++) {
        // NB: Physmem page items must have a page-aligned base.
        okl4_physmem_item_setrange(&items[i], POOL_START(new_page_pool) + (i * PAGE_SIZE), PAGE_SIZE);
        rv = okl4_physmem_pagepool_alloc(new_page_pool, &items[i]);
        fail_unless (rv == OKL4_OK, "could not allocate page");
    }

    okl4_physmem_item_setsize(&item_failer, PAGE_SIZE);
    rv = okl4_physmem_pagepool_alloc(new_page_pool, &item_failer);
    fail_if (rv == OKL4_OK, "successfully allocated beyond pool capacity");

    for (i = 0; i < NUM_PAGES; i++) {
        okl4_physmem_pagepool_free(new_page_pool, &items[i]);
    }
}
END_TEST

/*
 * Attempt to allocate a specific page outside the range of the pool.
 * Repeat for various locations.
 */
START_TEST(PAGEPOOL0301)
{
    int rv;
    okl4_physmem_item_t item;

    okl4_physmem_item_setrange(&item, 0, PAGE_SIZE);
    rv = okl4_physmem_pagepool_alloc(new_page_pool, &item);
    fail_if(rv == OKL4_OK, "successfully allocated below pool range");

    okl4_physmem_item_setrange(&item, POOL_START(new_page_pool) - PAGE_SIZE, PAGE_SIZE);
    rv = okl4_physmem_pagepool_alloc(new_page_pool, &item);
    fail_if(rv == OKL4_OK, "successfully allocated below pool range");

    okl4_physmem_item_setrange(&item, POOL_END(new_page_pool), PAGE_SIZE);
    rv = okl4_physmem_pagepool_alloc(new_page_pool, &item);
    fail_if(rv == OKL4_OK, "successfully allocated above pool range");

    okl4_physmem_pagepool_destroy(new_page_pool);
    free(new_page_pool);
}
END_TEST

#ifdef BAD_ASSERT_CHECKS
/* Attempt to allocate a page providing a non-page-aligned base */
START_TEST(PAGEPOOL0302)
{
    int rv;
    okl4_physmem_item_t item;
    okl4_physmem_pool_attr_t attr;

    okl4_physmem_pool_attr_init(&attr);
    okl4_physmem_pool_attr_setsize(&attr, POOL_SIZE);
    okl4_physmem_pool_attr_setparent(&attr, root_physseg_pool);
    okl4_physmem_pool_attr_setpagesize(&attr, 4096);

    new_page_pool = malloc(OKL4_PHYSICAL_POOL_SIZE_ATTR(attr));
    assert(new_page_pool != NULL);

    rv = okl4_physmem_pagepool_alloc_pool(new_page_pool, &attr);
    fail_unless (rv == OKL4_OK, "could not create new pool");

    /* this should trigger an assertion error */
    okl4_physmem_item_setrange(&item, POOL_START(new_page_pool) + 1, PAGE_SIZE);
    rv = okl4_physmem_pagepool_alloc(new_page_pool, &item);
    fail_if(1, "assert was not triggered on attempt to alloc page item "
                    "with non-page-aligned base. shouldn't be here!");

    okl4_physmem_pagepool_destroy(new_page_pool);
    free(new_page_pool);
}
END_TEST
#endif // BAD_ASSERT_CHECKS
#endif // specific allocation bug

TCase *
make_page_pool_tcase(void)
{
    TCase *tc;

    tc = tcase_create("Physical Page Pool");
    tcase_add_test(tc, PAGEPOOL0100);
    tcase_add_test(tc, PAGEPOOL0101);
#ifdef BAD_ASSERT_CHECKS
    tcase_add_test(tc, PAGEPOOL0102);
#endif
    tcase_add_test(tc, PAGEPOOL0200);
    tcase_add_test(tc, PAGEPOOL0300);
    tcase_add_test(tc, PAGEPOOL0301);
#ifdef BAD_ASSERT_CHECKS
    tcase_add_test(tc, PAGEPOOL0302);
#endif
    return tc;
}
