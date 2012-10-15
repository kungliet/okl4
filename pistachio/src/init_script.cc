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
 * Description: Kernel initialization operations filled by Elfweaver.
 *
 * Elfweaver fills the init script, which consists of a header and a
 * sequence of operations. The kernel parses and executes this
 * operation.
 *
 * Some operations treat the object created by the previous operation
 * as an implicit argument. The indentation of below diagram shows
 * this relationship.
 *
 * thread_handle
 * heap
 *   clist
 *     space
 *       segment
 *         memory_mapping
 *       irq
 *       mutex
 *       thread
 * ipc_cap
 * mutex_cap
 *
 */

#include <l4.h>
#include <debug.h>

#include <tcb.h>
#include <smp.h>
#include <schedule.h>
#include <space.h>
#include <arch/memory.h>
#include <config.h>
#include <arch/hwspace.h>
#include <kernel/generic/lib.h>
#include <mutex.h>
#include <clist.h>
#include <caps.h>
#include <init.h>
#include <l4/map.h>
#include <interrupt.h>

#define KI_MAGIC        0x06150128
#define KI_VERSION      1
typedef struct
{
    word_t magic;
    word_t version;
} ki_header_t;

typedef enum
{
    KI_OP_CREATE_HEAP           = 1,

    KI_OP_INIT_IDS              = 2,
    KI_OP_CREATE_THREAD_HANDLES = 3,
    KI_OP_CREATE_CLIST          = 4,
    KI_OP_CREATE_SPACE          = 5,
    KI_OP_CREATE_THREAD         = 6,
    KI_OP_CREATE_MUTEX          = 7,
    KI_OP_CREATE_SEGMENT        = 8,
    KI_OP_MAP_MEMORY            = 9,

    KI_OP_CREATE_IPC_CAP        = 10,
    KI_OP_CREATE_MUTEX_CAP      = 11,

    KI_OP_ASSIGN_IRQ            = 12,
    KI_OP_ALLOW_PLATFORM_CONTROL= 13,
} ki_op_t;

typedef union
{
    struct {
        BITFIELD3(
            word_t,
            size         : (BITS_WORD/2), /* word unit, including header */
            op           : (BITS_WORD/2-1),
            end_of_phase : 1);
    };
} ki_hdr_t;

#define SHIFT_4K (12)
#define SHIFT_1K (10)

typedef struct
{
    ki_hdr_t    hdr;
    BITFIELD2(
        word_t,
        size      : SHIFT_4K,                 /* Treat as multiples of 4K */
        phys      : (BITS_WORD - SHIFT_4K));  /* 4K Aligned */
} ki_create_heap_t;


/*
 * this operation must come after the first create_heap operation.
 */
typedef struct
{
    ki_hdr_t    hdr;
    BITFIELD2(
        word_t,
        max_space_ids    : (BITS_WORD / 2),
        max_clist_ids    : (BITS_WORD / 2));
    word_t      max_mutex_ids; /* Leave as word_t as any packing advantages
                                  lost */
} ki_init_ids_t;

/*
 * this operation must come after the first create_heap operation.
 */
typedef struct
{
    ki_hdr_t    hdr;
    BITFIELD2(
        word_t,
        size     : SHIFT_1K,                /* Treat as 4-word multiples
                                             * (giving a maximum of 4092
                                             * threads) */
        phys     : (BITS_WORD - SHIFT_1K)); /* 1K Aligned */
} ki_create_thread_handles_t;


/*
 * implicit argument: heap
 */
typedef struct
{
    ki_hdr_t    hdr;
    BITFIELD2(
        word_t,
        id        : (BITS_WORD / 2),
        max_caps  : (BITS_WORD / 2));
} ki_create_clist_t;

/*
 * implicit argument: clist
 */
