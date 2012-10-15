/*
 * Copyright (c) 2005, National ICT Australia
 */
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
 * We use TIMER2_3 since L4 uses TIMER0_1
 */

#include "sp804_timer.h"

#if 0
#define dprintf(...) printf (__VA_ARGS__)
#else
#define dprintf(...)
#endif


#define SP804_TIMER_RATE        1000000

#define SP804_CTRL_16BIT        0
#define SP804_CTRL_32BIT        1

#define SP804_CTRL_DIV1         0
#define SP804_CTRL_DIV16        1
#define SP804_CTRL_DIV256       2

/*
 * SP804 supports full 32 bits for initial timer value but we use only
 * half of that in order to simply solve the wrap-around problem.
 *
 * If 'tload' was set to 0xFFFFFFFF and get_ticks_impl() is called
 * just before it becomes 0, the current timer value register can
 * have, for example, 0xFFFFFFFE after wrap-around because timer is
 * free-running.  Here we can not tell it's just 1 tick elapsed or
 * 0xFFFFFFFF + 1 ticks elapsed. If we use only half of the 0xFFFFFFFF
 * as maximum value of 'tload', we can always tell the difference.
 */
#define TIME_MAX (0xFFFFFFFF/2)

static void
set_timeout(struct sp804_timer *device, uint32_t timeout)
{
    device->count_from = timeout;
    tload_write(timeout);
}

static void
update_ticks(struct sp804_timer *device)
{
    uint32_t tick = tvalue_read();

    /* 'count_from - tick' always returns correct value even if there
     * was a wrap-around. For example, if count_from was 0x1 and tick
     * becomes 0xffffffff, 'count_from - tick' becomes 2 as we
     * expected.
     */
    device->ticks += device->count_from - tick;
    device->count_from = tick;
}

static uint64_t
timer_get_ticks_impl(struct timer_interface *ti, struct sp804_timer *device)
{
    update_ticks(device);
    dprintf("get_ticks() : tick = %lld\n", device->ticks);
    return device->ticks;
}

static int
timer_timeout_impl(struct timer_interface *ti, struct sp804_timer *device,
                   uint64_t timeout)
{
    int64_t remaining_ticks;
    update_ticks(device);
    remaining_ticks = timeout - device->ticks;
    if (ti) dprintf("timeout() B: tick = %lld, timeout = %lld(%lld)\n",
                    device->ticks, timeout, remaining_ticks);
    if (remaining_ticks <= 0) {
        /* goto free-running timer phase*/
        device->timeout = 0;
        set_timeout(device, TIME_MAX);
        dprintf("expired\n");
        return 1;
    }
    set_timeout(device,
                (remaining_ticks > TIME_MAX)? TIME_MAX : remaining_ticks);
    device->timeout = timeout;
    return 0;
}

static void
timer_set_ticks_impl(struct timer_interface *ti, struct sp804_timer *device,
                     uint64_t ticks)
{
    device->ticks = ticks;
}

static uint64_t
timer_set_tick_frequency_impl(struct timer_interface *ti,
                              struct sp804_timer *device,
                              uint64_t hz)
{
    return SP804_TIMER_RATE;
}

static uint64_t
timer_get_tick_frequency_impl(struct timer_interface *ti,
                              struct sp804_timer *device)
{
    return SP804_TIMER_RATE;
}


static int
device_interrupt_impl(struct device_interface *di,
                      struct sp804_timer *device,
                      int irq)
{
    int ret = 0;

    if (di) dprintf("interrupt() : timeout = %lld\n", device->timeout);
    if (device->timeout) {
        ret = timer_timeout_impl(NULL, device, device->timeout);
    } else {
        update_ticks(device);
        set_timeout(device, TIME_MAX);
    }

    /* ack the interrupt */
    tintclr_write(0x0);
    return ret;
}

static int
device_setup_impl(struct device_interface *di, struct sp804_timer *device,
                  struct resource *resources)
{
    int i, n_mem = 0;
    for (i=0 ; i<8; i++) {
        switch (resources->type) {
        case MEMORY_RESOURCE:
            if (n_mem == 0)
                device->main = *resources;
            else
                printf("sp804_timer: got more memory than expected!\n");
            n_mem++;
            break;
        case INTERRUPT_RESOURCE:
        case BUS_RESOURCE:
        case NO_RESOURCE:
            /* do nothing */
            break;
        default:
            printf("sp804_timer: Invalid resource type %d!\n", resources->type);
            break;
        }
        resources++;

    }

    device->timer.device = device;
    device->timer.ops = timer_ops;
    device->ticks = 0;

    tload_write(TIME_MAX);
    tcontrol_set_oneshot(0);
    tcontrol_set_size(SP804_CTRL_32BIT);
    tcontrol_set_prescale(SP804_CTRL_DIV1);
    tcontrol_set_periodic(0);
    tcontrol_set_intenable(1);
    tcontrol_set_enable(0);
    device->state = STATE_DISABLED;
    return DEVICE_SUCCESS;
}

static int
device_enable_impl(struct device_interface *di, struct sp804_timer *device)
{
    /*
     * The interrupt shall be automatically enabled because the
     * the set_timeout() function shall start the timer. -gl
     */
    device->state = STATE_TIMEKEEP;
    set_timeout(device, TIME_MAX);
    device->timeout = 0;
    tcontrol_set_enable(1);
    return DEVICE_ENABLED;
}

static int
device_disable_impl(struct device_interface *di, struct sp804_timer *device)
{
    device->state = STATE_DISABLED;
    tcontrol_set_enable(0);
    return DEVICE_DISABLED;
}

static int
device_poll_impl(struct device_interface *di, struct sp804_timer *device)
{
    return 0;
}

static int
device_num_interfaces_impl(struct device_interface *di,
                           struct sp804_timer *dev)
{
    return 1;
}

static struct generic_interface *
device_get_interface_impl(struct device_interface *di,
                          struct sp804_timer *device,
                          int interface)
{
    return (struct generic_interface *)(void *)&device->timer;
}
