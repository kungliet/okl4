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

#ifndef __OKL4__MUTEX_H__
#define __OKL4__MUTEX_H__

#include <okl4/types.h>

#if defined(OKL4_KERNEL_MICRO)
#include <atomic_ops/atomic_ops.h>
#endif
#if defined(OKL4_KERNEL_NANO)
#include <nano/nano.h>
#endif

/**
 * @file
 *
 * A mutex is a simple lock object, allowing only one thread to hold the
 * lock at any point in time. If a second thread attempts to acquire the
 * lock while another is already holding it, the second thread will block
 * until the lock next becomes free again.
 *
 * Such a lock may be used to prevent multiple threads from accessing
 * shared data structures or code paths at the same time.
 */

/**
 * This attribute object contains parameters specifying how a 'mutex'
 * object should be created.
 *
 * Before using this attributes object, the function
 * @a okl4_mutex_attr_init should first be called to initialise
 * the memory and setup default values for the object. Once performed,
 * the properties of the object may be modified using attribute setters.
 *
 * Finally, once the attribute object has been setup, the function
 * @a okl4_mutex_create may be called to create the mutex
 * object.
 */
struct okl4_mutex_attr
{
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif
    OKL4_MICRO(okl4_kmutexid_t id;)
    okl4_word_t is_counted;
};

/**
 * Initialise the mutex attribute object to its default values.
 * This must be performed before any other properties of this attributes
 * object are used.
 *
 * @param attr
 *     The attributes object to initialise.
 */
INLINE void okl4_mutex_attr_init(okl4_mutex_attr_t *attr);

#if defined(OKL4_KERNEL_MICRO)
/**
 * Set the kernel mutex identifier to use when creating this mutex. This is
 * only required for the Micro kernel.
 *
 * @param attr
 *     The attributes object to modify.
 *
 * @param id
 *     The value to update this field to.
 */
INLINE void okl4_mutex_attr_setid(okl4_mutex_attr_t *attr, okl4_kmutexid_t id);
#endif /* OKL4_KERNEL_MICRO */

/**
 * Set whether the mutex to be created should be a counted mutex, also
 * known as recursive mutexes.
 *
 * Counted mutexes allow the lock holder to lock a held mutex multiple
 * times. For each lock operation that occurs, a corresponding unlock
 * operation must take place before the lock will be released.
 *
 * This value should be non-zero to indicate that a counted mutex should
 * be created.
 *
 * @param attr
 *     The attributes object to modify.
 *
 * @param is_counted
 *     The value to update this field to.
 */
INLINE void okl4_mutex_attr_setiscounted(okl4_mutex_attr_t *attr, okl4_word_t is_counted);

/**
 * The mutex object.
 */
struct okl4_mutex
{
    OKL4_NANO(okn_futex_t futex;)
    OKL4_MICRO(okl4_atomic_word_t value;)
    OKL4_MICRO(okl4_u8_t auto_allocated;)
    OKL4_MICRO(okl4_kmutexid_t id;)
    OKL4_NANO(okl4_kcap_t owner;)
    okl4_word_t count;
    okl4_u8_t is_counted;
};

/**
 * Create a new mutex object, based on parameters provided in the given
 * attributes object.
 *
 * @param mutex
 *     The mutex object to create.
 *
 * @param attr
 *     Creation parameters for the mutex object. This attributes object must
 *     have first been setup with a call to "okl4_mutex_attr_init", and
 *     optionally modified with calls to other functions modifying this
 *     attributes object.
 *
 * @retval OKL4_OK
 *     The mutex was successfully created.
 *
 * @retval OKL4_INVALID_ARGUMENT
 *     An invalid mutex identifier was specified.
 */
int okl4_mutex_create(
        okl4_mutex_t * mutex,
        okl4_mutex_attr_t * attr);

/**
 * Aquire the given mutex. If the mutex is already held by another thread,
 * block until it becomes free again.
 *
 * Unless the mutex was created as a counted mutex, this call must not be
 * called if the lock is already held by the current thread. Failure to do
 * so will result in undefined behaviour.
 *
 * @param mutex
 *     The mutex to acquire.
 */
void okl4_mutex_lock(okl4_mutex_t * mutex);

/**
 * Attempt to acquire the given mutex. If the mutex is already held, return
 * an error without blocking.
 *
 * @param mutex
 *     The mutex to attempt to acquire.
 *
 * @retval OKL4_OK
 *     The mutex was successfully acquired.
 *
 * @retval OKL4_WOULD_BLOCK
 *     The mutex was already held by another thread.
 */
int okl4_mutex_trylock(okl4_mutex_t * mutex);

/**
 * Release the given mutex, allowing other threads to acquire it. The mutex
 * must already be held by the current thread.
 *
 * If the mutex was created as a counted mutex and has been locked multiple
 * times, the mutex must be unlocked the same number of times as it was
 * locked before it becomes free for other threads to acquire.
 *
 * @param mutex
 *     The mutex to release.
 */
void okl4_mutex_unlock(okl4_mutex_t * mutex);

/**
 * Delete the given mutex object, releasing all kernel resources
 * associated with it.
 *
 *
 * The mutex must not be locked when this function is called.
 *
 * @param mutex
 *     The mutex object to delete.
 */
void okl4_mutex_delete(okl4_mutex_t * mutex);


INLINE void
okl4_mutex_attr_init(okl4_mutex_attr_t *attr)
{
    assert(attr != NULL);

    OKL4_SETUP_MAGIC(attr, OKL4_MAGIC_MUTEX_ATTR);
    OKL4_MICRO(attr->id.raw = ~0UL;)
    attr->is_counted = 0;
}

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_mutex_attr_setid(okl4_mutex_attr_t *attr, okl4_kmutexid_t id)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_MUTEX_ATTR);

    attr->id = id;
}
#endif /* OKL4_KERNEL_MICRO */

INLINE void
okl4_mutex_attr_setiscounted(okl4_mutex_attr_t *attr, okl4_word_t is_counted)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_MUTEX_ATTR);

    attr->is_counted = is_counted;
}

#endif /* !__OKL4__MUTEX_H__ */

