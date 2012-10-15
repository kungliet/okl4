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

#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <util/format_bin.h>
#include <nand_devices/nand_devices.h>
#include <vmtd/mtd.h>
#include <vtimer/timer.h>

#include <l4/kdebug.h>
#include <l4/schedule.h>
#include <l4/ipc.h>

#include <mutex/mutex.h>

#include "s3c2410_nand.h"

#if 0
    #define NAND_DEBUG(x, ...) printf("NAND: " x, ##__VA_ARGS__);
#else
    #define NAND_DEBUG(x, ...)
#endif

int busy = 0;

static int      read_id(struct s3c2410_nand *device, uint8_t *buf);
static uint8_t     read_status(struct s3c2410_nand *device);

static void read_spare_request(struct s3c2410_nand *device);
static void read_ecc_request(struct s3c2410_nand *device);
static void write_spare_request(struct s3c2410_nand *device);
static void write_ecc_request(struct s3c2410_nand *device);
static void erase_block_request(struct s3c2410_nand *device);

static void cmd_done_packet(struct cmd_interface *ci, struct cmd_packet *packet);
void (*continue_func)(struct s3c2410_nand *device);

typedef void (*tm_callback_t)(struct device_interface*);

#define NAND_PAGE_OUT   1
#define NAND_ID_OUT     2
#define NAND_STATUS_OUT 4

#define NAND_SPARE_ONLY     1
#define NAND_MAIN_AND_SPARE 0

#define NAND_ECC_BYPASS 0
#define NAND_ECC_AUTO   1

#define NAND_UNLOCK     4
#define NAND_LOCK       2
#define NAND_TIGHTLOCK  1

#define NAND_ECC_OK             0x0
#define NAND_ECC_CORRECTED      0x1
#define NAND_ECC_UNCORRECTABLE  0x2

int use_sleep = 0;

/* Simple delay function for scanning */
/* XXX makes use of L4 primatives */
static void delay(struct s3c2410_nand *device, uintptr_t time)
{
#define TIMER_MASK    0x8000        // XXX
    uintptr_t mask;

    L4_Set_NotifyMask(TIMER_MASK);

    if (use_sleep) {
        // delay in uints of 10 microsecs
        printf("timeout request %ld * 10 microsecs\n", time);
        //timeout_request(&device->device, MICROSECS(time*10), NULL);
        L4_WaitNotify(&mask);
    } else {
        volatile int i;
        for(i = 0; i < time; i++) {
            L4_Yield();
        }
    }
    L4_Set_NotifyMask(~0);

#undef TIMER_MASK
}

static void
nand_wait_int(struct s3c2410_nand *device)
{
    while (nf_status_get_ready() == 0){
        ;
    }
//    printf("waiting for a nand int?\n");
}


static inline void
nand_flashrow_in(struct s3c2410_nand *device, uint32_t addr)
{
    printf("flashing in %08lx\n", addr);
}



static inline void
nand_flashaddr_in(struct s3c2410_nand *device, uint32_t addr)
{
    uint8_t col, pg1, pg2, pg3;

    /* Split the address into its various parts:
     *  0-7  encode the column number
     *  8    becomes part of the command and selects the high or low
     *       half of the page
     *  9-25 the page number, fed to the controller 8 bits (or part thereof) at a time.
     */
    col = addr & 0xff;
    pg1 = (addr>>9) & 0xff;
    pg2 = (addr>>17) & 0xff;
    pg3 = (addr>>25) & 0xff;
    NAND_DEBUG("flashaddr in %08lx (col: %02x p1: %02x, p2: %02x, p3: %02x)\n",
            addr, col, pg1, pg2, pg3);
    nf_addr_set_addr(col);
    //printf("(A%02d)-",(int) col);

    nf_addr_set_addr( pg1);
    //printf("(A%02d)-", (int)pg1);

    nf_addr_set_addr( pg2);
    //printf("(A%02d)-", (int)pg2);

    nf_addr_set_addr( pg3);
    //printf("(A%02d)-", (int)pg3);
    nand_wait_int(device);
}


static inline void
nand_request_data(struct s3c2410_nand *device, uint16_t fdo, uint16_t spare, uint16_t rambuff)
{
    /* Enable ECC auto-correction.
     *
     * NOTE: NOT blindly setting sp_en as the doc seems to imply, as this
     * will apparently _only_ write to spare area and never to main buffers
     */
    printf("nand reqiest data: fdo: %d, spare %d, rambuff %d\n", fdo, spare, rambuff);
    nand_wait_int(device);  // XXX buggy mx21 - have to poll :(
    // wait for interrupt!
}