typedef struct
{
    ki_hdr_t    hdr;
    word_t      id;
    BITFIELD2(
        word_t,
        space_id_base  : (BITS_WORD / 2),
        space_id_num   : (BITS_WORD / 2));

    BITFIELD2(
        word_t,
        clist_id_base  : (BITS_WORD / 2),
        clist_id_num   : (BITS_WORD / 2));

    BITFIELD2(
        word_t,
        mutex_id_base  : (BITS_WORD / 2),
        mutex_id_num   : (BITS_WORD / 2));

    BITFIELD2(
        word_t,
        utcb_area_log2_size   : SHIFT_1K,
        /*
         * Address aligned to 1K,
         * which matches the
         * fpage_t definition
         */
        utcb_area_base        : (BITS_WORD - SHIFT_1K));

    BITFIELD3(
        word_t,
        num_physical_segs : (BITS_WORD / 2),
        has_kresources    : 1,
        max_priority      : ((BITS_WORD / 2) - 1));

} ki_create_space_t;

/*
 * implicit argument: space
 */
typedef struct
{
    ki_hdr_t    hdr;
    BITFIELD2(
        word_t,
        cap_slot         : (BITS_WORD / 2),
        priority         : (BITS_WORD / 2));
    word_t      ip;
    word_t      sp;
    word_t      utcb_address;
    word_t      mr1;
} ki_create_thread_t;

/*
 * implicit argument: space
 */
typedef struct
{
    ki_hdr_t    hdr;
    word_t      id;
} ki_create_mutex_t;

/*
 * implicit argument: space
 */
typedef struct
{
    ki_hdr_t       hdr;
    word_t         seg_num;
    phys_segment_t segment;
} ki_create_segment_t;

/*
 * implicit argument: space, segment
 */
typedef struct
{
    ki_hdr_t     hdr;
    L4_MapItem_t desc;
} ki_map_memory_t;

typedef struct
{
    ki_hdr_t    hdr;
    BITFIELD2(
        word_t,
        clist_ref        : (BITS_WORD / 2),
        cap_slot         : (BITS_WORD / 2));
    word_t      thread_ref;
} ki_create_ipc_cap_t;

typedef struct
{
    ki_hdr_t    hdr;
    BITFIELD2(
        word_t,
        clist_ref        : (BITS_WORD / 2),
        cap_slot         : (BITS_WORD / 2));
    word_t      mutex_ref;
} ki_create_mutex_cap_t;

/*
 * implicit argument: space
 */
typedef struct
{
    ki_hdr_t    hdr;
    word_t      irq;
} ki_assign_irq_t;

/*
 * implicit argument: space
 */
typedef struct
{
    ki_hdr_t    hdr;
} ki_allow_platform_control_t;

/*
 * points to the an array of operations filled by Elfweaver
 */
static ki_header_t *ki_init_script;

/*
 * Reuse operation hdr storage to set object ptr.  Note that there
 * must be no alignment problem for this.
 */
static INLINE
void ki_set_ptr(ki_hdr_t* hdr, void* ptr)
{
    *((word_t *)hdr) = (word_t)ptr;
}

/*
 * ref is the byte offset from ki_init_script to the specific
 * operation header, on which the resulting object ptr was written.
 */
static INLINE
void* ki_get_ptr(word_t ref)
{
    char * ptr = (char *)ki_init_script + ref;
    void * ret = *(void**)ptr;
    return ret;
}

extern clist_t *
create_clist(kmem_resource_t* res, word_t id, word_t max_caps);

static kmem_resource_t SECTION(SEC_INIT)*
ki_create_heap(ki_create_heap_t *args)
{
    TRACE_INIT("CREATE_HEAP: phys=%x, size=%x\n",
               args->phys << SHIFT_4K, args->size * (1024 * 4));

    kmem_resource_t* res = (kmem_resource_t *)phys_to_virt(args->phys << SHIFT_4K);
    addr_t end_mem = (addr_t)((word_t)res + (args->size * (1024 * 4)));
    res->init(end_mem);

    return res;
}

static void SECTION(SEC_INIT)
    ki_init_ids(ki_init_ids_t *args, kmem_resource_t *kresource)
{
    TRACE_INIT("INIT_IDS: max_ids(space/mutex/clist)=%d/%d/%d\n",
               args->max_space_ids, args->max_clist_ids, args->max_mutex_ids);

    init_spaceids(args->max_space_ids, kresource);
    init_clistids(args->max_clist_ids, kresource);
    init_mutex(args->max_mutex_ids, kresource);
}

