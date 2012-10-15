/*
 * Copyright (c) 2002-2004, Karlsruhe University
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Copyright (c) 2005-2006, National ICT Australia (NICTA)
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
 * Description:   thread manipulation
 */

#include <l4.h>
#include <config.h>
#include <tcb.h>
#include <ipc.h>
#include <arch/ipc.h>
#include <capid.h>
#include <schedule.h>
#include <space.h>
#include <syscalls.h>
#include <smp.h>
#include <linear_ptab.h>
#include <kernel/generic/lib.h>
#include <ref_inline.h>
#include <kdb/tracepoints.h>

DECLARE_TRACEPOINT(SYSCALL_THREAD_CONTROL);

//#define TRACE_XCPU(x...) printf(x)
#define TRACE_XCPU(x...)

extern "C" CONTINUATION_FUNCTION(initial_to_user);
void handle_ipc_error (void);

/**
 * Stub invoked after a startup message has been received from the
 * thread's pager.
 *
 * This is a continuation function that completes by activating the
 * initial_to_user continuation function
 */
CONTINUATION_FUNCTION(thread_startup)
{
    tcb_t * current = get_current_tcb();

#if 0
    printf ("Startup %t: ip=%p  sp=%p\n", current,
            current->get_mr (1), current->get_mr (2));
#endif

    /** Poke received IP/SP into exception frame (or whatever is used
     *  by the architecture).  No need to check for valid
     *  IP/SP. Thread will simply fault if values are not valid.
     */
    current->set_user_ip((addr_t)current->get_mr(1));
    current->set_user_sp((addr_t)current->get_mr(2));
    current->set_saved_state(thread_state_t::aborted);
    current->sys_data.set_action(tcb_syscall_data_t::action_ipc);
    ACTIVATE_CONTINUATION(initial_to_user);
}


/**
 * Fake that thread is waiting for IPC startup message.
 *
 * @param tcb           thread to wait for startup message
 * @param pager         thread id to receive startup message from
 */
static void
fake_wait_for_startup(tcb_t * tcb)
{
    /* Setup a dummy IPC receive from our pager, waiting
     * for them to start us up. */
    tcb->initialise_state(thread_state_t::waiting_forever);
    tcb->set_partner(tcb->get_pager());
    tcb->set_acceptor(acceptor_t::untyped_words());

    // Make sure that unwind will work on waiting thread.
    tcb->set_saved_partner(NULL);

    /* #warning VU: revise fake_wait_for_startup */
    // Make sure that IPC abort will restore user-level exception
    // frame instead of trying to return from IPC syscall.
    tcb->set_saved_state (thread_state_t::running);
}

#define END_TCB_FREE_LIST 0x0FFFFFFFUL

/*
 * Initialize TCB memory allocator, setting object
 * size and max number of objects
 */
void SECTION(SEC_INIT)
init_tcb_allocator(void)
{
    extern word_t num_tcbs;
    extern tcb_t** thread_handle_array;
    word_t i;

    ASSERT(ALWAYS, num_tcbs < END_TCB_FREE_LIST);

    /* Initialize free tcb list */
    for (i = 0; i < num_tcbs; i++)
    {
        thread_handle_array[i] = (tcb_t *)(i+1);
    }
    thread_handle_array[num_tcbs - 1] = (tcb_t *)END_TCB_FREE_LIST;
}

tcb_t*
allocate_tcb(capid_t tid, kmem_resource_t *kresource)
{
    /* Get the head of the free list */
    extern word_t free_tcb_idx;
    extern tcb_t** thread_handle_array;
    word_t me = free_tcb_idx;
    /* if there is a free tcb */
    if (me != END_TCB_FREE_LIST) {
        /* advance the free pointer */
        tcb_t* ret = (tcb_t *)kresource->alloc(kmem_group_tcb, true);
        if (ret != NULL)
        {
            free_tcb_idx = (word_t)thread_handle_array[me];
            thread_handle_array[me] = ret;
            /* set up the allocated tcb */
            ret->tcb_idx = me;
        }
        return ret;
    } else {
        return NULL;
    }
}

void
free_tcb(tcb_t *tcb)
{
    /* Add the tcb to the head of the free list */
    extern word_t free_tcb_idx;
#ifdef CONFIG_DEBUG
    extern word_t num_tcbs;
#endif
    extern tcb_t** thread_handle_array;

    ASSERT(ALWAYS, tcb->tcb_idx < num_tcbs);
    thread_handle_array[tcb->tcb_idx] = (tcb_t *)free_tcb_idx;
    free_tcb_idx = tcb->tcb_idx;
    /* Free tcb */
    get_current_kmem_resource()->free(kmem_group_tcb, tcb);
}

/**
 * Initialize a new thread
 */
