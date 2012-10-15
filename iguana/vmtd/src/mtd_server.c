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
 * NOTE: At this point, I'm assuming that there is one and only one 
 * controller on the board.  This will need to be revisited.
 */

#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <assert.h>

#include <l4/ipc.h>
#include <l4/thread.h>
#include <l4/misc.h>
#include <l4/kdebug.h>
#include <l4/interrupt.h>

#include <mutex/mutex.h>

#include <iguana/hardware.h>
#include <iguana/memsection.h>
#include <iguana/thread.h>
#include <iguana/session.h>
#include <iguana/object.h>
#include <iguana/pd.h>
#include <iguana/cap.h>

#include <driver/driver.h>
#include <driver/device.h>
#include <driver/cmd.h>
#include <driver/mtd.h>
#include <vmtd/mtd.h>
#include <vtimer/timer.h>

#include <circular_buffer/cb.h>
#include <range_fl/range_fl.h>

#include <interfaces/vmtd_serverdecls.h>

#include "mtd_server.h"

extern struct driver MTD_DRIVER;

#define NUM_PACKETS 20
static int const PAGE_SIZE = 4096;


#if 0
 #define DEBUG_PRINT(x, args...) printf(x, args)
 #define INFO_PRINT(x, args...) printf(x, args)
#else
 #define DEBUG_PRINT(x, args...)
 #define INFO_PRINT(x, args...)
#endif

static int         buffer[8] = { 0x01234567 };
static mem_space_t iguana_granted_physmem[4] = { (mem_space_t)0xdeadbeef }; /* XXX: will not work if no physmem is granted */
static int         iguana_granted_interrupt[4] = { 0xffffffff }; /* will always work because irq -1 is invalid */
static struct virtual_mtd virtual_device_instance[4] = { { 1 } }; /* valid but unused instances are ok */

static struct resource          resources[8];
static struct mtd_device        mtd_device;
static struct cmd_packet        mtd_packet[NUM_PACKETS];

int
main(void)
{
    int i, r;
    void *device;

    for (i = 0, r = 0; i < 4; i++)
    {   
        if (iguana_granted_physmem[i] != NULL)
        {   
            resources[r].type = MEMORY_RESOURCE;
            resources[r++].resource.mem = iguana_granted_physmem[i];
        }
        if (iguana_granted_interrupt[i] != -1)
        {   
            resources[r].type = INTERRUPT_RESOURCE;
            resources[r++].resource.interrupt = iguana_granted_interrupt[i];
        }
    }
    for (/*none*/; r < 8; r++)
    {   
        resources[r].type = NO_RESOURCE;
    }

    /*allocate memory for the device */
    device = malloc(MTD_DRIVER.size);
    if (device == NULL) {
        free(device);
    }
   /* Initialize the device */
    {   
        mtd_device.next = NULL;
        mtd_device.resources = resources;
    }
    /*Create and enable the device */
    mtd_device.dev_if = setup_device_instance(&MTD_DRIVER, device);
    device_setup(mtd_device.dev_if, mtd_device.resources);

    /* Final setup */
    mtd_device.cmd_if = (struct cmd_interface*) device_get_interface(mtd_device.dev_if, 0);
    mtd_device.mtd_if = (struct mtd_interface*) device_get_interface(mtd_device.dev_if, 1);
    device_enable(mtd_device.dev_if);

    /* Create freelist of packets */
    mtd_device.cmd_if->free_list = mtd_packet;
    mtd_device.cmd_if->pend_list = NULL;
    mtd_device.cmd_if->done_list = NULL;
    for (i=0; i < NUM_PACKETS; i++)
    {
        mtd_device.cmd_if->free_list[i].next = &(mtd_device.cmd_if->free_list[i+1]);
    }
    mtd_device.cmd_if->free_list[i-1].next = NULL;

    mtd_get_info(mtd_device.mtd_if, &mtd_device.info);
    mtd_device.timer_device = 0;


    for (i = 0; i < 4; i++)
    {
        if (virtual_device_instance[i].valid_instance)
        {
            virtual_device_instance[i].mtd = (void *)(&mtd_device);
            virtual_device_instance[i].offset = 0;
            virtual_device_instance[i].size = (mtd_device.info.blocks * mtd_device.info.block_size);
            virtual_device_instance[i].next = mtd_device.client_list;
            mtd_device.client_list = &virtual_device_instance[i];

        }
    }


    L4_Accept(L4_NotifyMsgAcceptor);
    L4_Set_NotifyMask(~0);
    vmtd_server_loop();
    return 0;
    (void)iguana_granted_physmem;
    (void)iguana_granted_interrupt;
    (void)buffer;
    (void)virtual_device_instance;


}


