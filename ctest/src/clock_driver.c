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

#include "clock_driver.h"

#include <okl4/types.h>

#if defined(TIMER_DRIVER)

#include <stdio.h>
#include <ctest/ctest.h>

#include <okl4/env.h>
#include <okl4/kthread.h>
#include <okl4/irqset.h>

#include <driver/driver.h>
#include <driver/device.h>
#include <driver/timer.h>

#include "helpers.h"


/**
 * TIMER_DRIVER is defined in the build system, and indicates
 * which device driver to use
 */
extern struct driver TIMER_DRIVER;

/* IRQ to use for the clock */
static uint32_t CLOCK_IRQ;

/**
 * The timer device struct holds information for the physical timer
 */
struct timer_device {
    struct resource *resources;

    struct device_interface *di;
    struct timer_interface  *ti;

    uint64_t frequency;
};

/* Statically allocated memory */
static struct resource      resources[8];
static struct timer_device  timer_device;

/* Has the clock been successfully initialised? */
int init_performed = 0;

/*
 * Initialise the timer subsystem. Return non-zero on error.
 */
int
init_clock(void)
{
    int result;
    int r = 0;
    void *device;
    okl4_env_segment_t *timer_seg;
    okl4_env_device_irqs_t *timer_irqs;

    /* Setup memory resources. */
    timer_seg = okl4_env_get_segment("MAIN_TIMER_MEM0");
    if (timer_seg == NULL) {
        return 1;
    }
    resources[r].resource.mem = (mem_space_t)timer_seg->virt_addr;
    resources[r].type = MEMORY_RESOURCE;
    r++;

    /* Setup IRQ resources. */
    timer_irqs = (okl4_env_device_irqs_t *)okl4_env_get("TIMER_DEV_IRQ_LIST");
    if (timer_irqs == NULL) {
        return 1;
    }
    resources[r].type = INTERRUPT_RESOURCE;
    resources[r].resource.interrupt = (int)timer_irqs->irqs[0];
    CLOCK_IRQ = timer_irqs->irqs[0];
    r++;

    /* Fill up the rest of the resources. */
    for (/* none */; r < 8; r++) {
        resources[r].type = NO_RESOURCE;
    }

    /* Allocate memory for the device. */
    device = malloc(TIMER_DRIVER.size);
    assert(device);

    /* Initialise the device state. */
    timer_device.resources = resources;

    /* Create and enable the device. */
    timer_device.di = setup_device_instance(&TIMER_DRIVER,
            device /*lint -e429*/);
    result = device_setup(timer_device.di, timer_device.resources);
    assert(result == DEVICE_SUCCESS);
    timer_device.ti = (struct timer_interface *)(void *)device_get_interface(
            timer_device.di, 0);
    timer_device.frequency = timer_get_tick_frequency(timer_device.ti);

    result = device_enable(timer_device.di);
    assert(result == DEVICE_SUCCESS);

    init_performed = 1;
    return 0;
}

/* Determine if interrupts are available. */
int
interrupts_available(void)
{
    return init_performed;
}

/* Get the IRQ used for tests. */
okl4_word_t
get_test_irq(void)
{
    return CLOCK_IRQ;
}

/* Generate an interrupt. */
void
generate_interrupt(void)
{

    uint64_t clock = timer_get_ticks(timer_device.ti);
    uint64_t ticks = 2;
    while (timer_timeout(timer_device.ti, ticks + clock)) {
        ticks = (ticks * 3) / 2;
    }
}

/*
 * Process an interrupt generated by generate_interrupt. Must be called before
 * the next 'generate_interrupt' call.
 *
 * If this function returns non-zero, the received interrupt was not the
 * expected interrupt, and the caller must continue waiting for interrupts
 * before next calling 'generate_interrupt'.
 */
int
process_interrupt(void)
{
    int success = device_interrupt(timer_device.di, (int)CLOCK_IRQ);
    return success ? 0 : 1;
}

#else /* !TIMER_DRIVER */

/*
 * If no timer driver is available, we just implemented minimal stubs
 * so that the user may still compile ctest.
 */

int init_clock(void)           { return 1; }
int interrupts_available(void) { return 0; }
okl4_word_t get_test_irq(void) { return ~0UL; }
void generate_interrupt(void)  { }
int process_interrupt(void)   { return 0; }

#endif /* !TIMER_DRIVER */

