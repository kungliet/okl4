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


#include <okl4/memsec.h>
#include <okl4/static_memsec.h>

#define IS_POWER_TWO(x) (((x) & ((x) - 1)) == 0)

static int
lookup(okl4_memsec_t *memsec, okl4_word_t vaddr,
        okl4_physmem_item_t *map_item, okl4_word_t *dest_vaddr)
{
    okl4_static_memsec_t *self = (okl4_static_memsec_t *)memsec;
    *map_item = self->target;
    *dest_vaddr = self->base.super.range.base;
    return OKL4_OK;
}

static void
destroy(okl4_memsec_t *memsec)
{
    /* Nothing to do. */
}

/*
 * Round the interval [base, base + size] outwards to nearest page boundaries.
 */
static void
round_to_pagesizes(okl4_word_t *base, okl4_word_t *size, okl4_word_t page_size)
{
    okl4_word_t excess;
    assert(IS_POWER_TWO(page_size));

    /* Round 'base' down. */
    *size += (*base % page_size);
    *base -= (*base % page_size);

    /* Round 'size' up. */
    excess = (*size % page_size);
    if (excess > 0) {
        *size += (page_size - excess);
    }
}

void
okl4_static_memsec_init(okl4_static_memsec_t *memsec,
        okl4_static_memsec_attr_t *attr)
{
    assert(memsec != NULL);
    assert(attr != NULL);

    if (attr != OKL4_WEAVED_OBJECT) {
        _okl4_memsec_init(&memsec->base);

        /* Copy common properties. */
        memsec->base.page_size = attr->page_size;

        if (attr->segment != NULL) {
            /* If the user provided us with a segment in the attribute,
             * pull values from there. */
            memsec->base.perms = attr->segment->rights;
            memsec->base.attributes = attr->segment->cache_policy;
            memsec->base.super.range.base = attr->segment->virt_addr;
            memsec->base.super.range.size = attr->segment->size;
            round_to_pagesizes(&memsec->base.super.range.base,
                    &memsec->base.super.range.size, memsec->base.page_size);

            /* Setup target. */
            memsec->target.range.size = memsec->base.super.range.size;
            memsec->target.range.base = attr->segment->offset;
            memsec->target.segment_id = attr->segment->segment;
            round_to_pagesizes(&memsec->target.range.base,
                    &memsec->target.range.size, memsec->base.page_size);

        } else {
            /* Otherwise, just use the values provided by the user. */
            memsec->base.perms = attr->perms;
            memsec->base.attributes = attr->attributes;
            memsec->base.super.range = attr->range;
            memsec->target = attr->target;
        }
    }

    /* Setup our own callbacks. */
    memsec->base.premap_callback = lookup;
    memsec->base.access_callback = lookup;
    memsec->base.destroy_callback = destroy;

    /* Ensure mappings are valid. */
    assert(IS_POWER_TWO(memsec->base.page_size));
    assert(memsec->target.range.size == memsec->base.super.range.size);
    assert(memsec->base.super.range.base % memsec->base.page_size == 0);
    assert(memsec->base.super.range.size % memsec->base.page_size == 0);
    assert(memsec->target.range.base % memsec->base.page_size == 0);
    assert(memsec->target.range.size % memsec->base.page_size == 0);
}

