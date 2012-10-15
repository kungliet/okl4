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

#include <soc/interface.h>
#include <soc/soc.h>
#include <kernel/arch/continuation.h>
#include <simplesoc.h>
#include <interrupt.h>

#if defined(USE_SIMPLESOC)

/*
 * We assume the the platform implements the following functionality in
 * interrupt.h:
 *
 *    #define IRQS ...
 *    void soc_mask_irq(int irq);
 *    void soc_unmask_irq(int irq);
 *
 * Ensure appropriate constants are defined:
 */
#if !defined(IRQS)
#error "Platform code needs to define constant 'IRQS'; the number of IRQs " \
       "supported by the platform."
#endif

/*
 * Internal data structures.
 */

typedef struct {
    soc_ref_t handler;
    word_t notify_mask;
} irq_mapping_t;

typedef struct {
    space_h owner;
} irq_owner_t;

/* Map interrupt numbers to handler/notify mask information. */
static irq_mapping_t irq_mapping[IRQS];

/* Get the owner space for each interrupt. */
static irq_owner_t irq_owners[IRQS];

/* Bitmap of pending IRQs. */
static bitmap_t irq_pending[IRQS];

/* Initialise interrupt data strctures. */
void
simplesoc_init(void)
{
    int i;

    /* Setup memory. */
    for (i = 0; i < IRQS; i++) {
        kernel_ref_init(&irq_mapping[i].handler);
        soc_mask_irq(i);
    }
    bitmap_init(irq_pending, IRQS, false);
}

/*
 * Handle an IRQ. If continuation is non-NULL, we activate it after delivering
 * an interrupt, or return if an error occurred. If it is NULL, return 1 if a
 * reschedule is required, 0 otherwise.
 */
static int
handle_irq(word_t irq, continuation_t cont)
{
    tcb_h handler;
    utcb_t *utcb;
    word_t mask;

    /* Mask off the IRQ. */
    soc_mask_irq(irq);

    /* Ensure that we have a handler for it. */
    handler = kernel_ref_get_tcb(irq_mapping[irq].handler);
    if (EXPECT_FALSE(!handler)) {
        /* Spurious interrupt. */
        return 0;
    }

    /* Fetch the handler thread's IRQ UTCB word. */
    utcb = kernel_get_utcb(handler);
    SOC_ASSERT(DEBUG, utcb);
    word_t *irq_desc = &utcb->platform_reserved[0];

    /* If the handler hasn't dealt with the previous interrupt yet,
     * mark the interrupt as pending. */
    if (EXPECT_FALSE(*irq_desc != ~0UL)) {
        bitmap_set(irq_pending, irq);
        return 0;
    }

    /* Otherwise, deliver the nofitication, and return whether
     * we require a reschedule. */
    *irq_desc = irq;
    mask = irq_mapping[irq].notify_mask;
    return kernel_deliver_notify(handler, mask, cont);
}

/*
 * Handle an IRQ. Return whether a kernel reschedule is required.
 */
int
simplesoc_handle_irq(word_t irq)
{
    return handle_irq(irq, NULL);
}

/*
 * Handle an IRQ, and perform a reschedule.
 */
NORETURN void
simplesoc_handle_irq_reschedule(word_t irq, continuation_t cont)
{
    /* Deliver the IRQ. */
    (void)handle_irq(irq, cont);

    /* If the IRQ could not be delivered, just activate the continuation. */
    ACTIVATE_CONTINUATION(cont);
}

/**
 * Register and unregister interrupts
 */
