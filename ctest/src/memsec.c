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

#include <stdio.h>
#include <ctest/ctest.h>
#include <okl4/memsec.h>
#include <okl4/static_memsec.h>

#define PAGE_FAULT_OFFSET   41

static int memsec0100_state;

static int
memsec0100_lookup_callback(okl4_memsec_t *memsec, okl4_word_t vaddr,
        okl4_physmem_item_t *map_item, okl4_word_t *dest_vaddr)
{
    memsec0100_state = 1;
    return 1;
}

static int
memsec0100_map_callback(okl4_memsec_t *memsec, okl4_word_t vaddr,
        okl4_physmem_item_t *map_item, okl4_word_t *dest_vaddr)
{
    memsec0100_state = 2;
    return 2;
}

static void
memsec0100_destroy_callback(okl4_memsec_t *memsec)
{
    memsec0100_state = 3;
}

/*
 * Create a custom memsec, and ensure callbacks are correctly used.
 */
START_TEST(MEMSEC0100)
{
    okl4_memsec_t ms;
    okl4_memsec_attr_t attr;
    okl4_virtmem_item_t virt;
    int ret;
    okl4_word_t dummy;
    okl4_physmem_item_t dummy_item;

    /* Create a virtmem item. */
    okl4_range_item_setrange(&virt, 0x10000000, 0x20000);

    /* Create a memsec. */
    okl4_memsec_attr_init(&attr);
    okl4_memsec_attr_setpremapcallback(&attr, memsec0100_lookup_callback);
    okl4_memsec_attr_setaccesscallback(&attr, memsec0100_map_callback);
    okl4_memsec_attr_setdestroycallback(&attr, memsec0100_destroy_callback);
    okl4_memsec_attr_setrange(&attr, virt);
    okl4_memsec_init(&ms, &attr);

    /* Ensure the callbacks are correct. */
    memsec0100_state = 0;
    ret = okl4_memsec_premap(&ms, 0,
            &dummy_item, &dummy, &dummy, &dummy, &dummy);
    fail_unless(memsec0100_state == 1, "Callback did not get called.");
    fail_unless(ret == 1, "Invalid return code");

    ret = okl4_memsec_access(&ms, 0,
            &dummy_item, &dummy, &dummy, &dummy, &dummy);
    fail_unless(memsec0100_state == 2, "Callback did not get called.");
    fail_unless(ret == 2, "Invalid return code");

    okl4_memsec_destroy(&ms);
    fail_unless(memsec0100_state == 3, "Callback did not get called.");
}
END_TEST


/* Ensure that the results of a map/lookup memsec callback are correct. */
static void memsec0200_ensure_correct_map_result(int error_code,
        okl4_virtmem_item_t *virtmem_item,
        okl4_physmem_item_t *expected,
        okl4_physmem_item_t *actual,
        okl4_word_t output_vaddr)
{
    fail_unless(error_code == OKL4_OK, "map call failed");
    fail_unless(okl4_physmem_item_getoffset(expected) ==
            okl4_physmem_item_getoffset(actual),
            "physmem item offsets don't match");
    fail_unless(okl4_physmem_item_getsize(expected) ==
            okl4_physmem_item_getsize(actual),
            "physmem item sizes don't match");
    fail_unless(output_vaddr == okl4_range_item_getbase(virtmem_item),
            "dest_vaddr is incorrect");
}

/*
 * Create a statically mapped memsec, and ensure that correct results are
 * returned.
 */
START_TEST(MEMSEC0200)
{
    struct okl4_memsec *ms;
    struct okl4_static_memsec static_ms;
    okl4_static_memsec_attr_t attr;
    okl4_virtmem_item_t virt;
    okl4_physmem_item_t phys;
    okl4_physmem_item_t phys_result;
    okl4_word_t dest_vaddr;
    okl4_word_t page_size;
    okl4_word_t perms;
    okl4_word_t attributes;
    int ret;

    /* Create a virtmem item. */
    okl4_range_item_setrange(&virt, 0x10000000, 0x20000);

    /* Create a physseg item. */
    okl4_range_item_setrange(&phys.range, 0x20000000, 0x20000);

    /* Create a static memsec. */
    okl4_static_memsec_attr_init(&attr);
    okl4_static_memsec_attr_setrange(&attr, virt);
    okl4_static_memsec_attr_settarget(&attr, phys);
    okl4_static_memsec_attr_setperms(&attr, OKL4_PERMS_RX);
    okl4_static_memsec_attr_setattributes(&attr, OKL4_DEFAULT_MEM_ATTRIBUTES);
    okl4_static_memsec_init(&static_ms, &attr);
    ms = okl4_static_memsec_getmemsec(&static_ms);

    /* Perform a test map. */
    ret = okl4_memsec_access(ms, okl4_range_item_getbase(&virt) +
            PAGE_FAULT_OFFSET, &phys_result, &dest_vaddr, &page_size, &perms,
            &attributes);
    memsec0200_ensure_correct_map_result(ret, &virt, &phys,
            &phys_result, dest_vaddr);

    fail_unless(perms == OKL4_PERMS_RX, "Incorrect permissions");
    fail_unless(attributes == OKL4_DEFAULT_MEM_ATTRIBUTES, "Incorrect attributes");

    /* Perform a test lookup. */
    ret = okl4_memsec_premap(ms, okl4_range_item_getbase(&virt) +
            PAGE_FAULT_OFFSET, &phys_result, &dest_vaddr, &page_size, &perms,
            &attributes);
    memsec0200_ensure_correct_map_result(ret, &virt, &phys,
            &phys_result, dest_vaddr);
    fail_unless(perms == OKL4_PERMS_RX, "Incorrect permissions");
    fail_unless(attributes == OKL4_DEFAULT_MEM_ATTRIBUTES, "Incorrect attributes");

    /* Destroy items. */
    okl4_memsec_destroy(ms);
}
END_TEST

TCase *
make_memsec_tcase(void)
{
    TCase * tc;

    tc = tcase_create("Memsections");
    tcase_add_test(tc, MEMSEC0100);
    tcase_add_test(tc, MEMSEC0200);

    return tc;
}
