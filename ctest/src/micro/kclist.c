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

#include <okl4/kclist.h>
#include <okl4/errno.h>

#include <stdlib.h>

/* arbitrary */
#define ALLOC_BASE 2
#define ALLOC_SIZE 10

/* L4 seems to like clist ids between 2 and 15 */
#define POOL_BASE 2
#define POOL_SIZE 14

OKL4_BITMAP_ALLOCATOR_DECLARE_OFRANGE(test_clistidpool, POOL_BASE, POOL_SIZE);

/*
 * Dynamic initialization by size, and destruction
 */
START_TEST(CLIST0100)
{
    int ret;
    struct okl4_kclist *clist;
    okl4_kclist_attr_t attr;
    okl4_kclistid_t clist_id;

    ret = okl4_kclistid_allocany(test_clistidpool, &clist_id);
    fail_unless(ret == OKL4_OK, "Failed to allocate any clist id.");
    okl4_kclist_attr_init(&attr);
    okl4_kclist_attr_setid(&attr, clist_id);

    okl4_kclist_attr_setsize(&attr, ALLOC_SIZE);

    clist = malloc(OKL4_KCLIST_SIZE_ATTR(&attr));
    fail_if(clist == NULL, "Failed to malloc clist");

    ret = okl4_kclist_create(clist, &attr);
    fail_unless(ret == OKL4_OK,"Failed to create the clist, "
                                "possibly invalid clist id");

    okl4_kclist_delete(clist);
    free(clist);
}
END_TEST


/*
 * Dynamic initialization by range, and destruction
 */
START_TEST(CLIST0101)
{
    int ret;
    struct okl4_kclist *clist;
    okl4_kclist_attr_t attr;
    okl4_kclistid_t clist_id;

    ret = okl4_kclistid_allocany(test_clistidpool, &clist_id);
    fail_unless(ret == OKL4_OK, "Failed to allocate any clist id.");
    okl4_kclist_attr_init(&attr);
    okl4_kclist_attr_setid(&attr, clist_id);

    okl4_kclist_attr_setrange(&attr, ALLOC_BASE, ALLOC_SIZE);

    clist = malloc(OKL4_KCLIST_SIZE_ATTR(&attr));
    fail_if(clist == NULL, "Failed to malloc clist");

    ret = okl4_kclist_create(clist, &attr);
    fail_unless(ret == OKL4_OK,"Failed to create the clist, "
                                "possibly invalid clist id");

    okl4_kclist_delete(clist);
    free(clist);
}
END_TEST

static struct okl4_kclist *
alloc_dyn_clist_init(void)
{
    int ret;
    struct okl4_kclist *clist;
    okl4_kclist_attr_t attr;
    okl4_kclistid_t clist_id;

    ret = okl4_kclistid_allocany(test_clistidpool, &clist_id);
    fail_unless(ret == OKL4_OK, "Failed to allocate any clist id.");
    okl4_kclist_attr_init(&attr);
    okl4_kclist_attr_setid(&attr, clist_id);

    okl4_kclist_attr_setrange(&attr, ALLOC_BASE, ALLOC_SIZE);

    clist = malloc(OKL4_KCLIST_SIZE_ATTR(&attr));
    fail_if(clist == NULL, "Failed to malloc clist");

    ret = okl4_kclist_create(clist, &attr);
    fail_unless(ret == OKL4_OK,"Failed to create the clist, "
                                "possibly invalid clist id");

    return clist;
}

/*
 * Dynamically create a number of clists and delete them
 */
START_TEST(CLIST0102)
{
    int ret;
    int i;
    okl4_kclistid_t clist_id;
    okl4_kclist_attr_t clist_attr;
    struct okl4_kclist *clists[POOL_SIZE];

    for (i = 0; i < POOL_SIZE; i++) {
        ret = okl4_kclistid_allocany(test_clistidpool, &clist_id);
        fail_unless(ret == OKL4_OK, "Failed to allocate any clist id.");
        okl4_kclist_attr_init(&clist_attr);
        okl4_kclist_attr_setid(&clist_attr, clist_id);
        okl4_kclist_attr_setrange(&clist_attr, ALLOC_BASE, ALLOC_SIZE);

        clists[i] = malloc(OKL4_KCLIST_SIZE_ATTR(&clist_attr));
        fail_if(clists[i] == NULL, "Failed to malloc clist");

        ret = okl4_kclist_create(clists[i], &clist_attr);
        fail_unless(ret == OKL4_OK, "Failed to create the clist, "
                                    "possibly invalid clist id");
    }

    for (i = 0; i < POOL_SIZE; i++) {
        okl4_kclist_delete(clists[i]);
    }
}
END_TEST