void tcb_t::init(void)
{
    ASSERT(DEBUG, this);

    /* clear utcb and space */
    utcb = NULL;
    space = NULL;

    /* make sure, nobody messes around with the thread */
    initialise_state(thread_state_t::aborted);
    set_saved_state(get_state());
    partner = NULL;
    set_saved_partner(NULL);

    /* Initialize queues */
    ready_list.next = NULL;
    blocked_list.next = NULL;

    /* initialize thread resources */
    resources.init(this);

    /* flags */
    runtime_flags = 0;
    post_syscall_callback = NULL;

    /* Initialise IPC end-point. */
    this->end_point.init(this);

    /* Lock initialization */
    this->thread_lock.init();

    PROFILE_TCB_INIT(this);

    /* IPC Control initialization */
    space_id = spaceid_t::kernelspace();

#if defined(CONFIG_MDOMAINS) || defined(CONFIG_MUNITS)

    /* #warning VU: uninitialized threads assigned to CPU */
    /* initially assign to this CPU */
    okl4_atomic_set(&this->context, 0xffff);
#endif

    /* initialize scheduling */
#if defined(CONFIG_MUNITS)
    (void)okl4_atomic_set(&reserved, 0);
#endif
    get_current_scheduler()->init_tcb(this);

    /* enqueue into present list, do not enqueue
     * the idle thread since they are CPU local and break
     * the pointer list */
    enqueue_present();

    init_stack();

    this->pager.init();
    this->exception_handler.init();
}


bool SECTION(SEC_INIT)
tcb_t::create_kernel_thread(utcb_t * new_utcb)
{
    //TRACEF("dest=%t\n", TID(dest));
    this->init();
    this->utcb = new_utcb;
    this->utcb_location = (word_t)new_utcb;
    this->page_directory = get_kernel_space()->pgent(0);

    // kernel threads have prio 0 by default
    get_current_scheduler()->set_priority(this, 0);

    get_kernel_space()->add_tcb(this);
    return true;
}

bool tcb_t::activate(void (*startup_func)(), kmem_resource_t *kresource)
{
    ASSERT(DEBUG, this);
    ASSERT(NORMAL, this->space);
    ASSERT(NORMAL, !this->get_utcb());

    // UTCB location has already been checked during thread creation.
    // No need to check it again.  Just do an assert.

    //TRACEF("%x (%x)\n", get_utcb_location(), ((1 << get_kip()->utcb_info.alignment) - 1));

    /* allocate UTCB */
    this->utcb = get_space()->allocate_utcb(this, kresource);
    if (!this->utcb)
        return false;
    //ASSERT(DEBUG, check_utcb_location());

#ifdef CONFIG_PLAT_SURF
    /* Init debug profiling */
    prof_set_id_init(this->utcb);
#endif

    this->space_id = get_space()->get_space_id();

    /* update mutex thread handle in UTCB */
    set_mutex_thread_handle(threadhandle(this->tcb_idx));

    /* initialize the startup stack */
    create_startup_stack(startup_func);
    return true;
}

/**
 * Check if supplied address is a valid UTCB location.  It is assumed
 * that the space of the thread is properly initialized.
 *
 * @param utcb_address          location of UTCB
 *
 * @return true if UCTB location is valid, false otherwise
 */
bool generic_space_t::check_utcb_location (word_t utcb_address)
{
#ifdef NO_UTCB_RELOCATE
    /* UTCB is kernel allocated, utcb_address must be -1UL */
    return utcb_address == ~0UL;
#else
    return (((word_t)addr_align((addr_t)utcb_address, 
                                CONFIG_UTCB_SIZE) == utcb_address) &&
            get_utcb_area().is_range_in_fpage((addr_t) utcb_address,
                                              (addr_t) (utcb_address +
                                                        sizeof (utcb_t))));
#endif
}

/**
 * Cancel all pending IPCs related to this thread, and change the thread
 * into an aborted state.
 *
 * This will unwind the IPC of any thread currently sending to us, and if we
 * are currently blocked on a send or receive, cancel that as well.
 *
 * Function must be called with current thread locked.
 */
void
tcb_t::cancel_ipcs(void)
{
    /* Unwind any IPC that we happen to be in the middle of. */
    if (this->is_partner_valid())
    {
        tcb_t *partner = get_partner();
        if (partner == this) {
            this->unwind(partner);
        } else if (((word_t)partner < SPECIAL_RAW_LIMIT) && this->get_space()) {
            this->unwind(partner);
        }
        /* do nothing for partner == anythread / kernel / uninitialized threads */
    }

    /* Cancel any IPCs of other threads that are sending to us. */
    while (this->end_point.get_send_head()) {
        // SMT: this is safe to do where
        tcb_t * tcb = this->end_point.get_send_head();

        ASSERT(DEBUG, tcb != this);

        /* Unwind the sender's IPC, and notify them of the error. */
        get_current_scheduler()->pause(tcb);
        tcb->unwind(this);
        tcb->notify(handle_ipc_error);
        get_current_scheduler()->activate(tcb, thread_state_t::running);
    }

    /* Cancel any IPCs of other threads that are receiving from us. */
    while (this->end_point.get_recv_head()) {
        // SMT: this is safe to do where
        tcb_t * tcb = this->end_point.get_recv_head();

        ASSERT(DEBUG, tcb != this);

        /* Unwind the receiver's IPC, and notify them of the error. */
        get_current_scheduler()->pause(tcb);
        tcb->unwind(this);
        tcb->notify(handle_ipc_error);
        get_current_scheduler()->activate(tcb, thread_state_t::running);
    }
}

