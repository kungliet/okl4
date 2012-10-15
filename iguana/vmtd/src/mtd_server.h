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

#include <mutex/mutex.h>
#ifndef _MTD_SERVER_H
#define _MTD_SERVER_H

struct virtual_mtd;

#define TIMER_MASK    0x8000
typedef void (*tm_callback_t)(struct device_interface *);
void timeout_request(struct device_interface *device, uintptr_t nanosecs, tm_callback_t call);

/**
 * @brief The struct describing the actual device, from the perspective of a 
 * the server.  One per physical device
 */
struct mtd_device {
    struct mtd_device           *next;
    struct resource *resources;
    struct device_interface     *dev_if; /**< Generic device interface */
    struct cmd_interface        *cmd_if;
    struct mtd_interface        *mtd_if;

    struct virtual_mtd          *client_list;

    uint32_t irq;
    /* timer interface */
    objref_t                 timer_device;
    L4_ThreadId_t            timer_server, device_core;
    tm_callback_t            timer_callback;

    struct mtd_info info;
    uint32_t partitions;    // XXX <= 32
    uint32_t part_mask;     // bitmask of available partitions
};


struct memsect_attach {
    void *base;
    uintptr_t size;
};

struct client_cmd_packet;

struct control_block {
    uintptr_t   status;

    struct mtd_info info;

    struct okl4_libmutex queue_lock;

    /* list of pending packets (doubly linked) */
    struct client_cmd_packet *pend_list;
    /* list of completed packets (singley linked) */
    struct client_cmd_packet *done_list;
};

#define MTD_MAX_MEMSEC  2

/**
   @brief A "virtual mtd device" as used by a client; one per client.
 */
struct virtual_mtd {
    int                 valid_instance;
    struct virtual_mtd  *next;
    struct mtd_device   *mtd;
    L4_ThreadId_t       thread;

    struct control_block *control;

    struct memsect_attach memsect_attach[MTD_MAX_MEMSEC];

    uintptr_t           offset;
    uintptr_t           size;
    // notify mask
    uintptr_t           mask;
};

#endif