OKL4_KCLIST_DECLARE_OFSIZE(test0200_clist, ALLOC_SIZE);

/*
 * Static initialization by length
 */
START_TEST(CLIST0200)
{
    int ret;
    okl4_kclist_attr_t attr;
    okl4_kclistid_t clist_id;

    ret = okl4_kclistid_allocany(test_clistidpool, &clist_id);
    fail_unless(ret == OKL4_OK, "Failed to allocate any clist id.");
    okl4_kclist_attr_init(&attr);
    okl4_kclist_attr_setid(&attr, clist_id);

    okl4_kclist_attr_setsize(&attr, ALLOC_SIZE);

    ret = okl4_kclist_create(test0200_clist, &attr);
    fail_unless(ret == OKL4_OK,"Failed to create statically declared clist, "
                                    "possibly invalid clist id");

    okl4_kclist_delete(test0200_clist);
}
END_TEST

OKL4_KCLIST_DECLARE_OFRANGE(test0201_clist, ALLOC_BASE, ALLOC_SIZE);

/*
 * Static initialization by interval
 */
START_TEST(CLIST0201)
{
    int ret;
    okl4_kclist_attr_t attr;
    okl4_kclistid_t clist_id;

    ret = okl4_kclistid_allocany(test_clistidpool, &clist_id);
    fail_unless(ret == OKL4_OK, "Failed to allocate any clist id.");
    okl4_kclist_attr_init(&attr);
    okl4_kclist_attr_setid(&attr, clist_id);

    okl4_kclist_attr_setrange(&attr, ALLOC_BASE, ALLOC_SIZE);

    ret = okl4_kclist_create(test0201_clist, &attr);
    fail_unless(ret == OKL4_OK,"Failed to create statically declared clist, "
                                    "possibly invalid clist id");

    okl4_kclist_delete(test0201_clist);
}
END_TEST

/*
 * (De)allocation of any cap id
 */
START_TEST(CLIST0300)
{
    int ret;
    struct okl4_kclist *clist;
    struct okl4_kcap_item kcap_item;

    clist = alloc_dyn_clist_init();

    /* Allocate any item. */
    ret = okl4_kclist_kcap_allocany(clist, &kcap_item);
    fail_unless(ret == OKL4_OK, "Could not allocany kcap item ");

    /* Free the item. */
    okl4_kclist_kcap_free(clist, &kcap_item);

    /* Allocate another item. */
    ret = okl4_kclist_kcap_allocany(clist, &kcap_item);
    fail_unless(ret == OKL4_OK, "Could not allocany kcap item "
                                "a 2nd time, after freeing");

    /* Free the item. */
    okl4_kclist_kcap_free(clist, &kcap_item);

    okl4_kclist_delete(clist);
    free(clist);
}
END_TEST

/*
 * Try to allocate any cap item, with no ids left
 */
START_TEST(CLIST0301)
{
    int i;
    int ret;
    struct okl4_kclist *clist;
    struct okl4_kcap_item kcap_item[ALLOC_SIZE];
    struct okl4_kcap_item kcap_item_failer;

    clist = alloc_dyn_clist_init();

    /* Allocate all the ids in the clist. */
    for (i = 0; i < ALLOC_SIZE; i++)
    {
        ret = okl4_kclist_kcap_allocany(clist, &kcap_item[i]);
        fail_unless(ret == OKL4_OK, "Could not allocany kcap item");
    }

    /* Try allocating another item, passing the same one in again. */
    ret = okl4_kclist_kcap_allocany(clist, &kcap_item_failer);
    fail_unless(ret == OKL4_ALLOC_EXHAUSTED, "Attempt to allocany kcap item "
                                "with no ids left in the clist "
                                "did not fail appropriately");

    /* Free and test for 2nd-time allocation */
    for (i = 0; i < ALLOC_SIZE; i++)
    {
        okl4_kclist_kcap_free(clist, &kcap_item[i]);

        ret = okl4_kclist_kcap_allocany(clist, &kcap_item[i]);
        fail_unless(ret == OKL4_OK, "Could not allocany kcap item "
                                    "a 2nd time, after freeing");

        /* Finally, free the item. */
        okl4_kclist_kcap_free(clist, &kcap_item[i]);
    }

    /* Try again, now with ids available */
    ret = okl4_kclist_kcap_allocany(clist, &kcap_item_failer);
    fail_unless(ret == OKL4_OK, "Could not allocany kcap item "
                                "after freeing up ids in the clist");

    okl4_kclist_kcap_free(clist, &kcap_item_failer);

    /* Delete the pool. */
    okl4_kclist_delete(clist);
    free(clist);
}
END_TEST