/**
 * Release any mutexes that we currently hold.
 *
 * @return  A boolean value indicating if a schedule() call is required.
 */
bool
tcb_t::release_mutexes(void)
{
    bool need_reschedule = false;
    scheduler_t * scheduler = get_current_scheduler();

    /* Ensure that we are not currently on the ready queue. */
    ASSERT(ALWAYS, !this->ready_list.is_queued());

    /* If we are blocked on a mutex, unwind that. */
    if (get_state().is_waiting_mutex()) {
        this->unwind(NULL);
        get_current_scheduler()->activate(this, thread_state_t::running);
        need_reschedule = true;
    }

    /* Clean up all currently held mutexes. */
    mutex_t * mutex;

    /**
     *  @todo FIXME: need to grab the scheduler lock to serialise held
     *  list access - awiggins.
     */
    while ((mutex = this->mutexes_head) != NULL) {
        tcb_t * new_holder;

        mutex->mutex_lock.lock();
        new_holder = mutex->release();
        /* If the mutex has a new holder, make them runnable. */
        if (new_holder != NULL) {
            scheduler->activate(new_holder, thread_state_t::running);
            need_reschedule = true;
        }
        mutex->mutex_lock.unlock();
    }
    return need_reschedule;
}

/**
 * Try to aquire a read-lock on a thread
 *
 * @param tcb tcb of thread to try lock
 *
 * @returns NULL if thread's cap deleted, tcb if lock acquired
 */
tcb_t *
acquire_read_lock_tcb(tcb_t *tcb, tcb_t *tcb_locked)
{
    cap_t lookup;
    tcb_t *cap_tcb;

try_valid_cap_lookup:
    okl4_atomic_barrier_smp();
    cap_t *master_cap = tcb->master_cap;

    if (EXPECT_TRUE(master_cap != NULL)) {
        lookup = *master_cap;

        if (EXPECT_FALSE(!lookup.is_thread_cap())) {
            return NULL;
        }

        cap_tcb = lookup.get_tcb();

        /* if cap still points to this tcb */
        if (EXPECT_TRUE(cap_tcb == tcb)) {
            if (tcb == tcb_locked) {
                tcb->lock_read_already_held();
            } else if (EXPECT_FALSE(!tcb->try_lock_read())) {
                okl4_atomic_barrier_smp();
                goto try_valid_cap_lookup;
            }
            return tcb;
        }
    }
    return NULL;
}

/**
 * Lookup a TCB by its thread-handle
 *
 * @param threadhandle Thread handle of thread to lookup
 *
 * @returns NULL if non-existing, else tcb (locked)
 */
tcb_t*
lookup_tcb_by_handle_locked(word_t threadhandle)
{
    extern tcb_t ** thread_handle_array;
    extern word_t num_tcbs;
    tcb_t *tcb;

    word_t idx = (threadhandle << 1) >> 1;

    if (idx < (num_tcbs))
    {
        if (((word_t)thread_handle_array[threadhandle] > VIRT_ADDR_BASE))
        {
            tcb = thread_handle_array[threadhandle];
            return acquire_read_lock_tcb(tcb);
        }
        else
            return NULL;
    }
    else
    {
        return NULL;
    }
}

/**
 * Delete a thread
 */
void
tcb_t::delete_tcb(kmem_resource_t *kresource)
{
    scheduler_t *scheduler = get_current_scheduler();

    scheduler->pause(this);

    this->cancel_ipcs();

    /* Unwind ourselves thread into an aborted state. */
    this->unwind(NULL);

    /* Release any syncpoints owned by the thread. */
    (void)this->release_mutexes();

    /*
     * XXX: For SMT, references to this TCB in other units must also
     * be cleared. It means 'schedule' fields in
     * scheduler_t::smt_threads[],
     */
    if (get_active_schedule() == this) {
        set_active_schedule(get_current_tcb());
    }
    scheduler->delete_tcb(this);

    PROFILE_STOP_ALL(this);

    // free any used resources
    resources.free(this, kresource);

    set_pager(NULL);
    set_exception_handler(NULL);

    this->set_space(NULL);
    dequeue_present();

#if defined(CONFIG_THREAD_NAMES)
    debug_name[0] = '\0';
#endif
    sys_data.set_action(tcb_syscall_data_t::action_none);
    free_tcb(this);

}


/**
 * Calculate the sender and receiver errorcodes when an IPC operation
 * has been aborted either by exchange_registers.
 *
 * @param reason                reason for abort
 * @param snd                   sender thread
 * @param rcv                   receiver thread
 * @param err_s                 returned sender error code
 * @param err_r                 returned receiver error code
 */
