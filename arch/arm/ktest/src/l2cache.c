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
#include <ktest/ktest.h>
#include <ktest/utility.h>
#include <stddef.h>
#include <l4/kdebug.h>
#include <l4/config.h>
#include <l4/map.h>
#include <l4/space.h>
#include <l4/cache.h>
#include <okl4/env.h>
#include <assert.h>

#if defined(MACHINE_KZM_ARM11) && !defined(RUN_ON_SIMULATOR)

#define TEST_VALUE1 0xdeadbeef
#define TEST_VALUE2 0x12345678
#define TEST_VALUE3 0x87654321

START_TEST(ARM_OUTER_CACHE_0001)
{
    okl4_env_segment_t *uncached_region, *cached_region;
    L4_Word_t uncached_virt, cached_virt, phys;
    L4_MapItem_t map;

    uncached_region = okl4_env_get_segment("MAIN_OUTER_CACHE_TEST_UNCACHED_SEGMENT");
    cached_region = okl4_env_get_segment("MAIN_OUTER_CACHE_TEST_VIRT_SEGMENT");

    assert(uncached_region != NULL);
    assert(cached_region != NULL);

    uncached_virt = uncached_region->virt_addr;
    cached_virt = cached_region->virt_addr;

    phys = *(L4_Word_t*)okl4_env_get("OUTER_CACHE_TEST_UNCACHED_PHYSICAL");

    /* Map cached_region to L1 uncacheable, L2 cachable Write-back, Write-allocate */
    L4_MapItem_Map(&map, uncached_region->segment, 0, cached_virt, 12, CACHE_ATTRIB_CUSTOM(CACHE_ATTRIB_NC, CACHE_ATTRIB_WBa), L4_Readable | L4_Writable);
    if (L4_ProcessMapItem(KTEST_SPACE, map) == 0)
    {
        assert(!"Should not come here");
    }

    /* Make sure that no old data in L2, this is not really necessary. */
    L4_Outer_CacheFlushAll();

    /* Write value 1 to L2 cachable area, should change value in L2 cache line. */
    *((L4_Word_t *)cached_virt) = TEST_VALUE1;
#if 0
    printf("write to cached area [0x%lx] = 0xdeadbeef and read back 0x%lx, read uncached area get 0x%lx\n", cached_virt, *((L4_Word_t *)cached_virt), *((L4_Word_t *)uncached_virt));
#else
    assert(*((L4_Word_t *)cached_virt) == TEST_VALUE1);
#endif

    /* Write value 2 to uncached area, bypass L2. */
    *((L4_Word_t *)uncached_virt) = TEST_VALUE2;
#if 0
    printf("write to uncached area [0x%lx] = 0x12345678 and read back 0x%lx, read cached area get 0x%lx\n", uncached_virt, *((L4_Word_t *)uncached_virt), *((L4_Word_t *)cached_virt));
#else
    assert(*((L4_Word_t *)uncached_virt) == TEST_VALUE2);
    assert(*((L4_Word_t *)cached_virt) == TEST_VALUE1);
#endif
    /* Now flush Outer Cache, this should clean L2 cache line and update
     * physical memory with L2 cached value (value1).
     */
    L4_Outer_CacheFlushRange(phys, (phys + (1 << 12)));
#if 0
    printf("Afte flush Outer, read cached area get 0x%lx, read uncached area get 0x%lx\n", *((L4_Word_t *)cached_virt), *((L4_Word_t *)uncached_virt));
#else
    fail_unless(*((L4_Word_t *)uncached_virt) == *((L4_Word_t *)cached_virt), "Flush outer cache doesn't make physical memory consistant with outer cache!");
#endif

}
END_TEST
#endif

TCase * arm_outer_cache(TCase *tc);

TCase * arm_outer_cache(TCase *tc)
{
#if defined(MACHINE_KZM_ARM11) && !defined(RUN_ON_SIMULATOR)
    tcase_add_test(tc, ARM_OUTER_CACHE_0001);
#endif
    return tc;
}