int
virtual_mtd_add_memsection_impl(CORBA_Object _caller, device_t handle,
        objref_t memsection, uintptr_t passwd, int idx, 
        idl4_server_environment * _env)
{
    struct virtual_mtd *vmtd = virtual_device_instance + handle;
    cap_t cap;
    int ret;

    if (idx < 0 || idx >= MTD_MAX_MEMSEC) {
        /* ERROR */
        return -1;
    }
    vmtd->memsect_attach[idx].base = memsection_base(memsection);
    vmtd->memsect_attach[idx].size = memsection_size(memsection);

    cap.ref.obj = memsection;
    cap.passwd = passwd;
    ret = clist_insert(default_clist, cap);



    return 0;
}

int
virtual_mtd_register_control_block_impl(CORBA_Object _caller, device_t handle, int idx, uintptr_t offset, idl4_server_environment *_env)
{
    struct virtual_mtd *vmtd = virtual_device_instance + handle;
    void *vaddr;

    if (idx < 0 || idx >= MTD_MAX_MEMSEC) {
        /* ERROR */
        return -1;
    }
    if (offset >= (vmtd->memsect_attach[idx].size - sizeof(*vmtd->control))) {
        /* ERROR */
        return -1;
    }
    /* FIXME: Check valid etc */
    vaddr = (void*) ((uintptr_t) vmtd->memsect_attach[idx].base + offset);

    vmtd->control = vaddr;

    vmtd->control->info.blocks = vmtd->size / vmtd->mtd->info.block_size;
    vmtd->control->info.block_size = vmtd->mtd->info.block_size;
    vmtd->control->info.erase_size = vmtd->mtd->info.erase_size;
    vmtd->control->info.page_size = vmtd->mtd->info.page_size;
    vmtd->control->info.oob_size = vmtd->mtd->info.oob_size;
    return 0;
}

int
virtual_mtd_init_impl(CORBA_Object _caller, device_t handle, L4_Word_t owner, uint32_t mask, idl4_server_environment * _env)
{
    struct virtual_mtd *vmtd = virtual_device_instance + handle;
    L4_ThreadId_t   owner_tid;
    owner_tid.raw = owner;

    assert(!L4_IsThreadEqual(owner_tid, L4_nilthread));
    assert(!L4_IsThreadEqual(owner_tid, L4_myselfconst));
    /* assert owner is not thread handle? */

    if (!vmtd->valid_instance)
    {
        printf("vmtd: accessing invalid instance (handle: %d)\n", handle);
        return -1;
    }

    /* Re-set the owner if owner argument is different from the existing owner */
    if (!L4_IsThreadEqual(owner_tid, vmtd->thread))
    {
        if (!L4_IsNilThread(vmtd->thread))
        {
            printf
            (
                "vmtd: re-setting owner of instance (old_owner: %lx, new_owner: %lx, handle: %d)\n",
                vmtd->thread.raw, owner_tid.raw, handle
            );
        }

        vmtd->thread = owner_tid;
    }

    vmtd->mask = mask;

    printf
    (
        "vmtd: init done (handle: %d, owner: %lx, mask: %x)\n",
        handle, owner_tid.raw, (unsigned int)mask
    );

    return 0;
}

static struct cmd_packet *
do_alloc_packet(struct cmd_interface *ci)
{
    struct cmd_packet *packet;
    if (ci->free_list == NULL) {
        return NULL;
    }
    packet = ci->free_list;
    ci->free_list = packet->next;

    // setup packet to not-ready
    packet->status = PACKET_SETUP;

    okl4_libmutex_lock(&(ci->queue_lock));

    // add packet to pending queue

    if (ci->pend_list) {
        packet->next = ci->pend_list;
        packet->prev = ci->pend_list->prev;
        ci->pend_list->prev = packet;
    } else {
        packet->next = ci->pend_list = NULL;
        packet->prev = packet;
    }

    ci->pend_list = packet;

    okl4_libmutex_unlock(&(ci->queue_lock));

    return packet;
}

