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
 * Neo Scheduler
 */
#include <nano.h>

#include <thread.h>
#include <schedule.h>
#include <serial.h>

/* The idle tcb. */
extern tcb_t idle_tcb;

/* Pointer to the idle TCB. */
#ifndef IDLE_TCB_PTR
#define IDLE_TCB_PTR  (&idle_tcb)
#endif

/* Lock protecting this data structure. */
spinlock_t schedule_lock = SPIN_LOCK_INIT;

/* Priorities that each execution unit is running at. */
#if NUM_EXECUTION_UNITS > 1
uint8_t unit_priorities[NUM_EXECUTION_UNITS + 2] = {0, 0, 0, 0, 0, 0, 0xff, 0xff};
#endif

/* Priority bitmap. A set bit indicates that there
 * is a thread at a given priority. */
MASHED word_t priority_bitmap = -1;

/* The first enqueued thread at each priority. */
MASHED tcb_t **priority_heads;

/* Cause the next available low-priority thread to perform a reschedule. */
#define flag_reschedule()                                                    \
    do {                                                                     \
        __asm__ __volatile__ ("swi(%0)\n" : : "r"(RESCHEDULE_IRQ_MASK));     \
    } while (0)

/*
 * Return the highest priority thread available in the system,
 * or (-1) if no threads are currently ready.
 */
INLINE static int
get_highest_priority(void)
{
    return MAX_PRIORITY - clz(priority_bitmap);
}

/*
 * Return true if the current unit's priority is up-to-date.
 */
INLINE static int
is_current_unit_prio_correct(void)
{
#if NUM_EXECUTION_UNITS > 1
    if (current_tcb == IDLE_TCB_PTR) {
        return unit_priorities[get_current_unit()] == 0;
    } else {
        return unit_priorities[get_current_unit()] = current_tcb->priority + 1;
    }
#else
    return 1;
#endif
}

/*
 * If there is a thread running on another unit with a priority less than
 * 'min_thread', trigger a reschedule operation.
 *
 * If the current execution unit is scheduling a new thread, a
 * 'set_unit_priority' operation should already have taken place
 * prior to a 'smt_reschedule' operation taking place.
 *
 * TRICKY: In rare circumstances, we may end up finding ourselves as
 * the thread that we need to smt_reschedule. This occurs when the
 * following series of actions occur:
 *
 *     A: Priority 2 thread enters kernel. IPIs masked.
 *     B: Priority 4 thread enters kernel.
 *     B: Wakes up thread of priority '3'. Performs an 'smt_reschedule()'
 *        operation, that sends an IPI to unit 'A'.
 *     A: Wakes up thread of priority '1'. 'activate_schedule()' does
 *        not do a full schedule, but just compares the priorities '1' and
 *        '2', and decides that '2' should continue running.
 *     A: 'smt_reschedule()' takes place, determines that the currently
 *        running thread at priority '2' should be the guy to be rescheduled.
 *        the IPI is resent.
 *
 * This race does not actually affect correctness as, once unit 'A' returns
 * back to userspace, it will get the IPI, perform its reschedule, and fix
 * everything up. It is just slight counter-intuative that smt_reschedule()
 * could select a thread just performed a scheduling decision. De-optimising
 * 'activate_schedule()' to perform a full schedule would fix this race at the
 * cost of performance.
 */
#if !defined(ASM_SMT_RESCHEDULE)
#if NUM_EXECUTION_UNITS > 1
static void
smt_reschedule(void)
{
    int min_thread = get_highest_priority() + 1;

    /* Search for a thread with priority less than 'min_thread'. */
    for (int i = 0; i < NUM_EXECUTION_UNITS; i++) {
        if ((int)unit_priorities[i] < min_thread) {
            flag_reschedule();
            return;
        }
    }
}
#endif
#else
void smt_reschedule(void);
#endif

/*
 * Set the priority that the current execution unit is running
 * at.
 */
INLINE static void
set_unit_priority(int prio)
{
#if NUM_EXECUTION_UNITS > 1
    unit_priorities[get_current_unit()] = prio + 1;
#endif
}

/*
 * Enqueue the given thread onto the scheduler queue.
 */
#if !ASM_ENQUEUE
static void
enqueue(tcb_t *tcb)
{
    ASSERT(tcb != IDLE_TCB_PTR && tcb->next == NULL && tcb->prev == NULL);

    if (priority_heads[tcb->priority] == NULL) {
        priority_heads[tcb->priority] = tcb;
        tcb->next = tcb;
        tcb->prev = tcb;
        priority_bitmap |= 1 << tcb->priority;
    } else {
        tcb_t *first = priority_heads[tcb->priority];
        tcb_t *prev = first->prev;

        tcb->next = first;
        tcb->prev = prev;
        first->prev = tcb;
        prev->next = tcb;
    }
}
#else
void enqueue(tcb_t *tcb);
#endif

/*
 * Dequeue the head of the priority queue.
 */
#if !ASM_DO_SCHEDULE
static tcb_t *
dequeue_head(int priority)
{
    /* Get the head of the list. */
    tcb_t *tcb = priority_heads[priority];
    ASSERT(tcb != NULL);

    /* Determine if we are the final element on the list. */
    if (tcb->next == tcb) {
        /* We are indeed the final element. Clear out the priority array
         * and bitmap. */
        priority_bitmap = CLEAR_BIT(priority_bitmap, priority);
        priority_heads[priority] = NULL;
    } else {
        /* There are more threads. Dequeue ourselves. */
        tcb_t *prev = tcb->prev;
        tcb_t *next = tcb->next;
        prev->next = next;
        next->prev = prev;
        priority_heads[priority] = next;
    }

    /* Clear out our previous/next pointers. */
    tcb->next = NULL;
    tcb->prev = NULL;

    /* Return the element dequeued. */
    return tcb;
}
#endif

