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
#include <okl4/range.h>
#include <okl4/allocator_attr.h>

#include <stdlib.h>

#define ALLOC_BASE 3
#define ALLOC_SIZE 100
#define ALLOC_END  (ALLOC_BASE + ALLOC_SIZE)

/**
 *  @test RANGEALLOC0100 : Dynamic initialization by size, and destruction
 *  @stat enabled
 *  @func Dynamic initialization of a range allocator by size,
 *          and destruction.
 *
 *  @overview
 *      @li The size of the range allocator to be created is specified,
 *          by creating and setting an allocator attribute with a size value.
 *      @li The allocator attribute is used to determine the amount of memory
 *          required for the bitmap allocator, which is dynamically allocated.
 *      @li The allocator attribute is used to initialize the
 *          bitmap allocator object.
 *      @li The range allocator is destroyed and freed.
 *      @li For the test to pass, all operations must succeed.
 *
 *  @special  None.
 *
 */
START_TEST(RANGEALLOC0100)
{
    struct okl4_range_allocator *alloc;
    okl4_allocator_attr_t attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setsize(&attr, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0101 : Dynamic initialization by range, and destruction
 *  @stat enabled
 *  @func Dynamic initialization of a bitmap allocator by range,
 *          and destruction.
 *
 *  @overview
 *      @li (Procedure and criteria as for RANGEALLOC0100, except
 *          that the range of the allocator is explicitly set
 *          by specifying both a base and a size value.)
 *
 */
START_TEST(RANGEALLOC0101)
{
    struct okl4_range_allocator *alloc;
    okl4_allocator_attr_t attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, ALLOC_BASE, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0102 : Static initialization by size
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

OKL4_RANGE_ALLOCATOR_DECLARE_OFSIZE(test0102_alloc, ALLOC_SIZE);

START_TEST(RANGEALLOC0102)
{
    okl4_allocator_attr_t attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setsize(&attr, ALLOC_SIZE);

    okl4_range_allocator_init(test0102_alloc, &attr);

    okl4_range_allocator_destroy(test0102_alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0103 : Static initialization by range
 *  @stat enabled
 *  @func Static initialization of a bitmap allocator, by range.
 *
 *  @overview
 *      @li (Procedure and criteria as for RANGEALLOC0102, except
 *          that the range of the allocator is explicitly set
 *          by specifying both a base and a size value.)
 *
 */

OKL4_RANGE_ALLOCATOR_DECLARE_OFRANGE(test0103_alloc, ALLOC_BASE, ALLOC_SIZE);

START_TEST(RANGEALLOC0103)
{
    okl4_allocator_attr_t attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, ALLOC_BASE, ALLOC_SIZE);

    okl4_range_allocator_init(test0103_alloc, &attr);

    okl4_range_allocator_destroy(test0103_alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0200 : Anonymous allocation within available bounds
 *  @stat enabled
 *  @func That one single anonymous allocation succeeds.
 *
 *  @overview
 *      @li Attempt to allocate one single anonymous range item
 *          from a newly initialized allocator.
 *      @li For the test to pass, the allocation must succeed and
 *          be within the space the allocator was initialized with.
 */

START_TEST(RANGEALLOC0200)
{
    int                         rv;
    okl4_word_t                 base, size;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item      range;
    okl4_allocator_attr_t       attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    assert(alloc);
    okl4_range_allocator_init(alloc, &attr);

    okl4_range_item_setsize(&range, 80);
    rv = okl4_range_allocator_alloc(alloc, &range);
    fail_unless(rv == OKL4_OK,"failed to allocate range");

    base = okl4_range_item_getbase(&range);
    size = okl4_range_item_getsize(&range);

    fail_unless(size == 80, "allocated range is the wrong size");
    fail_unless(base + size <= ALLOC_END, "allocated range overlaps upper bound");

    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0201 : Named allocation within available bounds
 *  @stat enabled
 *  @func That one single named allocation succeeds.
 *
 *  @overview
 *      @li Attempt to allocate one single named range item,
 *          well within the bounds of a newly initialized allocator.
 *      @li For the test to pass, the allocation must succeed and
 *          be the value requested.
 *
 */

START_TEST(RANGEALLOC0201)
{
    int                         rv;
    okl4_word_t                 base, size;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item      range;
    okl4_allocator_attr_t       attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    okl4_range_item_setrange(&range, 10, 80);
    rv = okl4_range_allocator_alloc(alloc, &range);
    fail_unless(rv == OKL4_OK,"failed to allocate range");

    base = okl4_range_item_getbase(&range);
    size = okl4_range_item_getsize(&range);

    fail_unless(size == 80, "allocated range is the wrong size");
    fail_unless(base == 10, "allocated range has the wrong start value");
    fail_unless(base + size == 90, "allocated range has the wrong end value");

    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0202 : Anonymous allocation filling available bounds
 *  @stat enabled
 *  @func That a single anonymous allocation filling the allocator space succeeds.
 *
 *  @overview
 *      @li Attempt to allocate one single named range item,
 *          of size equal to that of a newly initialized allocator.
 *      @li For the test to pass, the allocation must succeed in
 *          returning the entire space.
 *
 */

START_TEST(RANGEALLOC0202)
{
    int                         rv;
    okl4_word_t                 base, size;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item      range;
    okl4_allocator_attr_t       attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    okl4_range_item_setsize(&range, ALLOC_SIZE);
    rv = okl4_range_allocator_alloc(alloc, &range);
    fail_unless(rv == OKL4_OK,"failed to allocate range");

    base = okl4_range_item_getbase(&range);
    size = okl4_range_item_getsize(&range);

    fail_unless(size == ALLOC_SIZE, "allocated range is the wrong size");
    fail_unless(base + size <= ALLOC_SIZE, "allocated range overlaps upper bound");

    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0203 : Named allocation filling available bounds
 *  @stat enabled
 *  @func That a single named allocation filling the allocator space succeeds.
 *
 *  @overview
 *      @li Attempt to allocate one single named range item,
 *          with base and size values equal to those of a
 *          newly initialized allocator.
 *      @li For the test to pass, the allocation must succeed in
 *          returning the entire space.
 *
 */

START_TEST(RANGEALLOC0203)
{
    int                         rv;
    okl4_word_t                 base, size;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item      range;
    okl4_allocator_attr_t       attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    okl4_range_item_setrange(&range, 0, ALLOC_SIZE);
    rv = okl4_range_allocator_alloc(alloc, &range);
    fail_unless(rv == OKL4_OK,"failed to allocate range");

    base = okl4_range_item_getbase(&range);
    size = okl4_range_item_getsize(&range);

    fail_unless(size == ALLOC_SIZE, "allocated range is the wrong size");
    fail_unless(base == 0, "allocated range has the wrong start value");
    fail_unless(base + size == ALLOC_SIZE, "allocated range has the wrong end value");

    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST


/**
 *  @test RANGEALLOC0300 : Deallocation
 *  @stat enabled
 *  @func Range item deallocation.
 *
 *  @overview
 *      @li A range is allocated anonymously
 *          (as in RANGEALLOC0200), then freed.
 *      @li For the test to pass, all operations must succeed.
 *
 */

START_TEST(RANGEALLOC0300)
{
    int                         rv;
    okl4_word_t                 base, size;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item      range;
    okl4_allocator_attr_t       attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    okl4_range_item_setsize(&range, 80);
    rv = okl4_range_allocator_alloc(alloc, &range);
    fail_unless(rv == OKL4_OK,"failed to allocate range");

    base = okl4_range_item_getbase(&range);
    size = okl4_range_item_getsize(&range);

    fail_unless(size == 80, "allocated range is the wrong size");
    fail_unless(base + size <= ALLOC_SIZE, "allocated range overlaps upper bound");

    okl4_range_allocator_free(alloc, &range);
    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0301 : Anonymous allocation after deallocation
 *  @stat enabled
 *  @func That anonymous allocation works after deallocation.
 *
 *  @overview
 *      @li A range is allocated anonymously
 *          (as in RANGEALLOC0200), then freed.
 *      @li The range is then allocated again and checked for validity.
 *      @li For the test to pass, all operations must succeed.
 */

START_TEST(RANGEALLOC0301)
{
    int                         rv;
    okl4_word_t                 base, size;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item      range;
    okl4_allocator_attr_t       attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    okl4_range_item_setsize(&range, 80);
    rv = okl4_range_allocator_alloc(alloc, &range);
    fail_unless(rv == OKL4_OK,"failed to allocate range");

    base = okl4_range_item_getbase(&range);
    size = okl4_range_item_getsize(&range);

    fail_unless(size == 80, "allocated range is the wrong size");
    fail_unless(base + size <= ALLOC_SIZE, "allocated range overlaps upper bound");

    okl4_range_allocator_free(alloc, &range);


    okl4_range_item_setsize(&range, ALLOC_SIZE);
    rv = okl4_range_allocator_alloc(alloc, &range);
    fail_unless(rv == OKL4_OK,"failed to allocate 2nd range");

    base = okl4_range_item_getbase(&range);
    size = okl4_range_item_getsize(&range);

    fail_unless(size == ALLOC_SIZE,
                "2nd allocated range is the wrong size");
    fail_unless(base + size <= ALLOC_SIZE, "allocated range overlaps upper bound");

    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0302 : Named allocation after deallocation
 *  @stat enabled
 *  @func That named allocation works after deallocation.
 *
 *  @overview
 *      @li A named range is allocated
 *          (as in RANGEALLOC0201), then freed.
 *      @li The range is then allocated again and checked for validity.
 *      @li For the test to pass, all operations must succeed.
 */

START_TEST(RANGEALLOC0302)
{
    int                         rv;
    okl4_word_t                 base, size;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item      range;
    okl4_allocator_attr_t       attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    okl4_range_item_setrange(&range, 10, 80);
    rv = okl4_range_allocator_alloc(alloc, &range);
    fail_unless(rv == OKL4_OK,"failed to allocate range");

    base = okl4_range_item_getbase(&range);
    size = okl4_range_item_getsize(&range);

    fail_unless(size == 80, "allocated range is the wrong size");
    fail_unless(base == 10, "allocated range overlaps lower bound");
    fail_unless(base + size <= ALLOC_SIZE, "allocated range overlaps upper bound");

    okl4_range_allocator_free(alloc, &range);

    okl4_range_item_setrange(&range, 0, ALLOC_SIZE);
    rv = okl4_range_allocator_alloc(alloc, &range);
    fail_unless(rv == OKL4_OK,"failed to allocate 2nd range");

    base = okl4_range_item_getbase(&range);
    size = okl4_range_item_getsize(&range);

    fail_unless(size == ALLOC_SIZE,
                "2nd allocated range is the wrong size");
    fail_unless(base + size <= ALLOC_SIZE, "allocated range overlaps upper bound");

    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0400 : Named allocation attempts overlapping bounds
 *  @stat enabled
 *  @func That all attempts at named allocation overlapping bounds fail.
 *
 *  @overview
 *      @li Attempts are made to allocate range items with named intervals
 *          overlapping the lower and upper bounds of the range available
 *          from the allocator.
 *      @li For the test to pass, the allocations must fail with the
 *          appropriate error codes (OKL4_NOT_AVAILABLE, or OKL4_IN_USE
 *          if only the upper bound is being crossed).
 */

START_TEST(RANGEALLOC0400)
{
    int                         rv;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item      range;
    okl4_allocator_attr_t       attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, ALLOC_BASE, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    okl4_range_item_setrange(&range, ALLOC_BASE + 1, ALLOC_SIZE + 1);
    rv = okl4_range_allocator_alloc(alloc, &range);
    fail_unless(rv == OKL4_IN_USE,
            "attempt to allocate range overlapping upper bound"
            " did not fail appropriately");

    okl4_range_item_setrange(&range, ALLOC_BASE - 1, ALLOC_SIZE - 1);
    rv = okl4_range_allocator_alloc(alloc, &range);
    fail_unless(rv == OKL4_ALLOC_EXHAUSTED,
            "attempt to allocate range overlapping lower bound"
            " did not fail appropriately");

    okl4_range_item_setrange(&range, ALLOC_BASE - 1, ALLOC_SIZE + 1);
    rv = okl4_range_allocator_alloc(alloc, &range);
    fail_unless(rv == OKL4_ALLOC_EXHAUSTED,
            "attempt to allocate range overlapping both bounds"
            " did not fail appropriately");

    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0401 : Anonymous allocation attempt with insufficient space
 *  @stat enabled
 *  @func That anonymous allocation fails if there is insufficient space
 *        due to previous allocations.
 *
 *  @overview
 *      @li An anonymous range item filling the allocator's available space
 *          is allocated (as in RANGEALLOC0202).
 *      @li An additional attempt is made to allocate an anonymous range.
 *      @li For the test to pass, the final allocation attempt must fail
 *          with the appropriate error code (OKL4_IN_USE).
 */

START_TEST(RANGEALLOC0401)
{
    int                         rv;
    okl4_word_t                 base, size;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item      range, range2;
    okl4_allocator_attr_t       attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    okl4_range_item_setsize(&range, ALLOC_SIZE);
    rv = okl4_range_allocator_alloc(alloc, &range);
    fail_unless(rv == OKL4_OK,"failed to allocate range");

    base = okl4_range_item_getbase(&range);
    size = okl4_range_item_getsize(&range);

    fail_unless(size == ALLOC_SIZE,
                "allocated range is the wrong size");
    fail_unless(base + size <= ALLOC_SIZE, "allocated range overlaps upper bound");


    okl4_range_item_setsize(&range2, 50);
    rv = okl4_range_allocator_alloc(alloc, &range2);
    fail_unless(rv == OKL4_IN_USE,
            "attempt to allocate range with insufficient space "
            "remaining did not fail appropriately");

    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0402 : Anonymous allocation attempt larger than bounds
 *  @stat enabled
 *  @func That anonymous allocation fails if attempting to allocate
 *        a range larger than the allocator space.
 *
 *  @overview
 *      @li An attempt is made to allocate an anonymous range larger
 *          than the space the range allocator was initialized with.
 *      @li For the test to pass, the allocation attempt must fail
 *          with the appropriate error code (OKL4_IN_USE).
 */

START_TEST(RANGEALLOC0402)
{
    int                         rv;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item      range;
    okl4_allocator_attr_t       attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    okl4_range_item_setsize(&range, ALLOC_SIZE + 1);
    rv = okl4_range_allocator_alloc(alloc, &range);
    fail_unless(rv == OKL4_IN_USE, "attempt to allocate range larger than bounds"
                " did not fail appropriately");

    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0500 : Deallocation attempt after failed deallocation
 *  @stat disabled, due to assertions
 *  @func That attempting to deallocate a range that has not been allocated
 *        triggers an assertion.
 *
 *  @overview
 *      @li An attempt is made to allocate an anonymous range with
 *          insufficient space (as in RANGEALLOC0401). This should fail.
 *      @li An attempt is made to free the range item.
 *      @li For the test to pass, the deallocation must trigger an assertion.
 *
 */
#ifdef BAD_ASSERTION_CHECKS

START_TEST(RANGEALLOC0500)
{
    int                         rv;
    okl4_word_t                 base, size;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item      range, range2;
    okl4_allocator_attr_t       attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    okl4_range_item_setsize(&range, ALLOC_SIZE);
    rv = okl4_range_allocator_alloc(alloc, &range);
    fail_unless(rv == OKL4_OK,"failed to allocate range");

    base = okl4_range_item_getbase(&range);
    size = okl4_range_item_getsize(&range);

    fail_unless(size == ALLOC_SIZE,
            "allocated range is the wrong size");
    fail_unless(base + size <= ALLOC_SIZE, "allocated range overlaps upper bound");


    okl4_range_item_setsize(&range2, 50);
    rv = okl4_range_allocator_alloc(alloc, &range2);
    fail_unless(rv == OKL4_IN_USE,
            "attempt to allocate range with insufficient space "
            "remaining did not fail appropriately");

    okl4_range_allocator_free(alloc, &range);
    okl4_range_allocator_free(alloc, &range2);
    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST
#endif

/**
 *  @test RANGEALLOC0501 : Allocation after failed deallocation
 *  @stat enabled
 *  @func That range items are reusable after a failed attempt to perform
 *        anonymous allocation.
 *
 *  @overview
 *      @li After a failed allocation attempt (as in RANGEALLOC0401),
 *          sufficient space is freed from the allocator such that
 *          a second attempt would succeed.
 *      @li A second attempt is made to allocate the previously failed item.
 *      @li For the test to pass, the second allocation must succeed.
 *
 */

START_TEST(RANGEALLOC0501)
{
    int                         rv;
    okl4_word_t                 base, size;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item      range, range2;
    okl4_allocator_attr_t       attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    okl4_range_item_setsize(&range, ALLOC_SIZE);
    rv = okl4_range_allocator_alloc(alloc, &range);
    fail_unless(rv == OKL4_OK,"failed to allocate range");

    base = okl4_range_item_getbase(&range);
    size = okl4_range_item_getsize(&range);

    fail_unless(size == ALLOC_SIZE,
                "allocated range is the wrong size");
    fail_unless(base + size <= ALLOC_BASE + ALLOC_SIZE, "allocated range overlaps upper bound");


    okl4_range_item_setsize(&range2, 50);
    rv = okl4_range_allocator_alloc(alloc, &range2);
    fail_unless(rv == OKL4_IN_USE,
            "attempt to allocate range with insufficient space "
            "remaining did not fail appropriately");

    okl4_range_allocator_free(alloc, &range);

    okl4_range_item_setsize(&range2, 60);
    rv = okl4_range_allocator_alloc(alloc, &range2);
    fail_unless(rv == OKL4_OK,"failed to allocate 2nd range after recently freeing 1st");

    base = okl4_range_item_getbase(&range2);
    size = okl4_range_item_getsize(&range2);

    fail_unless(size == 60, "2nd allocated range is the wrong size");
    fail_unless(base + size <= ALLOC_SIZE, "allocated range overlaps upper bound");

    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

#define MAX_RANGES (ALLOC_SIZE / 8)

/**
 *  @test RANGEALLOC0600 : Anonymous allocation stress test, byte-size ranges
 *  @stat enabled
 *  @func Anonymous range item allocation and deallocation.
 *
 *  @overview
 *      @li As many anonymous ranges of size 8 are allocated
 *          as there is space for them in the allocator.
 *          All must be of correct size and not overlap
 *          the bounds of the allocator space, nor each other.
 *      @li With the allocator space now filled, a final anonymous
 *          allocation attempt is made, which must fail
 *          with the appropriate error code (OKL4_IN_USE).
 *      @li The above procedure is repeated several times, deallocating
 *          different amounts at the end of each repetition and only
 *          attempting to allocate that many on the next one.
 *
 */

START_TEST(RANGEALLOC0600)
{
    int rv;
    okl4_word_t i, iterations, deallocount;
    okl4_word_t base, size;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item range[MAX_RANGES], range_failer;
    okl4_allocator_attr_t attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    iterations = 3;
    deallocount = MAX_RANGES;

    // Do it 3 times, at each step deallocating a different amount at the end
    // and at the next step only attempting to allocate that many
    while (iterations)
    {
        for (i = 0; i < deallocount; i++)
        {
            okl4_range_item_setsize(&range[i], 8);
            rv = okl4_range_allocator_alloc(alloc, &range[i]);
            fail_unless(rv == OKL4_OK,"failed to allocate range");
        }

        for (i = 0; i < MAX_RANGES; i++)
        {
            base = okl4_range_item_getbase(&range[i]);
            size = okl4_range_item_getsize(&range[i]);

            fail_unless(size == 8, "allocated range is the wrong size");
            fail_unless(base + size <= ALLOC_SIZE, "allocated range overlaps upper bound");
        }

        okl4_range_item_setsize(&range_failer, 8);
        rv = okl4_range_allocator_alloc(alloc, &range_failer);
        fail_unless(rv == OKL4_IN_USE,
                "attempt to allocate range with insufficient space "
                "remaining did not fail appropriately");

        switch (iterations)
        {
            case 3:
                deallocount = MAX_RANGES/3;
                break;
            case 2:
                deallocount = 2*MAX_RANGES/3;
                break;
            default:
                deallocount = MAX_RANGES;
        }

        for (i = 0; i < deallocount; i++)
        {
            okl4_range_allocator_free(alloc, &range[i]);
        }

        iterations--;
    }
    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0601 : Named allocation stress test, byte-size ranges
 *  @stat enabled
 *  @func Named range item allocation and deallocation.
 *
 *  @overview
 *      @li As many named ranges of size 8 are allocated
 *          as there is space for them in the allocator.
 *          All are checked for correct start / end values.
 *      @li Attempts are also made to allocate ranges of size 8
 *          overlapping the already-allocated ones and the
 *          top boundary of the allocator space, which must fail
 *          with the appropriate error code (OKL4_NOT_AVAILABLE
 *          and OKL4_IN_USE respectively).
 *      @li With the allocator space now filled, a final anonymous
 *          allocation attempt is made, which must fail
 *          with the appropriate error code (OKL4_IN_USE).
 *      @li The above procedure is repeated several times, deallocating
 *          different amounts at the end of each repetition and only
 *          attempting to allocate that many on the next one.
 *
 */

START_TEST(RANGEALLOC0601)
{
    int rv;
    okl4_word_t i;
    okl4_word_t iterations, deallocount;
    okl4_word_t base, size;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item range[MAX_RANGES], range_failer;
    okl4_allocator_attr_t attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    deallocount = MAX_RANGES;

    // Do it 3 times, at each step deallocating a different amount at the end
    // and at the next step only expecting that many successful allocations
    for (iterations = 0; iterations < 3; iterations++) {
        for (i = 0; i < ALLOC_SIZE; i++) {
            if ((i / 8) >= MAX_RANGES) {
                okl4_range_item_setrange(&range_failer, i, 8);
                rv = okl4_range_allocator_alloc(alloc, &range_failer);
                fail_unless(rv == OKL4_IN_USE,
                        "attempt to allocate range overlapping upper bound of allocator "
                        "did not fail appropriately");
                continue;
            }

            if (((i % 8) == 0) && ((i / 8) < deallocount)) {
                assert(i < MAX_RANGES * 8);
                okl4_range_item_setrange(&range[i / 8], i, 8);
                rv = okl4_range_allocator_alloc(alloc, &range[i / 8]);
                fail_unless(rv == OKL4_OK,"failed to allocate range");
            } else {
                okl4_range_item_setrange(&range_failer, i, 8);
                rv = okl4_range_allocator_alloc(alloc, &range_failer);
                fail_unless(rv == OKL4_ALLOC_EXHAUSTED,
                        "attempt to allocate range overlapping existing ranges "
                        "did not fail appropriately");
            }
        }

        for (i = 0; i < MAX_RANGES; i++) {
            base = okl4_range_item_getbase(&range[i]);
            size = okl4_range_item_getsize(&range[i]);

            fail_unless(size == 8, "allocated range is the wrong size");
            fail_unless(base + size == 8 * (i + 1), "allocated range has the wrong end value");
            fail_unless(base == 8 * i, "allocated range has the wrong start value");
        }

        okl4_range_item_setsize(&range_failer, 8);
        rv = okl4_range_allocator_alloc(alloc, &range_failer);
        fail_unless(rv == OKL4_IN_USE,
                "attempt to allocate range with insufficient space "
                "remaining did not fail appropriately");

        switch (iterations) {
            case 3:
                deallocount = MAX_RANGES/3;
                break;
            case 2:
                deallocount = 2*MAX_RANGES/3;
                break;
            default:
                deallocount = MAX_RANGES;
                break;
        }

        for (i = 0; i < deallocount; i++) {
            okl4_range_allocator_free(alloc, &range[i]);
        }
    }
    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0602 : Anonymous allocation stress test, unit-size ranges
 *  @stat enabled
 *  @func Anonymous range item allocation and deallocation.
 *
 *  @overview
 *      @li (Procedure and criteria as for RANGEALLOC0600,
 *          except that all ranges are of size 1.)
 *
 */

START_TEST(RANGEALLOC0602)
{
    int rv;
    okl4_word_t i, iterations, deallocount;
    okl4_word_t base, size;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item range[ALLOC_SIZE], range_failer;
    okl4_allocator_attr_t attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    iterations = 3;
    deallocount = ALLOC_SIZE;

    // Do it 3 times, at each step deallocating a different amount at the end
    // and at the next step only attempting to allocate that many
    while (iterations)
    {
        for (i = 0; i < deallocount; i++)
        {
            okl4_range_item_setsize(&range[i], 1);
            rv = okl4_range_allocator_alloc(alloc, &range[i]);
            fail_unless(rv == OKL4_OK,"failed to allocate range");
        }

        for (i = 0; i < ALLOC_SIZE; i++)
        {
            base = okl4_range_item_getbase(&range[i]);
            size = okl4_range_item_getsize(&range[i]);

            fail_unless(size == 1, "allocated range is the wrong size");
            fail_unless(base + size <= ALLOC_SIZE, "allocated range overlaps upper bound");
        }

        okl4_range_item_setsize(&range_failer, 1);
        rv = okl4_range_allocator_alloc(alloc, &range_failer);
        fail_unless(rv == OKL4_IN_USE,
                "attempt to allocate range with insufficient space "
                "remaining did not fail appropriately");

        switch (iterations)
        {
            case 3:
                deallocount = ALLOC_SIZE/3;
                break;
            case 2:
                deallocount = 2*ALLOC_SIZE/3;
                break;
            default:
                deallocount = ALLOC_SIZE;
        }

        for (i = 0; i < deallocount; i++)
        {
            okl4_range_allocator_free(alloc, &range[i]);
        }

        iterations--;
    }

    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0603 : Named allocation stress test, unit-size ranges
 *  @stat enabled
 *  @func Named range item allocation and deallocation.
 *
 *  @overview
 *      @li (Procedure and criteria as for RANGEALLOC0601,
 *          except that all ranges are of size 1, and there are no
 *          intentional overlap allocation attempts.)
 *
 */

START_TEST(RANGEALLOC0603)
{
    int rv;
    okl4_word_t i, iterations, deallocount;
    okl4_word_t base, size;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item range[ALLOC_SIZE], range_failer;
    okl4_allocator_attr_t attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    deallocount = ALLOC_SIZE;

    // Do it 3 times, at each step deallocating a different amount at the end
    // and at the next step only attempting to allocate that many
    for (iterations = 0; iterations < 3; iterations++) {
        for (i = 0; i < deallocount; i++) {
            okl4_range_item_setrange(&range[i], i, 1);
            rv = okl4_range_allocator_alloc(alloc, &range[i]);
            fail_unless(rv == OKL4_OK,"failed to allocate range");
        }

        for (i = 0; i < ALLOC_SIZE; i++) {
            base = okl4_range_item_getbase(&range[i]);
            size = okl4_range_item_getsize(&range[i]);

            fail_unless(size == 1, "allocated range is the wrong size");
            fail_unless(base + size == i + 1, "allocated range has the wrong end value");
            fail_unless(base == i, "allocated range has the wrong start value");
        }

        okl4_range_item_setsize(&range_failer, 1);
        rv = okl4_range_allocator_alloc(alloc, &range_failer);
        fail_unless(rv == OKL4_IN_USE,
                "attempt to allocate range with insufficient space "
                "remaining did not fail appropriately");

        switch (iterations) {
            case 3:
                deallocount = ALLOC_SIZE/3;
                break;
            case 2:
                deallocount = 2*ALLOC_SIZE/3;
                break;
            default:
                deallocount = ALLOC_SIZE;
        }

        for (i = 0; i < deallocount; i++) {
            okl4_range_allocator_free(alloc, &range[i]);
        }
    }

    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0604 : Mixed allocation stress test
 *  @stat enabled
 *  @func Named and anonymous range item allocation and deallocation
 *
 *  @overview
 *      @li Named and anonymous ranges of size 8 are allocated
 *          alternatingly until the space is filled (in a similar way to
 *          RANGEALLOC0600 and RANGEALLOC0601), deallocated in the order
 *          they were allocated, and allocated again.
 *      @li The above procedure is repeated several times, deallocating
 *          different amounts at the end of each repetition and only
 *          attempting to allocate that many on the next one.
 *      @li For the test to pass, all allocations must succeed.
 *
 */
START_TEST(RANGEALLOC0604)
{
    int rv;
    okl4_word_t i, iterations, deallocount;
    okl4_word_t base, size;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item range[MAX_RANGES], range_failer;
    okl4_allocator_attr_t attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    iterations = 3;
    deallocount = MAX_RANGES;

    // Do it 3 times, at each step deallocating a different amount at the end
    // and at the next step only attempting to allocate that many
    while (iterations)
    {
        for (i = 0; i < deallocount; i++)
        {
            // Every other iteration, attempt to allocate a named range
            if (i % 2)
            {
                okl4_range_item_setrange(&range[i], i * 8, 8);
                rv = okl4_range_allocator_alloc(alloc, &range[i]);
                if (rv == OKL4_OK)
                {
                    base = okl4_range_item_getbase(&range[i]);
                    size = okl4_range_item_getsize(&range[i]);

                    fail_unless(size == 8, "allocated range is the wrong size");
                    fail_unless(base + size == 8 * (i + 1), "allocated range has the wrong end value");
                    fail_unless(base == 8 * i, "allocated range has the wrong start value");
                    continue;
                }
            }

            // Otherwise just allocate an anonymous range
            okl4_range_item_setsize(&range[i], 8);
            rv = okl4_range_allocator_alloc(alloc, &range[i]);
            fail_unless(rv == OKL4_OK,"failed to allocate range");

            base = okl4_range_item_getbase(&range[i]);
            size = okl4_range_item_getsize(&range[i]);

            fail_unless(size == 8, "allocated range is the wrong size");
            fail_unless(base + size <= ALLOC_SIZE, "allocated range overlaps upper bound");
        }

        okl4_range_item_setsize(&range_failer, 8);
        rv = okl4_range_allocator_alloc(alloc, &range_failer);
        base = okl4_range_item_getbase(&range_failer);
        size = okl4_range_item_getsize(&range_failer);
        fail_unless(rv == OKL4_IN_USE,
                "attempt to allocate range with insufficient space "
                "remaining did not fail appropriately");

        switch (iterations)
        {
            case 3:
                deallocount = MAX_RANGES/3;
                break;
            case 2:
                deallocount = 2*MAX_RANGES/3;
                break;
            default:
                deallocount = MAX_RANGES;
        }

        for (i = 0; i < deallocount; i++)
        {
            okl4_range_allocator_free(alloc, &range[i]);
        }

        iterations--;
    }

    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0605 : Mixed-order deallocation stress test, anonymous
 *  @stat enabled
 *  @func Anonymous range item allocation and deallocation
 *
 *  @overview
 *      @li (Procedure and criteria as for RANGEALLOC0600, except
 *          that on each repetition the ranges are deallocated in
 *          an order different to that in which they were allocated.)
 *
 */

START_TEST(RANGEALLOC0605)
{
    int rv;
    okl4_word_t i, j, iterations, deallocount;
    okl4_word_t base, size;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item range[MAX_RANGES], range_failer;
    okl4_allocator_attr_t attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    iterations = 3;
    deallocount = 3;

    // Do it 3 times, at each step deallocating some ranges
    // and at the next step only attempting to allocate those ones
    while (iterations)
    {
        for (i = 0; i < MAX_RANGES / 3; i++)
        {
            for (j = 0; j < deallocount; j++)
            {
                okl4_range_item_setsize(&range[3*i + j], 8);
                rv = okl4_range_allocator_alloc(alloc, &range[3*i + j]);
                fail_unless(rv == OKL4_OK,"failed to allocate range");
            }
        }

        for (i = 0; i < MAX_RANGES; i++)
        {
            base = okl4_range_item_getbase(&range[i]);
            size = okl4_range_item_getsize(&range[i]);

            fail_unless(size == 8, "allocated range is the wrong size");
            fail_unless(base + size <= ALLOC_SIZE, "allocated range overlaps upper bound");
        }

        okl4_range_item_setsize(&range_failer, 8);
        rv = okl4_range_allocator_alloc(alloc, &range_failer);
        fail_unless(rv == OKL4_IN_USE,
                "attempt to allocate range with insufficient space "
                "remaining did not fail appropriately");

        switch (iterations)
        {
            case 3:
                deallocount = 1;
                break;
            case 2:
                deallocount = 2;
                break;
            default:
                deallocount = 3;
        }

        for (i = 0; i < MAX_RANGES / 3; i++)
        {
            for (j = 0; j < deallocount; j++)
            {
                okl4_range_allocator_free(alloc, &range[3*i + j]);
            }
        }

        iterations--;
    }
    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0606 : Mixed-order deallocation stress test, named
 *  @stat enabled
 *  @func Named range item allocation and deallocation
 *
 *  @overview
 *      @li (Procedure and criteria as for RANGEALLOC0601, except
 *          that on each repetition the ranges are deallocated in
 *          an order different to that in which they were allocated.)
 *
 */

START_TEST(RANGEALLOC0606)
{
    int rv;
    okl4_word_t i, j, iterations, deallocount;
    okl4_word_t base, size;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item range[MAX_RANGES], range_failer;
    okl4_allocator_attr_t attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    iterations = 3;
    deallocount = 3;

    // Do it 3 times, at each step deallocating some ranges
    // and at the next step only attempting to allocate those ones
    while (iterations)
    {
        for (i = 0; i < MAX_RANGES / 3; i++)
        {
            for (j = 0; j < deallocount; j++)
            {
                okl4_word_t k = 3*i + j;
                okl4_range_item_setrange(&range[k], 8*k, 8);
                rv = okl4_range_allocator_alloc(alloc, &range[k]);
                fail_unless(rv == OKL4_OK,"failed to allocate range");
            }
        }

        for (i = 0; i < MAX_RANGES; i++)
        {
            base = okl4_range_item_getbase(&range[i]);
            size = okl4_range_item_getsize(&range[i]);

            fail_unless(size == 8, "allocated range is the wrong size");
            fail_unless(base + size == 8 * (i + 1), "allocated range has the wrong end value");
            fail_unless(base == 8 * i, "allocated range has the wrong start value");
        }

        okl4_range_item_setsize(&range_failer, 8);
        rv = okl4_range_allocator_alloc(alloc, &range_failer);
        fail_unless(rv == OKL4_IN_USE,
                "attempt to allocate range with insufficient space "
                "remaining did not fail appropriately");

        switch (iterations)
        {
            case 3:
                deallocount = 1;
                break;
            case 2:
                deallocount = 2;
                break;
            default:
                deallocount = 3;
        }

        for (i = 0; i < MAX_RANGES / 3; i++)
        {
            for (j = 0; j < deallocount; j++)
            {
                okl4_range_allocator_free(alloc, &range[3*i + j]);
            }
        }

        iterations--;
    }
    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

/**
 *  @test RANGEALLOC0607 : Mixed allocation with mixed-order deallocation
 *  @stat enabled
 *  @func Named and anonymous range item allocation and deallocation
 *
 *  @overview
 *      @li (Procedure and criteria as for RANGEALLOC0604, except
 *          that on each repetition the ranges are deallocated in
 *          an order different to that in which they were allocated.)
 *
 */

START_TEST(RANGEALLOC0607)
{
    int rv;
    okl4_word_t i, j, k, iterations, deallocount;
    okl4_word_t base, size;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item range[MAX_RANGES], range_failer;
    okl4_allocator_attr_t attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    iterations = 3;
    deallocount = 3;

    // Do it 3 times, at each step deallocating a different amount at the end
    // and at the next step only attempting to allocate that many
    while (iterations)
    {
        for (k = 0; k < MAX_RANGES / 3; k++)
        {
            for (j = 0; j < deallocount; j++)
            {
                i = k*3 + j;

                // Every other iteration, attempt to allocate a named range
                if (i % 2)
                {
                    okl4_range_item_setrange(&range[i], i * 8, 8);
                    rv = okl4_range_allocator_alloc(alloc, &range[i]);
                    if (rv == OKL4_OK)
                    {
                        base = okl4_range_item_getbase(&range[i]);
                        size = okl4_range_item_getsize(&range[i]);

                        fail_unless(size == 8, "allocated range is the wrong size");
                        fail_unless(base + size == 8 * (i + 1), "allocated range has the wrong end value");
                        fail_unless(base == 8 * i, "allocated range has the wrong start value");
                        continue;
                    }
                }

                // Otherwise just allocate an anonymous range
                okl4_range_item_setsize(&range[i], 8);
                rv = okl4_range_allocator_alloc(alloc, &range[i]);
                fail_unless(rv == OKL4_OK,"failed to allocate range");

                base = okl4_range_item_getbase(&range[i]);
                size = okl4_range_item_getsize(&range[i]);

                fail_unless(size == 8, "allocated range is the wrong size");
                fail_unless(base + size <= ALLOC_SIZE, "allocated range overlaps upper bound");
            }
        }

        okl4_range_item_setsize(&range_failer, 8);
        rv = okl4_range_allocator_alloc(alloc, &range_failer);
        fail_unless(rv == OKL4_IN_USE,
                "attempt to allocate range with insufficient space "
                "remaining did not fail appropriately");

        switch (iterations)
        {
            case 3:
                deallocount = 1;
                break;
            case 2:
                deallocount = 2;
                break;
            default:
                deallocount = 3;
        }

        for (i = 0; i < MAX_RANGES / 3; i++)
        {
            for (j = 0; j < deallocount; j++)
            {
                okl4_range_allocator_free(alloc, &range[3*i + j]);
            }
        }

        iterations--;
    }

    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST


/**
 *  @test RANGEALLOC0608 : Mixed size allocation with mixed-order deallocation
 *  @stat enabled
 *  @func Named and anonymous range item allocation and deallocation
 *
 *  @overview
 *      @li (Procedure and criteria as for RANGEALLOC0607, except
 *          that ranges are of various sizes.)
 *
 */

START_TEST(RANGEALLOC0608)
{
    int rv;
    okl4_word_t i, j, k, iterations, deallocount, allocSize;
    okl4_word_t base, size;
    struct okl4_range_allocator *alloc;
    struct okl4_range_item range[MAX_RANGES];
    okl4_allocator_attr_t attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 0, ALLOC_SIZE);
    alloc = malloc(OKL4_RANGE_ALLOCATOR_SIZE_ATTR(&attr));
    okl4_range_allocator_init(alloc, &attr);

    allocSize = 1;
    iterations = 3;
    deallocount = 3;

    // Do it 3 times, at each step deallocating a different amount at the end
    // and at the next step only attempting to allocate that many
    while (iterations)
    {
        for (k = 0; k < MAX_RANGES / 3; k++)
        {
            for (j = 0; j < deallocount; j++)
            {
                i = k*3 + j;

                // Every other iteration, attempt to allocate a named range
                if (i % 2)
                {
                    okl4_range_item_setrange(&range[i], i * allocSize, allocSize);
                    rv = okl4_range_allocator_alloc(alloc, &range[i]);

                    // If it failed, maybe there's no space left? Try named again with size 1
                    if (rv != OKL4_OK)
                    {
                        allocSize = 1;
                        okl4_range_item_setrange(&range[i], i * allocSize, allocSize);
                        rv = okl4_range_allocator_alloc(alloc, &range[i]);
                    }

                    // Do a check.
                    if (rv == OKL4_OK)
                    {
                        base = okl4_range_item_getbase(&range[i]);
                        size = okl4_range_item_getsize(&range[i]);

                        fail_unless(size == allocSize, "allocated range is the wrong size");
                        fail_unless(base + size == allocSize * (i + 1), "allocated range has the wrong end value");
                        fail_unless(base == allocSize * i, "allocated range has the wrong start value");

                        // If it worked, raise! Move on.
                        allocSize *= 2;
                        continue;
                    }

                    // If this still failed then the requested location is really just occupied
                    // Give up trying named allocation here.
                }

                // Otherwise just allocate an anonymous range
                okl4_range_item_setsize(&range[i], allocSize);
                rv = okl4_range_allocator_alloc(alloc, &range[i]);

                // If it failed, maybe there's no space left? Try anonymous again with size 1
                if (rv != OKL4_OK)
                {
                    allocSize = 1;
                    okl4_range_item_setsize(&range[i], allocSize);
                    rv = okl4_range_allocator_alloc(alloc, &range[i]);
                }

                fail_unless(rv == OKL4_OK,"failed to allocate range");

                base = okl4_range_item_getbase(&range[i]);
                size = okl4_range_item_getsize(&range[i]);

                fail_unless(size == allocSize, "allocated range is the wrong size");
                fail_unless(base + size <= ALLOC_SIZE, "allocated range overlaps upper bound");
            }
        }

        switch (iterations)
        {
            case 3:
                deallocount = 1;
                break;
            case 2:
                deallocount = 2;
                break;
            default:
                deallocount = 3;
        }

        for (i = 0; i < MAX_RANGES / 3; i++)
        {
            for (j = 0; j < deallocount; j++)
            {
                okl4_range_allocator_free(alloc, &range[3*i + j]);
            }
        }

        iterations--;
    }

    okl4_range_allocator_destroy(alloc);
    free(alloc);
}
END_TEST

TCase *
make_range_allocator_tcase(void)
{
    TCase *tc;

    tc = tcase_create("Range Allocator");
    tcase_add_test(tc, RANGEALLOC0100);
    tcase_add_test(tc, RANGEALLOC0101);
    tcase_add_test(tc, RANGEALLOC0102);
    tcase_add_test(tc, RANGEALLOC0103);
    tcase_add_test(tc, RANGEALLOC0200);
    tcase_add_test(tc, RANGEALLOC0201);
    tcase_add_test(tc, RANGEALLOC0202);
    tcase_add_test(tc, RANGEALLOC0203);
    tcase_add_test(tc, RANGEALLOC0300);
    tcase_add_test(tc, RANGEALLOC0301);
    tcase_add_test(tc, RANGEALLOC0302);
    tcase_add_test(tc, RANGEALLOC0400);
    tcase_add_test(tc, RANGEALLOC0401);
    tcase_add_test(tc, RANGEALLOC0402);
#ifdef BAD_ASSERTION_CHECKS
    tcase_add_test(tc, RANGEALLOC0500);
#endif
    tcase_add_test(tc, RANGEALLOC0501);
    tcase_add_test(tc, RANGEALLOC0600);
    tcase_add_test(tc, RANGEALLOC0601);
    tcase_add_test(tc, RANGEALLOC0602);
    tcase_add_test(tc, RANGEALLOC0603);
    tcase_add_test(tc, RANGEALLOC0604);
    tcase_add_test(tc, RANGEALLOC0605);
    tcase_add_test(tc, RANGEALLOC0606);
    tcase_add_test(tc, RANGEALLOC0607);
    tcase_add_test(tc, RANGEALLOC0608);
    return tc;
}
