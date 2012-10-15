/*
 * Copyright (c) 2003-2006, National ICT Australia (NICTA)
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
 * Description:   ARM TCB handling functions
 */
#ifndef __ARM__TCB_H__
#define __ARM__TCB_H__

#include <kernel/arch/asm.h>
#include <kernel/arch/continuation.h>
#include "asm_prototypes.h"

#ifndef __TCB_H__
#error not for stand-alone inclusion
#endif

extern stack_t __stack;


/**
 * Return current tcb pointer.
 */
PURE INLINE tcb_t *
get_current_tcb (void)
{
    return get_arm_globals()->current_tcb;
}

INLINE clist_t* get_current_clist(void) PURE;

INLINE clist_t* get_current_clist(void)
{
    return get_arm_globals()->current_clist;
}

INLINE void tcb_t::enable_preempt_recover(continuation_t continuation)
{
    /* can't jump straight to the continuation as will have invalid stack pointer */
    preemption_continuation = continuation;
}

INLINE void tcb_t::disable_preempt_recover()
{
    preemption_continuation = 0;
}

/* End - stack/continuation functions. */

#include <syscalls.h>           /* for sys_ipc */
#include <arch/thread.h>
#include <cpu/cache.h>
#include <arch/resource_functions.h>
#include <cpu/syscon.h>

/* include ARM version specific implementations */
#include <arch/ver/tcb.h>

INLINE word_t tcb_t::set_tls(word_t *mr)
{
    get_utcb()->kernel_reserved[0] = mr[0];
    return EOK;
}

/**
 * read value of message register
 * @param index number of message register
 */
INLINE word_t tcb_t::get_mr(word_t index)
{
    return get_utcb()->mr[index];
}

/**
 * set the value of a message register
 * @param index number of message register
 * @param value value to set
 */
INLINE void tcb_t::set_mr(word_t index, word_t value)
{
    get_utcb()->mr[index] = value;
}

/**
 * read value of the acceptor
 */
INLINE acceptor_t tcb_t::get_acceptor(void)
{
    return get_utcb()->acceptor;
}

/**
 * set the value of the acceptor register
 * @param value value to set
 */
INLINE void tcb_t::set_acceptor(const acceptor_t value)
{
    get_utcb()->acceptor = value;
}

/**
 * copies a set of message registers from one UTCB to another
 * @param dest destination TCB
 * @param start MR start index
 * @param count number of MRs to be copied
 * @return whether operation succeeded
 */
INLINE bool tcb_t::copy_mrs(tcb_t * dest, word_t start, word_t count)
{
    word_t *dest_mr = &dest->get_utcb()->mr[start];
    word_t *src_mr = &get_utcb()->mr[start];

    if ((start + count) > IPC_NUM_MR)
        return false;

    /* This will always copy at least 1 register,
     * assuming IPCs with 0 MRs are rare.
     */
#if defined(__GNUC__)
    word_t temp1, temp2;
    __asm__ __volatile__ (
        "1:                             \n"
        "ldr    %[t1],  [%[src]], #4    \n"
        "ldr    %[t2],  [%[src]], #4    \n"
        "subs   %[num], %[num], #2      \n"
        "str    %[t1],  [%[dst]], #4    \n"
        "strpl  %[t2],  [%[dst]], #4    \n"
        "bgt    1b                      \n"
        : [t1] "=r" (temp1),
          [t2] "=r" (temp2),
          [src] "+r" (src_mr),
          [dst] "+r" (dest_mr),
          [num] "+r" (count)
    );
#else
    do {
            *dest_mr++ = *src_mr++;
    } while(--count > 0);
#endif
    return true;
}

INLINE void tcb_t::set_exception_ipc(word_t num)
{
    arch.exc_num = num;
}

INLINE bool tcb_t::in_exception_ipc(void)
{
    /*
     * EXCEPTIONFP indicates the thread is doing a syscall exception ipc, this
     * function returns true if the thread is either doing a syscall exception
     * ipc, or a general exception ipc.
     */
    return (resource_bits & (1UL << (word_t)EXCEPTIONFP)) || (arch.exc_num != 0);
}

INLINE void tcb_t::clear_exception_ipc(void)
{
    arch.exc_num = 0;
    resources.clear_except_fp(this);
}

/**
 * set the address space a TCB belongs to
 * @param space address space the TCB will be associated with
 */