/*
 * Perform a full system schedule, assuming that all runnable threads,
 * (including the current thread, if it is runnable), are on the scheduler's
 * queue.
 *
 * Return the thread that should be switch_to()'ed.
 */
#if !ASM_DO_SCHEDULE
static tcb_t *
do_schedule(void)
{
    /* Ensure our 'unit_priority' is currently up to date. */
    ASSERT(is_current_unit_prio_correct());

    /* If there are no other threads in the system, switch to idle. */
    if (EXPECT_FALSE(priority_bitmap == 0)) {
        set_unit_priority(-1);
        return IDLE_TCB_PTR;
    }

    /* Get the priority of the highest schedulable thread in the system. */
    int next_prio = get_highest_priority();

    /* Switch to the new thread instead. */
    tcb_t *next = dequeue_head(next_prio);
    ASSERT(next != NULL);

    /* Update our unit's priority. */
    set_unit_priority(next_prio);

    /* Perform an SMT balancing operation, if required. */
    smt_reschedule();

    return next;
}
#else
tcb_t *do_schedule(void);
#endif

/*
 * Perform a full system schedule, assuming that all runnable threads,
 * (including the current thread, if it is runnable), are on the scheduler's
 * queue.
 *
 * Return the thread that should be switch_to()'ed.
 */
#if !defined(ASM_SCHEDULE)
tcb_t *
schedule(void)
{
    tcb_t *result;
    spin_lock(&schedule_lock);
    result = do_schedule();
    spin_unlock(&schedule_lock);

    return result;
}
#endif

/*
 * Yield to another thread at the same priority, if such a thread
 * exists.
 */
tcb_t *
yield(void)
{
    /* Ensure the current thread is runnable. */
    ASSERT(current_tcb != NULL
            && current_tcb->thread_state == THREAD_STATE_RUNNABLE);

    /* Ensure our 'unit_priority' is currently up to date. */
    ASSERT(is_current_unit_prio_correct());

    spin_lock(&schedule_lock);

    tcb_t *result;
    if (get_highest_priority() >= current_tcb->priority) {
        enqueue(current_tcb);
        result = do_schedule();
    } else {
        result = current_tcb;
    }

    spin_unlock(&schedule_lock);
    return result;
}

/*
 * Deactivate the current TCB, without performing a schedule.
 */
void
deactivate_self(thread_state_t thread_state)
{
    ASSERT(current_tcb != IDLE_TCB_PTR);
    ASSERT(current_tcb->thread_state == THREAD_STATE_RUNNABLE);
    ASSERT(thread_state != THREAD_STATE_RUNNABLE);

    spin_lock(&schedule_lock);
    current_tcb->thread_state = thread_state;
    spin_unlock(&schedule_lock);
}

/*
 * Deactivate the current TCB, setting its thread state to a
 * given value and perform a schedule.
 */
tcb_t *
deactivate_self_schedule(thread_state_t thread_state)
{
    /* We should currently be running, and off the queue. */
    ASSERT(current_tcb != IDLE_TCB_PTR);
    ASSERT(current_tcb->thread_state == THREAD_STATE_RUNNABLE);

    /* Ensure our 'unit_priority' is currently up to date. */
    ASSERT(is_current_unit_prio_correct());

    /* Our new state should be a blocked state. */
    ASSERT(thread_state != THREAD_STATE_RUNNABLE);

    spin_lock(&schedule_lock);
    current_tcb->thread_state = thread_state;
    tcb_t *next = do_schedule();
    spin_unlock(&schedule_lock);

    return next;
}

/*
 * Activate a TCB, without performing a schedule.
 */
void
activate(tcb_t *dest)
{
    ASSERT(dest != IDLE_TCB_PTR);
    ASSERT(dest->thread_state != THREAD_STATE_RUNNABLE);

    spin_lock(&schedule_lock);
    dest->thread_state = THREAD_STATE_RUNNABLE;
    enqueue(dest);
    spin_unlock(&schedule_lock);
}

/*
 * Activate a new thread, and then perform a full system schedule.
 * Assumes that the current thread is runnable or idle.
 */
#if !ASM_ACTIVATE_SCHEDULE
tcb_t *
activate_schedule(tcb_t *dest)
{
    /* We must either be running a runnable thread or be running idle. */
    ASSERT(dest != IDLE_TCB_PTR);
    ASSERT(current_tcb == IDLE_TCB_PTR
            || current_tcb->thread_state == THREAD_STATE_RUNNABLE);

    /* Ensure our 'unit_priority' is currently up to date. */
    ASSERT(is_current_unit_prio_correct());

    /* Thread to be activated must be in a blocked state. */
    ASSERT(dest->thread_state != THREAD_STATE_RUNNABLE);

    /* Grab the scheduler lock. */
    spin_lock(&schedule_lock);

    /* Activate the new thread. */
    dest->thread_state = THREAD_STATE_RUNNABLE;

    /* Work out who to schedule next. */
    tcb_t *next;
    if (current_tcb == IDLE_TCB_PTR) {
        next = dest;
        set_unit_priority(dest->priority);
    } else if (dest->priority > current_tcb->priority) {
        next = dest;
        enqueue(current_tcb);
        set_unit_priority(dest->priority);
        smt_reschedule();
    } else {
        next = current_tcb;
        enqueue(dest);
        smt_reschedule();
    }

    spin_unlock(&schedule_lock);

    /* Return that thread. */
    return next;
}
#endif

