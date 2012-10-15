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

#include <okl4/types.h>
#include <okl4/bitmap.h>
#include <okl4/allocator_attr.h>
#include "bitmap_internal.h"

static int anonymous_allocation(okl4_bitmap_allocator_t * allocator,
        okl4_bitmap_item_t * item);

static int named_allocation(okl4_bitmap_allocator_t * allocator,
        okl4_bitmap_item_t * item);

int
okl4_bitmap_allocator_alloc(okl4_bitmap_allocator_t * allocator,
        okl4_bitmap_item_t * item)
{
    assert(allocator != NULL);
    assert(item != NULL);

    if (item->unit != OKL4_ALLOC_ANONYMOUS) {
        return named_allocation(allocator, item);
    }
    else {
        return anonymous_allocation(allocator, item);
    }
}

/* Request any unit. */
static int
anonymous_allocation(okl4_bitmap_allocator_t * allocator,
        okl4_bitmap_item_t * item)
{
    okl4_word_t i;
    int pos;
    okl4_word_t bitmap_words = (allocator->size + OKL4_WORD_T_BIT - 1) / OKL4_WORD_T_BIT;

    /* Search for a free unit. */
    for (i = allocator->pos_guess; i < bitmap_words; i++) {
        if (allocator->data[i] != ~(okl4_word_t)0) {
            goto found;
        }
    }
    /* Other possible locations for availble units. */
    for (i = 0; i < allocator->pos_guess; i++) {
        if (allocator->data[i] != ~(okl4_word_t)0) {
            goto found;
        }
    }

    return OKL4_ALLOC_EXHAUSTED;

found:

    /* Update pos_guess to point to a likely location for free units. */
    allocator->pos_guess = i;

    pos = find_first_free(allocator->data[i]);
    assert(pos >= 0);

    /* Allocate and return. */
    OKL4_SET_BIT(allocator->data[i], pos);
    item->unit = (i * OKL4_WORD_T_BIT) + (okl4_word_t)pos + allocator->base;

    return OKL4_OK;
}

/* Request for a specific unit. */
static int
named_allocation(okl4_bitmap_allocator_t * allocator,
        okl4_bitmap_item_t * item)
{
    okl4_word_t unit, unit_word, unit_bit;

    /* Check that the requested unit is within range. */
    if (item->unit < allocator->base ||
            item->unit >= allocator->base + allocator->size) {
        return OKL4_OUT_OF_RANGE;
    }

    /* Make adjustment for the fact that the first element in the bitmap is
     * always at position zero, not allocator->base */
    unit = item->unit - allocator->base;
    unit_word = unit / OKL4_WORD_T_BIT;
    unit_bit  = unit % OKL4_WORD_T_BIT;

    /* Check if requested unit has already been allocated. */
    if (OKL4_TEST_BIT(allocator->data[unit_word], unit_bit)) {
        return OKL4_IN_USE;
    }

    /* Allocate. */
    OKL4_SET_BIT(allocator->data[unit_word], unit_bit);

    return OKL4_OK;
}


