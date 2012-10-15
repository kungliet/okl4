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

#include <okl4/mutex.h>

#include "helpers.h"

/*
 * Simple single-threaded mutex lock/unlock.
 */
START_TEST(MUTEX0100)
{
    int error;
    okl4_mutex_attr_t attr;
    okl4_mutex_t mutex;

    /* Create a mutex. */
    okl4_mutex_attr_init(&attr);
    error = okl4_mutex_create(&mutex, &attr);
    assert(!error);

    /* Lock it. */
    okl4_mutex_lock(&mutex);

    /* Attempt to lock it again, failing. */
    error = okl4_mutex_trylock(&mutex);
    assert(error);

    /* Unlock it. */
    okl4_mutex_unlock(&mutex);

    /* Trylock it again, this time succeeding. */
    error = okl4_mutex_trylock(&mutex);
    assert(!error);

    /* Unlock and finish. */
    okl4_mutex_unlock(&mutex);
    okl4_mutex_delete(&mutex);
}
END_TEST

/*
 * Multithreaded mutex lock/unlock.
 */

volatile okl4_word_t mutex0200_shared_mem;
okl4_mutex_t mutex0200_lock;

static void
mutex0200_child(okl4_word_t id, okl4_kcap_t *peers, void *args)
{
    int i;

    for (i = 0; i < 10000; i++) {
        /* Progress indicator. */
        if (i % 1024 == 0) {
            printf(".");
        }

        /* Try to lock. */
        okl4_mutex_lock(&mutex0200_lock);

        /* Modify shared memory. */
        mutex0200_shared_mem++;
        mutex0200_shared_mem--;

        /* Unlock. */
        okl4_mutex_unlock(&mutex0200_lock);
    }

    /* Done. */
}

START_TEST(MUTEX0200)
{
    int error;
    okl4_mutex_attr_t attr;

    /* Create a mutex. */
    okl4_mutex_attr_init(&attr);
    error = okl4_mutex_create(&mutex0200_lock, &attr);
    fail_unless(!error, "Could not create mutex.");

    /* Run children threads. */
    mutex0200_shared_mem = 0;
    multithreaded_test(5, mutex0200_child, NULL);
    printf("\n");

    /* Ensure the mutex is free, and delete it. */
    error = okl4_mutex_trylock(&mutex0200_lock);
    fail_unless(!error, "Mutex was left locked.");
    fail_unless(mutex0200_shared_mem == 0, "Shared memory was corrupted.");
    okl4_mutex_unlock(&mutex0200_lock);
    okl4_mutex_delete(&mutex0200_lock);
}
END_TEST

/*
 * Multithreaded counted mutex lock/unlock.
 *
 * Counted locks are only supported under Micro.
 */

volatile okl4_word_t mutex1000_shared_mem;
okl4_mutex_t mutex1000_lock;

static void
mutex1000_child(okl4_word_t id, okl4_kcap_t *peers, void *args)
{
    int i;
    int error;

    for (i = 0; i < 4000; i++) {
        /* Progress indicator. */
        if (i % 1024 == 0) {
            printf(".");
        }

        /* Double lock / unlock. */
        okl4_mutex_lock(&mutex1000_lock);
        mutex1000_shared_mem++;
        okl4_mutex_lock(&mutex1000_lock);
        mutex1000_shared_mem++;
        mutex1000_shared_mem--;
        okl4_mutex_unlock(&mutex1000_lock);
        mutex1000_shared_mem--;
        okl4_mutex_unlock(&mutex1000_lock);

        /* Double try-lock / unlock. */
        okl4_mutex_lock(&mutex1000_lock);
        mutex1000_shared_mem++;
        error = okl4_mutex_trylock(&mutex1000_lock);
        assert(!error);
        mutex1000_shared_mem++;
        mutex1000_shared_mem--;
        okl4_mutex_unlock(&mutex1000_lock);
        mutex1000_shared_mem--;
        okl4_mutex_unlock(&mutex1000_lock);
    }

    /* Done. */
}

START_TEST(MUTEX1000)
{
    int error;
    okl4_mutex_attr_t attr;

    /* Create a mutex. */
    okl4_mutex_attr_init(&attr);
    okl4_mutex_attr_setiscounted(&attr, 1);
    error = okl4_mutex_create(&mutex1000_lock, &attr);
    fail_unless(!error, "Could not create mutex.");

    /* Run children threads. */
    mutex1000_shared_mem = 0;
    multithreaded_test(5, mutex1000_child, NULL);
    printf("\n");

    /* Ensure the mutex is free, and delete it. */
    error = okl4_mutex_trylock(&mutex1000_lock);
    fail_unless(!error, "Mutex was left locked.");
    fail_unless(mutex1000_shared_mem == 0, "Shared memory was corrupted.");
    okl4_mutex_unlock(&mutex1000_lock);
    okl4_mutex_delete(&mutex1000_lock);
}
END_TEST

TCase *
make_mutex_tcase(void)
{
    TCase * tc;

    tc = tcase_create("Mutex");
    tcase_add_test(tc, MUTEX0100);
    tcase_add_test(tc, MUTEX0200);
    tcase_add_test(tc, MUTEX1000);

    return tc;
}