INLINE void
calculate_errorcodes( tcb_t * snd, tcb_t * rcv,
                     word_t * err_s, word_t * err_r)
{
    *err_s = IPC_SND_ERROR(ERR_IPC_ABORTED);
    *err_r = IPC_RCV_ERROR(ERR_IPC_ABORTED);
}

DECLARE_TRACEPOINT(UNWIND);

/**
 * Unwinds a thread from an ongoing IPC.
 *
 * @pre Thread is currently paused by caller.
 * @pre !get_state().is_runnable().
 *
 * @param partner  Partner TCB of the thread
 */
void
tcb_t::unwind(tcb_t *partner)
{
    tcb_t *ipc_partner = partner;

    TRACEPOINT(UNWIND,
               printf("Unwind: tcb=%t p=%t s=%s (saved: p=%t s=%s)\n",
                      this,
                      is_partner_valid() ? get_partner() : NULL,
                      get_state().string(), get_saved_partner(),
                      get_saved_state().string()));

    thread_state_t orig_cstate = get_state();
    thread_state_t orig_sstate = get_saved_state();
    tcb_t * orig_cpartner UNUSED = is_partner_valid() ? get_partner() : NULL;
    tcb_t * orig_spartner UNUSED = get_saved_partner();


    thread_state_t cstate = get_state();

    if (cstate.is_running()) {
        /* Abort the thread. */
        get_current_scheduler()->deactivate(this, thread_state_t::aborted);
        return;
    }

    if (cstate.is_aborted()) {
        /* Thread is already aborted. */
        return;
    }

    if (cstate.is_halted()) {
        /* Thread is halted. Transition it to the aborted state. */
        get_current_scheduler()->update_inactive_state(this,
                thread_state_t::aborted);
        return;
    }

    if (cstate.is_polling() || cstate.is_waiting() ||
        cstate.is_waiting_notify())
    {
        msg_tag_t tag = get_tag();

        // IPC operation has not yet started.  I.e., ipc_partner is not
        // yet involved.

        if (cstate.is_polling()) {
            // The thread is enqueued in the send queue of the ipc_partner.
            ASSERT(ALWAYS, ipc_partner != NULL);
            ipc_partner->end_point.dequeue_send(this);
        }
        else
        {
            /* Thread is not yet receiving. Cancel the receive phase. */
            tag = tag.error_tag();
            this->remove_dependency();
        }

        // Set appropriate error code
        clear_exception_ipc();
        tag.set_error ();
        set_tag (tag);
        word_t err = ERR_IPC_CANCELED;
        set_error_code ((cstate.is_polling ()) ?
                       IPC_SND_ERROR (err) : IPC_RCV_ERROR (err));
        get_current_scheduler()->update_inactive_state(this, thread_state_t::aborted);
        PROFILE_STOP_IF_RUNNING(this, sys_ipc_e);
        return;
    }

    if (cstate.is_waiting_mutex()) {
        mutex_t * mutex = TCB_SYSDATA_MUTEX(this)->mutex;

        get_current_scheduler()->scheduler_lock();
        mutex->sync_point.unblock(this);
        get_current_scheduler()->scheduler_unlock();
        TCB_SYSDATA_MUTEX(this)->mutex = NULL;
        get_current_scheduler()->update_inactive_state(this,
                thread_state_t::aborted);
        return;
    }

#ifdef CONFIG_DEBUG
    panic("Unresolved unwind: tcb=%t p=%t s=%s (saved: p=%t s=%s)\n",
          this,
          orig_cpartner, orig_cstate.string(),
          orig_spartner, orig_sstate.string());
#else
    panic("Unresolved unwind: tcb=%t p=%t (saved: p=%t)\n",
          this,
          orig_cpartner,
          orig_spartner);
#endif
}

/**
 * Handler invoked when IPC errors occur.  Any IPC resources are
 * released and control is transferred to user-level.  If the aborted
 * IPC happens to be a pagefault IPC (caused by user-level memory
 * access) we need to restore the thread prior to the pagefault
 * and return to user-level.
 *
 * This function returns by shortcutting the normal
 * continuations and proceeding directly to the ASM return to user routines
 */
CONTINUATION_FUNCTION(handle_ipc_error)
{
    tcb_t * current = get_current_tcb();
    thread_state_t saved_state = current->get_saved_state();

    //TRACEF("saved state == %x\n", (word_t)current->get_saved_state ());

    // We're going to skip the last part of the switch_to() function
    // invoked when switching from the current thread.  Make sure that
    // we still manage to load up the appropriate resources when
    // switching to the thread.

    if (EXPECT_FALSE (current->resource_bits))
        current->resources.load (current);

    if (saved_state.is_running())
    {
        // Thread was doing a pagefault IPC.  Restore thread state
        // prior to IPC operation and return directly to user-level.
        current->restore_state(3);
        current->return_from_user_interruption();
    }
    else
    {
        current->set_saved_state(thread_state_t::aborted); // sanity
        TCB_SYSDATA_IPC(current)->from_tid = capid_t::nilthread();
        current->return_from_ipc();
    }

    NOTREACHED();
}


