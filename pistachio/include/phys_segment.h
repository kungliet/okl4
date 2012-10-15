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
/*
 * Description:  Physical Segments and Segments Table
 */
#ifndef __PHYS_SEGMENT_H__
#define __PHYS_SEGMENT_H__

#include <arch/pgent.h>

#include <arch/phys_segment.h>

extern const word_t hw_pgshifts[];

#define MIN_PAGE_SHIFT  hw_pgshifts[pgent_t::size_min]

class phys_segment_t
{
public:
    inline bool attribs_allowed(memattrib_e attr)
    {
        word_t physattr;
        physattr = to_physattrib(attr);

        return (this->base.attribs & physattr) == physattr;
    }

    /**
     * Check if a region of memory is inside the segment.
     * addr_offset + map_size is not included in the region to be checked.
     * @param addr_offset   the start of the region to be checked as an
     *                      offset from the beginning of the segment.
     * @param map_size      The size of the region to be checked.
     * @return              true if the region is fully contained in the segment,
     *                      false otherwise.
     */
    inline bool is_contained(word_t addr_offset, word_t map_size)
    {
        word_t offset = addr_offset >> MIN_PAGE_SHIFT;
        word_t size = map_size >> MIN_PAGE_SHIFT;

        if ((offset + size) <= this->size.size) {
            return true;
        } else {
            return false;
        }
    }

    inline void set_base(u64_t offset)
    {
        base.base = offset >> MIN_PAGE_SHIFT;
    }

    inline void set_size(u64_t s)
    {
        size.size = s >> MIN_PAGE_SHIFT;
    }

    inline u64_t get_base()
    {
        return (u64_t)base.base << MIN_PAGE_SHIFT;
    }

    inline u64_t get_size()
    {
        return (u64_t)size.size << MIN_PAGE_SHIFT;
    }

    inline void set_rwx(word_t rwx)
    {
        size.rwx = rwx;
    }

    inline word_t get_rwx()
    {
        return size.rwx;
    }

    inline void set_attribs(word_t attr)
    {
        base.attribs = attr;
    }

    inline word_t get_attribs()
    {
        return base.attribs;
    }

private:
    union {
        struct {
            BITFIELD2(word_t,
                    attribs     : 8,
                    base        : BITS_WORD - 8);
        } base;
        word_t raw_base;
    };
    union {
        struct {
            BITFIELD3(word_t,
                    rwx         : 4,
                    _res        : 6,
                    size        : BITS_WORD - 10);
        } size;
        word_t raw_size;
    };
};

class segment_list_t
{
public:
    inline bool valid_segment(word_t id)
    {
        return id < num_segments;
    }

    inline phys_segment_t *lookup_segment(word_t id)
    {
        if (!valid_segment(id)) {
            return NULL;
        }
        return &segments[id];
    }

    inline word_t get_num_segments(void)
    {
        return num_segments;
    }

    /* XXX: This will be removed soon */
    inline void set_num_segments(word_t n)
    {
        num_segments = n;
    }
private:
    word_t num_segments;
public:
    phys_segment_t segments[1];
};

#undef MIN_PAGE_SHIFT

#endif /* __PHYS_SEGMENT_H__ */
