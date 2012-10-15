/*
 * Copyright (c) 2002, 2003, Karlsruhe University
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Copyright (c) 2007 Open Kernel Labs, Inc. (Copyright Holder).
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
 * Description:   Kernel Memory Manager
 */
#ifndef __KMEMORY_H__
#define __KMEMORY_H__

#include <debug.h>
#include <sync.h>
#include <kernel/generic/lib.h>

#if defined(CONFIG_KMEM_TRACE)
#include <kdb/macro_set.h>
class kmem_group_t
{
public:
    word_t      mem;
    const char *      name;
};

extern const char* __kmem_group_names[];

#else
typedef word_t kmem_group_t;
#endif

#define KMEM_CHUNKSIZE  (1024)

class kmem_t
{
    word_t *kmem_free_list;
    word_t free_chunks;

    void free (void * address, word_t size);
    void * alloc (word_t size, bool zeroed);
    void * alloc_aligned (word_t size, word_t alignement, word_t mask, bool zeroed);
#if defined(CONFIG_KMEM_DEBUG)
    void check_free(void * address, word_t size);
#endif

public:
    void init (void * start, void * end);
    void free (kmem_group_t * const group, void * address, word_t size);
    void * alloc (kmem_group_t * const group, word_t size, bool zeroed);
    void * alloc_aligned (kmem_group_t * const group, word_t size, word_t alignment,
                          word_t mask, bool zeroed);

    void add (void * address, word_t size)
        { free (address, size); }

    word_t chunks_left(void)
        { return free_chunks; }
    friend class kdb_t;
};


#if defined(CONFIG_KMEM_TRACE)

INLINE void * kmem_t::alloc (kmem_group_t * const group, word_t size, bool zeroed)
{
    void * ret = alloc (size, zeroed);
    if (ret)
        group->mem += max(size, (word_t)KMEM_CHUNKSIZE);
    return ret;
}

INLINE void * kmem_t::alloc_aligned (kmem_group_t * const group, word_t size,
                                     word_t alignment, word_t mask, bool zeroed)
{
    void * ret = alloc_aligned (size, alignment, mask, zeroed);
    if (ret)
        group->mem += max(size, (word_t)KMEM_CHUNKSIZE);
    return ret;
}

INLINE void kmem_t::free (kmem_group_t * const group, void * address, word_t size)
{
    ASSERT (NORMAL, group->mem >= size);
    group->mem -= max(size, (word_t)KMEM_CHUNKSIZE);
    free (address, size);
}

#else /* !CONFIG_KMEM_TRACE */

INLINE void * kmem_t::alloc (kmem_group_t * const group, word_t size, bool zeroed)
{
    return alloc (size, zeroed);
}

INLINE void * kmem_t::alloc_aligned (kmem_group_t * const group, word_t size,
                                     word_t alignment, word_t mask, bool zeroed)
{
    return alloc_aligned (size, alignment, mask, zeroed);
}

INLINE void kmem_t::free (kmem_group_t * const group, void * address, word_t size)
{
    free (address, size);
}

#endif

#if defined(CONFIG_KMEM_DEBUG)
/* Borrow the values used in Linux */
#define KMEM_POISON_ALLOC               0x5a
#define KMEM_POISON_FREE                0x6b
void poison_area(void * addr, word_t size, unsigned char poison_value);
void check_poisoned_area(void * addr, word_t size,
                         unsigned char poison_value, void *obj_addr);
#endif



#endif /* !__KMEMORY_H__ */
