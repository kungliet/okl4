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

#include "uart.h"
#include <l4/kdebug.h>
#include <util/trace.h>

#define UART0_REF_CLK 48000000
#define BAUD_RATE 115200
#define DATA_SIZE 8

#if 0
static void
dprintf(const char *format, ...)
{
    int ret;
    va_list ap;
    char buf[256];
    char *p = buf;

    va_start(ap, format);
    ret = vsprintf(buf, format, ap);
    va_end(ap);

    while(*p) L4_KDB_PrintChar(*p++);
}
#else
#define dprintf(arg...)
#endif

/* fw decl */
static void do_xmit_work(struct stn8815_uart *device);
static int do_rx_work(struct stn8815_uart *device);


static int
device_setup_impl(struct device_interface *di, struct stn8815_uart *device,
                  struct resource *resources)
{
    int n_mem = 0;
    for (; resources; resources = resources->next) {
        switch (resources->type) {
        case MEMORY_RESOURCE:
            if (n_mem == 0)
                device->main = *resources;
            else
                printf("stn8815_uart: got more memory than expected!\n");
            n_mem++;
            break;

        case INTERRUPT_RESOURCE:
        case BUS_RESOURCE:
            /* do nothing */
            break;

        default:
            printf("stn8815_uart: Invalid resource type %d!\n", resources->type);
            break;
        }

        if (n_mem)
            break;
    }

    if (!resources)
        return 0;

    device->tx.device = device;
    device->rx.device = device;
    device->tx.ops = stream_ops;
    device->rx.ops = stream_ops;

    /* disable uart */
    uartcr_set_uarten(0);

    {
        uint32_t tmp = 16 * BAUD_RATE;
        uint32_t divint = UART0_REF_CLK / tmp;
        uint32_t remainder = UART0_REF_CLK % tmp;
        uint32_t divfrac;
        tmp = (DATA_SIZE * remainder) / BAUD_RATE;
        divfrac = (tmp >> 1) + (tmp & 1);

        uartibrd_set_baud_divint( divint );
        uartfbrd_set_baud_divfrac ( divfrac );
        uartlcr_h_set_wlen(3); /* word length: 8 */
    }

    /*
     * TX interrupt when FIFO becomes <= 1/64
     * RX interrupt when FIFO becomes >= 1/64
     */
    uartifls_set_txiflsel(0);
    uartifls_set_rxiflsel(0);

    uartlcr_h_set_fen(1);
    uartcr_set_rxe(1);
    uartcr_set_txe(1);


    /* disable TX interrupt. It will be enabled after full */
    uartimsc_set_txim(0);
    /* enable RX and RT interrupt */
    uartimsc_set_rxim(1);
    uartimsc_set_rtim(1);

    /* enable uart */
    uartcr_set_uarten(1);

    return DEVICE_SUCCESS;
}


static int
device_enable_impl(struct device_interface *di, struct stn8815_uart *device)
{
    device->state = STATE_ENABLED;
    dprintf("%s\n", __func__);

    return DEVICE_ENABLED;
}

static int
device_disable_impl(struct device_interface *di, struct stn8815_uart *device)
{
    device->state = STATE_DISABLED;
    dprintf("%s\n", __func__);

    return DEVICE_DISABLED;
}

static int
device_poll_impl(struct device_interface *di, struct stn8815_uart *device)
{
    dprintf("%s\n", __func__);

    return device_interrupt_impl(di, device, -1);
}

static int
device_num_interfaces_impl(struct device_interface *di, 
                           struct stn8815_uart *device)
{
    dprintf("%s\n", __func__);

    return 2;
}

static struct generic_interface *
device_get_interface_impl(struct device_interface *di, 
                          struct stn8815_uart *device, int i)
{
    switch (i) {
        case 0:
            return (struct generic_interface *)((void *)&device->tx);
        case 1:
            return (struct generic_interface *)((void *)&device->rx);
        default:
            return NULL;
    }
}

static int
device_interrupt_impl(struct device_interface *di, 
                      struct stn8815_uart *device,
                      int irq)
{
    int status = 0;

    // TX
    if (uartmis_get_txmis())
    {
        uartimsc_set_txim(0); // disable TX intr
        do_xmit_work(device);
    }

    // RX
    if (uartmis_get_rtmis() || uartmis_get_rxmis()) {
        uartimsc_set_rxim(0); // disable RX intr
        status = do_rx_work(device);
    }

    return status;
}

static void
do_xmit_work(struct stn8815_uart *device)
{
    struct stream_interface *si = &device->tx;
    struct stream_pkt *packet = stream_get_head(si);

    if (packet == NULL)
        return;

    while (!uartfr_get_txff()) {
        assert(packet->data);
        dprintf("TX %d\n",
                packet->data[packet->xferred]);
        uartdr_set_data(packet->data[packet->xferred++]);
        if (packet->xferred == packet->length) {
            /* Finished this packet */
            packet = stream_switch_head(si);
            if (packet == NULL)
                break;
        }
    }
    if (uartfr_get_txff()) {
        /* make TX intr to occur when TX fifo become empty */
        uartimsc_set_txim(1);
    }
}

static int
do_rx_work(struct stn8815_uart *device)
{
    struct stream_interface *si = &device->rx;
    struct stream_pkt *packet = stream_get_head(si);
    uint16_t data;

    // while read fifo is non-empty
    while (!uartfr_get_rxfe())
    {
        data = uartdr_get_data();
        if ((data & 0xff) == 0xb) { /* Ctrl-k */
            L4_KDB_Enter("breakin");
            continue;
        }
        if (!packet) continue;
        packet->data[packet->xferred++] = (data & 0xff);
        if (packet->xferred == packet->length)
        {
            packet = stream_switch_head(si);
            if (packet == NULL) 
            {
                /*
                 * Empty the FIFO by dropping the characters if we
                 * have space to put them.
                 */
                while (!uartfr_get_rxfe())
                {
                    if ((uartdr_get_data() & 0xff) == 0xb)
                         L4_KDB_Enter("breakin");
                }
                break;
            }
        }
    }

    if (packet) {
        uartimsc_set_rxim(1);   /* restart RX intr */
        if (packet->xferred)
            packet = stream_switch_head(si);
    }

    dprintf("%s: done.\n", __func__);

    return 0;
}

static int
stream_sync_impl(struct stream_interface *si, struct stn8815_uart *device)
{
    int retval = 0;

    dprintf("%s: stream %p dev %p\n", __func__, si, device);

    if (si == &device->tx)
        do_xmit_work(device);
    if (si == &device->rx)
        retval = do_rx_work(device);

    dprintf("%s: done.\n", __func__);

    return retval;
}