static void SECTION(SEC_INIT)
ki_create_thread_handles(ki_create_thread_handles_t *args)
{
    extern tcb_t** thread_handle_array;
    extern word_t num_tcbs;

    /*
     * A reminder of the comment against the declaraion of
     * ki_create_thread_handles_t - size describes 4-word multiples.  As
     * each tcb_t pointer is 1 word this equates to num_tcbs * 4.
     */
    TRACE_INIT("CREATE_THREAD_HANDLES: phys=%x, size=%x\n",
               (args->phys << SHIFT_1K), 
               (args->size * (sizeof(tcb_t) * 4)));
    thread_handle_array = (tcb_t **)phys_to_virt(args->phys << SHIFT_1K);
    num_tcbs = (args->size * 4);
}

static clist_t SECTION(SEC_INIT)*
ki_create_clist(kmem_resource_t *res, ki_create_clist_t *args)
{
    TRACE_INIT("CREATE_CLIST: id=%d, max_caps=%d\n",
               args->id, args->max_caps);

    clist_t *clist = create_clist(res, args->id, args->max_caps);
    if (!clist) {
        panic("CREATE_CLIST: id=%d, max_caps=%d\n",
              args->id, args->max_caps);
    }

    return clist;
}

static space_t SECTION(SEC_INIT)*
ki_create_space(kmem_resource_t * res, clist_t* clist, ki_create_space_t* args)
{
    TRACE_INIT("CREATE_SPACE: id=%d, space{%d:%d}, clist{%d:%d}, mutex{%d:%d), "
               "max_phys_segs=%d, utcb_area{0x%x:2^0x%x}, max_prio=%ld "
               "has_kresources %d\n",
               args->id,
               args->space_id_base, args->space_id_num,
               args->clist_id_base, args->clist_id_num,
               args->mutex_id_base, args->mutex_id_num,
               args->num_physical_segs,
               args->utcb_area_base << SHIFT_1K, args->utcb_area_log2_size,
               args->max_priority, args->has_kresources);

    space_t * space = allocate_space(res, spaceid(args->id), clist);
    if (!space) {
        panic("CREATE_SPACE: id=%d, space{%d:%d}, clist{%d:%d}, mutex{%d:%d), "
              "max_phys_segs=%d, utcb_area{0x%x:2^0x%x}, max_prio=%ld\n",
              args->id,
              args->space_id_base, args->space_id_num,
              args->clist_id_base, args->clist_id_num,
              args->mutex_id_base, args->mutex_id_num,
              args->num_physical_segs,
              args->utcb_area_base << SHIFT_1K, args->utcb_area_log2_size,
              args->max_priority);
    }

    space->space_range.set_range(args->space_id_base, args->space_id_num);
    space->mutex_range.set_range(args->mutex_id_base, args->mutex_id_num);
    space->clist_range.set_range(args->clist_id_base, args->clist_id_num);
    space->set_maximum_priority(args->max_priority);

    fpage_t utcb_area;
    utcb_area.set((args->utcb_area_base << SHIFT_1K),
                   args->utcb_area_log2_size, 0, 0, 0);

    if (!space->init(utcb_area, res)) {
        panic("Failed to map the kernel area\n");
    }

    if (args->has_kresources) {
        space->set_kmem_resource(res);
    }

    if (args->num_physical_segs > 0) {
        space->phys_segment_list = (segment_list_t*)
            res->alloc(kmem_group_physseg_list, sizeof(segment_list_t) +
                    ((args->num_physical_segs - 1) * sizeof(phys_segment_t*)),
                    true);

        space->phys_segment_list->set_num_segments(args->num_physical_segs);

        TRACE_INIT("CREATE_SEGMENT_LIST: entries=%d\n", args->num_physical_segs);
    }

    return space;

}

extern "C" CONTINUATION_FUNCTION(initial_to_user);

/* equivalent to scheduler_t::activate(thread_state_t::running)
 * without the smt_reschedule() */
INLINE void set_running_and_enqueue(tcb_t * tcb)
{
    tcb->set_state(thread_state_t::running);
    get_current_scheduler()->enqueue(tcb);
}

