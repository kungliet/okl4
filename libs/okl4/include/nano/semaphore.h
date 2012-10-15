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

#ifndef __OKL4__SEMAPHORE_H__
#define __OKL4__SEMAPHORE_H__

#include <okl4/types.h>

#include <atomic_ops/atomic_ops.h>
#include <nano/nano.h>

/**
 * @file
 *
 * A simple semaphore synchronisation object. Semaphore objects
 * conceptually consist of a single numeric counter, an operation to
 * increment this counter and an operation to decrement this counter. The
 * semaphore will not allow the counter to ever be decremented below zero.
 * Threads that attempt to do so will become blocked until another thread
 * first increases the counter.
 */

/**
 * This attribute object contains parameters specifying how a 'semaphore'
 * object should be created.
 *
 * Before using this attributes object, the function
 * @a okl4_semaphore_attr_init should first be called to initialise
 * the memory and setup default values for the object. Once performed,
 * the properties of the object may be modified using attribute setters.
 *
 * Finally, once the attribute object has been setup, the function
 * @a okl4_semaphore_create may be called to create the semaphore
 * object.
 */
struct okl4_semaphore_attr
{
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif
    okl4_word_t init_value;
};

/**
 * Initialise the semaphore attribute object to its default values.
 * This must be performed before any other properties of this attributes
 * object are used.
 *
 * @param attr
 *     The attributes object to initialise.
 */
INLINE void okl4_semaphore_attr_init(okl4_semaphore_attr_t *attr);

/**
 * Set the initial value of the semaphore's counter.
 *
 * @param attr
 *     The attributes object to modify.
 *
 * @param init_value
 *     The value to update this field to.
 */
INLINE void okl4_semaphore_attr_setinitvalue(okl4_semaphore_attr_t *attr, okl4_word_t init_value);

/**
 * The semaphore object.
 */
struct okl4_semaphore
{
    okn_semaphore_t sem;
};

/**
 * Create a new semaphore object, based on parameters provided in the given
 * attributes object.
 *
 * @param semaphore
 *     The semaphore object to create.
 *
 * @param attr
 *     Creation parameters for the semaphore object. This attributes object
 *     must have first been setup with a call to "okl4_semaphore_attr_init",
 *     and optionally modified with calls to other functions modifying this
 *     attributes object.
 *
 * @retval OKL4_OK
 *     The semaphore was successfully created.
 */
int okl4_semaphore_create(
        okl4_semaphore_t * semaphore,
        okl4_semaphore_attr_t * attr);

/**
 * Attempt to decrease the given semaphore's count. If the decrement
 * operation would cause the counter to become negative, the current thread
 * will block until another thread increases the semaphore's count.
 *
 * @param semaphore
 *     The semaphore to decrease.
 */
void okl4_semaphore_down(okl4_semaphore_t * semaphore);

/**
 * Attempt to decrease the given semaphore's count. If the decrement
 * operation would cause the counter to become negative, return an error
 * without blocking.
 *
 * @param semaphore
 *     The semaphore to decrease.
 *
 * @retval OKL4_OK
 *     The semaphore value was decremented.
 *
 * @retval OKL4_WOULD_BLOCK
 *     The semaphore value could not be decremented without blocking the
 *     current thread.
 */
int okl4_semaphore_trydown(okl4_semaphore_t * semaphore);

/**
 * Increase the given semaphore's count. If one or more other threads are
 * currently blocked attempting to decrease the count of the semaphore
 * below zero, this operation will cause one of those threads to wake.
 *
 * @param semaphore
 *     The semaphore to increase.
 */
void okl4_semaphore_up(okl4_semaphore_t * semaphore);

/**
 * Delete the given semaphore object, releasing all kernel resources
 * associated with it.
 *
 *
 * The semaphore must not have any threads waiting on it when this function
 * is called.
 *
 * @param semaphore
 *     The semaphore object to delete.
 */
void okl4_semaphore_delete(okl4_semaphore_t * semaphore);


INLINE void
okl4_semaphore_attr_init(okl4_semaphore_attr_t *attr)
{
    assert(attr != NULL);

    OKL4_SETUP_MAGIC(attr, OKL4_MAGIC_SEMAPHORE_ATTR);
    attr->init_value = 0;
}

INLINE void
okl4_semaphore_attr_setinitvalue(okl4_semaphore_attr_t *attr, okl4_word_t init_value)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_SEMAPHORE_ATTR);

    attr->init_value = init_value;
}

#endif /* !__OKL4__SEMAPHORE_H__ */