/*
 * (De)allocation of a specific cap id
 */
START_TEST(CLIST0400)
{
    int ret;
    struct okl4_kclist *clist;
    struct okl4_kcap_item kcap_item;

    clist = alloc_dyn_clist_init();

    /* Allocate a specific item. */
    ret = okl4_kclist_kcap_allocunit(clist, &kcap_item, ALLOC_BASE);
    fail_unless(ret == OKL4_OK, "Could not alloc specific kcap item ");

    /* Free the item. */
    okl4_kclist_kcap_free(clist, &kcap_item);

    /* Allocate another item. */
    ret = okl4_kclist_kcap_allocunit(clist, &kcap_item, ALLOC_BASE);
    fail_unless(ret == OKL4_OK, "Could not alloc specific kcap item "
                                "a 2nd time, after freeing");

    /* Free the item. */
    okl4_kclist_kcap_free(clist, &kcap_item);

    okl4_kclist_delete(clist);
    free(clist);
}
END_TEST

/*
 * Double-allocate an already allocated kcap item
 */
START_TEST(CLIST0401)
{
    int ret;
    struct okl4_kclist *clist;
    struct okl4_kcap_item kcap_item;

    clist = alloc_dyn_clist_init();

    /* Allocate a specific item. */
    ret = okl4_kclist_kcap_allocunit(clist, &kcap_item, ALLOC_BASE);
    fail_unless(ret == OKL4_OK, "Could not alloc specific kcap item ");

    /* Try allocating the same item again. */
    ret = okl4_kclist_kcap_allocunit(clist, &kcap_item, ALLOC_BASE);
    fail_unless(ret == OKL4_IN_USE, "Attempt to double-allocate a kcap item "
                                "did not fail appropriately");

    /* Free the item. */
    okl4_kclist_kcap_free(clist, &kcap_item);

    okl4_kclist_delete(clist);
    free(clist);
}
END_TEST

/*
 * Double-allocate an already allocated kcap id
 */
START_TEST(CLIST0402)
{
    int ret;
    struct okl4_kclist *clist;
    struct okl4_kcap_item kcap_item;
    struct okl4_kcap_item kcap_item_failer;

    clist = alloc_dyn_clist_init();

    /* Allocate a specific item. */
    ret = okl4_kclist_kcap_allocunit(clist, &kcap_item, ALLOC_BASE);
    fail_unless(ret == OKL4_OK, "Could not alloc specific kcap item ");

    /* Try allocating a different item but with the same id. */
    ret = okl4_kclist_kcap_allocunit(clist, &kcap_item_failer, ALLOC_BASE);
    fail_unless(ret == OKL4_IN_USE, "Attempt to double-allocate a kcap id "
                                "did not fail appropriately");

    /* Free the item. */
    okl4_kclist_kcap_free(clist, &kcap_item);

    okl4_kclist_delete(clist);
    free(clist);
}
END_TEST



static void test_setup(void)
{
    okl4_allocator_attr_t clistidpool_attr;

    okl4_allocator_attr_init(&clistidpool_attr);
    okl4_allocator_attr_setrange(&clistidpool_attr, POOL_BASE, POOL_SIZE);

    okl4_bitmap_allocator_init(test_clistidpool, &clistidpool_attr);
}

static void test_teardown(void)
{
    okl4_bitmap_allocator_destroy(test_clistidpool);
}

TCase *
make_kclist_tcase(void)
{
    TCase * tc;
    tc = tcase_create("kclist");
    tcase_add_checked_fixture(tc, test_setup, test_teardown);
    tcase_add_test(tc, CLIST0100);
    tcase_add_test(tc, CLIST0101);
    tcase_add_test(tc, CLIST0102);
    tcase_add_test(tc, CLIST0200);
    tcase_add_test(tc, CLIST0201);
    tcase_add_test(tc, CLIST0300);
    tcase_add_test(tc, CLIST0301);
    tcase_add_test(tc, CLIST0400);
    tcase_add_test(tc, CLIST0401);
    tcase_add_test(tc, CLIST0402);

    return tc;
}
