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

#ifndef __OKL4__INTSET_H__
#define __OKL4__INTSET_H__

#include <okl4/types.h>


/**
 * @file
 *
 * A interrupt set or 'irqset' represents a set of one or more hardware
 * interrupts. Each irqset is associated with a single 'owner' thread.
 *
 * The owner of a irqset may wait on the set of interrupts, causing the
 * thread to block until one of the interrupts in the set triggers. Once
 * woken, the thread is informed of which interrupt was triggered, and can
 * take any necessary action to handle to the interrupt.
 */

#define OKL4_INTSET_SIZE(irqs) \
        (sizeof(okl4_irqset_t) + (((irqs) - 1) * sizeof(okl4_word_t)))

#define OKL4_INTSET_SIZE_ATTR(attr) \
        OKL4_INTSET_SIZE((attr)->max_interrupts)

/**
 * This attribute object contains parameters specifying how a irqset object
 * should be created.
 *
 * Before using this attributes object, the function
 * okl4_irqset_attr_init() should first be called to initialise
 * the memory and setup default values for the object. Once performed,
 * the properties of the object may be modified using attribute setters.
 */
struct okl4_irqset_attr
{
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif
    okl4_kthread_t * owner;
    OKL4_MICRO(okl4_word_t notify_bits;)
    okl4_word_t max_interrupts;
};

/**
 * This attribute object contains parameters specifying how an interrupt
 * should be registered to an irqset object.
 *
 * Before using this attributes object, the function
 * okl4_irqset_register_attr_init() should first be called to initialise
 * the memory and setup default values for the object. Once performed,
 * the properties of the object may be modified using attribute setters.
 */
struct okl4_irqset_register_attr
{
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif
    okl4_word_t irq;
};

/**
 * This attribute object contains parameters specifying how an interrupt
 * should be deregistered from an irqset object.
 *
 * Before using this attributes object, the function
 * okl4_irqset_deregister_attr_init() should first be called to initialise
 * the memory and setup default values for the object. Once performed,
 * the properties of the object may be modified using attribute setters.
 */
struct okl4_irqset_deregister_attr
{
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif
    okl4_word_t irq;
};

/**
 * Initialise the irqset attribute object to its default values.
 * This must be performed before any other properties of this attributes
 * object are used.
 *
 * @param attr
 *     The attributes object to initialise.
 */
INLINE void okl4_irqset_attr_init(okl4_irqset_attr_t *attr);

/**
 * Set the owner of the irqset object. The irqset may only be used by the
 * owner specified at creation time. Additionally, each thread may own only
 * own a single irqset at a time.
 *
 * @param attr
 *     The attributes object to modify.
 *
 * @param owner
 *     The value to update this field to.
 */
INLINE void okl4_irqset_attr_setowner(okl4_irqset_attr_t *attr, okl4_kthread_t * owner);

#if defined(OKL4_KERNEL_MICRO)
/**
 * Set a mask indicating which notification bits should be used for
 * handling interrupts. The Micro kernel informs threads of interrupts by
 * setting bits in their notification mask. This may interfere with the
 * libokl4 notification API.
 *
 * Callers must ensure that the bits specified in this mask are not used
 * also used in the notification API, and vice-versa.
 *
 * @param attr
 *     The attributes object to modify.
 *
 * @param notify_bits
 *     The value to update this field to.
 */
INLINE void okl4_irqset_attr_setnotifybits(okl4_irqset_attr_t *attr, okl4_word_t notify_bits);
#endif /* OKL4_KERNEL_MICRO */

/**
 * The maximum number of interrupts that may be simultaneously registered
 * in this irqset object.
 *
 * @param attr
 *     The attributes object to modify.
 *
 * @param max_interrupts
 *     The value to update this field to.
 */
INLINE void okl4_irqset_attr_setmaxinterrupts(okl4_irqset_attr_t *attr, okl4_word_t max_interrupts);

/**
 * Initialise the irqset_register attribute object to its default values.
 * This must be performed before any other properties of this attributes
 * object are used.
 *
 * @param attr
 *     The attributes object to initialise.
 */
INLINE void okl4_irqset_register_attr_init(okl4_irqset_register_attr_t *attr);

/**
 * The interrupt number to register.
 *
 * @param attr
 *     The attributes object to modify.
 *
 * @param irq
 *     The value to update this field to.
 */
INLINE void okl4_irqset_register_attr_setirq(okl4_irqset_register_attr_t *attr, okl4_word_t irq);

/**
 * Initialise the irqset_deregister attribute object to its default values.
 * This must be performed before any other properties of this attributes
 * object are used.
 *
 * @param attr
 *     The attributes object to initialise.
 */
INLINE void okl4_irqset_deregister_attr_init(okl4_irqset_deregister_attr_t *attr);

/**
 * The interrupt number to deregister. This interrupt must already be
 * registered in this irqset.
 *
 * @param attr
 *     The attributes object to modify.
 *
 * @param irq
 *     The value to update this field to.
 */
INLINE void okl4_irqset_deregister_attr_setirq(okl4_irqset_deregister_attr_t *attr, okl4_word_t irq);

/**
 * The irqset object.
 */
struct okl4_irqset
{
    okl4_kthread_t * owner;
    okl4_word_t last_irq;
    okl4_word_t num_interrupts;
    okl4_word_t max_interrupts;
    OKL4_MICRO(okl4_word_t notify_bits;)
    okl4_word_t irqs[1];
};