static inline void
nand_cmd_in(struct s3c2410_nand *device, uint8_t cmd)
{
    //printf("nand cmd in %d\n", cmd);
    
//    printf("(C%02x)-", cmd);
    nf_cmd_set_command(cmd);
    nand_wait_int(device);

    NAND_DEBUG("Wrote command: %02x\n", cmd);
    NAND_DEBUG("Latched command\n");
}

static inline void
nand_cmd_reset(struct s3c2410_nand *device)
{
    printf("nand reset\n");
    nand_cmd_in(device, NAND_CMD_RESET);
    NAND_DEBUG("Wrote reset command\n");
}

static int
read_id(struct s3c2410_nand *device, uint8_t *buf)
{
    volatile uint16_t *buf16;

    nf_conf_set_enable(1);
    nf_conf_set_nf_ce(0);
    nand_cmd_in(device, NAND_CMD_READ_ID);
    nand_flashaddr_in(device, 0x00);
    buf[0] = nf_data_get_data();
    buf[1] = nf_data_get_data();
    buf[2] = nf_data_get_data();
    buf[3] = nf_data_get_data();
    nf_conf_set_enable(0);

//    nand_data_out(device, NAND_ID_OUT, NAND_MAIN_AND_SPARE, buffer);

    buf16 = (uint16_t*)buf;
    printf("Read ID: %02x:%02x\n", buf16[0], buf16[1]);

    return 0;
}

static uint8_t
read_status(struct s3c2410_nand *device)
{
    uint8_t ret;
    
    nand_cmd_in(device, NAND_CMD_READ_STATUS);
    ret = nf_status_get_ready();
    NAND_DEBUG("Read STATUS: %02x\n", ret);

    return ret;
}


static inline void
nand_protect_rows(struct s3c2410_nand *device, uint32_t start, uint32_t end, uint16_t protection)
{
    printf("porotecting %08lx-%08lx %d\n", start, end, protection);
    NAND_DEBUG("Block protection of %lx->%lx is %d\n", start, end, protection);
}

// packet we are processing
static struct cmd_packet *pend_packet;
static long data_length;
static int ecc_error;


static uint8_t cache_oob[32];

static void
read_spare_request(struct s3c2410_nand *device)
{
    uintptr_t page = pend_packet->args[0];
    uint8_t *data = pend_packet->data;
    int size = pend_packet->length;
    int orig_size = size;

    NAND_DEBUG("Reading oob from page =  %lx\n", page);

    nf_conf_set_enable(1);
    nf_conf_set_nf_ce(0);


    if (pend_packet->args[1] != 0){
        NAND_DEBUG("wants to read %d from %08lx:%08lx\n", 
                size, pend_packet->args[0], pend_packet->args[1]);
        L4_KDB_Enter("read spare  offset not 0!");
    }
    
    int done = 0;
    while (1){
        nand_cmd_in(device, NAND_CMD_READ_OOB);
        nand_flashaddr_in(device, (page * device->page_size));


        int count = (size <= 16)?size:16;

        int i;
        for (i = 0; i < count; i++){
            data[done] = nf_data_get_data();
            done++;
        }
        size -= count;
        page ++;
        if (size == 0) break;
    }

    nf_conf_set_enable(0);

    pend_packet->length = orig_size;
    cmd_done_packet(&device->cmd_if, pend_packet);
    busy=0;

}