static tcb_t SECTION(SEC_INIT) *
    ki_create_thread(space_t *space, ki_create_thread_t *args,
                     kmem_resource_t *res)
{
    TRACE_INIT("CREATE_THREAD: cap_slot=%d, priroity=%d, ip=0x%x, sp=0x%x, utcb_addr=0x%x\n",
               args->cap_slot, args->priority,
               args->ip, args->sp,
               args->utcb_address);


    capid_t dest_tid = capid_t::capid(TYPE_CAP, args->cap_slot);
    tcb_t *tcb = allocate_tcb(dest_tid, res);
    if (!tcb) {
        panic ("Failed to allocate TCB\n");
    }

    if (!space->get_clist()->add_thread_cap(dest_tid, tcb)) {
        panic ("Failed to add thread CAP\n");
    }

    tcb->init();
    tcb->set_pager(NULL);
    tcb->set_exception_handler(NULL);

    /* set the space */
    tcb->set_space(space);
    space->add_tcb(tcb);

    tcb->set_utcb_location(args->utcb_address);

    /* activate the guy */
    if (!tcb->activate(initial_to_user, res)) {
        panic("failed to activate TCB\n");
    }
    get_current_scheduler()->set_priority(tcb, args->priority);
    tcb->set_user_ip((addr_t)args->ip);
    tcb->set_user_sp((addr_t)args->sp);

    /* Set root task's thread handle in root server's MR0 */
    tcb->set_mr(0, threadhandle(tcb->tcb_idx).get_raw());
    tcb->set_mr(1, args->mr1);
    /* Give rootserver location of tracebuffer */
#ifdef CONFIG_TRACEBUFFER                       \
    // XXX
    tcb->set_mr(2, (word_t)virt_to_phys(trace_buffer));
    tcb->set_mr(3, TBUFF_SIZE);
#else
    tcb->set_mr(2, 0);
    tcb->set_mr(3, 0);
#endif

    set_running_and_enqueue(tcb);
    return tcb;
}

static mutex_t SECTION(SEC_INIT)*
ki_create_mutex(space_t* space, ki_create_mutex_t* args)
{
    TRACE_INIT("CREATE_MUTEX: id=%d\n", args->id);
    ASSERT(ALWAYS, space->mutex_range.is_valid(args->id));

    kmem_resource_t *res = space->get_kmem_resource();
    mutex_t * mutex = (mutex_t *) res->alloc(kmem_group_mutex, true);
    ASSERT(ALWAYS, mutex);

    mutexid_t mutexid;
    mutexid.set_number(args->id);
    mutex->init(mutexid);
    mutexid_table.insert(mutexid, mutex);
    return mutex;
}

static void SECTION(SEC_INIT)
ki_create_segment(space_t* space, ki_create_segment_t *args)
{
    phys_segment_t *segment;

    ASSERT(ALWAYS, space->phys_segment_list);
    segment = space->phys_segment_list->lookup_segment(args->seg_num);
    ASSERT(ALWAYS, segment);

    TRACE_INIT("SETUP_SEGMENT: P:0x%llx..0x%llx, rwx:%lx, attrib:%lx\n",
            args->segment.get_base(),
            args->segment.get_base() + args->segment.get_size(),
            args->segment.get_rwx(), args->segment.get_attribs());
    *segment = args->segment;
}

extern bool map_region (space_t * space, word_t vaddr, word_t paddr, word_t size, word_t attr, word_t rwx, kmem_resource_t *kresource);

/*
 * This macro is temporary; once map_region() changes to the new form it
 * will go away.
 */
#define MAP_PGSIZE(pgs) (1UL << pgs)

