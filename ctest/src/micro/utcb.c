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

#include <stdlib.h>

#include <okl4/utcb.h>

/* UTCB area start/length for used in these tests. */
#define TEST_UTCB_AREA_BASE   0xe0000000U
#define TEST_UTCB_AREA_SIZE  OKL4_DEFAULT_PAGESIZE
#define NUM_THREADS  (TEST_UTCB_AREA_SIZE / UTCB_SIZE)

/*
 * Create a UTCB area object, and delete it again.
 */
START_TEST(UTCB0100)
{
    int ret;
    okl4_utcb_area_attr_t attr;
    struct okl4_utcb_item utcb_bottom_item;
    struct okl4_utcb_item utcb_top_item;
    struct okl4_utcb_item utcb_top_failer;
    struct okl4_utcb_area *area;

    /* Setup attributes. */
    okl4_utcb_area_attr_init(&attr);
    okl4_utcb_area_attr_setrange(&attr, TEST_UTCB_AREA_BASE,
            TEST_UTCB_AREA_SIZE);

    /* Allocate memory of appropriate size. */
    fail_unless(OKL4_UTCB_AREA_SIZE_ATTR(&attr) > 0,
            "Invalid size returned by OKL4_UTCB_AREA_SIZE_ATTR() macro");
    area = malloc(OKL4_UTCB_AREA_SIZE_ATTR(&attr));
    fail_if(area == NULL, "Unable to malloc utcb area");

    /* Create the object. */
    okl4_utcb_area_init(area, &attr);

    /* Allocate the item at the bottom of the range. */
    ret = okl4_utcb_allocunit(area, &utcb_bottom_item, 0);
    fail_unless(ret == OKL4_OK, "Could not allocate the bottom utcb");

    /* Allocate the item at the top of the range. */
    ret = okl4_utcb_allocunit(area, &utcb_top_item, NUM_THREADS - 1);
    fail_unless(ret == OKL4_OK, "Could not allocate the top utcb");

    /* Try allocating an item above the top of the range. */
    ret = okl4_utcb_allocunit(area, &utcb_top_failer, NUM_THREADS);
    fail_unless(ret == OKL4_OUT_OF_RANGE, "Attempt to allocate utcb above range "
                                "did not fail appropriately");

    /* Free the items. */
    okl4_utcb_free(area, &utcb_bottom_item);
    okl4_utcb_free(area, &utcb_top_item);

    /* Delete the pool. */
    okl4_utcb_area_destroy(area);
    free(area);
}
END_TEST

static struct okl4_utcb_area *
utcb_area_malloc_init(void)
{
    okl4_utcb_area_attr_t attr;
    struct okl4_utcb_area *area;

    /* Setup attributes. */
    okl4_utcb_area_attr_init(&attr);
    okl4_utcb_area_attr_setrange(&attr, TEST_UTCB_AREA_BASE,
            TEST_UTCB_AREA_SIZE);

    /* Allocate memory of appropriate size. */
    fail_unless(OKL4_UTCB_AREA_SIZE_ATTR(&attr) > 0,
            "Invalid size returned by OKL4_UTCB_AREA_SIZE_ATTR() macro");
    area = malloc(OKL4_UTCB_AREA_SIZE_ATTR(&attr));
    fail_if(area == NULL, "Unable to malloc utcb area");

    /* Create the object. */
    okl4_utcb_area_init(area, &attr);

    return area;
}

OKL4_UTCB_AREA_DECLARE_OFSIZE(utcb0200_area, TEST_UTCB_AREA_BASE,
        TEST_UTCB_AREA_SIZE);

/*
 * Initialize a statically declared UTCB area object
 */
START_TEST(UTCB0200)
{
    int ret;
    okl4_utcb_area_attr_t attr;
    struct okl4_utcb_item utcb_bottom_item;
    struct okl4_utcb_item utcb_top_item;
    struct okl4_utcb_item utcb_top_failer;

    /* Setup attributes. */
    okl4_utcb_area_attr_init(&attr);
    okl4_utcb_area_attr_setrange(&attr, TEST_UTCB_AREA_BASE,
            TEST_UTCB_AREA_SIZE);
    fail_if(utcb0200_area == NULL, "Static declaration of utcb area failed!");

    /* Init the object. */
    okl4_utcb_area_init(utcb0200_area, &attr);

    /* Allocate the item at the bottom of the range. */
    ret = okl4_utcb_allocunit(utcb0200_area, &utcb_bottom_item, 0);
    fail_unless(ret == OKL4_OK, "Could not allocate the bottom utcb");

    /* Allocate the item at the top of the range. */
    ret = okl4_utcb_allocunit(utcb0200_area, &utcb_top_item, NUM_THREADS - 1);
    fail_unless(ret == OKL4_OK, "Could not allocate the top utcb");

    /* Try allocating an item above the top of the range. */
    ret = okl4_utcb_allocunit(utcb0200_area, &utcb_top_failer, NUM_THREADS);
    fail_unless(ret == OKL4_OUT_OF_RANGE, "Attempt to allocate utcb above range "
                                "did not fail appropriately");

    /* Free the items. */
    okl4_utcb_free(utcb0200_area, &utcb_bottom_item);
    okl4_utcb_free(utcb0200_area, &utcb_top_item);

    /* Delete the pool. */
    okl4_utcb_area_destroy(utcb0200_area);
}
END_TEST

/*
 * (De)allocate any UTCB
 */
