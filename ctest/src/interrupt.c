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

#include <okl4/env.h>
#include <okl4/kthread.h>
#include <okl4/irqset.h>

#include "helpers.h"
#include "clock_driver.h"

/* Create an irqset registered to the given thread. */
static okl4_irqset_t *
create_irqset(okl4_kthread_t *owner)
{
    int error;
    okl4_irqset_attr_t attr;
    okl4_irqset_t *irqset;

    okl4_irqset_attr_init(&attr);
    okl4_irqset_attr_setowner(&attr, root_thread);
    irqset = malloc(OKL4_INTSET_SIZE_ATTR(&attr));
    assert(irqset);
    error = okl4_irqset_create(irqset, &attr);
    assert(!error);

    return irqset;
}

/* Delete an irqset. */
static void
delete_irqset(okl4_irqset_t *set)
{
    okl4_irqset_delete(set);
    free(set);
}

/* Register an irqset for an interrupt. */
static void
register_interrupt(okl4_irqset_t *set, okl4_word_t irq)
{
    int error;
    okl4_irqset_register_attr_t reg_attr;

    okl4_irqset_register_attr_init(&reg_attr);
    okl4_irqset_register_attr_setirq(&reg_attr, irq);
    error = okl4_irqset_register(set, &reg_attr);
    assert(!error);
}

/* Deregister an interrupt from an irqset. */
static void
deregister_interrupt(okl4_irqset_t *set, okl4_word_t irq)
{
    int error;
    okl4_irqset_deregister_attr_t dereg_attr;

    okl4_irqset_deregister_attr_init(&dereg_attr);
    okl4_irqset_deregister_attr_setirq(&dereg_attr, irq);
    error = okl4_irqset_deregister(set, &dereg_attr);
    assert(!error);
}

static void
test_interrupts(int ack_after_receive)
{
    int i, j;
    int error;
    okl4_irqset_t *irqset;
    okl4_word_t test_irq = get_test_irq();

    /* Create an irqset. */
    irqset = create_irqset(root_thread);

    /* Perform multiple registers/deregisters. */
    for (j = 0; j < 10; j++) {

        /* Register for an interrupt. */
        register_interrupt(irqset, test_irq);

        /* Wait for the interrupt. */
        for (i = 0; i < 10; i++) {
            int incorrect_interrupt;
            okl4_word_t irq;

            /* Generate an interrupt. */
            generate_interrupt();

            /* Wait for the interrupt to fire. */
            do {
                error = okl4_irqset_wait(irqset, &irq);
                assert(!error);
                assert(irq == test_irq);

                /* If the clock driver was expecting a different interrupt,
                 * keep waiting. */
                incorrect_interrupt = process_interrupt();
            } while (incorrect_interrupt);

#if defined(OKL4_KERNEL_MICRO)
            /* Acknowledge the interrupt. We can't do much to ensure that it
             * actually was acknowledged (and this call is not just a no-op),
             * but we test this to exercise the codepath at least a small
             * amount. */
            if (ack_after_receive) {
                error = okl4_irqset_acknowledge(irqset);
                assert(!error);
            }
#else
            assert(!ack_after_receive);
#endif
        }

        /* Deregister from the interrupt. */
        deregister_interrupt(irqset, test_irq);
    }

    /* Done. */
    delete_irqset(irqset);
}

/*
 * Fire multiple interrupts, ensuring that we can wait on the same interrupt
 * multiple times.
 */
START_TEST(INT0100)
{
    test_interrupts(0);
}
END_TEST

#if defined(OKL4_KERNEL_MICRO)
/*
 * Fire multiple interrupts, acknowledging each one before waiting for the
 * next. Only the Micro kernel supports "Acknowledge".
 */
START_TEST(INT1000)
{
    test_interrupts(1);
}
END_TEST
#endif /* OKL4_KERNEL_MICRO */

TCase *
make_interrupt_tcase(void)
{
    TCase * tc;

    tc = tcase_create("Interrupts");

    /* If there is no clock source available, do not perform interrupt
     * tests. */
    if (!interrupts_available()) {
        return tc;
    }

    tcase_add_test(tc, INT0100);
#if defined(OKL4_KERNEL_MICRO)
    tcase_add_test(tc, INT1000);
#endif

    return tc;
}

