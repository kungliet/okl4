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
 * Thread safety compatability layer.
 *
 * This interface attempts to abstract away differences in locking mechanisms
 * for differnt interfaces.
 */

#ifndef _THREADSAFETY_H_
#define _THREADSAFETY_H_

#if defined(NANOKERNEL)
/* Currently, Nano only uses libokl4 locks. */
#define LIBOKL4_LOCKS
#else
/* Currently, Micro only uses libmutex locks. */
#define LIBMUTEX_LOCKS
#endif

#if defined(THREAD_SAFE) && defined(LIBOKL4_LOCKS)

/*
 * Using 'libokl4' to perform locking operations.
 */

#include <okl4/mutex.h>
#include <assert.h>

/*
 * Setup
 */

/* The data type for a mutex. */
#define OKL4_MUTEX_TYPE  okl4_mutex_t

/* Initialise a mutex. */
#define OKL4_MUTEX_INIT(x) \
    do { \
        int _okl4_err; \
        okl4_mutex_attr_t _okl4_attr; \
        okl4_mutex_attr_init(&_okl4_attr); \
        okl4_mutex_attr_setiscounted(&_okl4_attr, 1); \
        _okl4_err = okl4_mutex_create(x, &_okl4_attr); \
        assert(!_okl4_err); \
    } while (0)

/* Destroy a mutex. */
#define OKL4_MUTEX_FREE(x) \
    okl4_mutex_delete(x);

/*
 * Stream locking / unlocking.
 */
#define lock_stream(s) \
    do { \
        if (!(s)->mutex_initialised) { \
            OKL4_MUTEX_INIT(&(s)->mutex); \
            (s)->mutex_initialised = 1; \
        } \
        okl4_mutex_lock(&(s)->mutex); \
    } while (0)

#define unlock_stream(s) \
    do {\
        assert((s)->mutex_initialised); \
        okl4_mutex_unlock(&(s)->mutex); \
    } while (0)

/*
 * Mutex subsystem locking / unlocking.
 */

#define MALLOC_LOCK \
    okl4_mutex_lock(&malloc_mutex)

#define MALLOC_UNLOCK \
    okl4_mutex_unlock(&malloc_mutex)

extern OKL4_MUTEX_TYPE malloc_mutex;

#elif defined(THREAD_SAFE) && defined(LIBMUTEX_LOCKS)

/*
 * Using 'libmutex' to perform locking operations.
 */

#include <mutex/mutex.h>

/*
 * Setup
 */

/* The data type for a mutex. */
#define OKL4_MUTEX_TYPE  struct okl4_libmutex

/* Initialise a mutex. */
#define OKL4_MUTEX_INIT(x) \
    okl4_libmutex_init(x)

/* Destroy a mutex. */
#define OKL4_MUTEX_FREE(x) \
    okl4_libmutex_free(x)

/*
 * Stream locking / unlocking.
 */
#define lock_stream(s) \
    do { \
        if (!(s)->mutex_initialised) {\
            okl4_libmutex_init(&(s)->mutex); \
            (s)->mutex_initialised = 1; \
        } \
        okl4_libmutex_count_lock(&(s)->mutex); \
    } while (0)

#define unlock_stream(s) \
    do {\
        assert((s)->mutex_initialised); \
        okl4_libmutex_count_unlock(&(s)->mutex);\
    } while (0)

/*
 * Mutex subsystem locking / unlocking.
 */

#define MALLOC_LOCK \
    okl4_libmutex_count_lock(&malloc_mutex)

#define MALLOC_UNLOCK \
    okl4_libmutex_count_unlock(&malloc_mutex)

extern OKL4_MUTEX_TYPE malloc_mutex;

#elif !defined(THREAD_SAFE)

/* Setup / teardown. */
#define OKL4_MUTEX_TYPE  int
#define OKL4_MUTEX_INIT(x) do { } while (0)
#define OKL4_MUTEX_FREE(x) do { } while (0)

/* Stream locking / unlocking. */
#define lock_stream(s)    do { } while (0)
#define unlock_stream(s)  do { } while (0)

/* Mutex subsystem locking / unlocking. */
#define MALLOC_LOCK       do { } while (0)
#define MALLOC_UNLOCK     do { } while (0)

#else
#error "Unknown locking mechanism in use."
#endif

#endif /* !_THREADSAFETY_H_ */

