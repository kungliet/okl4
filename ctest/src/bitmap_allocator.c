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
#include <okl4/bitmap.h>
#include <okl4/errno.h>
#include <okl4/env.h>

#include <stdlib.h>

#define ALLOC_BASE  3UL
#define ALLOC_SIZE  99UL

/**
 *  @test BITMAPALLOC0100 : Dynamic initialization by size, and destruction
 *  @stat enabled
 *  @func Dynamic initialization of a bitmap allocator by size,
 *          and destruction.
 *
 *  @overview
 *      @li The size of the bitmap allocator to be created is specified,
 *          by creating and setting an allocator attribute with a size value.
 *      @li The allocator attribute is used to determine the amount of memory
 *          required for the bitmap allocator, which is dynamically allocated.
 *      @li The allocator attribute is used to initialize the
 *          bitmap allocator object.
 *      @li The bitmap allocator is destroyed and freed.
 *      @li For the test to pass, all operations must succeed.
 *
 *  @special  None.
 *
 */
START_TEST(BITMAPALLOC0100)
{
    okl4_bitmap_allocator_t *alloc;
    okl4_allocator_attr_t attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setsize(&attr, ALLOC_SIZE);

    alloc = malloc(OKL4_BITMAP_ALLOCATOR_SIZE_ATTR(&attr));

    okl4_bitmap_allocator_init(alloc, &attr);

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test BITMAPALLOC0101 : Dynamic initialization by range, and destruction
 *  @stat enabled
 *  @func Dynamic initialization of a bitmap allocator by range,
 *          and destruction.
 *
 *  @overview
 *      @li (Procedure and criteria as for BITMAPALLOC0100, except
 *          that the range of the allocator is explicitly set
 *          by specifying both a base and a size value.)
 *
 */

START_TEST(BITMAPALLOC0101)
{
    okl4_bitmap_allocator_t *alloc;
    okl4_allocator_attr_t attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, ALLOC_BASE, ALLOC_SIZE);

    alloc = malloc(OKL4_BITMAP_ALLOCATOR_SIZE_ATTR(&attr));

    okl4_bitmap_allocator_init(alloc, &attr);

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test BITMAPALLOC0102 : Static initialization by size
 *  @stat enabled
 *  @func Static initialization of a bitmap allocator, by size.
 *
 *  @overview
 *      @li A bitmap allocator is statically declared with a size value.
 *      @li An allocator attribute is also created, with that same value.
 *      @li The allocator attribute is used to initialize the
 *          bitmap allocator object.
 *      @li For the test to pass, all operations must succeed.
 *
 */

OKL4_BITMAP_ALLOCATOR_DECLARE_OFSIZE(test0102_balloc, ALLOC_SIZE);

START_TEST(BITMAPALLOC0102)
{
    okl4_allocator_attr_t attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setsize(&attr, ALLOC_SIZE);

    okl4_bitmap_allocator_init(test0102_balloc, &attr);

    okl4_bitmap_allocator_destroy(test0102_balloc);
}
END_TEST


/**
 *  @test BITMAPALLOC0103 : Static initialization by range
 *  @stat enabled
 *  @func Static initialization of a bitmap allocator, by range.
 *
 *  @overview
 *      @li (Procedure and criteria as for BITMAPALLOC0102, except
 *          that the range of the allocator is explicitly set
 *          by specifying both a base and a size value.)
 *
 */

OKL4_BITMAP_ALLOCATOR_DECLARE_OFRANGE(test0103_balloc, ALLOC_BASE, ALLOC_SIZE);

START_TEST(BITMAPALLOC0103)
{
    okl4_allocator_attr_t attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, ALLOC_BASE, ALLOC_SIZE);

    okl4_bitmap_allocator_init(test0103_balloc, &attr);

    okl4_bitmap_allocator_destroy(test0103_balloc);
}
END_TEST


static okl4_bitmap_allocator_t *
alloc_dyn_init_range(okl4_word_t base, okl4_word_t size)
{
    okl4_bitmap_allocator_t *alloc;
    okl4_allocator_attr_t attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, base, size);

    alloc = malloc(OKL4_BITMAP_ALLOCATOR_SIZE_ATTR(&attr));

    if (alloc == NULL) {
        return NULL;
    }

    okl4_bitmap_allocator_init(alloc, &attr);

    return alloc;
}

/**
 *  @test BITMAPALLOC0200 : Named allocation within available bounds
 *  @stat enabled
 *  @func That one single named allocation succeeds.
 *
 *  @overview
 *      @li Attempt to allocate one single named bitmap item,
 *          well within the bounds of a newly initialized allocator.
 *      @li For the test to pass, the allocation must succeed and
 *          be the value requested.
 *
 */