INLINE void tcb_t::set_space(space_t * new_space)
{
    this->space = new_space;

    if (EXPECT_TRUE(new_space)) {
        this->space_id = new_space->get_space_id();
        this->page_directory = new_space->pgent(0);
    } else {
        this->space_id = spaceid_t::nilspace();
        this->page_directory = NULL;
    }
}

/**
 * set the mutex thread handle in a UTCB
 * @param handle mutex thread handle
 */
INLINE void tcb_t::set_mutex_thread_handle(capid_t handle)
{
    get_utcb()->mutex_thread_handle = handle;
}

/*
 * Return back to user_land when an IPC is aborted
 * We short circuit the restoration of any saved registers/state
 */
INLINE void tcb_t::return_from_ipc (void)
{
    //extern continuation_t vector_ipc_syscall_return;

    /*lint -e611 this is a suspicious cast, but it is OK */
    ACTIVATE_CONTINUATION(vector_ipc_syscall_return);
}


/**
 * Short circuit a return path from a user-level interruption or
 * exception.  That is, restore the complete exception context and
 * resume execution at user-level.
 */
INLINE void tcb_t::return_from_user_interruption(void)
{
    //extern continuation_t vector_arm_abort_return;

#ifdef CONFIG_IPC_FASTPATH
    tcb_t * current = get_current_tcb();
    current->resources.clear_kernel_ipc( current );
    current->resources.clear_except_fp( current );
#endif

    ACTIVATE_CONTINUATION(vector_arm_abort_return);
    // NOT REACHED
}


/**
 * intialize stack for given thread
 */
INLINE void tcb_t::init_stack()
{
    /* Clear the exception context */
    arm_irq_context_t *context = &arch.context;
    word_t *t, *end;
    word_t size = sizeof(arch.context);

    /* clear whole context */
    t = (word_t*)context;
    end = (word_t*)((word_t)context + (size & ~(15)));

    do {
        *t++ = 0;
        *t++ = 0;
        *t++ = 0;
        *t++ = 0;
    } while (t < end);

    while (t < (word_t*)(context+1))
        *t++ = 0;

#if CONFIG_ARM_VER <= 5
    context->pc = 1;    /* Make it look like an exception context. */
#endif
}



/**********************************************************************
 *
 *            access functions for ex-regs'able registers
 *
 **********************************************************************/

/**
 * read the user-level stack pointer
 * @return      the user-level stack pointer
 */
INLINE addr_t tcb_t::get_user_sp()
{
    arm_irq_context_t * context = &arch.context;

    return (addr_t) (context)->sp;
}

/**
 * set the user-level stack pointer
 * @param sp    new user-level stack pointer
 */
INLINE void tcb_t::set_user_sp(addr_t sp)
{
    arm_irq_context_t *context = &arch.context;

    context->sp = (word_t)sp;
}

INLINE word_t tcb_t::get_utcb_location()
{
    return (word_t)this->utcb_location;
}

INLINE void tcb_t::set_utcb_location(word_t location)
{
    utcb_location = location;
}


/**
 * read the user-level flags (one word)
 * @return      the user-level flags
 */
INLINE word_t tcb_t::get_user_flags (void)
{
    arm_irq_context_t * context = &(arch.context);

    return (word_t) (context)->cpsr & ARM_USER_FLAGS_MASK;
}

/**
 * set the user-level flags
 * @param flags new user-level flags
 */
INLINE void tcb_t::set_user_flags (const word_t flags)
{
    arm_irq_context_t *context = &(arch.context);

    context->cpsr = (context->cpsr & ~ARM_USER_FLAGS_MASK) |
            ((word_t)flags & ARM_USER_FLAGS_MASK);
}

/**********************************************************************
 *
 *                       preemption callback function
 *
 **********************************************************************/

/**
 * set the address where preemption occured
 */
INLINE void tcb_t::set_preempted_ip(addr_t ip)
{
#ifdef CONFIG_ARM_THUMB_SUPPORT
    arm_irq_context_t *context = &(arch.context);

    /* CPU has thumb support, fix ip if needed */
    if (context->cpsr & CPSR_THUMB_BIT)
        ip = (addr_t)((word_t)ip | 1);
#endif
    get_utcb()->preempted_ip = (word_t)ip;
}

INLINE addr_t tcb_t::get_preempted_ip()
{
    return (addr_t)get_utcb()->preempted_ip;
}

/**
 * get the preemption callback ip
 */
INLINE addr_t tcb_t::get_preempt_callback_ip()
{
    return (addr_t)get_utcb()->preempt_callback_ip;
}

#endif /* !__ARM__TCB_H__ */
