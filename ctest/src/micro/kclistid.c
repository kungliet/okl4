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
#include <okl4/kclistid_pool.h>
#include <okl4/env.h>

#include <stdlib.h>

OKL4_KCLISTID_POOL_DECLARE_OFRANGE(clistid_pool, 3, 50);

/* Allocate a Clist ID */

START_TEST(KCLISTID0100)
{
    int ret;
    okl4_kclistid_t id;
    okl4_allocator_attr_t attr;

    okl4_allocator_attr_init(&attr);
    okl4_allocator_attr_setrange(&attr, 3, 50);
    okl4_kclistid_pool_init(clistid_pool, &attr);

    okl4_kclistid_getattribute(clistid_pool, &attr);
    fail_unless(attr.base == 3, "okl4_kclistid_getattribute returned wrong base");
    fail_unless(attr.size == 50, "okl4_kclistid_getattribute returned wrong size");

    ret = okl4_kclistid_allocany(clistid_pool, &id);
    fail_unless(ret == OKL4_OK, "Failed to allocate any clist id.");

}
END_TEST

START_TEST(KCLISTID0200)
{
    int ret;
    okl4_kclistid_t id;

    id.clist_no = 10;

    ret = okl4_kclistid_isallocated(clistid_pool, id);
    fail_unless(ret == 0, "okl4_clistid_isallocated returned wrong value");

    ret = okl4_kclistid_allocunit(clistid_pool, &id);
    fail_unless(ret == OKL4_OK, "Failed to allocate the specified clist id.");
    fail_unless(id.clist_no == 10, "Didn't allocate the right id");

    ret = okl4_kclistid_isallocated(clistid_pool, id);
    fail_unless(ret == 1, "okl4_clistid_isallocated returned wrong value");
}
END_TEST

START_TEST(KCLISTID0300)
{
    int ret;
    okl4_kclistid_t id;

    id.clist_no = 81;

    ret = okl4_kclistid_allocunit(clistid_pool, &id);
    fail_unless(ret != OKL4_OK, "Wrongly allocated a clist id from outside the pool.");

}
END_TEST

START_TEST(KCLISTID0400)
{
    int ret;
    okl4_kclistid_t id;

    id.clist_no = 5;
    ret = okl4_kclistid_allocunit(clistid_pool, &id);
    ret = okl4_kclistid_allocunit(clistid_pool, &id);
    fail_unless(ret != OKL4_OK,"Reallocated the same clist id again");

}
END_TEST

START_TEST(KCLISTID0500)
{
    int ret;
    okl4_kclistid_t id;

    id.clist_no = 7;
    ret = okl4_kclistid_allocunit(clistid_pool, &id);
    fail_unless(ret == OKL4_OK, "Could not allocate the id");
    okl4_kclistid_free(clistid_pool, id);
    ret = okl4_kclistid_allocunit(clistid_pool, &id);
    fail_unless(ret == OKL4_OK, "Could not free an allocated unit.");
}
END_TEST

/**
 * Weaved allocation of the kclist id allocator
 */
START_TEST(KCLISTID0600)
{
    int ret;
    okl4_kclistid_t id;
    okl4_kclistid_pool_t *kclistid_pool = NULL;
    kclistid_pool = okl4_env_get("MAIN_CLIST_ID_POOL");
    assert(kclistid_pool != NULL);
    fail_unless(kclistid_pool != NULL, "couldn't find the weaved kclist id pool");

    ret = okl4_kclistid_allocany(kclistid_pool, &id);
    fail_unless(ret == OKL4_OK, "Failed to allocate any clist id.");

    okl4_kclistid_free(kclistid_pool, id);
}
END_TEST

TCase *
make_kclist_id_tcase(void)
{
    TCase *tc;
    tc = tcase_create("kclistid");
    tcase_add_test(tc, KCLISTID0100);
    tcase_add_test(tc, KCLISTID0200);
    tcase_add_test(tc, KCLISTID0300);
    tcase_add_test(tc, KCLISTID0400);
    tcase_add_test(tc, KCLISTID0500);
    tcase_add_test(tc, KCLISTID0600);

    return tc;
}

