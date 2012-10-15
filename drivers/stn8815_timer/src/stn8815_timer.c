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

#include "stn8815_timer.h"

#define TIMER_RATE 0

/*
 * This is a stub timer driver for the stn8815 as nobody currently
 * has any need for this driver.
 */
static int
device_setup_impl(struct device_interface *di, struct stn8815_timer *device,
                          struct resource *resources)
{
    int i, n_mem = 0;

    for (i = 0; i < 8; i++) {
        switch (resources->type) {
        case MEMORY_RESOURCE:
            if (n_mem == 0)
                device->main = *resources;
            else
                printf("stn8815_timer: got more memory than expected!\n");
            n_mem++;
            break;
            
        case INTERRUPT_RESOURCE:
        case BUS_RESOURCE:
        case NO_RESOURCE:
            /* do nothing */
            break;
            
        default:
            printf("stn8815_timer: Invalid resource type %d!\n", resources->type);
            break;
        }
        resources++;
    }
   
    printf("stn8815_timer: this is a stub driver.\n");
 
    device->timer.device = device;
    device->timer.ops = timer_ops;
    device->state = STATE_DISABLED;

    return DEVICE_ERROR;
}

static inline uint32_t
set_timeout(struct stn8815_timer *device, uint32_t timeout)
{
    return 0;
}

static int
timer_timeout_impl(struct timer_interface *ti, struct stn8815_timer *device,
        uint64_t timeout)
{
    return 0;
}

static int
device_enable_impl(struct device_interface *di, struct stn8815_timer *device)
{
    device->state = STATE_TIMEKEEP;
    return DEVICE_ENABLED;
}

static int
device_disable_impl(struct device_interface *di, struct stn8815_timer *device)
{
    device->state = STATE_DISABLED;
    return DEVICE_DISABLED;
}

static uint64_t
timer_set_tick_frequency_impl(struct timer_interface *ti, struct stn8815_timer *device, uint64_t hertz)
{
    // Dont support changing frequency?
    return TIMER_RATE;
}

static uint64_t
timer_get_tick_frequency_impl(struct timer_interface *ti, struct stn8815_timer *device )
{
    return TIMER_RATE;
}


static uint64_t
timer_get_ticks_impl(struct timer_interface *ti, struct stn8815_timer *device)
{
    return 0;
}

static void
timer_set_ticks_impl(struct timer_interface *ti, struct stn8815_timer *device, uint64_t ticks)
{
}

static int
device_poll_impl(struct device_interface *di, struct stn8815_timer *device)
{
    return 0;
}

static int
device_interrupt_impl(struct device_interface *di, struct stn8815_timer *device, int irq)
{
    return 0;
}

static int
device_num_interfaces_impl(struct device_interface *di, struct stn8815_timer *device)
{
    return 1;
}

static struct generic_interface *
device_get_interface_impl(struct device_interface *di,
                          struct stn8815_timer *device,
                          int interface)
{
    return (struct generic_interface *)(void *)&device->timer;
}