// XXX : use segment
static void SECTION(SEC_INIT)
    ki_map_memory(space_t* space, ki_map_memory_t *args, kmem_resource_t *cur_heap)
{
    word_t size = args->desc.size.X.num_pages * MAP_PGSIZE(args->desc.size.X.page_size);
    phys_segment_t *segment = space->phys_segment_list->lookup_segment(args->desc.seg.X.segment);
    word_t phys = segment->get_base() + (args->desc.seg.X.offset << SHIFT_1K);

    TRACE_INIT("MAP_MEMORY: V:0x%lx O:0x%lx (P=%lx) S:0x%lx N:0x%x A=0x%x R=0x%x\n",
               args->desc.virt.X.vbase << SHIFT_1K,
               args->desc.seg.X.offset << SHIFT_1K,
               phys,
               size,
               args->desc.seg.X.segment,
               args->desc.virt.X.attr,
               args->desc.size.X.rwx);

    if (!map_region(space, args->desc.virt.X.vbase << SHIFT_1K,
                    phys, size, args->desc.virt.X.attr,
                    args->desc.size.X.rwx, cur_heap)) {
        panic("MAP_MEMORY: V:0x%lx O:0x%lx (P=%lx) S:0x%lx N:0x%x A=0x%x\n",
              args->desc.virt.X.vbase << SHIFT_1K,
              args->desc.seg.X.offset << SHIFT_1K,
              phys,
              size,
              args->desc.seg.X.segment,
              args->desc.virt.X.attr);
    }
}

static void SECTION(SEC_INIT)
ki_create_ipc_cap(ki_create_ipc_cap_t *args)
{
    TRACE_INIT("CREATE IPC CAP: clist_ref=%x, cap_slot=%d, thread_ref=%x\n",
               args->clist_ref,
               args->cap_slot,
               args->thread_ref);

    tcb_t* tcb = (tcb_t*)ki_get_ptr(args->thread_ref);
    clist_t* clist = (clist_t*)ki_get_ptr(args->clist_ref);
    capid_t cap = capid(args->cap_slot);
    if (!clist->add_ipc_cap(cap, tcb)) {
        panic("CREATE IPC CAP: clist_ref=%x, cap_slot=%d, thread_ref=%x\n",
              args->clist_ref,
              args->cap_slot,
              args->thread_ref);
    }
}

static void SECTION(SEC_INIT)
ki_create_mutex_cap(ki_create_mutex_cap_t *args)
{
    TRACE_INIT("CREATE MUTEX CAP: clist_ref=%x, cap_slot=%d, mutex_ref=%x\n",
               args->clist_ref,
               args->cap_slot,
               args->mutex_ref);
    UNIMPLEMENTED();
#if 0
    mutex_t* tcb = (mutex_t*)ki_get_ptr(args->mutex_ref);
    clist_t* clist = (clist_t*)ki_get_ptr(args->clist_ref);
    capid_t cap = capid(args->cap_slot);


    if (!clist->add_mutex_cap(cap, tcb)) {
        panic("CREATE MUTEX CAP: clist_ref=%x, cap_slot=%d, mutex_ref=%x\n",
              args->clist_ref,
              args->cap_slot,
              args->mutex_ref);
    }
#endif
}

static void SECTION(SEC_INIT)
ki_assign_irq(space_t* space, ki_assign_irq_t *args)
{
    TRACE_INIT("ASSIGN IRQ: irq=0x%x\n",
               args->irq);

    if (security_control_interrupt(
            (struct irq_desc *)&args->irq, space, 0)) {
        panic("ASSIGN IRQ: irq=0x%x\n", args->irq);
    }
}

static void SECTION(SEC_INIT)
ki_allow_platform_control(space_t* space, ki_allow_platform_control_t *args)
{
    TRACE_INIT("ALLOW_PLATFORM_CONTROL\n");
    space->allow_plat_control();
}

static ki_hdr_t* SECTION(SEC_INIT)
ki_get_first_op()
{
    word_t init_script_paddr = get_init_data()->init_script;
    TRACE_INIT("init_script paddr : %x\n", init_script_paddr);

    ki_init_script = (ki_header_t *)phys_to_virt(init_script_paddr);
    ASSERT(ALWAYS, ki_init_script->magic == KI_MAGIC);
    ASSERT(ALWAYS, ki_init_script->version == KI_VERSION);
    return (ki_hdr_t *)(ki_init_script + 1);
}

