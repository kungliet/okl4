/*
 * Copyright (c) 2004, National ICT Australia
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

/* Timer specific section */
#include "pxa250_timer.h"
#include "pxa250.h"

#define TIME_MAX 0xffffffff

/* We set the initial clock value to wrap quite quickly to
 * catch overflow errors early on. */
#define CLOCK_START_VALUE  ((uint32_t)-1000)

static int
device_setup_impl(struct device_interface *di, struct pxa250_timer *device,
                          struct resource *resources)
{
    int i, n_mem = 0;
    for (i = 0; i < 8; i++)
    {
        switch (resources->type)
        {
        case MEMORY_RESOURCE:
            if (n_mem == 0)
                device->main = *resources;
            else
                printf("pxa250_timer: got more memory than expected!\n");
            n_mem++;
            break;
            
        case INTERRUPT_RESOURCE:
        case BUS_RESOURCE:
        case NO_RESOURCE:
            /* do nothing */
            break;
            
        default:
            printf("pxa250_timer: Invalid resource type %d!\n", resources->type);
            break;
        }
        resources++;
    }
    
    device->timer.device = device;
    device->timer.ops = timer_ops;

    device->ticks = 0;


    device->state = STATE_DISABLED;
    return DEVICE_SUCCESS;
}

static inline uint32_t
set_timeout(struct pxa250_timer *device, uint32_t timeout)
{
    /* Write out the real value. */
    mr2_write(timeout);

    /* Turn on the interrupt. */
    ier_write(0x7);

     /*
      * BUG: A nasty bug on hardware requires us to wait at least 2 cycles for
      * the match register to update, whereas the simulator only requires 1
      * cycle. A hacky fix is to spin for the one extra cycle in the rare case
      * that we actually hit this.
      *
      * BUG: Testing on certain pieces of real hardware shows that sometimes
      * even 2 cycles is not sufficient. There does not appear do be any
      * documentation available describing this, so we wait for 3 cycles and
      * hope for the best...
      */
     while (timeout - cr_read() <= 3);

    /* Has it already passed without raising an interrupt? */
    return (timeout <= cr_read()) && ((sr_read() & 0x4) == 0);
}

static uint64_t next_timeout = 0;

static int
timer_timeout_impl(struct timer_interface *ti, struct pxa250_timer *device,
        uint64_t timeout)
{
    /* Delete any pending timeouts. */
    next_timeout = 0;

    /* Should we disable the timer? */
    if (timeout == -1) {
        ier_write(0x3);
        return 0;
    }

    /* Is the timeout in the past? */
    if (timeout < device->ticks) {
        ier_write(0x3);
        return 1;
    }

    /* Take away any ticks that have already passed. */
    timeout -= device->ticks;

    /* Is the timeout going to happen after the next clock-overflow
     * interrupt? */
    if (timeout > TIME_MAX) {
        next_timeout = timeout;
        return 0;
    }

    /* Setup the next timeout. */
    if (set_timeout(device, timeout)) {
        /* Expired already. Disable the timeout interrupt. */
        ier_write(0x3);
        return 1;
    }

    device->state = STATE_SHOT_SHORT;
    return 0;
}

static int
device_enable_impl(struct device_interface *di, struct pxa250_timer *device)
{
#if defined(NANOKERNEL)
    /*
     * Set match register in the past. Micro uses this as it's kernel timer
     * too, so we don't mess with it in that case.
     *
     * We need to keep writing it until the value latches in.
     */
    do {
        cr_write(CLOCK_START_VALUE);
    } while (cr_read() != CLOCK_START_VALUE);
#endif

    /* Timeout interrupt */
    mr2_write(0);

    /* Wrap interrupt */
    mr1_write(0);

    /* Acknowledge our two interrupts if they are already asserted. */
    sr_write(0x6);

    /* Enable wrap interrupt, disable timeout interrupt, leave kernel interrupt
     * enabled. */
    ier_write(0x3);

    device->state = STATE_TIMEKEEP;
    return DEVICE_ENABLED;
}

static int
device_disable_impl(struct device_interface *di, struct pxa250_timer *device)
{
    /* Disable the timer */
    ier_write(0x01);  /* leave micro's timer enabled */

    device->state = STATE_DISABLED;
    return DEVICE_DISABLED;
}

static uint64_t
timer_set_tick_frequency_impl(struct timer_interface *ti, struct pxa250_timer *device, uint64_t hertz)
{
    // Dont support changing frequency?
    return TIMER_RATE;
}

static uint64_t
timer_get_tick_frequency_impl(struct timer_interface *ti, struct pxa250_timer *device )
{
    return TIMER_RATE;
}


static uint64_t
timer_get_ticks_impl(struct timer_interface *ti, struct pxa250_timer *device)
{
    uint32_t ticks;

    do {
        /* Read the clock. */
        ticks = cr_read();

        /* If there is a pending interrupt, take that into account. */
        if (sr_read() & 0x2) {
            ticks += 0x100000000LL;
        }

        /* If we overflowed during that short amount of time, the clock value
         * may be incorrect. Restart from scratch. */
    } while (ticks > cr_read());

    return (device->ticks + ticks);
}

static void
timer_set_ticks_impl(struct timer_interface *ti, struct pxa250_timer *device, uint64_t ticks)
{
    uint32_t clock;
    uint64_t target;

    do {
        target = ticks;

        /* Read the clock. */
        clock = cr_read();

        /* If there is a pending interrupt, take that into account. */
        if (sr_read() & 0x2) {
            target -= 0x100000000LL;
        }

        /* If we overflowed during that short amount of time, the clock value
         * may be incorrect. Restart from scratch. */
    } while (clock > cr_read());

    device->ticks = target - clock;
}

static int
device_poll_impl(struct device_interface *di, struct pxa250_timer *device)
{
    return 0;
}

static int
device_interrupt_impl(struct device_interface *di, struct pxa250_timer *device, int irq)
{
    int ret = 0;
    uint32_t mask = sr_read();

    /* Overflow interrupt? */
    if (mask & 0x2){
        /* Increase our ticks. */
        device->ticks += 0x100000000LL;

        /* Ack the interrupt. */
        sr_write(0x2);

        /* If we have a pending timeout, see if that is going to fire
         * before the timer next overflows. */
        if (next_timeout) {
            ret = timer_timeout_impl(&device->timer, device, next_timeout);
        }
    }

    /* Timeout interrupt? */
    if (mask & 0x4){
        ret = 1;
        /* Turn off timeout interrupt. */
        ier_write(0x3);

        /* Ack. */
        sr_write(0x4);
    }

    return ret;
}

static int
device_num_interfaces_impl(struct device_interface *di, struct pxa250_timer *device)
{
    return 1;
}

static struct generic_interface *
device_get_interface_impl(struct device_interface *di,
                          struct pxa250_timer *device,
                          int interface)
{
    return (struct generic_interface *)(void *)&device->timer;
}

