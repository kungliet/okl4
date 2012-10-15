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
 * Authors: Carl van Schaik, Geoffrey Lee, Robert Sison
 */

#include "uart.h"
#if !defined(NANOKERNEL)
#include <l4/kdebug.h>
#endif
#include <util/trace.h>

enum parity { PARITY_NONE, PARITY_EVEN, PARITY_ODD /* xxx mark/space */};
//#define BAUD_RATE_CONSTANT 3686400 /* Frequency of the crystal on the SA1100 */

#define DEFAULT_BAUD 115200
#define DEFAULT_SIZE 8
#define DEFAULT_PARITY PARITY_NONE
#define DEFAULT_STOP 1

#define UARTICR_TXIC_MASK 32 // bit 5
#define UARTICR_RTIC_MASK 64 // bit 6

//#define TRACE_UART printf
#define TRACE_UART(x...)

#if 0
#define dprintf(arg...) printf(arg)
#else
#define dprintf(arg...) do { } while (0/*CONSTCOND*/);
#endif

/* fw decl */
static int serial_set_params(struct pl011_uart_v2 *device, 
                  unsigned baud,
                  int data_size,
                  enum parity parity,
                  int stop_bits);
static void do_xmit_work(struct pl011_uart_v2 *device);
static int do_rx_work(struct pl011_uart_v2 *device);


