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

#include "range_helpers.h"
#include <stdlib.h>
#include <okl4/range.h>
#include <okl4/allocator_attr.h>
#include <stdio.h>

/**
 *  @file
 *  An resource pool allocation strategy for contiguous ranges.
 */

/**
 *  Allocates a range of values from @a allocator.  The start and end
 *  values of the allocation is specified in @a range.  Returns -1 if
 *  any one of the values within the specified range has already been
 *  allocated.  Returns 1 on success.
 */
static int named_allocation(okl4_range_allocator_t * allocator,
        okl4_range_item_t * range);

/**
 *  Allocates a range of values from @a allocator.  The siz of the
 *  allocation is specified in @a range, but the start of the
 *  allocation will be determined by the allocator.  Returns -1 if
 *  there are no possible allocations with the given size.
 *  Returns 1 on success.
 */
static int anonymous_allocation(okl4_range_allocator_t * allocator,
        okl4_range_item_t * range);

/* Implementation */

int okl4_range_allocator_alloc(okl4_range_allocator_t * allocator,
        okl4_range_item_t * range)
{
    assert(allocator != NULL);
    assert(range != NULL);

    if (range_is_anonymous(range)) {
        return anonymous_allocation(allocator, range);
    } else {
        return named_allocation(allocator, range);
    }
}

static int
named_allocation(okl4_range_allocator_t * allocator,
        okl4_range_item_t * range)
{
    okl4_range_item_t * victim;
    okl4_word_t victim_free_size;


    /*
     * We search for the range item (node) that we allocate from, and call
     * that the victim.
     */
    RANGE_LIST_ITERATE(victim, &allocator->head) {
        if (range_free_hasvalue(victim, range->base)) {
            break;
        }
    }

    if (victim == NULL) {
        return OKL4_ALLOC_EXHAUSTED;
    }

    /*
     * Remove the free region that is being allocated out of victim,
     * if it is large enough.
     */
    victim_free_size = victim->base + victim->total_size - range->base;
    if (victim_free_size < range->size) {
        return OKL4_IN_USE;
    }
    range->total_size = victim->base + victim->total_size - range->base;
    victim->total_size -= range->total_size;

    /* Insert range item object into list */
    range->next = victim->next;
    victim->next = range;

    return OKL4_OK;
}

static int
anonymous_allocation(okl4_range_allocator_t * allocator,
        okl4_range_item_t * range)
{
    okl4_word_t range_size = range->size;
    okl4_range_item_t * victim;

    /* Find the first range item that has a sufficient free region */
    RANGE_LIST_ITERATE(victim, &allocator->head) {
        if (range_free_getsize(victim) >= range_size) {
            break;
        }
    }
    if (victim == NULL) {
        return OKL4_IN_USE;
    }

    /*
     * Remove the free region that is being allocated out of victim.
     * Write allocated region to the range item object.  There is no
     * free region.
     */
    victim->total_size -= range_size;
    range->base = victim->base + victim->total_size;
    range->total_size = range_size;

    /* Insert range item into appropriate list */
    range->next = victim->next;
    victim->next = range;

    return OKL4_OK;
}
