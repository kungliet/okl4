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
 * Description:   Support functions for ARM platforms.
 */

#include <stdarg.h>     /* for va_list, ... comes with gcc */
#include <l4.h>
#include <soc/interface.h>
#include <schedule.h>
#include <debug.h>
#include <space.h>
#include <arch/pgent.h>
#include <arch/page.h>


int do_printf(const char* format_p, va_list args);

extern "C" void kernel_enter_kdebug(const char* s)
{
    enter_kdebug(s);
}

extern "C" void kernel_fatal_error(const char* s)
{
#if defined(CONFIG_KDB_CONS)
    enter_kdebug(s);
#endif
    panic(s);
}

#if defined(CONFIG_KDB_CONS)
extern "C" int kernel_printf(const char * format, ...)
{
    va_list args;
    int i;

    va_start(args, format);
    i = do_printf(format, args);
    va_end(args);
    return i;
}
#endif

#define MAX_IO_AREAS 16


bitmap_t mappings = 0;

#define ROUND_SIZE(sz, pgsz, fsz, failure) do {  \
    if (sz <= PAGE_SIZE_4K) {                 \
        pgsz = pgent_t::size_4k;              \
        fsz  = PAGE_SIZE_4K;                  \
    } else if (sz <= PAGE_SIZE_1M) {          \
        pgsz = pgent_t::size_1m;              \
        fsz = PAGE_SIZE_1M;                   \
    } else {                                  \
        fsz = failure;                        \
    }                                         \
} while(0)


extern "C" addr_t
kernel_add_mapping(word_t size,
                   addr_t phys,
                   word_t cache_attr)
{
    space_t           *space = get_kernel_space();
    addr_t             base;
    pgent_t::pgsize_e  pgsize = pgent_t::size_4k; // Avoiding flint warning
    word_t             final_size;
    word_t             i;

    /*
     * This is very simple (and wasteful of virtual addr space.
     */
    ROUND_SIZE(size, pgsize, final_size, 0);

    if (final_size == 0) {
        return NULL;
    }

    for (i = 0 ; i < MAX_IO_AREAS ; i++) {
        if (!bitmap_isset(&mappings, i))
            {
                break;
            }
    }

    if (i == MAX_IO_AREAS) {
        return NULL; // Out of spaces
    }

    base = (addr_t)(IO_AREA_START + ARM_SECTION_SIZE * i);

    bitmap_set(&mappings, i);

    kmem_resource_t *kresource = get_kernel_space()->get_kmem_resource();

    if (space->add_mapping(base, phys, pgsize, space_t::read_write_ex, true,
                           (memattrib_e)cache_attr, kresource)) {
    } else {
        base = (addr_t)0;
    }
    return base;
}

extern "C" bool
kernel_add_mapping_va(addr_t virt,
                      word_t size,
                      addr_t phys,
                      word_t cache_attr)
{
    space_t           *space = get_kernel_space();
    word_t             end = (word_t)virt + size;
    pgent_t::pgsize_e  pgsize = pgent_t::size_4k; // Avoiding flint warning
    word_t             area;
    word_t             final_size;

    /* The address needs to sit within the IO address areas */
    if (!((word_t)virt >= IO_AREA_START && end < IO_AREA_END)) {
        return false;
    }

    ROUND_SIZE(size, pgsize, final_size, 0);

    if (final_size == 0) {
        return false;
    }

    /*
     * Find where in the IO space they want - is it free?
     */
    area = ((word_t)virt - IO_AREA_START) / ARM_SECTION_SIZE;
    if (bitmap_isset(&mappings, area)) {
        return false;
    }

    kmem_resource_t *kresource = get_kernel_space()->get_kmem_resource();

    if (space->add_mapping(virt, phys, pgsize, space_t::read_write_ex, true,
                           (memattrib_e)cache_attr,
                           kresource)) {
        bitmap_set(&mappings, area);
        return true;
    } else {
        return false;
    }
}