static int
device_setup_impl(struct device_interface *di, struct pl011_uart_v2 *device,
                  struct resource *resources)
{
    int n_mem = 0;
    for (; resources; resources = resources->next) {
        switch (resources->type) {
        case MEMORY_RESOURCE:
            if (n_mem == 0)
                device->main = *resources;
            else
                printf("pl011_uart_v2: got more memory than expected!\n");
            n_mem++;
            break;
            
        case INTERRUPT_RESOURCE:
        case BUS_RESOURCE:
            /* do nothing */
            break;
            
        default:
            printf("pl011_uart_v2: Invalid resource type %d!\n", resources->type);
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

    device->txen = 0;
    device->writec = 0;

    device->rxen = 0;
    device->readc = 0;

    // Disable UART0
    uartcr_set_uarten(0);

    if (serial_set_params(device, DEFAULT_BAUD, DEFAULT_SIZE, DEFAULT_PARITY,
        DEFAULT_STOP) != 0)
        return 0;

    // Transmit interrupt when FIFO becomes <= 1/8 full
    uartifls_set_txiflsel(0);

    uartlcr_h_set_fen(1);   // Enable FIFOs
    uarticr_write(UARTICR_TXIC_MASK);    // Clear transmit interrupt
    uartcr_set_rxe(1);      // Receive enable
    //uartcr_set_txe(1);    // Transmit enable

    // Enable UART0
    uartcr_set_uarten(1);

    // Unmask Interrupts
    TRACE_UART("%s: Transmit Set Mask curr:%d\n", __func__, uartmis_get_txmis());
    uartimsc_set_txim(1);   // Transmit Interrupt Unmask (?)
    uartimsc_set_rtim(1);   // Receive timeout interrupt mask

    device->fifodepth = 1;    /* set fifodeth of 1 to start with */

    dprintf("%s: done\n", __func__);
    return DEVICE_SUCCESS;
}

#define UART0_REF_CLK 24000000

static int
serial_set_params(struct pl011_uart_v2 *device, 
                  unsigned baud,
                  int data_size,
                  enum parity parity,
                  int stop_bits)
{
#if 0
    uint32_t tmp;
    uint32_t divint;
    uint32_t remainder;
    uint32_t divfrac;
#endif

    if( data_size < 7 || data_size > 8 )
    {
        /* Invalid data_size */
        return -1;
    }

    if( stop_bits < 1 || stop_bits > 2 )
    {
        /* Invalid # of stop bits */
        return -1;
    }

#if 0
    /* Disable high baud rate for regression testing - philipo */
    /* Set baud rate */
    tmp = 16 * baud;
    divint = UART0_REF_CLK / tmp;
    remainder = UART0_REF_CLK % tmp;
    tmp = (data_size * remainder) / baud;
    divfrac = (tmp >> 1) + (tmp & 1);

    //printf("%s: divint:%d divfrac:%d\n", __func__, divint, divfrac);
    uartibrd_set_baud_divint( divint );
    uartfbrd_set_baud_divfrac ( divfrac );
#endif

    uartlcr_h_set_wlen(3); /* word length: 8 */

    return 0;
}

static int
device_enable_impl(struct device_interface *di, struct pl011_uart_v2 *device)
{
    device->state = STATE_ENABLED;
    dprintf("%s: done\n", __func__);

    return DEVICE_ENABLED;
}

static int
device_disable_impl(struct device_interface *di, struct pl011_uart_v2 *device)
{
    device->state = STATE_DISABLED;
    dprintf("%s: done\n", __func__);

    return DEVICE_DISABLED;
}

static int
device_poll_impl(struct device_interface *di, struct pl011_uart_v2 *device)
{
    dprintf("%s: called.\n", __func__);
    return 0;
}

/*
 * XXX clagged from imx21 serial driver: should be autogenerated.
 */
static int
device_num_interfaces_impl(struct device_interface *di, 
                           struct pl011_uart_v2 *device)
{
    dprintf("%s: called.\n", __func__);

    return 2;    /* XXX: in reality there are more UARTs than this. -gl */
}

/*
 * XXX clagged from imx21 serial driver: should be autogenerated.
 */
static struct generic_interface *
device_get_interface_impl(struct device_interface *di, 
                          struct pl011_uart_v2 *device, int i)
{
    switch (i) {
        case 0:
            return (struct generic_interface *)(void *)&device->tx;
        case 1:
            return (struct generic_interface *)(void *)&device->rx;
        default:
            return NULL;
    }
}


static int
device_interrupt_impl(struct device_interface *di, 
                      struct pl011_uart_v2 *device,
                      int irq)
{
    int status = 0;

    dprintf("%s: in interrupt.\n", __func__);

    /*
     * Interrupt handling.  The TX interrupt means TX done so we check
     * to see if the TX buffer is empty.  If so we do xmit.
     *
     * The RX interrupt means there is data in the receive buffer so we
     * read it off and pass it to the user.
     *
     * -gl
     */

    // If a transmit interrupt
    if ( uartmis_get_txmis() )
    {
        uartimsc_set_txim(0); // Transmit interrupt unmask
        uarticr_write(UARTICR_TXIC_MASK);  // Clear transmit interrupt
        do_xmit_work(device);
    }

    // Else if a receive timeout interrupt & the receive FIFO is non-empty
    else if ( uartmis_get_rtmis() && !uartfr_get_rxfe() )
    {
        uarticr_write(UARTICR_RTIC_MASK);  // Clear receive interrupt
        status = do_rx_work(device);
    }

    dprintf("%s: interrupt done status %d.\n", __func__, status);
    return status;
}

static void
do_xmit_work(struct pl011_uart_v2 *device)
{
    struct stream_interface *si = &device->tx;
    struct stream_pkt *packet = stream_get_head(si);

    dprintf("%s: called  FIFO depth = %d.\n", __func__, device->fifodepth);

    if (packet == NULL)
        return;

    while (1) {
        device->fifodepth--;
        /* Place character on the UART FIFO */
        assert(packet->data);
        dprintf("Transmitting character ASCII %d\n",
            packet->data[packet->xferred]);

        // Turn on the transmitter
        uartcr_set_txe(1);

        uartdr_set_data(packet->data[packet->xferred++]);
        while (uartfr_get_busy());    // XXX -gl
        assert(packet->xferred <= packet->length);
        if (packet->xferred == packet->length) {
            /* Finished this packet */
            packet = stream_switch_head(si);
            if (packet == NULL)
                break;
        }
    }

    uartcr_set_txe(0);

    dprintf("%s: done.\n", __func__);
}

static int
do_rx_work(struct pl011_uart_v2 *device)
{
    struct stream_interface *si = &device->rx;
    struct stream_pkt *packet = stream_get_head(si);
    uint16_t data;

    // while read fifo is non-empty
    while ( !uartfr_get_rxfe() )
    {
        data = uartdr_get_data();
#if !defined(NANOKERNEL)
        if ((data & 0xff) == 0xb) { /* Ctrl-k */
            L4_KDB_Enter("breakin");
            continue;
        }
#endif
        if (!packet) continue;
        packet->data[packet->xferred++] = (data & 0xff);
        if (packet->xferred == packet->length)
        {
            if ((packet = stream_switch_head(si)) == NULL)
            {
                /*
                 * Empty the FIFO by dropping the characters if we
                 * have space to put them.
                 */
#if !defined(NANOKERNEL)
                while ( !uartfr_get_rxfe() )
                {
#if !defined(NANOKERNEL)
                    if ((uartdr_get_data() & 0xff) == 0xb) {
                         L4_KDB_Enter("breakin");
                    }
#endif
                }
                break;
#endif
            }
        }
    }

    if (packet) {
        if (packet->xferred)
            packet = stream_switch_head(si);
    }

    dprintf("%s: done.\n", __func__);

    return 0;
}

static int
stream_sync_impl(struct stream_interface *si, struct pl011_uart_v2 *device)
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