START_TEST(BITMAPALLOC0200)
{
    int rv;
    okl4_word_t value;
    okl4_bitmap_allocator_t *alloc;
    okl4_bitmap_item_t item;

    alloc = alloc_dyn_init_range(ALLOC_BASE, ALLOC_SIZE);
    fail_if(alloc == NULL, "failed to dynamically create bitmap allocator by range");

    okl4_bitmap_item_setunit(&item, ALLOC_BASE);

    rv = okl4_bitmap_allocator_isallocated(alloc, &item);
    fail_unless(rv == 0, "isallocated returned wrong value");

    rv = okl4_bitmap_allocator_alloc(alloc, &item);
    fail_unless(rv == OKL4_OK, "failed to allocate named bitmap item");

    value = okl4_bitmap_item_getunit(&item);
    fail_unless(value == ALLOC_BASE, "allocated bitmap item is the wrong value");

    rv = okl4_bitmap_allocator_isallocated(alloc, &item);
    fail_unless(rv == 1, "isallocated returned wrong value");

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test BITMAPALLOC0201 : Named allocations filling available bounds
 *  @stat enabled
 *  @func That all named allocation within bounds and
 *        without duplicates succeeds.
 *
 *  @overview
 *      @li Iterate through the entire space of a newly initialized allocator,
 *          attempting to allocate one bitmap item per iteration.
 *      @li For the test to pass, all allocations must be the value requested.
 *
 */
START_TEST(BITMAPALLOC0201)
{
    int rv;
    okl4_word_t i;
    okl4_word_t value;
    okl4_bitmap_allocator_t *alloc;
    okl4_bitmap_item_t item[ALLOC_SIZE - ALLOC_BASE];

    alloc = alloc_dyn_init_range(ALLOC_BASE, ALLOC_SIZE);
    fail_if(alloc == NULL, "failed to dynamically create bitmap allocator by range");

    for (i = 0; i < ALLOC_SIZE - ALLOC_BASE; i++) {
        okl4_bitmap_item_setunit(&item[i], i + ALLOC_BASE);

        rv = okl4_bitmap_allocator_alloc(alloc, &item[i]);
        fail_unless(rv == OKL4_OK, "failed to allocate named bitmap item");

        value = okl4_bitmap_item_getunit(&item[i]);

        fail_unless(value == i + ALLOC_BASE, "allocated bitmap item is the wrong value");
    }

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

static int
allocate_all_named(okl4_bitmap_allocator_t *alloc, okl4_bitmap_item_t
        *item_array, okl4_word_t base, okl4_word_t size)
{
    int rv;
    okl4_word_t i;
    okl4_word_t value;

    for (i = 0; i < size; i++) {
        okl4_bitmap_item_setunit(&item_array[i], i + base);

        rv = okl4_bitmap_allocator_alloc(alloc, &item_array[i]);
        if (rv != OKL4_OK) {
            return 1;
        }

        value = okl4_bitmap_item_getunit(&item_array[i]);
        if (value != i + ALLOC_BASE) {
            return 1;
        }
     }

    return 0;
}

static void
check_all_named(okl4_bitmap_item_t *item, okl4_word_t base, okl4_word_t size)
{
    okl4_word_t i;
    okl4_word_t value;

    for (i = 0; i < size; i++) {
        value = okl4_bitmap_item_getunit(&item[i]);
        assert(value == i + base);

        fail_unless(value == i + base, "allocated bitmap item is the wrong value");
    }
}

/**
 *  @test BITMAPALLOC0202 : Named allocation attempts requesting values in use
 *  @stat enabled
 *  @func That all attempts for named allocation of already-allocated
 *        values fail.
 *
 *  @overview
 *      @li All valid values in an allocator's space are allocated
 *          (as in BITMAPALLOC0201).
 *      @li The space is once again iterated through for a second pass
 *          of attempts to allocate each value.
 *      @li For the test to pass, all allocations should fail
 *          with the appropriate error code (OKL4_IN_USE).
 *
 */
START_TEST(BITMAPALLOC0202)
{
    int rv;
    okl4_word_t i;
    okl4_bitmap_allocator_t *alloc;
    okl4_bitmap_item_t item[ALLOC_SIZE], item_failer;

    alloc = alloc_dyn_init_range(ALLOC_BASE, ALLOC_SIZE);
    fail_if(alloc == NULL, "failed to dynamically create bitmap allocator by range");

    rv = allocate_all_named(alloc, item, ALLOC_BASE, ALLOC_SIZE);
    fail_unless(rv == OKL4_OK, "failed to allocate named bitmap items");

    for (i = ALLOC_BASE; i < ALLOC_SIZE; i++) {
        okl4_bitmap_item_setunit(&item_failer, i);

        rv = okl4_bitmap_allocator_alloc(alloc, &item_failer);
        fail_unless(rv == OKL4_IN_USE, "attempt to allocate already-"
                "allocated id did not fail appropriately");
    }

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test BITMAPALLOC0203 : Named allocation attempts outside available bounds
 *  @stat enabled
 *  @func That attempts at named allocations outside bounds fail.
 *
 *  @overview
 *      @li Attempts are made to allocate a bitmap item for a value
 *          one below the beginning of the allocator's valid range,
 *          and one above the end of the range.
 *      @li For the test to pass, all allocations should fail with
 *          the appropriate error code (OKL4_NON_EXISTENT).
 *
 */
START_TEST(BITMAPALLOC0203)
{
    int rv;
    okl4_bitmap_allocator_t *alloc;
    okl4_bitmap_item_t item[ALLOC_SIZE], item_failer;

    alloc = alloc_dyn_init_range(ALLOC_BASE, ALLOC_SIZE);
    fail_if(alloc == NULL, "failed to dynamically create bitmap allocator "
                           "by range");

    rv = allocate_all_named(alloc, item, ALLOC_BASE, ALLOC_SIZE);
    fail_unless(rv == OKL4_OK, "failed to allocate named bitmap items");

    // Try allocating unit id below bounds
    okl4_bitmap_item_setunit(&item_failer, ALLOC_BASE - 1);

    rv = okl4_bitmap_allocator_alloc(alloc, &item_failer);
    fail_unless(rv == OKL4_OUT_OF_RANGE, "attempt to allocate named id "
                "below bounds did not fail appropriately");

    // Try allocating unit id higher than bounds
    okl4_bitmap_item_setunit(&item_failer, ALLOC_BASE + ALLOC_SIZE);

    rv = okl4_bitmap_allocator_alloc(alloc, &item_failer);
    fail_unless(rv == OKL4_OUT_OF_RANGE, "attempt to allocate named id "
                "higher than bounds did not fail appropriately");

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test BITMAPALLOC0300 : Anonymous allocation within available bounds
 *  @stat enabled
 *  @func That one single anonymous allocation succeeds.
 *
 *  @overview
 *      @li Attempt to allocate one single anonymous bitmap item
 *          from a newly initialized allocator.
 *      @li For the test to pass, the allocation must succeed and
 *          be within the space the allocator was initialized with.
 */
START_TEST(BITMAPALLOC0300)
{
    int rv;
    okl4_word_t value;
    okl4_bitmap_allocator_t *alloc;
    okl4_bitmap_item_t item;

    alloc = alloc_dyn_init_range(ALLOC_BASE, ALLOC_SIZE);
    fail_if(alloc == NULL, "failed to dynamically create bitmap allocator by range");

    okl4_bitmap_item_setany(&item);

    rv = okl4_bitmap_allocator_alloc(alloc, &item);
    fail_unless(rv == OKL4_OK, "failed to allocate anonymous bitmap item");

    value = okl4_bitmap_item_getunit(&item);

    fail_unless(value >= ALLOC_BASE, "allocated bitmap item has value smaller than bounds");
    fail_unless(value < ALLOC_SIZE, "allocated bitmap item has value larger than bounds");

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test BITMAPALLOC0301 : Anonymous allocations filling available bounds
 *  @stat enabled
 *  @func That all anonymous allocation with sufficient space succeeds.
 *
 *  @overview
 *      @li Iterate through the entire space of a newly initialized allocator,
 *          attempting to allocate one bitmap item per iteration.
 *      @li For the test to pass, all allocations must be within the valid
 *          space and unique from all the values previously allocated.
 */
START_TEST(BITMAPALLOC0301)
{
    int rv;
    okl4_word_t i, j;
    okl4_word_t value, value_j;
    okl4_bitmap_allocator_t *alloc;
    okl4_bitmap_item_t item[ALLOC_SIZE - ALLOC_BASE];

    alloc = alloc_dyn_init_range(ALLOC_BASE, ALLOC_SIZE);
    fail_if(alloc == NULL, "failed to dynamically create bitmap allocator by range");

    for (i = 0; i < ALLOC_SIZE - ALLOC_BASE; i++) {
        okl4_bitmap_item_setany(&item[i]);

        rv = okl4_bitmap_allocator_alloc(alloc, &item[i]);
        fail_unless(rv == OKL4_OK, "failed to allocate anonymous bitmap item");

        value = okl4_bitmap_item_getunit(&item[i]);

        // Check value is within bounds
        fail_unless(value >= ALLOC_BASE, "allocated item below lower bounds");
        fail_unless(value < ALLOC_BASE + ALLOC_SIZE, "allocated item exceeds upper bounds");

        // Check for duplicate allocations
        for (j = 0; j < i; j++) {
            value_j = okl4_bitmap_item_getunit(&item[j]);
            fail_if(value == value_j, "allocated item has a duplicated value");
        }
    }

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

static int
allocate_all_anonymous(okl4_bitmap_allocator_t *alloc, okl4_bitmap_item_t *item_array, okl4_word_t qty)
{
    int rv;
    okl4_word_t i;

    assert(alloc != NULL);
    assert(item_array != NULL);

    for (i = 0; i < qty; i++) {
        okl4_bitmap_item_setany(&item_array[i]);

        rv = okl4_bitmap_allocator_alloc(alloc, &item_array[i]);
        if (rv != OKL4_OK) {
            return 1;
        }
    }

    return 0;
}

static void
check_all_anonymous(okl4_bitmap_item_t *item, okl4_word_t base, okl4_word_t size)
{
    okl4_word_t i, j;
    okl4_word_t value, value_j;

    for (i = 0; i < size; i++) {
        value = okl4_bitmap_item_getunit(&item[i]);

        // Check value is within bounds
        fail_unless(value >= base, "allocated item below lower bounds");
        fail_unless(value < base + size, "allocated item exceeds upper bounds");

        // Check for duplicate allocations
        for (j = 0; j < size; j++) {
            if (j == i) {
                continue;
            }
            value_j = okl4_bitmap_item_getunit(&item[j]);
            fail_if(value == value_j, "allocated item has a duplicated value");
        }
    }
}

/**
 *  @test BITMAPALLOC0302 : Anonymous allocation attempt with insufficient space
 *  @stat enabled
 *  @func That anonymous allocation with insufficient space fails.
 *
 *  @overview
 *      @li All of the available values are allocated (as in BITMAPALLOC0301).
 *      @li An additional attempt is made to allocate a bitmap item of any value.
 *      @li For the test to pass, the final allocation must fail with
 *          the appropriate error code (OKL4_NON_EXISTENT).
 *
 */
START_TEST(BITMAPALLOC0302)
{
    int rv;
    okl4_bitmap_allocator_t *alloc;
    okl4_bitmap_item_t item[ALLOC_SIZE], item_failer;

    alloc = alloc_dyn_init_range(ALLOC_BASE, ALLOC_SIZE);
    fail_if(alloc == NULL, "failed to dynamically create bitmap allocator by range");

    rv = allocate_all_anonymous(alloc, item, ALLOC_SIZE);
    fail_unless(rv == OKL4_OK, "failed to anonymously allocate bitmap items");

    okl4_bitmap_item_setany(&item_failer);

    rv = okl4_bitmap_allocator_alloc(alloc, &item_failer);
    fail_unless(rv == OKL4_ALLOC_EXHAUSTED, "attempt to allocate anonymous "
            "bitmap item with insufficient space did not fail appropriately");

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test BITMAPALLOC0400 : Deallocation
 *  @stat enabled
 *  @func Bitmap item deallocation.
 *
 *  @overview
 *      @li All valid values in an allocator's space are allocated
 *          (as in BITMAPALLOC0201).
 *      @li All are freed, in the order they were allocated.
 *      @li For the test to pass, all operations must succeed.
 */

START_TEST(BITMAPALLOC0400)
{
    int rv;
    okl4_word_t i;
    okl4_bitmap_allocator_t *alloc;
    okl4_bitmap_item_t item[ALLOC_SIZE];

    alloc = alloc_dyn_init_range(ALLOC_BASE, ALLOC_SIZE);
    fail_if(alloc == NULL, "failed to dynamically create bitmap allocator by range");

    rv = allocate_all_named(alloc, item, ALLOC_BASE, ALLOC_SIZE);
    fail_unless(rv == OKL4_OK, "failed to allocate named bitmap items");

    for (i = 0; i < ALLOC_SIZE; i++) {
        okl4_bitmap_allocator_free(alloc, &item[i]);
    }

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

static void
free_all(okl4_bitmap_allocator_t *alloc, okl4_bitmap_item_t *item_array, okl4_word_t qty)
{
    okl4_word_t i;

    for (i = 0; i < qty; i++) {
        okl4_bitmap_allocator_free(alloc, &item_array[i]);
    }
}

/**
 *  @test BITMAPALLOC0401 : Named allocation after deallocation
 *  @stat enabled
 *  @func That named allocation works after deallocation.
 *
 *  @overview
 *      @li All valid values in an allocator's space are allocated
 *          then freed (as in BITMAPALLOC0400).
 *      @li All are allocated again and tested for validity
 *          (as in BITMAPALLOC0201).
 *      @li For the test to pass, all operations must succeed.
 */

START_TEST(BITMAPALLOC0401)
{
    int rv;
    okl4_bitmap_allocator_t *alloc;
    okl4_bitmap_item_t item[ALLOC_SIZE];

    alloc = alloc_dyn_init_range(ALLOC_BASE, ALLOC_SIZE);
    fail_if(alloc == NULL, "failed to dynamically create bitmap allocator by range");

    rv = allocate_all_named(alloc, item, ALLOC_BASE, ALLOC_SIZE);
    fail_unless(rv == OKL4_OK, "failed to allocate named bitmap items");

    free_all(alloc, item, ALLOC_SIZE);

    rv = allocate_all_named(alloc, item, ALLOC_BASE, ALLOC_SIZE);
    fail_unless(rv == OKL4_OK, "failed to allocate named bitmap items a 2nd time after freeing");

    check_all_named(item, ALLOC_BASE, ALLOC_SIZE);

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test BITMAPALLOC0402 : Anonymous allocation after deallocation
 *  @stat enabled
 *  @func That anonymous allocation works after deallocation.
 *
 *  @overview
 *      @li All valid values in an allocator's space are allocated
 *          then freed (as in BITMAPALLOC0400).
 *      @li All are allocated again and tested for validity
 *          (as in BITMAPALLOC0202).
 *      @li For the test to pass, all operations must succeed.
 */

START_TEST(BITMAPALLOC0402)
{
    int rv;
    okl4_bitmap_allocator_t *alloc;
    okl4_bitmap_item_t item[ALLOC_SIZE];

    alloc = alloc_dyn_init_range(ALLOC_BASE, ALLOC_SIZE);
    fail_if(alloc == NULL, "failed to dynamically create bitmap allocator by range");

    rv = allocate_all_anonymous(alloc, item, ALLOC_SIZE);
    fail_unless(rv == OKL4_OK, "failed to anonymously allocate bitmap items");

    free_all(alloc, item, ALLOC_SIZE);

    rv = allocate_all_anonymous(alloc, item, ALLOC_SIZE);
    fail_unless(rv == OKL4_OK, "failed to anonymously allocate bitmap items a 2nd time after freeing");

    check_all_anonymous(item, ALLOC_BASE, ALLOC_SIZE);

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test BITMAPALLOC0403 : Bad deallocation attempts, named
 *  @stat disabled, due to assertions
 *  @func Test that deallocation of an unallocated named item fails.
 *
 *  @overview
 *      @li Attempts are made to free named bitmap items that are in
 *          various stages of initialization, but are not allocated yet
 *          (whether because no allocation has yet been made,
 *          or because of a failed allocation).
 *      @li For the test to pass, all deallocations must trigger an assertion.
 *
 */

#ifdef BAD_ASSERT_CHECKS
/*
 * That deallocation of an unallocated named item fails. XXX: candidate for removal?
 */
START_TEST(BITMAPALLOC0403)
{
    int rv;
    okl4_bitmap_allocator_t *alloc;
    okl4_bitmap_item_t item_failer;

    alloc = alloc_dyn_init_range(ALLOC_BASE, ALLOC_SIZE);
    fail_if(alloc == NULL, "failed to dynamically create bitmap allocator by range");

    // Try to free uninitialized bitmap item
    // This should fail.
    okl4_bitmap_allocator_free(alloc, &item_failer);

    okl4_bitmap_item_setunit(&item_failer, ALLOC_SIZE);

    // Try to free an un-allocated named item
    // This should fail.
    okl4_bitmap_allocator_free(alloc, &item_failer);

    rv = okl4_bitmap_allocator_alloc(alloc, &item_failer);
    fail_unless(rv == OKL4_OUT_OF_RANGE, "attempt to allocate named id "
                "higher than bounds did not fail appropriately");

    // Try to free a named item unallocated due to a failed attempt
    // This should fail.
    okl4_bitmap_allocator_free(alloc, &item_failer);

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST
#endif


/**
 *  @test BITMAPALLOC0404 : Bad deallocation attempts, anonymous
 *  @stat disabled, due to assertions
 *  @func Test that deallocation of an unallocated anonymous item fails.
 *
 *  @overview
 *      @li (Procedure and criteria as for BITMAPALLOC0403, but instead
 *          for anonymous bitmap items.)
 */
#ifdef BAD_ASSERT_CHECKS
/*
 * That deallocation of an unallocated anonymous item fails. XXX: candidate for removal?
 */
START_TEST(BITMAPALLOC0404)
{
    int rv;
    okl4_bitmap_allocator_t *alloc;
    okl4_bitmap_item_t item[ALLOC_SIZE - ALLOC_BASE], item_failer;

    alloc = alloc_dyn_init_range(ALLOC_BASE, ALLOC_SIZE);
    fail_if(alloc == NULL, "failed to dynamically create bitmap allocator"
                           " by range");

    // Try to free uninitialized bitmap item
    // This should fail.
    okl4_bitmap_allocator_free(alloc, &item_failer);

    okl4_bitmap_item_setany(&item_failer);

    // Try to free an unallocated anonymous item
    // This should fail.
    okl4_bitmap_allocator_free(alloc, &item_failer);

    rv = allocate_all_anonymous(alloc, item, ALLOC_SIZE - ALLOC_BASE);
    fail_unless(rv == OKL4_OK, "failed to anonymously allocate bitmap items");

    rv = okl4_bitmap_allocator_alloc(alloc, &item_failer);
    fail_unless(rv == OKL4_ALLOC_EXHAUSTED, "attempt to allocate anonymous bitmap"
                " item with insufficient space did not fail appropriately");

    // Try to free an anonymous item unallocated due to a failed attempt
    // This should fail.
    okl4_bitmap_allocator_free(alloc, &item_failer);

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST
#endif

/**
 *  @test BITMAPALLOC0500 : Named allocation after failed attempts
 *  @stat enabled
 *  @func That bitmap items are reusable after a failed attempt
 *        to perform named allocation.
 *
 *  @overview
 *      @li All valid values in an allocator's space are allocated.
 *      @li A single attempt is made to allocate a bitmap item of
 *          a certain value, which should fail.
 *      @li The item occupying that value is freed.
 *      @li A second attempt is made to allocate the bitmap item
 *          for which allocation had failed previously.
 *      @li For the test to pass, this second attempt must succeed
 *          and return the requested value.
 */

START_TEST(BITMAPALLOC0500)
{
    int rv;
    okl4_word_t halfway;
    okl4_word_t value;
    okl4_bitmap_allocator_t *alloc;
    okl4_bitmap_item_t item[ALLOC_SIZE], item_failer;

    alloc = alloc_dyn_init_range(ALLOC_BASE, ALLOC_SIZE);
    fail_if(alloc == NULL, "failed to dynamically create bitmap allocator"
                           " by range");

    rv = allocate_all_named(alloc, item, ALLOC_BASE, ALLOC_SIZE);
    fail_unless(rv == OKL4_OK, "failed to allocate named bitmap items");

    halfway = (ALLOC_SIZE + ALLOC_BASE) / 2;

    okl4_bitmap_item_setunit(&item_failer, halfway);

    rv = okl4_bitmap_allocator_alloc(alloc, &item_failer);
    fail_unless(rv == OKL4_IN_USE, "attempt to allocate already-allocated id"
                                   " did not fail appropriately");

    okl4_bitmap_allocator_free(alloc, &item[halfway - ALLOC_BASE]);

    okl4_bitmap_item_setunit(&item_failer, halfway);

    rv = okl4_bitmap_allocator_alloc(alloc, &item_failer);
    fail_unless(rv == OKL4_OK, "failed to allocate named bitmap item"
                               " after previous failed attempt");

    value = okl4_bitmap_item_getunit(&item_failer);

    fail_unless(value == halfway, "allocated bitmap item is the wrong value");

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test BITMAPALLOC0501 : Named allocation after failed attempts
 *  @stat enabled
 *  @func That bitmap items are reusable after a failed attempt
 *        to perform anonymous allocation.
 *
 *  @overview
 *      @li All valid values in an allocator's space are allocated.
 *      @li A single attempt is made to allocate a bitmap item of
 *          any value, which should fail.
 *      @li Any item is freed.
 *      @li A second attempt is made to allocate the bitmap item
 *          for which allocation had failed previously.
 *      @li For the test to pass, this second attempt must succeed
 *          and return a unique value within bounds.
 */
START_TEST(BITMAPALLOC0501)
{
    int rv;
    okl4_word_t i, halfway;
    okl4_word_t value, value_i;
    okl4_bitmap_allocator_t *alloc;
    okl4_bitmap_item_t item[ALLOC_SIZE];
    okl4_bitmap_item_t item_failer;

    alloc = alloc_dyn_init_range(ALLOC_BASE, ALLOC_SIZE);
    fail_if(alloc == NULL, "failed to dynamically create bitmap allocator by range");

    rv = allocate_all_anonymous(alloc, item, ALLOC_SIZE);
    fail_unless(rv == OKL4_OK, "failed to anonymously allocate bitmap items");

    okl4_bitmap_item_setany(&item_failer);

    rv = okl4_bitmap_allocator_alloc(alloc, &item_failer);
    fail_unless(rv == OKL4_ALLOC_EXHAUSTED, "attempt to allocate anonymous bitmap"
                " item with insufficient space did not fail appropriately");

    halfway = (ALLOC_SIZE) / 2;

    okl4_bitmap_allocator_free(alloc, &item[halfway]);

    rv = okl4_bitmap_allocator_alloc(alloc, &item_failer);
    fail_unless(rv == OKL4_OK, "failed to allocate bitmap item");

    value = okl4_bitmap_item_getunit(&item_failer);

    // Check value is within bounds
    fail_unless(value >= ALLOC_BASE, "allocated bitmap item below lower bounds");
    fail_unless(value < ALLOC_SIZE, "allocated bitmap item exceeds upper bounds");

    // Check for duplicate allocations
    for (i = 0; i < ALLOC_SIZE; i++) {
        if (i == halfway) {
            continue;
        }
        value_i = okl4_bitmap_item_getunit(&item[i]);
        fail_if(value == value_i, "allocated bitmap item has a duplicated value");
    }

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test BITMAPALLOC0600 : Mixed allocation stress test
 *  @stat enabled
 *  @func Named and anonymous bitmap item allocation and deallocation.
 *
 *  @overview
 *      @li Named and anonymous items are allocated alternatingly.
 *      @li All items are deallocated in the order they were allocated.
 *      @li The alternating allocation step is repeated.
 *      @li For the test to pass, all operations must succeed.
 */
START_TEST(BITMAPALLOC0600)
{
    int rv;
    okl4_word_t i, j, iterations = 3;
    okl4_word_t value, value_j;
    okl4_bitmap_allocator_t *alloc;
    okl4_bitmap_item_t item[ALLOC_SIZE - ALLOC_BASE];

    alloc = alloc_dyn_init_range(ALLOC_BASE, ALLOC_SIZE);
    fail_if(alloc == NULL, "failed to dynamically create bitmap allocator by range");

    // Do it 3 times
    while (iterations) {
        for (i = 0; i < ALLOC_SIZE - ALLOC_BASE; i++) {
            // Every other iteration, attempt to allocate a named range
            if (i % 2) {
                okl4_bitmap_item_setunit(&item[i], i + ALLOC_BASE);

                rv = okl4_bitmap_allocator_alloc(alloc, &item[i]);

                // If the requested value is not already occupied
                if (rv == OKL4_OK) {
                    value = okl4_bitmap_item_getunit(&item[i]);

                    // Check it is the value requested
                    fail_unless(value == i + ALLOC_BASE,
                            "allocated bitmap item "
                            "is the wrong value");

                    continue;
                }
            }

            // Otherwise just allocate it anonymously
            okl4_bitmap_item_setany(&item[i]);

            rv = okl4_bitmap_allocator_alloc(alloc, &item[i]);
            fail_unless(rv == OKL4_OK, "failed to allocate "
                    "anonymous bitmap item");

            value = okl4_bitmap_item_getunit(&item[i]);

            // Check value is within bounds
            fail_unless(value >= ALLOC_BASE, "allocated item "
                    "below lower bounds");
            fail_unless(value < ALLOC_BASE + ALLOC_SIZE, "allocated item "
                    "exceeds upper bounds");

            // Check for duplicate allocations
            for (j = 0; j < i; j++) {
                value_j = okl4_bitmap_item_getunit(&item[j]);
                fail_if(value == value_j, "allocated item "
                        "has a duplicated value");
            }
        }

        // Free them in the order allocated
        free_all(alloc, item, ALLOC_SIZE - ALLOC_BASE);

        iterations--;
    }

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

static void
free_all_reverse(okl4_bitmap_allocator_t *alloc,
        okl4_bitmap_item_t *item_array, okl4_word_t qty)
{
    okl4_word_t i;

    for (i = qty; i > 0; i--) {
        okl4_bitmap_allocator_free(alloc, &item_array[i - 1]);
    }
}

static void
free_all_random(okl4_bitmap_allocator_t *alloc, okl4_bitmap_item_t *item_array, okl4_word_t qty)
{
    okl4_word_t i;
    okl4_word_t max;

    /* Find the next power of two larger than or equal to 'qty'. */
    for (max = 2; max < qty; max *= 2) {}

    /* Free all elements in the bitmap using a badly-random permutation. */
    for (i = 0; i < max; i++) {
        okl4_word_t t = (i ^ 0xa5a5a5a5UL) % max;
        if (t >= qty) {
            continue;
        }
        okl4_bitmap_allocator_free(alloc, &item_array[t]);
    }
}

/**
 *  @test BITMAPALLOC0601 : Mixed-order deallocation stress test, named
 *  @stat enabled
 *  @func That named allocation works after deallocation
 *        in no particular order.
 *
 *  @overview
 *      @li (Procedure and criteria as for BITMAPALLOC0401, except that
 *          items are not deallocated in the order they were allocated.)
 *      @li For the test to pass, all operations must succeed.
 */
START_TEST(BITMAPALLOC0601)
{
    int rv;
    okl4_bitmap_allocator_t *alloc;
    okl4_bitmap_item_t item[ALLOC_SIZE];

    alloc = alloc_dyn_init_range(ALLOC_BASE, ALLOC_SIZE);
    fail_if(alloc == NULL, "failed to dynamically create bitmap allocator by range");

    rv = allocate_all_named(alloc, item, ALLOC_BASE, ALLOC_SIZE);
    fail_unless(rv == OKL4_OK, "failed to allocate named bitmap items");

    free_all_reverse(alloc, item, ALLOC_SIZE);

    rv = allocate_all_named(alloc, item, ALLOC_BASE, ALLOC_SIZE);
    fail_unless(rv == OKL4_OK, "failed to allocate named bitmap items"
                               " a 2nd time after freeing all in reverse order");

    check_all_named(item, ALLOC_BASE, ALLOC_SIZE);

    free_all_random(alloc, item, ALLOC_SIZE);

    rv = allocate_all_named(alloc, item, ALLOC_BASE, ALLOC_SIZE);
    fail_unless(rv == OKL4_OK, "failed to allocate named bitmap items a 3rd time"
                               " after freeing all in no particular order");

    check_all_named(item, ALLOC_BASE, ALLOC_SIZE);

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test BITMAPALLOC0602 : Mixed-order deallocation stress test, anonymous
 *  @stat enabled
 *  @func That anonymous allocation works after deallocation
 *        in no particular order.
 *
 *  @overview
 *      @li (Procedure and criteria as for BITMAPALLOC0402, except that
 *          items are not deallocated in the order they were allocated.)
 *      @li For the test to pass, all operations must succeed.
 */
START_TEST(BITMAPALLOC0602)
{
    int rv;
    okl4_bitmap_allocator_t *alloc;
    okl4_bitmap_item_t item[ALLOC_SIZE];
    alloc = alloc_dyn_init_range(ALLOC_BASE, ALLOC_SIZE);
    fail_if(alloc == NULL, "failed to dynamically create bitmap allocator by range");

    rv = allocate_all_anonymous(alloc, item, ALLOC_SIZE);
    fail_unless(rv == OKL4_OK, "failed to anonymously allocate bitmap items");

    free_all_reverse(alloc, item, ALLOC_SIZE);

    rv = allocate_all_anonymous(alloc, item, ALLOC_SIZE);
    fail_unless(rv == OKL4_OK, "failed to anonymously allocate bitmap items"
                               " a 2nd time after freeing all in reverse order");

    check_all_anonymous(item, ALLOC_BASE, ALLOC_SIZE);

    free_all_random(alloc, item, ALLOC_SIZE);

    rv = allocate_all_anonymous(alloc, item, ALLOC_SIZE);
    fail_unless(rv == OKL4_OK, "failed to anonymously allocate bitmap items"
                               " a 3rd time"
                               " after freeing all in no particular order");

    check_all_anonymous(item, ALLOC_BASE, ALLOC_SIZE);

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test BITMAPALLOC0603 : Mixed allocation stress test with mixed-order deallocation
 *  @stat enabled
 *  @func Named and anonymous bitmap item allocation and deallocation.
 *
 *  @overview
 *      @li (Procedure and criteria as for BITMAPALLOC0600, except
 *          that items are deallocated in a different order to that
 *          in which they were allocated.)
 *      @li For the test to pass, all operations must succeed.
 */
START_TEST(BITMAPALLOC0603)
{
    int rv;
    okl4_word_t i, j, k, l, iterations, deallocount;
    okl4_word_t value, value_j;
    okl4_bitmap_allocator_t *alloc;
    okl4_bitmap_item_t item[ALLOC_SIZE];

    alloc = alloc_dyn_init_range(ALLOC_BASE, ALLOC_SIZE);
    fail_if(alloc == NULL, "failed to dynamically create bitmap allocator by range");

    iterations = 5;
    deallocount = 3;

    // Do it 3 times
    while (iterations) {
        for (k = 0; k < ALLOC_SIZE / 3; k++) {
            for (l = 0; l < deallocount; l++) {
                i = 3*k + l;

                // Every other iteration, attempt to allocate a named range
                if (i % 2) {
                    okl4_bitmap_item_setunit(&item[i], i + ALLOC_BASE);

                    rv = okl4_bitmap_allocator_alloc(alloc, &item[i]);

                    // If the requested value is not already occupied
                    if (rv == OKL4_OK) {
                        value = okl4_bitmap_item_getunit(&item[i]);

                        // Check it is the value requested
                        fail_unless(value == i + ALLOC_BASE,
                                "allocated bitmap item "
                                "is the wrong value");

                        continue;
                    }
                }

                // Otherwise just allocate it anonymously
                okl4_bitmap_item_setany(&item[i]);

                rv = okl4_bitmap_allocator_alloc(alloc, &item[i]);
                fail_unless(rv == OKL4_OK, "failed to allocate "
                        "anonymous bitmap item");

                value = okl4_bitmap_item_getunit(&item[i]);

                // Check value is within bounds
                fail_unless(value >= ALLOC_BASE, "allocated item "
                        "below lower bounds");
                fail_unless(value < ALLOC_BASE + ALLOC_SIZE, "allocated item "
                        "exceeds upper bounds");

                // Check for duplicate allocations
                for (j = 0; j < i; j++) {
                    value_j = okl4_bitmap_item_getunit(&item[j]);
                    fail_if(value == value_j, "allocated item "
                            "has a duplicated value");
                }
            }
        }

        switch (iterations) {
            case 5:
                free_all_reverse(alloc, item, ALLOC_SIZE);
                break;
            case 4:
                free_all_random(alloc, item, ALLOC_SIZE);
                break;
            case 3:
                deallocount = 1;
                break;
            case 2:
                deallocount = 2;
                break;
            default:
                deallocount = 3;
                break;
        }

        if (iterations <= 3) {
            for (i = 0; i < (ALLOC_SIZE) / 3; i++) {
                for (j = 0; j < deallocount; j++) {
                    okl4_bitmap_allocator_free(alloc, &item[3*i + j]);
                }
            }
        }

        iterations--;
    }

    okl4_bitmap_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

TCase *
make_bitmap_allocator_tcase(void)
{
    TCase *tc;

    tc = tcase_create("Bitmap Allocator");
    tcase_add_test(tc, BITMAPALLOC0100);
    tcase_add_test(tc, BITMAPALLOC0101);
    tcase_add_test(tc, BITMAPALLOC0102);
    tcase_add_test(tc, BITMAPALLOC0103);
    tcase_add_test(tc, BITMAPALLOC0200);
    tcase_add_test(tc, BITMAPALLOC0201);
    tcase_add_test(tc, BITMAPALLOC0202);
    tcase_add_test(tc, BITMAPALLOC0203);
    tcase_add_test(tc, BITMAPALLOC0300);
    tcase_add_test(tc, BITMAPALLOC0301);
    tcase_add_test(tc, BITMAPALLOC0302);
    tcase_add_test(tc, BITMAPALLOC0400);
    tcase_add_test(tc, BITMAPALLOC0401);
    tcase_add_test(tc, BITMAPALLOC0402);
#ifdef BAD_ASSERT_CHECKS
    tcase_add_test(tc, BITMAPALLOC0403);
    tcase_add_test(tc, BITMAPALLOC0404);
#endif
    tcase_add_test(tc, BITMAPALLOC0500);
    tcase_add_test(tc, BITMAPALLOC0501);
    tcase_add_test(tc, BITMAPALLOC0600);
    tcase_add_test(tc, BITMAPALLOC0601);
    tcase_add_test(tc, BITMAPALLOC0602);
    tcase_add_test(tc, BITMAPALLOC0603);

    return tc;
}