static void
read_ecc_request(struct s3c2410_nand *device)
{
    uintptr_t page = pend_packet->args[0];
    int offset = pend_packet->args[1];
    uint8_t *data = (uint8_t*)pend_packet->data;
    int size = data_length = pend_packet->length;

    NAND_DEBUG("Reading from page with ecc =  %lx\n", page);

    nf_conf_set_enable(1);
    nf_conf_set_nf_ce(0);

    int done = 0;
    while(1){
        NAND_DEBUG("want to read from address%08lx (page:%08lx:offset:%08x)\n",
                (page*device->page_size)+offset, page, offset);
        uint8_t cmd = (offset&0x100)?NAND_CMD_READ_HALF1:NAND_CMD_READ_HALF0;
        
        nand_cmd_in(device, cmd);
        nand_flashaddr_in(device, (page * device->page_size)+offset);

        int count = (size <= 512)?size:512;
        if (count + offset >= 512) {
            count = 512 - offset;
        }
        offset = 0;

        int i;
        for (i = 0; i < count; i++){
            data[done] = nf_data_get_data();
            done++;
        }
        size -= count;
        page++;
        if (size == 0) break;
    }

    /* FIXME(benno): This code doesn't seem to do much */
    uint8_t *data_oob = (uint8_t *)pend_packet->args[2];
    if (data_oob){
        NAND_DEBUG("do want oob!\n");
    } else {
        data_oob = cache_oob;
    }

    nf_conf_set_enable(0);
    pend_packet->length = data_length;
    cmd_done_packet(&device->cmd_if, pend_packet);
    busy=0;
}

static void
write_spare_continue(struct s3c2410_nand *device)
{
    uint8_t *data = pend_packet->data;
    uint8_t status;
    int size = pend_packet->length;
    int col = pend_packet->args[1] % device->oob_size;

    NAND_DEBUG("nand: %s\n", __func__);

    size = (col+size) > device->oob_size ? device->oob_size - col : size;

    status = read_status(device);
    while (!status) {
        delay(device,2);
        status = read_status(device);
    }
    data_length += size;

    pend_packet->length -= size;

    /* write next page? */
    if (pend_packet->length) {
        pend_packet->args[0] ++;    // page++
        pend_packet->args[1] = 0;   // offset = 0;
        data += size;
        pend_packet->data = data;

        write_spare_request(device);
    } else {
        // how much data was read
        pend_packet->length = data_length;

        cmd_done_packet(&device->cmd_if, pend_packet);
        busy = 0;
    }
}

static void
write_spare_request(struct s3c2410_nand *device)
{
    uintptr_t page = pend_packet->args[0];
    uintptr_t address = page * device->page_size;
    uint8_t *data = pend_packet->data;
    (void)data;
    int size = pend_packet->length;
    int col = pend_packet->args[1] % device->oob_size;
    int i;

    NAND_DEBUG("nand: %s\n", __func__);

    continue_func = write_spare_continue;

    printf("Writing oob to page =  %lx\n", page);

    // XXX write to BUF

    size = (col+size) > device->oob_size ? device->oob_size - col : size;

    nand_cmd_in(device, NAND_CMD_READ_OOB);  /* Select spare area */
    nand_cmd_in(device, NAND_CMD_WRITE);  /* Write command */
    nand_flashaddr_in(device, address);
    for (i = 0; i < size; i++) {
            nf_data_write(data[i]);
    }   
    nand_cmd_in(device, NAND_CMD_WRITE_CONFIRM);  /* Write confirmation */
    continue_func(device);
}

static void
write_ecc_continue(struct s3c2410_nand *device)
{
    uint8_t *data = pend_packet->data;
    uint8_t status;
    int size = pend_packet->length;
    int col = pend_packet->args[1] % device->page_size;

    NAND_DEBUG("nand: %s\n", __func__);

    size = (col+size) > device->page_size ? device->page_size - col : size;

    // for now, we poll till done
    status = read_status(device);

    while (!status) {
        delay(device, 2);
        status = read_status(device);
    }
    data_length += size;

    pend_packet->length -= size;

    /* write next page? */
    if (pend_packet->length) {
        pend_packet->args[0] ++;    // page++
        pend_packet->args[1] = 0;   // offset = 0;
        data += size;
        pend_packet->data = data;

        write_ecc_request(device);
    } else {
        // how much data was read
        pend_packet->length = data_length;

        cmd_done_packet(&device->cmd_if, pend_packet);
        busy = 0;
    }
}

static void
write_ecc_request(struct s3c2410_nand *device)
{
    uintptr_t page = pend_packet->args[0];
    int offset =  pend_packet->args[1];
    uintptr_t address = page * device->page_size + offset;
    uint8_t *data = pend_packet->data;
    int size = pend_packet->length;
    int col = pend_packet->args[1] % device->page_size;
    int i;

    nf_conf_set_enable(1);
    nf_conf_set_nf_ce(0);

    continue_func = write_ecc_continue;

    NAND_DEBUG("Writing + ecc to page =  %lx\n", page);

    size = (col+size) > device->page_size ? device->page_size - col : size;

    nand_cmd_in(device, NAND_CMD_READ_HALF0); /* Select page main area */
    nand_cmd_in(device, NAND_CMD_WRITE);  /* Write command */
    nand_flashaddr_in(device, address);
    for (i = 0; i < size; i++) {
            nf_data_write(data[i]);
    }
    nand_cmd_in(device, NAND_CMD_WRITE_CONFIRM);  /* Write confirmation */
    continue_func(device);
    
}