/* Assumes thread_state_lock is held by caller */
void tcb_t::save_state (word_t mrs)
{
    for (word_t i = 0; i < mrs; i++) {
        TCB_SYSDATA_IPC(this)->saved_mr[i] = get_mr(i);
    }

    TCB_SYSDATA_IPC(this)->saved_notify_mask = get_notify_mask();
    TCB_SYSDATA_IPC(this)->saved_error = get_error_code();
    ASSERT (NORMAL, get_saved_partner() == NULL);
    ASSERT (NORMAL, get_saved_state() == thread_state_t::aborted);

    saved_partner = partner;
    saved_sent_from = sent_from;

    set_saved_state (get_state ());
}

/**
 *  Restores a thread's message registers, state, IPC partner and notify mask
 *  to the values saved in save_state().
 *
 *  The thread may or may not be runnable at this stage: We may have been
 *  aborted an just woken up, or we may be restoring state after a pagefault
 *  IPC has been completed (and hence we are runnable, because our IPC receiver
 *  just woke us up.)
 */
void
tcb_t::restore_state(word_t mrs)
{
    thread_state_t saved_state = get_saved_state();
    scheduler_t * scheduler = get_current_scheduler();

    for (word_t i = 0; i < mrs; i++) {
        set_mr(i, TCB_SYSDATA_IPC(this)->saved_mr[i]);
    }
    set_notify_mask(TCB_SYSDATA_IPC(this)->saved_notify_mask);

    sent_from = saved_sent_from;
    partner = saved_partner;

    if (get_state().is_runnable()) {
        if (saved_state.is_runnable()) {
            scheduler->update_active_state(this, saved_state);
        } else {
            scheduler->deactivate(this, saved_state);
        }
    } else {
        if (saved_state.is_runnable()) {
            scheduler->activate(this, saved_state);
        } else {
            scheduler->update_inactive_state(this, saved_state);
        }
    }
    set_error_code(TCB_SYSDATA_IPC(this)->saved_error);

    set_saved_partner(NULL);
    set_saved_state(thread_state_t::aborted);
}

CONTINUATION_FUNCTION(finish_kernel_ipc);
/**
 * Send a pagefault ipc to the current threads pager with the given
 * addr, ip, access
 *
 * This is a control function as the ipc may block.
 *
 * Note that this function will leave all thread state unmodified when the
 * continuation is activated (except obviously for any changes made by other
 * threads in the system, such as mapping the page)
 *
 * @param addr The address of the pagefault
 * @param ip The address of the faulting instruction
 * @param access The type of access which caused the fault
 * @param continuation The continuation to activate upon completion
 */
void tcb_t::send_pagefault_ipc (addr_t addr, addr_t ip,
                                space_t::access_e access, continuation_t continuation)
{
    if (this->get_pager() == NULL)
    {
        //TRACEF("%t: null pager: %s pagefault at %p, ip=%p - deny\n",
        //        this,
        //        access == space_t::write     ? "write" :
        //        access == space_t::read         ? "read"  :
        //        access == space_t::execute   ? "execute" :
        //        access == space_t::readwrite ? "read/write" :
        //        "unknown",
        //        addr, ip);
        ACTIVATE_CONTINUATION(continuation);
    }

    ASSERT(DEBUG, get_pager() != NULL);
    sys_data.set_action(tcb_syscall_data_t::action_ipc);
    save_state(3);

    /* generate pagefault message */
    msg_tag_t tag;
    tag.set(2, PAGEFAULT_TAG |
            ((access == space_t::read)      ? (1 << 2) : 0) |
            ((access == space_t::write)     ? (1 << 1) : 0) |
            ((access == space_t::execute)   ? (1 << 0) : 0) |
            ((access == space_t::readwrite) ? (1 << 2)+(1 << 1) : 0), true, true);

    set_tag(tag);
    set_mr(1, (word_t)addr);
    set_mr(2, (word_t)ip);
    set_notify_mask (0);


    //TRACEF("send pagefault IPC (%t)\n", TID(get_pager()));
    TCB_SYSDATA_IPC(this)->ipc_continuation = continuation;

    do_ipc(get_pager(), get_pager(), finish_kernel_ipc);
}

/**
 * Set new pager for a thread
 * @param tcb TCB of new pager
 */
void tcb_t::set_pager(tcb_t *tcb)
{
    if (get_pager()) {
        this->pager.remove_referenced(get_pager());
    }
    if (tcb) {
        this->pager.set_referenced(tcb);
    }
}

/**
 * Set new exception handler for a thread
 * @param tcb   TCB of new exception handler
 */
void tcb_t::set_exception_handler(tcb_t *tcb)
{
    if (get_exception_handler()) {
        this->exception_handler.remove_referenced(get_exception_handler());
    }
    if (tcb) {
        this->exception_handler.set_referenced(tcb);
    }
}

/**
 * Continuation function to finish up a pagefault ipc
 *
 * Simply restores the threads state before activating the next continuation
 *
 * All parameters are in the TCB
 *
 * @param ipc_continuation the continuation to activate upon completion (as passed to send_pagefault_ipc)
 */