word_t
soc_config_interrupt(struct irq_desc *desc, int notify_bit,
        tcb_h handler, space_h owner, soc_irq_action_e control, utcb_t *utcb)
{
    word_t irq;

    /* Get the IRQ to configure. */
    irq = *((word_t*)desc);
    if (irq >= IRQS) {
        return EINVALID_PARAM;
    }

    /* Ensure we have permission to configure this IRQ. */
    if (irq_owners[irq].owner != owner) {
        return EINVALID_PARAM;
    }

    switch (control) {
    case soc_irq_register:
        {
            /* Ensure that the owner is correct. */
            if (irq_owners[irq].owner != owner) {
                return EINVALID_PARAM;
            }

            /* Ensure that no other thread is already registered. */
            if (kernel_ref_get_tcb(irq_mapping[irq].handler)) {
                return EINVALID_PARAM;
            }

            /*
             * HACK: If the UTCB has not been initialised to (-1) do that now.
             * This is problematic if the user is using IRQ 0 and has received
             * such an IRQ when they do this register.
             *
             * The correct way to handle this is for the kernel to give the SoC
             * a callback each time a new thread is created, allowing us to
             * initialise it then.
             */
            if (utcb->platform_reserved[0] == 0) {
                utcb->platform_reserved[0] = ~0UL;
            }

            /* Store the information about the handler. */
            kernel_ref_set_referenced(handler, &irq_mapping[irq].handler);
            irq_mapping[irq].notify_mask = (1UL << notify_bit);

            /* Callback the SoC. */
            soc_register_irq_callback(irq);

            /* Unmask the IRQ. */
            soc_unmask_irq(irq);
            break;
        }

    case soc_irq_unregister:
        {
            /* Ensure that the owner is correct. */
            if (irq_owners[irq].owner != owner) {
                return EINVALID_PARAM;
            }

            /* Ensure that the handler is correct. */
            if (kernel_ref_get_tcb(irq_mapping[irq].handler) != handler) {
                return EINVALID_PARAM;
            }

            /* If userspace has not acknowledged the interrupt he is
             * deregistering, do that now. */
            if (utcb->platform_reserved[0] == irq) {
                utcb->platform_reserved[0] = ~0UL;
            }

            /* Clear out the reference. */
            kernel_ref_remove_referenced(&irq_mapping[irq].handler);
            irq_mapping[irq].notify_mask = 0;

            /* Mask off the interrupt. */
            soc_mask_irq(irq);

            /* Callback the SoC. */
            soc_deregister_irq_callback(irq);

            break;
        }
    }

    return 0;
}

/*
 * Get an interrupt pending for the given handler, or (-1) to
 * indicate that no such interrupt is pending.
 */
static int
get_pending_interrupt(tcb_h handler)
{
    word_t i;
    bitmap_t tmp_pending[BITMAP_SIZE(IRQS)];
    int pend;

    /* Are there any bits set at all in the bitmap? */
    pend = bitmap_findfirstset(irq_pending, IRQS);
    if (pend == -1) {
        return -1;
    }

    /* Make a copy of the bitmap, so we can destroy it as we search through it
     * below. */
    for (i = 0; i < BITMAP_SIZE(IRQS); i++) {
        tmp_pending[i] = irq_pending[i];
    }

    /* Search through the copy for a pending interrupt registered to the
     * handler. */
    do {
        bitmap_clear(tmp_pending, pend);

        tcb_h itr_handler = kernel_ref_get_tcb(irq_mapping[pend].handler);
        if (itr_handler == handler) {
            return pend;
        }
        pend = bitmap_findfirstset(tmp_pending, IRQS);
    } while (pend != -1);

    return -1;
}

/*
 * Ack the interrupt.
 */
word_t
soc_ack_interrupt(struct irq_desc *desc, tcb_h handler)
{
    int pend;
    word_t irq;
    utcb_t *utcb;

    irq = *(word_t *)desc;

    /* Ensure IRQ number is sane. */
    if (irq >= IRQS) {
        return EINVALID_PARAM;
    }

    /* Ensure the handler is associated with this IRQ. */
    if (kernel_ref_get_tcb(irq_mapping[irq].handler) != handler) {
        return EINVALID_PARAM;
    }

    /* Clear the IRQ in the thread's UTCB. */
    utcb = kernel_get_utcb(handler);
    if (utcb->platform_reserved[0] == irq) {
        utcb->platform_reserved[0] = ~0UL;
    }
    soc_unmask_irq(irq);

    /* Are there any pending interrupts? */
    pend = get_pending_interrupt(handler);
    if (pend == -1) {
        return 0;
    }

    /* Otherwise, handle the interrupt. */
    bitmap_clear(irq_pending, pend);
    utcb->platform_reserved[0] = pend;
    /* BUG: The kernel may need to perform a full schedule after this,
     * but we have no way of indicating this back to them. */
    (void)kernel_deliver_notify(handler, irq_mapping[pend].notify_mask, NULL);
    return 0;
}

/*
 * Configure access controls for interrupts
 */
word_t
soc_security_control_interrupt(struct irq_desc *desc,
        space_h owner, word_t control)
{
    word_t irq;

    /* Get the IRQ to configure. */
    irq = *(word_t *)desc;
    if (irq >= IRQS) {
        return EINVALID_PARAM;
    }

    /* Determine what type of operation we are performing. */
    switch (control >> 16) {
        case 0: /* Grant */
            if (irq_owners[irq].owner) {
                return EINVALID_PARAM;
            }
            irq_owners[irq].owner = owner;
            break;

        case 1: /* Revoke */
            if (irq_owners[irq].owner != owner) {
                return EINVALID_PARAM;
            }
            irq_owners[irq].owner = NULL;
            break;

        default:
            return EINVALID_PARAM;
    }

    return 0;
}

#endif /* USE_SIMPLESOC */

