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

#include <stdlib.h>
#include <string.h>
#include <okl4/env.h>
#include <serial/serial.h>

static int8_t serial_init_result = SERIAL_INIT_UNINITIALISED;

struct serial_device serial_device;
/* Hardware resources.*/
struct resource      resources[8];
/* Data packets passed to and from driver. */
struct stream_pkt    stream_packet[NUM_PACKETS];

/* Pop a packet off the free list for the underlying device */
struct stream_pkt *
do_alloc_packet(struct serial_device *serial_device)
{
    struct stream_pkt *retval;
    if (serial_device->freelist == NULL) {
        return NULL;
    }
    retval = serial_device->freelist;
    serial_device->freelist = retval->next;
    return retval;
}

int
free_packets(struct serial_device *serial_device,
        struct stream_interface *stream_interface)
{
    int count = 0;

    while (stream_interface->completed_start) {
        struct stream_pkt *packet;
        struct stream_pkt *next;

        packet = stream_interface->completed_start;
        next = packet->next;

        /* Add to the freelist */
        packet->next = serial_device->freelist;
        serial_device->freelist = packet;

        stream_interface->completed_start = next;
        count++;
    }
    stream_interface->completed_end = NULL;

    return count;
}

size_t
serial_write(const void *data, long int position, size_t count, void *handle)
{
    struct stream_pkt *packet;
    char *cr_data;
    size_t cr_count;
    int cr_added = 0;

    free_packets(&serial_device, serial_device.tx);

    /* Obtain a packet from the free list. */
    packet = do_alloc_packet(&serial_device);
    if (packet == NULL) {
        return 0;
    }

    /* Add carriage return if the last character is '\n' */
    if (((char *)data)[count - 1] == '\n') {
        cr_count = count + 1;
        cr_data = malloc(cr_count);
        if (cr_data == NULL) {
            return 0;
        }
        memcpy(cr_data, data, count);
        cr_data[cr_count - 2] = '\r';
        cr_data[cr_count - 1] = '\n';
        cr_added = 1;
    }
    else {
        cr_count = count;
        cr_data = (char *)data;
    }

    /* Set the attributes of this packet. */
    packet->data = (uint8_t *)cr_data;
    packet->length = cr_count;
    packet->xferred = 0;
    packet->ref = NULL;

    stream_add_stream_pkt(serial_device.tx, packet);

    /* poll device until all data is sent. */
    while ((stream_get_head(serial_device.tx) != NULL)) {
        device_poll(serial_device.di);
    }

    /* If we allocated cr_data then free */
    if (cr_added) {
        free(cr_data);
    }

    return count;
}

size_t
serial_read(void *data, long int position, size_t count, void *handle)
{
    struct stream_pkt *packet;

    free_packets(&serial_device, serial_device.rx);

    packet = do_alloc_packet(&serial_device);
    if (packet == NULL) {
        return 0;
    }

    packet->data = (uint8_t *)data;
    packet->length = count;
    packet->xferred = 0;
    packet->ref = NULL;

    stream_add_stream_pkt(serial_device.rx, packet);

    /* poll device until we read a packet. */
    while ((stream_get_head(serial_device.rx) == NULL)) {
        device_poll(serial_device.di);
    }

    return count;
}

int
resource_init(void)
{
    int r = 0;
    okl4_env_segment_t *serial_seg;

    resources[r].type = MEMORY_RESOURCE;
    serial_seg = okl4_env_get_segment("MAIN_SERIAL_MEM0");

    if (serial_seg) {
        resources[r++].resource.mem = (mem_space_t)serial_seg->virt_addr;
    }
    /* Request interrupt? ... no, polling implementation. */

    for (; r < 8; r++) {
        resources[r].type = NO_RESOURCE;
    }

    return 0;
}

int
serial_init(void)
{
    void *device = NULL;
    int i;

    /* If already initialized, return immediately */
    if (serial_init_result != SERIAL_INIT_UNINITIALISED) {
        return serial_init_result;
    }

    if (resource_init() != 0) {
        /* Error. */
        return (serial_init_result = 1);
    }

    device = malloc(SERIAL_DRIVER.size);
    if (device == NULL) {
        return (serial_init_result = 1);
    }
    memset(device, '\0', SERIAL_DRIVER.size);

    /* Initialise the device state with resources, device instance, tx and rx
     * stream interfaces. */
    serial_device.resources = resources;
    serial_device.di = setup_device_instance(&SERIAL_DRIVER, device);
    if (device_setup(serial_device.di, serial_device.resources)
            != DEVICE_SUCCESS) {
        return (serial_init_result = 1);
    }

    serial_device.tx = (struct stream_interface *)
                            device_get_interface(serial_device.di, 0);
    serial_device.rx = (struct stream_interface *)
                            device_get_interface(serial_device.di, 1);
    if (serial_device.tx == NULL || serial_device.rx == NULL) {
        return (serial_init_result = 1);
    }
    if (device_enable(serial_device.di) != DEVICE_ENABLED) {
        return (serial_init_result = 1);
    }

    /* Create a free list of packets that we can send to the device */
    serial_device.freelist = stream_packet;
    for (i = 0; i < NUM_PACKETS - 1; i++) {
        serial_device.freelist[i].next = &serial_device.freelist[i+1];
    }
    serial_device.freelist[i].next = NULL;


    return (serial_init_result = 0);
}