static void
do_done_packet(struct cmd_interface *ci)
{
    struct cmd_packet *drv_packet;
    struct virtual_mtd *vmtd;

    do {
        struct client_cmd_packet *client_packet;

        okl4_libmutex_lock(&(ci->queue_lock));

        drv_packet = ci->done_list;

        if (drv_packet == NULL) {
            okl4_libmutex_unlock(&(ci->queue_lock));
            return;
        }

        ci->done_list = drv_packet->next;
        okl4_libmutex_unlock(&(ci->queue_lock));

        // find client packet
        client_packet = (struct client_cmd_packet *)drv_packet->ref;
        vmtd = (struct virtual_mtd *)drv_packet->client;

        // update data
        client_packet->length = drv_packet->length;
        client_packet->data = drv_packet->data;
        // set packet to done
        client_packet->status = PACKET_DONE;

        // move to client done list
        {
            struct control_block *control = vmtd->control;

            okl4_libmutex_lock(&(control->queue_lock));

            // dequeue pending list
            if (client_packet->next == NULL) {
                control->pend_list = NULL;
            } else {
                client_packet->next->prev = client_packet->prev;
                client_packet->prev->next = client_packet->next;
                control->pend_list = client_packet->next;
            }

            // add packet to complete queue
            client_packet->next = control->done_list;
            control->done_list = client_packet;

            okl4_libmutex_unlock(&(control->queue_lock));
        }

        // enqueue free list
        drv_packet->next = ci->free_list;
        ci->free_list = drv_packet;
        L4_Notify(vmtd->thread, vmtd->mask);
    } while (ci->done_list);
}

static void
do_work(struct mtd_device *device, struct virtual_mtd *vmtd)
{
    struct client_cmd_packet *client_packet;
    struct cmd_packet *drv_packet;
    struct control_block *control = vmtd->control;
    int i, completed = 0;

    okl4_libmutex_lock(&(control->queue_lock));

    if (!control->pend_list)
        goto exit;

    client_packet = control->pend_list;
    do {
        DEBUG_PRINT("   - look at packet: %p  cmd = %lx, status %d\n",
                client_packet, client_packet->cmd, client_packet->status);

        if (client_packet->status == PACKET_PEND) {
            unsigned long addr = client_packet->args[0] * vmtd->control->info.page_size;
            addr += client_packet->length;
            if (addr > vmtd->size)
            {
                // XXX move to done_list
                client_packet->status = PACKET_DONE;
                client_packet->length = 0;
                INFO_PRINT("vmtd: packet %p error!\n", client_packet);
                while (1);
                goto next;
            }
            drv_packet = do_alloc_packet(device->cmd_if);
            /* No available packets */
            if (drv_packet == NULL) {
                INFO_PRINT("%s: no free packets?\n", __func__);
                goto exit;
            }

            DEBUG_PRINT("  - new drv_packet: %p\n", drv_packet);
            drv_packet->cmd = client_packet->cmd;
            /* do partition fixup */
            drv_packet->args[0] = client_packet->args[0] ;
//                (vmtd->offset / vmtd->control->info.page_size);

            for (i = 1; i < 3; i++)
                drv_packet->args[i] = client_packet->args[i];

            drv_packet->data = client_packet->data;
            drv_packet->length = client_packet->length;
            // set packet to pending
            drv_packet->status = PACKET_PEND;
            drv_packet->ref = (void*)client_packet;
            drv_packet->client = (void*)vmtd;

                // packet now being processed
            client_packet->status = PACKET_BUSY;

                // try execute command
            if (cmd_execute(device->cmd_if, drv_packet)){
                printf("why exit?\n");
                goto exit;
            }
            
            if (drv_packet->status == PACKET_DONE) {
                completed = 1;
            }

        }

       if ( client_packet == client_packet->next ) {
           break;
       }
next: 
       client_packet = client_packet->next;
    } while (client_packet);

exit:
    okl4_libmutex_unlock(&(control->queue_lock));
    if (completed) { 
        DEBUG_PRINT("vmtd: completed packets on if: %p\n", device->cmd_if);
        do_done_packet(device->cmd_if);
    }       
}


void        
vmtd_irq_handler(L4_ThreadId_t partner)
{           
            
}  

void vmtd_async_handler(L4_Word_t notify)
{
    if (notify & (1UL << 31)) {
        L4_Word_t irq;

        irq = __L4_TCR_PlatformReserved(0);
        device_interrupt(mtd_device.dev_if, irq);

        // ack IRQ
        L4_LoadMR(0, irq);
        L4_AcknowledgeInterrupt(0, 0);

    }

    if (notify & TIMER_MASK) {
        if (mtd_device.timer_callback) {
            tm_callback_t fn = mtd_device.timer_callback;
            mtd_device.timer_callback = NULL;
            fn(mtd_device.dev_if);
        }
    }
    notify &= ~(TIMER_MASK);
    if (!notify)
        return;

    struct virtual_mtd *vmtd;

    for (vmtd = mtd_device.client_list; vmtd != NULL; vmtd = vmtd->next)
    {

        DEBUG_PRINT(" process client: : %p -  %lx\n", vmtd, vmtd->thread.raw);
        do_work(&mtd_device, vmtd);
    }
    do_done_packet(mtd_device.cmd_if);

}    


