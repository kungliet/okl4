/*
 * Copyright (c) 2005, National ICT Australia
 */
/*
 * Copyright (c) 2007 Open Kernel Labs, Inc. (Copyright Holder).
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
 * The S3C2410 has 5 timers. We use timer 1 in this driver.
 */

/*
 * BUYER BEWARE:
 *
 *     Because we don't have access to pending interrupts, this driver is
 *     racey. It may lose time, make time go backwards or fire an interrupt too
 *     soon or too late. It has been written to avoid losing interrupts,
 *     though.
 */

/* Timer specific section */
#include "s3c2410.h"
#include "s3c2410_timer.h"
#include <l4/kdebug.h>

#define TIME_MAX 0xFFFF

#if 0
#define dprintf(x...) printf(x)
#else
#define dprintf(x...)
#endif

static void
disable_timeout(struct s3c2410_timer *device)
{
    tcon_set_timer1_enable(0x0);
    tcon_set_timer2_enable(0x0);
}

static void
set_timeout(struct s3c2410_timer *device, uint16_t timeout)
{
    /* Setup the new timeout. */
    tcntb1_write(timeout);

    /* Reset the timer so that the value in the timer count buffer
     * gets loaded into the main count. */
    tcon_set_timer1_manual_update(1);
    tcon_set_timer1_enable(0);
    tcon_set_timer1_enable(1);
    tcon_set_timer1_manual_update(0);

    /* It should automatically reload to TIME_MAX next time. */
    tcntb1_write(TIME_MAX);
    tcon_set_timer1_autoreload(1);
}

static int
device_setup_impl(struct device_interface *di, struct s3c2410_timer *device,
                  struct resource *resources)
{
    int i, n_mem = 0;
    for (i = 0; i < 8; i++) {
        switch (resources->type) {
        case MEMORY_RESOURCE:
            assert(n_mem == 0);
            n_mem++;
            device->main = *resources;
            break;

        case INTERRUPT_RESOURCE:
        case BUS_RESOURCE:
        case NO_RESOURCE:
            /* do nothing */
            break;

        default:
            assert(0);
            break;
        }
        resources++;
    }

    device->timer.device = device;
    device->timer.ops = timer_ops;
    device->ticks = 0;

    // timers->tcfg0 = 0x0707; // 1/8 to match pistachio
    tcfg0_write(0x0707);
    tcfg1_set_mux1(0x00);
    tcfg1_set_mux2(0x00);

    device->state = STATE_DISABLED;

    return DEVICE_SUCCESS;
}

static int
device_enable_impl(struct device_interface *di, struct s3c2410_timer *device)
{
    /*
     * The interrupt shall be automatically enabled because the
     * the set_timeout() function shall start the timer. -gl
     */
    device->state = STATE_TIMEKEEP;
    device->remaining_ticks = 0;
    device->pending_timeout = 0;

    set_timeout(device, TIME_MAX);
    return DEVICE_ENABLED;
}

static int
device_disable_impl(struct device_interface *di, struct s3c2410_timer *device)
{
    device->state = STATE_DISABLED;
    disable_timeout(device);
    return DEVICE_DISABLED;
}

static uint64_t
timer_set_tick_frequency_impl(struct timer_interface *ti,
                              struct s3c2410_timer *device,
                              uint64_t hz)
{
    return TIMER_RATE;
}

static uint64_t
timer_get_tick_frequency_impl(struct timer_interface *ti,
                              struct s3c2410_timer *device)
{
    return TIMER_RATE;
}

static uint64_t
timer_get_ticks_impl(struct timer_interface *ti, struct s3c2410_timer *device)
{
    uint32_t ticks, read;
    read = tcnto1_read();
    ticks = TIME_MAX - read;

    return (device->ticks + ticks);
}

static int
timer_timeout_impl(struct timer_interface *ti, struct s3c2410_timer *device,
                   uint64_t timeout)
{
    int64_t u6 = timer_get_ticks_impl(ti, device);
    int64_t next = timeout - u6;

    /* The timeout has already expired. We make an assumption here that the
     * 64-bit ticks counter will never overflow: This would take 584 years,
     * assuming a 1GHz clock. */
    if (next < 0) {
        return 1;
    }

    if (next < TIME_MAX) {
        /* Account for the number of ticks already passed. */
        device->ticks += TIME_MAX - tcnto1_read();

        /* Set the timer to fire in the correct amount of time. */
        set_timeout(device, next);
    }

    /* Keep track of the number of ticks to go. */
    device->remaining_ticks = next;
    device->pending_timeout = 1;

    return 0;
}

static void
timer_set_ticks_impl(struct timer_interface *ti, struct s3c2410_timer *device,
                     uint64_t ticks)
{
    device->ticks = ticks;
}

static int
device_poll_impl(struct device_interface *di, struct s3c2410_timer *device)
{
    return 0;
}

static int
device_interrupt_impl(struct device_interface *di, 
                      struct s3c2410_timer *device,
                      int irq)
{
    /* If we don't have a pending timeout, and the clock is just cycling
     * through its entire TIME_MAX period, we are done. */
    if (!device->pending_timeout) {
        device->ticks += TIME_MAX;
        return 0;
    }

    /* Was the timer just set to a small timeout value? This happens
     * when our next interrupt is due before the timer next overflows. */
    if (device->pending_timeout && device->remaining_ticks <= TIME_MAX) {
        device->ticks += device->remaining_ticks;
        device->remaining_ticks = 0;
        device->pending_timeout = 0;

        /* Reprogram the clock back to standard time-keeping mode. */
        set_timeout(device, TIME_MAX);
        return 1;
    }

    /* Otherwise, the clock would have gone through the entire TIME_MAX period
     * of time, but we are not finished yet. */
    device->remaining_ticks -= TIME_MAX;
    device->ticks += TIME_MAX;

    /* If it is going go elapse before the next overflow, program the clock
     * to go short. */
    if (device->remaining_ticks <= TIME_MAX) {
        set_timeout(device, TIME_MAX - device->remaining_ticks);
        return 0;
    }

    /* Otherwise, it should just go through the entire cycle, and we leave it
     * untouched.  */
    return 0;
}

/* XXX support multiple timer source? -gl */
static int
device_num_interfaces_impl(struct device_interface *di, 
                           struct s3c2410_timer *dev)
{
    return 1;
}

static struct generic_interface *
device_get_interface_impl(struct device_interface *di,
                          struct s3c2410_timer *device,
                          int interface)
{
    return (struct generic_interface *)(void *)&device->timer;
}
