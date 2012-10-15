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

#include <okl4/barrier.h>

#include "../helpers.h"

/*
 * Simple single-threaded barrier.
 */
START_TEST(BARRIER0100)
{
    int i;
    int error;
    okl4_barrier_attr_t attr;
    okl4_barrier_t barrier;

    /* Create the barrier. */
    okl4_barrier_attr_init(&attr);
    okl4_barrier_attr_setnumthreads(&attr, 1);
    error = okl4_barrier_create(&barrier, &attr);
    assert(!error);

    /* Should be able to wait on the barrier. */
    for (i = 0; i < 10; i++) {
        okl4_barrier_wait(&barrier);
    }

    /* Happy. */
    okl4_barrier_delete(&barrier);
}
END_TEST

/*
 * Multiple threads and a single barrier. Ensure that threads can not
 * get too far ahead of each other.
 */

okl4_barrier_t test0200_barrier;
volatile int test0200_counter;

static void
test0200_child(okl4_word_t id, okl4_kcap_t *peers, void *args)
{
    int i;

    /* Setup the global counter. */
    if (id == 0) {
        test0200_counter = 0;
    }

    /* Wait for other threads to start. */
    okl4_barrier_wait(&test0200_barrier);
    for (i = 0; i < 100; i++) {
        /* Ensure we are not too far ahead/behind. */
        assert(test0200_counter - i <= 1);
        assert(test0200_counter - i >= -1);

        /* The first thread should increase the counter. */
        if (id == 0) {
            test0200_counter++;
        }

        /* Wait for everyone else to catch up. */
        okl4_barrier_wait(&test0200_barrier);
    }
}

START_TEST(BARRIER0200)
{
    int error;
    okl4_barrier_attr_t attr;

    /* Create the barriers. */
    okl4_barrier_attr_init(&attr);
    okl4_barrier_attr_setnumthreads(&attr, 10);
    error = okl4_barrier_create(&test0200_barrier, &attr);
    assert(!error);

    /* Start the threads. */
    multithreaded_test(10, test0200_child, NULL);

    /* Clean up. */
    okl4_barrier_delete(&test0200_barrier);
}
END_TEST

TCase *
make_barrier_tcase(void)
{
    TCase * tc;

    tc = tcase_create("Barrier");
    tcase_add_test(tc, BARRIER0100);
    tcase_add_test(tc, BARRIER0200);

    return tc;
}