CONTINUATION_FUNCTION(finish_kernel_ipc)
{
    /*
     * explicitly don't care if the IPC failed or not,
     * we will re-fault if somthing was not handled correctly
     */
    tcb_t * current = get_current_tcb();

    current->restore_state(3);
    ACTIVATE_CONTINUATION(TCB_SYSDATA_IPC(current)->ipc_continuation);
}

/**
 * Return from the SYS_THREAD_CONTROL syscall with a success return code.
 */
static
CONTINUATION_FUNCTION(finish_sys_thread_control)
{
    tcb_t * current = get_current_tcb();
    continuation_t cont =
        TCB_SYSDATA_THREAD_CTRL(current)->thread_ctrl_continuation;
    PROFILE_STOP(sys_thread_ctrl);
    return_thread_control(1, cont);
}

/**
 * Return from the SYS_THREAD_CONTROL syscall with an error return code.
 */
static
CONTINUATION_FUNCTION(abort_sys_thread_control)
{
    tcb_t * current = get_current_tcb();

    continuation_t cont =
        TCB_SYSDATA_THREAD_CTRL(current)->thread_ctrl_continuation;
    PROFILE_STOP(sys_thread_ctrl);
    return_thread_control(0, cont);
}

#if defined(CONFIG_DEBUG)
/*
 * Translate a pointer to somewhere in a TCB to a TCB reference.
 */
tcb_t * get_tcb(void * ptr)
{
    tcb_t * walk = global_present_list;
    word_t target = (word_t)ptr;

    /* If there are no threads in the system, the pointer can't
     * be pointing to a TCB. */
    if (global_present_list == NULL) {
        return NULL;
    }

    /* Walk the present list, testing if the pointer lies in any
     * of the TCBs. */
    do {
        word_t start = (word_t)walk;
        word_t end = (word_t)walk + (word_t)KTCB_SIZE - 1;
        if (start <= target && target <= end) {
            return walk;
        }
        walk = walk->present_list.next;
    } while (walk != global_present_list);

    /* No such thread exists. */
    return NULL;
}
#endif

/*
 * Called when the user thread is just about to re-enter userspace, but
 * the TCB has requested that a given function be called prior to the
 * thread hitting userspace.
 */
extern "C" void
start_post_syscall_callback(void)
{
    continuation_t cont = ASM_CONTINUATION;
    tcb_t * current = get_current_tcb();

    ASSERT(DEBUG, current->get_post_syscall_callback() != NULL);
    current->get_post_syscall_callback()(cont);

    ACTIVATE_CONTINUATION(cont);
}

#ifdef CONFIG_SCHEDULE_INHERITANCE
/**
 * Determine the highest priority syncpoint we currently hold.
 */
#if !defined(CONFIG_ENABLE_FASTPATHS) \
        || !defined(HAVE_TCB_CALC_EFFECTIVE_PRIO_FASTPATH)
prio_t
tcb_t::calc_effective_priority()
{
    SMT_ASSERT(ALWAYS, get_current_scheduler()->schedule_lock.is_locked(true));

    prio_t max = this->base_prio;

    /*
     * Determine max priority on incoming IPCs.
     *
     * Threads attempting to send to us...
     */
    tcb_t * send_head = this->end_point.get_send_head();
    if (send_head != NULL && send_head->effective_prio > max) {
        max = send_head->effective_prio;
    }

    /* Threads attempting to receive from us... */
    tcb_t * recv_head = this->end_point.get_recv_head();
    if (recv_head != NULL && recv_head->effective_prio > max) {
        max = recv_head->effective_prio;
    }

    /* Determine max prioirty on held mutexes. */
    mutex_t * mutex_list = this->mutexes_head;
    mutex_t * first = mutex_list;

    if (mutex_list) {
        do {
            ASSERT(ALWAYS, mutex_list->sync_point.get_donatee() == this);
            tcb_t * blocked_head = mutex_list->sync_point.get_blocked_head();

            if (blocked_head != NULL && blocked_head->effective_prio > max) {
                max = blocked_head->effective_prio;
            }
            mutex_list = mutex_list->held_list.next;
        } while (mutex_list != first);
    }

    return max;
}
#endif
#endif

void
tcb_t::remove_dependency(void)
{
    syncpoint_t *syncpoint;

    get_current_scheduler()->schedule_lock.lock();
    syncpoint = waiting_for;
    if (syncpoint != NULL) {
        syncpoint->unblock(this);
    }
    get_current_scheduler()->schedule_lock.unlock();
}

/**********************************************************************
 *
 *             global thread management
 *
 **********************************************************************/

/**
 * @brief Setup the thread handlers
 *
 * #pager_tid and #exception_handler_tid must be thread resources.
 *
 * @retval true on success
 * @retval false if either tid is not a thread resource of the id does not
 *               return a valid tcb.
 * @todo XXX - This function is not SMT safe. upgradable reader-writer locks
 * are required to implement this correctly.
 */