static void
erase_block_request(struct s3c2410_nand *device)
{
    uintptr_t page = pend_packet->args[0];
    uintptr_t address = page * device->page_size;
    uint8_t status;

    nf_conf_set_enable(1);
    nf_conf_set_nf_ce(0);

    if ((address % device->block_size) ||
        (pend_packet->length % device->block_size) ||
        ((pend_packet->length) > device->block_size))
    {
        // non aligned address / size
        pend_packet->length = -1ul;  // error
        pend_packet->args[0] = page; // failed page
        cmd_done_packet(&device->cmd_if, pend_packet);
        busy = 0;
        return;
    }

    while (pend_packet->length)
    {
        NAND_DEBUG("nand: erase block @ %lx\n", address);

        nand_cmd_in(device, NAND_CMD_BLOCK_ERASE);  /* Erase command */
        nand_flashaddr_in(device, address);
        nand_cmd_in(device, NAND_CMD_ERASE_CONFIRM);/* Erase confirmation */

        status = read_status(device);
        while (!status) {
            delay(device, 2);
            status = read_status(device);
        }
        pend_packet->length -= device->block_size;
        data_length += device->block_size;
        address += device->block_size;
    }

    pend_packet->length = data_length;
    cmd_done_packet(&device->cmd_if, pend_packet);
    busy = 0;
}

/* Move packet to done_list */
static void cmd_done_packet(struct cmd_interface *ci, struct cmd_packet *packet)
{
    okl4_libmutex_lock(&(ci->queue_lock));

    packet->status = PACKET_DONE;

    // dequeue pending list
    packet = ci->pend_list;
    NAND_DEBUG("driver: packet is %p and packet->next  is %p\n", packet, packet->next);
    if (packet->next == NULL) {
        ci->pend_list = NULL;
    } else {
        packet->next->prev = packet->prev;
        packet->prev->next = packet->next;
        ci->pend_list = packet->next;
    }

    // add packet to complete queue
    packet->next = ci->done_list;
    ci->done_list = packet;

    okl4_libmutex_unlock(&(ci->queue_lock));
}

static int
cmd_execute_impl(struct cmd_interface *ci, struct s3c2410_nand *device,
        struct cmd_packet *packet)
{
    int ret = 0;
    if (busy){
        printf("busy ret 1!\n");
        return 1;
    }

    NAND_DEBUG("%s: packet:%p, packet->cmd: %lx \n", __func__, packet, packet->cmd);

    busy = 1;
    packet->status = PACKET_BUSY;
    pend_packet = packet;
    data_length = 0;
    ecc_error = 0;

    switch (packet->cmd) {
    case MTD_CMD_READ_OOB:
        NAND_DEBUG("%s: read oob\n", __func__);
        read_spare_request(device);
        break;
    case MTD_CMD_READ_ECC:
        NAND_DEBUG("%s: read with ecc\n", __func__);
        read_ecc_request(device);
        break;
    case MTD_CMD_WRITE_OOB:
        NAND_DEBUG("%s: write oob\n", __func__);
        break;
        write_spare_request(device);
    case MTD_CMD_WRITE_ECC:
        NAND_DEBUG("%s: write with ecc\n", __func__);
        write_ecc_request(device);
        break;
    case MTD_CMD_ERASE_BLOCK:
        NAND_DEBUG("%s: erase block\n", __func__);
        erase_block_request(device);
        break;
    default:
        L4_KDB_Enter("UNKNOWN CCMDMDSM<ADMSKLADJK  not implemented!");
        printf("nand: %s: unknown command %lx\n", __func__, packet->cmd);
        packet->length = -1ul;  // error
        cmd_done_packet(ci, packet);
        busy = 0;
        break;
    }
    return ret;
}

