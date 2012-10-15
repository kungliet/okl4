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
 * Generic timer driver
 */

#include <stdlib.h>
#include <okl4/env.h>
#include <nano/nano.h>
#include <nanotest/clock.h>
#include <driver/driver.h>
#include <driver/device.h>
#include <driver/timer.h>

/** 
 * TIMER_DRIVER is defined in the build system, and indicates
 * which device driver to use
 */
extern struct driver TIMER_DRIVER;

/* IRQ to use for the clock */
uint32_t CLOCK_IRQ;

/** 
 * The timer device struct holds information for the physical timer
 */
struct timer_device {
    struct resource         *resources;
    
    struct device_interface *di;
    struct timer_interface  *ti;
    
    uint64_t                frequency;
};

/* Statically allocated memory */

static struct resource      resources[8];
static struct timer_device  timer_device;


#define MEGA 1000000

static inline 
uint64_t
us_to_ticks(uint64_t frequency, uint64_t nanoseconds)
{
    uint32_t seconds = nanoseconds / MEGA;
    uint32_t overflow = nanoseconds % MEGA;
    /*
     * Assuming freq < 18.44 GHz (see below), this shouldn't overflow 
     * unless seconds > 10**9 == 31 years
     */
    uint64_t ticks = (frequency * seconds);
    /* 
     * This shouldn't overflow, 
     * unless frequency > (2**64 / 10**9 == 18.44 GHz)
     */
    ticks += ((frequency * overflow) / MEGA);
    return ticks;
}

static inline
uint64_t
ticks_to_us(uint64_t frequency, uint64_t ticks)
{
    uint64_t seconds = ticks / frequency;
    uint64_t overflow = ticks % frequency;
    uint64_t us = seconds * MEGA;
    us += overflow * MEGA / frequency;
    return us;
}



/**
 * The server entry point.
 */
void
clock_init(void)
{
    int r = 0;
    void *device;
    okl4_env_segment_t *timer_seg;

    /*
     * Setup resources for driver init - this should probably be weaved
     */
    resources[r].type = MEMORY_RESOURCE;
    timer_seg = okl4_env_get_segment("MAIN_TIMER_MEM0");

    if (timer_seg == NULL) {
        printf("couldn't find timer memory mapping\n");
        return;
    }
    resources[r++].resource.mem = (mem_space_t) timer_seg->virt_addr;

    struct okl4_env_device_irqs * timer_irqs = (struct okl4_env_device_irqs *)okl4_env_get("TIMER_DEV_IRQ_LIST");
    if (timer_irqs == NULL) {
        printf("couldn't find timer irq list\n");
        return;
    }

    /* Nano can only handle one interrupt anyway */
    resources[r].type = INTERRUPT_RESOURCE;
    resources[r++].resource.interrupt = timer_irqs->irqs[0];
    CLOCK_IRQ = timer_irqs->irqs[0];

    for (/*none*/; r < 8; r++)
    {
        resources[r].type = NO_RESOURCE;
    }
    
    /*
     * Allocate memory for the device
     */
    device = malloc(TIMER_DRIVER.size);
    if (device == NULL) {
        free(device);
        return;
    }
   
    /* Initialise the device state */
    {
        timer_device.resources = resources;
    }
    
    /* Create and enable the device */
    timer_device.di = setup_device_instance(&TIMER_DRIVER, device);
    device_setup(timer_device.di, timer_device.resources);
    
    timer_device.ti = (struct timer_interface *)device_get_interface(timer_device.di, 0);
    device_enable(timer_device.di);
    
    timer_device.frequency = timer_get_tick_frequency(timer_device.ti);
}

/* Wait for the specified time (in microseconds) */
void
clock_wait(uint64_t time)
{
    int error;
    uint64_t clock = clock_read();
    uint64_t timeout = clock + time;

    error = okn_syscall_interrupt_register(CLOCK_IRQ);
    assert(!error);
    if (timer_timeout(timer_device.ti, us_to_ticks(timer_device.frequency, timeout))) {
        error = okn_syscall_interrupt_deregister(CLOCK_IRQ);
        assert(!error);
        return; /* Already expired */
    }
    /* wait for the interrupt */
    while(!device_interrupt(timer_device.di, okn_syscall_interrupt_wait()));
    error = okn_syscall_interrupt_deregister(CLOCK_IRQ);
    assert(!error);
}

/* return the time in nanoseconds */
uint64_t
clock_read(void)
{
    uint64_t ticks = ticks_to_us(timer_device.frequency, timer_get_ticks(timer_device.ti));
    return ticks;
}

/* generate an interrupt after the specified time (in microseconds) */
void
clock_generate_interrupt(uint64_t time)
{
    uint64_t clock = clock_read();
    
    while(timer_timeout(timer_device.ti, us_to_ticks(timer_device.frequency, clock + time)))
        time *= 2;
        
}

/* register for a clock interrupt 
 * returns the interrupt to wait for
 */
int
clock_register_interrupt(void)
{
    int error;

    error = okn_syscall_interrupt_register(CLOCK_IRQ);
    assert(!error);

    return CLOCK_IRQ;
}

/* deregister for the clock interrupt */
void 
clock_deregister_interrupt(void)
{
    int error;

    error = okn_syscall_interrupt_deregister(CLOCK_IRQ);
    assert(!error);
}

/* process an interrupt generated by clock_generate_interrupt 
 * returns True if it was the generated interrupt, 
 * False if it was a different interrupt 
 */
int
clock_process_interrupt(void)
{
    return device_interrupt(timer_device.di, CLOCK_IRQ);
}

/*
 * Wait the given number of ticks for an interrupt. If the timeout was too
 * short such that we didn't need to receive an interrupt (because our setup
 * time exceeded the wait time), 'interrupt_received' will be 0. Otherwise,
 * it will be 1.
 */
static void
clock_pause(uint64_t ticks, int *interrupt_received)
{
    /* Wait for the timeout. */
    uint64_t time = timer_get_ticks(timer_device.ti);
    int already_passed = timer_timeout(timer_device.ti, time + ticks);
    if (already_passed) {
        /* If it happened while we were still setting up, just return. */
        *interrupt_received = 0;
        return;
    }

    /* Otherwise, wait for an interrupt. */
    while (!device_interrupt(timer_device.di, okn_syscall_interrupt_wait()));
    *interrupt_received = 1;
}

/* Calculate the maximum of two numbers. */
static int
max(int x, int y)
{
    if (x > y) {
        return x;
    }
    return y;
}

void
clock_test(void)
{
    int error;
    int border_timeout;

    error = okn_syscall_interrupt_register(CLOCK_IRQ);
    assert(!error);

    /* Find the border value that is the first value of timeout that will
     * actually generate an interrupt. */
    for (border_timeout = 1;; border_timeout++) {
        int interrupt_received;
        clock_pause(border_timeout, &interrupt_received);
        if (interrupt_received) {
            break;
        }
    }

    /* Test the range just above the border which is where most timeout
     * problems are. */
    for (int k = 0; k < 10; k++) {
        for (int timeout = max(1, border_timeout - 10);
                timeout < 3 * border_timeout + 10;
                timeout++) {
            for (int j = 0; j < 10; j++) {
                int interrupt_received;
                clock_pause(timeout, &interrupt_received);
            }
        }
    }

    error = okn_syscall_interrupt_deregister(CLOCK_IRQ);
    assert(!error);
}