static bool set_thread_handlers(tcb_t *dest_tcb, capid_t pager_tid,
                                capid_t except_handler_tid)
{
    ASSERT(DEBUG, dest_tcb->is_locked());
    tcb_t *tcb = NULL;

    if (pager_tid.is_cap_type()) {
        tcb = get_current_clist()->lookup_thread_cap_unlocked(pager_tid);
        if (tcb == NULL) {
            //TRACEF("Invalid pager thread cap\n");
            return false;
        }
        dest_tcb->set_pager(tcb);
    } else {
        /*
         * NOTE: according to the manuals, use of anythread is not
         * actually supported.  But it worked before the error reporting
         * was added, so we're letting it work for now.
         */
        if (!pager_tid.is_nilthread() && !pager_tid.is_anythread()) {
            //TRACEF("Invalid pager_tid\n");
            return false;
        }
    }

    if (except_handler_tid.is_cap_type()) {
        tcb = get_current_clist()->lookup_thread_cap_unlocked(except_handler_tid);
        if (tcb == NULL) {
            //TRACEF("Invalid exception handler thread cap\n");
            return false;
        }
        dest_tcb->set_exception_handler(tcb);
    } else {
        /*
         * NOTE: according to the manuals, use of anythread is not
         * actually supported.  But it worked before the error reporting
         * was added, so we're letting it work for now.
         */
        if (!except_handler_tid.is_nilthread() && !except_handler_tid.is_anythread()) {
            //TRACEF("Invalid except_handler_tid\n");
            return false;
        }
    }

    return true;
}