void SECTION(SEC_INIT)
run_init_script(word_t phase)
{
    static ki_hdr_t *cur;
    ki_hdr_t cur_saved;
    kmem_resource_t *cur_heap;
    clist_t *cur_clist = NULL;
    space_t *cur_space = NULL;
    tcb_t *cur_tcb = NULL;

    if (phase == INIT_PHASE_FIRST_HEAP) {
        cur = ki_get_first_op();
        cur_heap = NULL;
    } else {
        cur_heap = get_current_kmem_resource();
    }

    do {
        cur_saved = *cur;

        switch (cur->op) {
        case KI_OP_CREATE_HEAP: {
            ki_create_heap_t *args = (ki_create_heap_t *)cur;
            cur_heap = ki_create_heap(args);
            if (!get_kernel_space()->get_kmem_resource()) {
                get_kernel_space()->set_kmem_resource(cur_heap);
            }
            break;
        }

        case KI_OP_INIT_IDS: {
            ki_init_ids_t *args = (ki_init_ids_t *)cur;
            ki_init_ids(args, cur_heap);
            break;
        }

        case KI_OP_CREATE_THREAD_HANDLES: {
            ki_create_thread_handles_t *args = (ki_create_thread_handles_t *)cur;
            ki_create_thread_handles(args);
            break;
        }

        case KI_OP_CREATE_CLIST: {
            ASSERT(ALWAYS, cur_heap);
            ki_create_clist_t *args = (ki_create_clist_t *)cur;
            cur_clist = ki_create_clist(cur_heap, args);
            ki_set_ptr(cur, cur_clist);
            break;
        }

        case KI_OP_CREATE_SPACE: {
            ASSERT(ALWAYS, cur_clist);
            ki_create_space_t *args = (ki_create_space_t *)cur;
            cur_space = ki_create_space(cur_heap, cur_clist, args);
            break;
        }

        case KI_OP_CREATE_THREAD: {
            ASSERT(ALWAYS, cur_space);
            ki_create_thread_t *args = (ki_create_thread_t *)cur;
            cur_tcb = ki_create_thread(cur_space, args, cur_heap);
            ki_set_ptr(cur, cur_tcb);
            break;
        }

        case KI_OP_CREATE_MUTEX: {
            ASSERT(ALWAYS, cur_space);
            ki_create_mutex_t *args = (ki_create_mutex_t *)cur;
            mutex_t *mutex = ki_create_mutex(cur_space, args);
            ki_set_ptr(cur, mutex);
            break;
        }

        case KI_OP_CREATE_SEGMENT: {
            ASSERT(ALWAYS, cur_space);
            ki_create_segment_t *args = (ki_create_segment_t *)cur;
            ki_create_segment(cur_space, args);
            break;
        }

        case KI_OP_MAP_MEMORY: {
            ASSERT(ALWAYS, cur_space);
            // ASSERT(ALWAYS, cur_segment);   XXX
            ki_map_memory_t *args = (ki_map_memory_t *)cur;
            ki_map_memory(cur_space, args, cur_heap);
            break;
        }

        case KI_OP_CREATE_IPC_CAP: {
            ki_create_ipc_cap_t *args = (ki_create_ipc_cap_t *)cur;
            ki_create_ipc_cap(args);
            break;
        }

        case KI_OP_CREATE_MUTEX_CAP: {
            ki_create_mutex_cap_t *args = (ki_create_mutex_cap_t *)cur;
            ki_create_mutex_cap(args);
            break;
        }

        case KI_OP_ASSIGN_IRQ: {
            ASSERT(ALWAYS, cur_space);
            ki_assign_irq_t *args = (ki_assign_irq_t *)cur;
            ki_assign_irq(cur_space, args);
            break;
        }

        case KI_OP_ALLOW_PLATFORM_CONTROL: {
            ASSERT(ALWAYS, cur_space);
            ki_allow_platform_control_t *args =
                (ki_allow_platform_control_t *)cur;
            ki_allow_platform_control(cur_space, args);
            break;
        }

        default:
            panic("Unknown op(0x%x) at 0x%x : \n", (int)cur->op, cur);
        }

        cur = (ki_hdr_t *)((word_t *)cur  + cur_saved.size);
    } while (!cur_saved.end_of_phase);
}
