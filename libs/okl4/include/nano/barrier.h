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

#ifndef __OKL4__BARRIER_H__
#define __OKL4__BARRIER_H__

#include <okl4/types.h>

#include <atomic_ops/atomic_ops.h>
#include <nano/nano.h>

/**
 * @file
 *
 * A simple barrier synchronisation object. Each barrier object is
 * associated with a set of threads. When threads wait on the barrier
 * object, they will not be allowed to progress until all other threads
 * have also performed a wait on the same barrier. Once all threads have
 * been woken, the barrier will reset and may be used again by the same set
 * of threads.
 *
 * Barriers are often used in multithreaded applications to ensure that all
 * threads reach a certain stage of execution before allowing any thread to
 * move onto their next stage.
 */

/**
 * This attribute object contains parameters specifying how a 'barrier'
 * object should be created.
 *
 * Before using this attributes object, the function
 * @a okl4_barrier_attr_init should first be called to initialise
 * the memory and setup default values for the object. Once performed,
 * the properties of the object may be modified using attribute setters.
 *
 * Finally, once the attribute object has been setup, the function
 * @a okl4_barrier_create may be called to create the barrier
 * object.
 */
struct okl4_barrier_attr
{
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif
    okl4_word_t num_threads;
};

/**
 * Initialise the barrier attribute object to its default values.
 * This must be performed before any other properties of this attributes
 * object are used.
 *
 * @param attr
 *     The attributes object to initialise.
 */
INLINE void okl4_barrier_attr_init(okl4_barrier_attr_t *attr);

/**
 * Set the number of threads required to wait on the barrier before any
 * other thread waiting on the barrier will be allowed to progress.
 *
 * @param attr
 *     The attributes object to modify.
 *
 * @param num_threads
 *     The value to update this field to.
 */
INLINE void okl4_barrier_attr_setnumthreads(okl4_barrier_attr_t *attr, okl4_word_t num_threads);

/**
 * The barrier object.
 */
struct okl4_barrier
{
    okn_barrier_t barrier;
};

/**
 * Create a new barrier object, based on parameters provided in the given
 * attributes object.
 *
 * @param barrier
 *     The barrier object to create.
 *
 * @param attr
 *     Creation parameters for the barrier object. This attributes object must
 *     have first been setup with a call to "okl4_barrier_attr_init", and
 *     optionally modified with calls to other functions modifying this
 *     attributes object.
 *
 * @retval OKL4_OK
 *     The barrier was successfully created.
 */
int okl4_barrier_create(
        okl4_barrier_t * barrier,
        okl4_barrier_attr_t * attr);

/**
 * Block on the given barrier until all other threads have also performed a
 * wait operation on it.
 *
 * @param barrier
 *     The barrier to wait on.
 */
void okl4_barrier_wait(okl4_barrier_t * barrier);

/**
 * Delete the given barrier object, releasing all kernel resources
 * associated with it.
 *
 *
 * The barrier must not have any threads blocked on it when this function
 * is called.
 *
 * @param barrier
 *     The barrier object to delete.
 */
void okl4_barrier_delete(okl4_barrier_t * barrier);


INLINE void
okl4_barrier_attr_init(okl4_barrier_attr_t *attr)
{
    assert(attr != NULL);

    OKL4_SETUP_MAGIC(attr, OKL4_MAGIC_BARRIER_ATTR);
    attr->num_threads = 0;
}

INLINE void
okl4_barrier_attr_setnumthreads(okl4_barrier_attr_t *attr, okl4_word_t num_threads)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_BARRIER_ATTR);

    attr->num_threads = num_threads;
}

#endif /* !__OKL4__BARRIER_H__ */