SYS_THREAD_CONTROL (capid_t dest_tid, spaceid_t space_id,
                    capid_t unused, capid_t pager_tid,
                    capid_t except_handler_tid,
                    tc_resources_t thread_resources,
                    word_t utcb_address)
{
    LOCK_PRIVILEGED_SYSCALL();
    PROFILE_START(sys_thread_ctrl);
    tcb_t *dest_tcb;
    continuation_t continuation = ASM_CONTINUATION;
    NULL_CHECK(continuation);
    kmem_resource_t *kresource;

    TRACEPOINT (SYSCALL_THREAD_CONTROL,
                printf ("SYS_THREAD_CONTROL: dest=%t, space=%d, "
                        "pager=%t, except_handler=%t, utcb=%x\n",
                        TID (dest_tid), space_id.get_spaceno(),
                        TID (pager_tid), TID (except_handler_tid),
                        utcb_address));

    /* Get current TCB, and set continuation data to 'thread control' mode. */
    tcb_t * current = get_current_tcb();

    /* Flag determining if a full schedule is required prior to the
     * completion of the system call. */
    bool need_to_schedule = false;

    current->sys_data.set_action(tcb_syscall_data_t::action_thread_control);

    if (EXPECT_FALSE(dest_tid.is_myself() || pager_tid.is_myself() ||
        except_handler_tid.is_myself()))
    {
        //TRACEF("thread_control with myself as argument\n");
        current->set_error_code (EINVALID_THREAD);
        goto error_out;
    }

    /* Check whether the caller has any resources. */
    if (EXPECT_FALSE(!is_kresourced_space(get_current_space())))
    {
        current->set_error_code (EINVALID_SPACE);
        goto error_out;
    }

    /* Check for valid target cap identifier */
    if (EXPECT_FALSE(!dest_tid.is_cap_type()))
    {
        current->set_error_code (EINVALID_THREAD);
        goto error_out;
    }

    kresource = get_current_kmem_resource();

    if (space_id.is_nilspace())
    {
        /* === Thread modification/deletion === */
modify_try_again:
        /* Ensure a valid thread (master) cap is used */
        dest_tcb = get_current_clist()->lookup_thread_cap_locked(dest_tid, true);
        if (EXPECT_FALSE(dest_tcb == NULL))
        {
            current->set_error_code(EINVALID_THREAD);
            goto error_out;
        }

        if (utcb_address == 0)
        {
            /* === Thread deletion === */
            if (EXPECT_FALSE(dest_tcb == current))
            {
                /* do not allow deletion of ourself */
                current->set_error_code (EINVALID_THREAD);
                dest_tcb->unlock_write();
                goto error_out;
            }
            else
            {
                /* ok, delete the thread */
                space_t *space = dest_tcb->get_space();

                if (!get_current_clist()->remove_thread_cap(dest_tid)) {
                    /* the cap was modified underneath us */
                    dest_tcb->unlock_write();
                    goto modify_try_again;
                }
                space->remove_tcb(dest_tcb);

                UNLOCK_PRIVILEGED_SYSCALL();
                dest_tcb->delete_tcb(kresource);
                LOCK_PRIVILEGED_SYSCALL();
                need_to_schedule = true;
            }
        } // deletion
        else
        {
            /* === Thread modification === */
            if (EXPECT_FALSE(utcb_address != ~0UL))
            {
                /* cannot modify UTCB address */
                current->set_error_code (EUTCB_AREA);
                dest_tcb->unlock_write();
                goto error_out;
            }

            /* setup thread helpers */
            if (!set_thread_handlers(dest_tcb, pager_tid, except_handler_tid)) {
                current->set_error_code(EINVALID_PARAM);
                dest_tcb->unlock_write();
                goto error_out;
            }

            if (EXPECT_FALSE(thread_resources.is_valid())) {
                /* initialize thread resources */
                if (!dest_tcb->resources.control(dest_tcb, thread_resources,
                                                 kresource)) {
                    /* error code set in resources.control */
                    dest_tcb->unlock_write();
                    goto error_out;
                }
            }

            dest_tcb->unlock_write();
        } // modification
    }
    else
    {
        /* === Thread creation === */
        /* Check thread-id range, 0 is a valid thread-id */
        if (EXPECT_FALSE(!get_current_clist()->is_valid(dest_tid))) {
            current->set_error_code(EINVALID_THREAD);
            goto error_out;
        }

        space_t *space = get_current_space()->lookup_space(space_id);
        /* check for valid space */
        if (EXPECT_FALSE(space == NULL))
        {
            current->set_error_code(EINVALID_SPACE);
            goto error_out;
        }

        /* Check for valid UTCB location */
        if (EXPECT_FALSE(!space->check_utcb_location(utcb_address)))
        {
            current->set_error_code (EUTCB_AREA);
            goto error_out;
        }

        /* Allocate TCB */
        dest_tcb = allocate_tcb(dest_tid, kresource);

        if (EXPECT_FALSE(!dest_tcb)) {
            current->set_error_code (ENO_MEM);
            goto error_out;
        }

        /* Create the thread. */
        dest_tcb->init();

        dest_tcb->set_utcb_location(utcb_address);
        bool UNUSED locked = dest_tcb->try_lock_write();
        ASSERT(DEBUG, locked);

        /* Insert TCB into capability list */
        if (EXPECT_FALSE(!get_current_clist()->add_thread_cap(dest_tid, dest_tcb))) {
            current->set_error_code(EINVALID_THREAD);
            goto free_cap_error_out;
        }

        /* set the space for the tcb */
        dest_tcb->set_space (space);
        space->add_tcb (dest_tcb);

        /* setup thread helpers */
        if (!set_thread_handlers(dest_tcb, pager_tid, except_handler_tid)) {
            current->set_error_code(EINVALID_PARAM);
            goto free_tcb_error_out;
        }

        /* Threads start blocked on IPC */
        dest_tcb->sys_data.set_action(tcb_syscall_data_t::action_ipc);

        /* activate thread, which in turn allocates the UTCB */
        if (EXPECT_FALSE(!dest_tcb->activate(thread_startup,
                                             kresource)))
        {
            /* Error code is set in activate() (allocate_utcb - should be ENO_MEM) */
            goto free_tcb_error_out;
        }
        fake_wait_for_startup (dest_tcb);

        if (thread_resources.bits.valid) {
            /* initialize thread resources */
            if (EXPECT_FALSE(!dest_tcb->resources.control(
                                 dest_tcb, thread_resources,
                                 kresource)))
            {
                /* error code set in resources.control() */
                goto free_tcb_error_out;
            }
            need_to_schedule = true;
        }
        /* Set new thread's thread handle in MR0. */
        current->set_mr(0, threadhandle(dest_tcb->tcb_idx).get_raw());
        dest_tcb->unlock_write();
    }

    if (need_to_schedule) {
        UNLOCK_PRIVILEGED_SYSCALL();
        TCB_SYSDATA_THREAD_CTRL(current)->thread_ctrl_continuation = continuation;
        get_current_scheduler()->schedule(current, finish_sys_thread_control,
                scheduler_t::sched_default);
        NOTREACHED();
    }

    PROFILE_STOP(sys_thread_ctrl);
    UNLOCK_PRIVILEGED_SYSCALL();
    return_thread_control(1, continuation);
    NOTREACHED();

free_tcb_error_out:
    /* thread activation failed: clean up TCB */

    if (get_current_clist()->remove_thread_cap(dest_tid)) {
        /* We have exclusive access to dest_tcb now */
        space_t *space = dest_tcb->get_space();

        space->remove_tcb(dest_tcb);
    } else {
        panic("dest_tcb: %p deleted while write_lock held\n", dest_tcb);
    }

free_cap_error_out:
    dest_tcb->unlock_write();
    UNLOCK_PRIVILEGED_SYSCALL();
    dest_tcb->delete_tcb(kresource);
    LOCK_PRIVILEGED_SYSCALL();

error_out:
    /* Hit an error. Abort the system call. */

    if (need_to_schedule) {
        UNLOCK_PRIVILEGED_SYSCALL();
        TCB_SYSDATA_THREAD_CTRL(current)->thread_ctrl_continuation = continuation;
        get_current_scheduler()->schedule(current, abort_sys_thread_control,
                scheduler_t::sched_default);
        NOTREACHED();
    }

    PROFILE_STOP(sys_thread_ctrl);
    UNLOCK_PRIVILEGED_SYSCALL();
    return_thread_control(0, continuation);
    NOTREACHED();
}