/**
 * Create a new irqset object, based on parameters provided in the given
 * attributes object.
 *
 * @param irqset
 *     The irqset object to create.
 *
 * @param attr
 *     Creation parameters for the irqset object. This attributes object must
 *     have first been setup with a call to "okl4_irqset_attr_init", and
 *     optionally modified with calls to other functions modifying this
 *     attributes object.
 *
 * @retval OKL4_OK
 *     The interrupt set was successfully created.
 */
int okl4_irqset_create(
        okl4_irqset_t * irqset,
        okl4_irqset_attr_t * attr);

/**
 * Delete the given irqset object, releasing all kernel resources
 * associated with it.
 *
 *
 * Any interrupts registered when the irqset is deleted will be
 * automatically unregistered.
 *
 * @param irqset
 *     The irqset object to delete.
 */
void okl4_irqset_delete(okl4_irqset_t * irqset);

/**
 * Register an interrupt to this irqset. Once registration is performed,
 * the owner thread will be informed each time the interrupt fires when the
 * thread is waiting for interrupts.
 *
 * @param set
 *     The irqset to register the interrupt to.
 *
 * @param attr
 *     The parameters for this registration operation.
 *
 * @retval OKL4_OK
 *     The interrupt was successfully registered.
 *
 * @retval OKL4_INVALID_ARGUMENT
 *     The interrupt number was invalid, or is already in use.
 */
int okl4_irqset_register(
        okl4_irqset_t * set,
        okl4_irqset_register_attr_t * attr);

/**
 * Deregister an interrupt previously registered to this irqset. The given
 * interrupt will no longer cause the owner thread to wake when it is
 * blocked waiting for an interrupt.
 *
 * @param set
 *     The irqset to deregister the interrupt from.
 *
 * @param attr
 *     The parameters for this registration operation.
 *
 * @retval OKL4_OK
 *     The interrupt was successfully deregistered.
 *
 * @retval OKL4_INVALID_ARGUMENT
 *     The interrupt number was invalid.
 */
int okl4_irqset_deregister(
        okl4_irqset_t * set,
        okl4_irqset_deregister_attr_t * attr);

/**
 * Acknowledge the most recently received interrupt, without blocking the
 * current thread. This will cause the interrupt to be unmasked and allow
 * it to trigger again.
 *
 * @param set
 *     The irqset associated with the interrupt.
 *
 * @retval OKL4_OK
 *     The interrupt was successfully acknowledged.
 */
int okl4_irqset_acknowledge(okl4_irqset_t * set);

/**
 * Wait for an interrupt registered to the irqset to fire. The interrupt
 * number will be returned. Any unacknowledged interrupts previously
 * returned by this call will be automatically acknowledged.
 *
 * @param set
 *     The irqset to wait on.
 *
 * @param irq
 *     The interrupt number that triggered, causing the thread to wake again.
 *
 * @retval OKL4_OK
 *     An interrupt was received.
 */
int okl4_irqset_wait(
        okl4_irqset_t * set,
        okl4_word_t * irq);

INLINE void
okl4_irqset_attr_init(okl4_irqset_attr_t *attr)
{
    assert(attr != NULL);

    OKL4_SETUP_MAGIC(attr, OKL4_MAGIC_INTSET_ATTR);
    attr->owner = NULL;
    OKL4_MICRO(attr->notify_bits = (OKL4_WORD_T_BIT - 1);)
    attr->max_interrupts = 1;
}

INLINE void
okl4_irqset_register_attr_init(okl4_irqset_register_attr_t *attr)
{
    assert(attr != NULL);

    OKL4_SETUP_MAGIC(attr, OKL4_MAGIC_INTSET_REGISTER_ATTR);
    attr->irq = ~0UL;
}

INLINE void
okl4_irqset_deregister_attr_init(okl4_irqset_deregister_attr_t *attr)
{
    assert(attr != NULL);

    OKL4_SETUP_MAGIC(attr, OKL4_MAGIC_INTSET_DEREGISTER_ATTR);
    attr->irq = ~0UL;
}

INLINE void
okl4_irqset_attr_setowner(okl4_irqset_attr_t *attr, okl4_kthread_t * owner)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_INTSET_ATTR);

    attr->owner = owner;
}

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_irqset_attr_setnotifybits(okl4_irqset_attr_t *attr, okl4_word_t notify_bits)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_INTSET_ATTR);

    attr->notify_bits = notify_bits;
}
#endif /* OKL4_KERNEL_MICRO */

INLINE void
okl4_irqset_attr_setmaxinterrupts(okl4_irqset_attr_t *attr, okl4_word_t max_interrupts)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_INTSET_ATTR);

    attr->max_interrupts = max_interrupts;
}

INLINE void
okl4_irqset_register_attr_setirq(okl4_irqset_register_attr_t *attr, okl4_word_t irq)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_INTSET_REGISTER_ATTR);

    attr->irq = irq;
}

INLINE void
okl4_irqset_deregister_attr_setirq(okl4_irqset_deregister_attr_t *attr, okl4_word_t irq)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_INTSET_DEREGISTER_ATTR);

    attr->irq = irq;
}

#endif /* !__OKL4__INTSET_H__ */

