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

#include <okl4/semaphore.h>

#include "../helpers.h"

/*
 * Attempt to acquire and release a semaphore.
 */
START_TEST(SEMAPHORE0100)
{
    okl4_semaphore_attr_t attr;
    okl4_semaphore_t sem;
    int error;

    /* Create the semaphore. */
    okl4_semaphore_attr_init(&attr);
    okl4_semaphore_attr_setinitvalue(&attr, 3);
    error = okl4_semaphore_create(&sem, &attr);
    assert(!error);

    /* Try increasing and decreasing a few times. */
    okl4_semaphore_down(&sem);
    okl4_semaphore_down(&sem);
    error = okl4_semaphore_trydown(&sem);
    assert(!error);
    error = okl4_semaphore_trydown(&sem);
    assert(error);
    okl4_semaphore_up(&sem);
    error = okl4_semaphore_trydown(&sem);
    assert(!error);

    /* Happy. */
    okl4_semaphore_delete(&sem);
}
END_TEST

/*
 * Two threads, A and B, with two semaphores, X and Y. A attempts to
 * increase X and decrease Y, and B attempts to increase Y and decrease
 * X. Both should (eventually) succeed.
 */
okl4_semaphore_t test0200_x;
okl4_semaphore_t test0200_y;

static void
test0200_child(okl4_word_t id, okl4_kcap_t *peers, void *args)
{
    int i;
    okl4_semaphore_t *a = (id == 0 ? &test0200_x : &test0200_y);
    okl4_semaphore_t *b = (id == 0 ? &test0200_y : &test0200_x);

    for (i = 0; i < 1000; i++) {
        okl4_semaphore_down(b);
        okl4_semaphore_up(a);
    }
}

START_TEST(SEMAPHORE0200)
{
    int error;
    okl4_semaphore_attr_t attr;

    /* Create the semaphores. */
    okl4_semaphore_attr_init(&attr);
    okl4_semaphore_attr_setinitvalue(&attr, 0);
    error = okl4_semaphore_create(&test0200_x, &attr);
    assert(!error);

    okl4_semaphore_attr_setinitvalue(&attr, 1);
    error = okl4_semaphore_create(&test0200_y, &attr);
    assert(!error);

    /* Start the threads. */
    multithreaded_test(2, test0200_child, NULL);

    /* Clean up. */
    okl4_semaphore_delete(&test0200_x);
    okl4_semaphore_delete(&test0200_y);
}
END_TEST

/*
 * Multiple threads, using semaphores like a mutex.
 */
okl4_semaphore_t test0300_sem;
volatile okl4_word_t test0300_shared;

static void
test0300_child(okl4_word_t id, okl4_kcap_t *peers, void *args)
{
    int i;

    for (i = 0; i < 300; i++) {
        okl4_semaphore_down(&test0300_sem);
        test0300_shared++;
        test0300_shared--;
        okl4_semaphore_up(&test0300_sem);
    }
}

START_TEST(SEMAPHORE0300)
{
    int error;
    okl4_semaphore_attr_t attr;

    /* Create the semaphores. */
    okl4_semaphore_attr_init(&attr);
    okl4_semaphore_attr_setinitvalue(&attr, 1);
    error = okl4_semaphore_create(&test0300_sem, &attr);
    assert(!error);

    /* Start the threads. */
    test0300_shared = 0;
    multithreaded_test(10, test0300_child, NULL);
    fail_unless(test0300_shared == 0, "Shared memory corrupted.");

    /* Clean up. */
    okl4_semaphore_delete(&test0300_sem);
}
END_TEST

TCase *
make_semaphore_tcase(void)
{
    TCase * tc;

    tc = tcase_create("Semaphore");
    tcase_add_test(tc, SEMAPHORE0100);
    tcase_add_test(tc, SEMAPHORE0200);
    tcase_add_test(tc, SEMAPHORE0300);

    return tc;
}