static int
mtd_get_info_impl(struct mtd_interface *mi, struct s3c2410_nand *device, struct mtd_info *info)
{
    NAND_DEBUG("\n\tGET INFO:\nblock %d\n, pg_size %d\n, erase size: %d\n, block size: %d\n, oob size: %d\n",
        device->block_num, device->page_size, device->erase_size, device->block_size, device->oob_size);
    
    info->blocks = (uint32_t)device->block_num;
    info->page_size = (uint32_t)device->page_size;
    info->erase_size = (uint32_t)device->erase_size;
    info->block_size = (uint32_t)device->block_size;
    info->oob_size = (uint32_t)device->oob_size;
    
    return 0;
}


static int
device_poll_impl(struct device_interface *di, struct s3c2410_nand *device)
{
    NAND_DEBUG("nand: %s:\n", __func__);
    return 0;
}

static int
device_interrupt_impl(struct device_interface *di, struct s3c2410_nand *device, int irq)
{
    // This driver isn't interrupt driven
    return 0;
}

static int
device_setup_impl(struct device_interface *di, struct s3c2410_nand *device,
        struct resource *resources)
{
    uint8_t nand_id[4];
    int i;

    use_sleep = 0;

    for (   /* Nothing */;
            resources && (resources->type != BUS_RESOURCE && resources->type != MEMORY_RESOURCE);
            resources = resources->next)
        ;

    if (!resources){
        printf("Cannot find resources!\n");
        return 0;
    }

    device->regs = *resources;

    device->cmd_if.device = device;
    device->cmd_if.ops = cmd_ops;
    device->mtd_if.device = device;
    device->mtd_if.ops = mtd_ops;

    device->block_num = 0;
    device->page_size = 0;
    device->erase_size = 0;
    device->block_size = 0;
    device->oob_size = 0;

    /* Update our state */
    device->state = STATE_DISABLED;


    nand_cmd_reset(device);

    nf_conf_set_ecc_init(0);    
    read_id(device, nand_id);
    printf("NAND chip ID: %02x:%02x:%02x:%02x\n",
            nand_id[0], nand_id[1], nand_id[2], nand_id[3]);

    i = 0;
    while (nand_manf_ids[i].manf_name != NULL) {
        if (nand_manf_ids[i].manf_id == nand_id[0])
        {
            printf(" -- %s: ", nand_manf_ids[i].manf_name);
            break;
        }
        i++;
    }
    i = 0;
    /* Find device information */
    while (nand_device_ids[i].chip_name != NULL) {
        if ((nand_device_ids[i].manf_id == nand_id[0]) &&
                (nand_device_ids[i].chip_id == nand_id[1]))
        {
            printf("The Chip name is %s\n", nand_device_ids[i].chip_name);

            device->block_num   = nand_device_ids[i].blocks;
            device->block_size  = nand_device_ids[i].block_size;
            device->erase_size  = nand_device_ids[i].erase_size;
            device->page_size   = nand_device_ids[i].page_size;
            device->oob_size    = nand_device_ids[i].oob_size;
            break;
        }
        i++;
    }
    if (device->block_num == 0) {
        printf("Unknown device\n");
        L4_KDB_Enter("");
        return 0;
    }

    printf("NAND STATUS: %x\n", read_status(device));
    printf("\n\n");


    /* ensure flash not locked */
    nand_protect_rows(device, 0, 0xffff, NAND_UNLOCK);

    return DEVICE_SUCCESS;
}

static int
device_enable_impl(struct device_interface *di, struct s3c2410_nand *device)
{
    NAND_DEBUG("nand: %s:\n", __func__);

    device->state = STATE_ENABLED;

    use_sleep = 1;
    return DEVICE_ENABLED;
}

static int
device_disable_impl(struct device_interface *di, struct s3c2410_nand *device)
{
    NAND_DEBUG("nand: %s:\n", __func__);
    device->state = STATE_DISABLED;

    use_sleep = 0;
    return DEVICE_DISABLED;
}

static int
device_num_interfaces_impl(struct device_interface *di, struct s3c2410_nand *device)
{
    return 2;
}


/* FIXME: This can definately be autogenerated */
static struct generic_interface *
device_get_interface_impl(struct device_interface *di, struct s3c2410_nand *device, int i)
{
    switch (i) {
        case 0:
            return (struct generic_interface *)(void *) &device->cmd_if;
        case 1:
            return (struct generic_interface *)(void *) &device->mtd_if;
        default:
            return NULL;
    }
}