START_TEST(UTCB0400)
{
    int ret;
    struct okl4_utcb_item utcb_item;
    struct okl4_utcb_area *area;

    area = utcb_area_malloc_init();

    /* Allocate any item. */
    ret = okl4_utcb_allocany(area, &utcb_item);
    fail_unless(ret == OKL4_OK, "Could not allocany utcb item ");

    /* Free the item. */
    okl4_utcb_free(area, &utcb_item);

    /* Allocate another item. */
    ret = okl4_utcb_allocany(area, &utcb_item);
    fail_unless(ret == OKL4_OK, "Could not allocany utcb item "
                                "a 2nd time, after freeing");

    /* Free the item. */
    okl4_utcb_free(area, &utcb_item);

    /* Delete the pool. */
    okl4_utcb_area_destroy(area);
    free(area);
}
END_TEST


/*
 * Try to allocate any UTCB item, with no slots left
 */
START_TEST(UTCB0401)
{
    int ret;
    okl4_word_t i;
    struct okl4_utcb_item utcb_item[NUM_THREADS];
    struct okl4_utcb_item utcb_item_failer;
    struct okl4_utcb_area *area;

    area = utcb_area_malloc_init();

    /* Allocate all the slots in the UTCB area. */
    for (i = 0; i < NUM_THREADS; i++) {
        ret = okl4_utcb_allocany(area, &utcb_item[i]);
        fail_unless(ret == OKL4_OK, "Could not allocany utcb item");
    }

    /* Try allocating another item, passing the same one in again. */
    ret = okl4_utcb_allocany(area, &utcb_item_failer);
    fail_unless(ret == OKL4_ALLOC_EXHAUSTED, "Attempt to allocany UTCB item "
                                "with no slots left "
                                "did not fail appropriately");

    /* Free and test for 2nd-time allocation */
    for (i = 0; i < NUM_THREADS; i++) {
        okl4_utcb_free(area, &utcb_item[i]);

        ret = okl4_utcb_allocany(area, &utcb_item[i]);
        fail_unless(ret == OKL4_OK, "Could not allocany utcb item "
                                    "a 2nd time, after freeing");

        /* Finally, free the item. */
        okl4_utcb_free(area, &utcb_item[i]);
    }

    /* Try again, now with slots available */
    ret = okl4_utcb_allocany(area, &utcb_item_failer);
    fail_unless(ret == OKL4_OK, "Could not allocany utcb item "
                                "after freeing up slots");

    okl4_utcb_free(area, &utcb_item_failer);

    /* Delete the pool. */
    okl4_utcb_area_destroy(area);
    free(area);
}
END_TEST


/*
 * (De)allocate a specific UTCB
 */
START_TEST(UTCB0500)
{
    int ret;
    struct okl4_utcb_item utcb_item;
    struct okl4_utcb_area *area;

    area = utcb_area_malloc_init();

    /* Allocate a specific item. */
    ret = okl4_utcb_allocunit(area, &utcb_item, 0);
    fail_unless(ret == OKL4_OK, "Could not alloc specific utcb item ");

    /* Free the item. */
    okl4_utcb_free(area, &utcb_item);

    /* Allocate it again. */
    ret = okl4_utcb_allocunit(area, &utcb_item, 0);
    fail_unless(ret == OKL4_OK, "Could not alloc specific utcb item "
                                "a 2nd time, after freeing");

    /* Free the item. */
    okl4_utcb_free(area, &utcb_item);

    /* Delete the pool. */
    okl4_utcb_area_destroy(area);
    free(area);
}
END_TEST


/*
 * Double-allocate an already allocated UTCB item
 */
START_TEST(UTCB0501)
{
    int ret;
    struct okl4_utcb_item utcb_item;
    struct okl4_utcb_area *area;

    area = utcb_area_malloc_init();

    /* Allocate a specific item. */
    ret = okl4_utcb_allocunit(area, &utcb_item, 0);
    fail_unless(ret == OKL4_OK, "Could not alloc specific utcb item");

    /* Try allocating the same item again. */
    ret = okl4_utcb_allocunit(area, &utcb_item, 0);
    fail_unless(ret == OKL4_IN_USE, "Attempt to double-allocate a UTCB item "
                                "did not fail appropriately");

    /* Free the item. */
    okl4_utcb_free(area, &utcb_item);

    /* Delete the pool. */
    okl4_utcb_area_destroy(area);
    free(area);
}
END_TEST


/*
 * Double-allocate an already allocated UTCB slot
 */
START_TEST(UTCB0502)
{
    int ret;
    struct okl4_utcb_item utcb_item;
    struct okl4_utcb_item utcb_item_failer;
    struct okl4_utcb_area *area;

    area = utcb_area_malloc_init();

    /* Allocate a specific item. */
    ret = okl4_utcb_allocunit(area, &utcb_item, 0);
    fail_unless(ret == OKL4_OK, "Could not alloc specific utcb item");

    /* Try allocating a different item but with the same slot. */
    ret = okl4_utcb_allocunit(area, &utcb_item_failer, 0);
    fail_unless(ret == OKL4_IN_USE, "Attempt to double-allocate a UTCB slot "
                                "did not fail appropriately");

    /* Free the item. */
    okl4_utcb_free(area, &utcb_item);

    /* Delete the pool. */
    okl4_utcb_area_destroy(area);
    free(area);
}
END_TEST

TCase *
make_utcb_tcase(void)
{
    TCase *tc;
    tc = tcase_create("utcb");
    tcase_add_test(tc, UTCB0100);
    tcase_add_test(tc, UTCB0200);
    tcase_add_test(tc, UTCB0400);
    tcase_add_test(tc, UTCB0401);
    tcase_add_test(tc, UTCB0500);
    tcase_add_test(tc, UTCB0501);
    tcase_add_test(tc, UTCB0502);

    return tc;
}

