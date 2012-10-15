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

#include <stdio.h>

#include <okl4/env.h>
#include <okl4/types.h>
#include <okl4/physmem_item.h>

static void env1000_dummy_function(void)
{
    /* Do nothing. */
}

START_TEST(ENV1000)
{
    int error;
    int dummy;
    okl4_physmem_item_t phys;
    okl4_word_t offset, attributes, perms;

    /* Ensure we can lookup the address of the dummy variable. */
    error = okl4_env_get_physseg_page((okl4_word_t)&dummy,
            &phys, &offset, &attributes, &perms);
    fail_unless(error == 0, "Could not look up stack segment.");
    fail_unless(perms == OKL4_PERMS_RW, "Incorrect stack permissions.");
    fail_unless((offset % OKL4_DEFAULT_PAGESIZE)
            == ((okl4_word_t)&dummy % OKL4_DEFAULT_PAGESIZE),
                "Incorrect offset.");

    /* Ensure we can lookup the address of a function. */
    error = okl4_env_get_physseg_page((okl4_word_t)env1000_dummy_function,
            &phys, &offset, &attributes, &perms);
    fail_unless(error == 0, "Could not look up stack segment.");
#if defined (__RVCT__) || defined(__RVCT_GNU__) || defined (__ADS__)
    /* Use a bitmask for rvct because it uses other bits for custom flags */
    fail_unless((perms & 0xf) == OKL4_PERMS_RWX, "Incorrect '.text' permissions.");
#else
    fail_unless(perms == OKL4_PERMS_RX, "Incorrect '.text' permissions.");
#endif
    fail_unless((offset % OKL4_DEFAULT_PAGESIZE)
            == ((okl4_word_t)env1000_dummy_function % OKL4_DEFAULT_PAGESIZE),
                "Incorrect offset.");

}
END_TEST

TCase *
make_env_tcase(void)
{
    TCase * tc;

    tc = tcase_create("Environment");
    tcase_add_test(tc, ENV1000);

    return tc;
}
