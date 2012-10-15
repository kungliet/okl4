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
 * Fast userspace synchronisation object support.
 */

#include <nano.h>

#include <syscalls.h>
#include <thread.h>
#include <schedule.h>
#include <serial.h>

/*
 * Lock for futex data structures.
 */
static spinlock_t futex_lock = SPIN_LOCK_INIT;

/*
 * Tag Hash Table.
 *
 * The index contains the hash of the tag a thread is waiting on, while the
 * entry contains the index of the first thread and last thread waiting on that
 * particular tag. Threads are chained using 'next'.
 */
typedef struct futex_hash_entry {
    tcb_t *head;
    tcb_t *tail;
} futex_hash_entry_t;

/* Pointer to global hash-entry array. */
MASHED futex_hash_entry_t * const futex_hash;

/* Pointer to array of pending signals. */
MASHED word_t * const futex_pending_tags;

/* Number of pending signals. */
static word_t num_pending_tags = 0;

/* Size of the hash table. Patched at mash time to be the next power of
 * two bigger than (NUM_THREADS * 1.5). */
MASHED const int futex_hash_slots;
MASHED const int futex_hash_slots_lg2;

/*
 * Hash the given tag.
 */
CONST static int
hash_tag(unsigned int tag)
{
   unsigned int h1 = (tag >> 2)
      + (tag >> (1 * futex_hash_slots_lg2))
      + (tag >> (2 * futex_hash_slots_lg2));
   return h1 & (futex_hash_slots - 1);
}

/*
 * Futex functionality.
 */

/* Enqueue a thread onto the futex list, sorted by priority. */
static void
enqueue_futex(futex_hash_entry_t *entry, tcb_t *thread)
{
    tcb_t **prev = &entry->head;
    tcb_t *curr = entry->head;

    /* Attempt to find somewhere in the queue we can enqueue ourselves. */
    while (curr != NULL) {
        if (thread->priority > curr->priority) {
            *prev = thread;
            thread->next = curr;
            return;
        }
        prev = &curr->next;
        curr = curr->next;
    }

    /* Reached the end of the queue. */
    thread->next = NULL;
    entry->tail = thread;
    *prev = thread;
}

/* Dequeue a thread from the futex list. */
static void
dequeue_futex(futex_hash_entry_t *entry, tcb_t *prev, tcb_t *thread)
{
    /* Fixup previous pointer. */
    if (entry->head == thread) {
        ASSERT(prev == NULL);
        entry->head = thread->next;
    } else {
        ASSERT(prev != NULL);
        prev->next = thread->next;
    }

    /* Fixup tail. */
    if (entry->tail == thread) {
        entry->tail = prev;
    }

    thread->next = NULL;
}

/*
 * Wait on the given futex.
 */
ATTRIBUTE_NORETURN(sys_futex_wait);
NORETURN void
sys_futex_wait(word_t tag)
{
    /* The zero tag is reserved for system use. */
    if (EXPECT_FALSE(tag == 0))
        syscall_return_error(1, EINVAL);

    spin_lock(&futex_lock);

    /* If there is a pending signal matching our tag, remove it
     * and return early. */
    for (int i = num_pending_tags - 1; i >= 0; i--) {
        if (futex_pending_tags[i] == tag) {
            futex_pending_tags[i] = futex_pending_tags[num_pending_tags - 1];
            num_pending_tags--;
            spin_unlock(&futex_lock);
            syscall_return_success(0);
        }
    }

    /* Otherwise, add us to the wait list. */
    int h = hash_tag(tag);

    /* Deactivate the current thread. */
    set_syscall_return_val_success(current_tcb, 0);
    tcb_t *next = deactivate_self_schedule(THREAD_STATE_WAIT_FUTEX);

    /* Enqueue ourself into the hash table. */
    enqueue_futex(&futex_hash[h], current_tcb);
    current_tcb->futex_tag = tag;

    spin_unlock(&futex_lock);

    /* Find the next person to schedule. */
    switch_to(next);
}

/*
 * Find a thread sleeping on 'tag', and dequeue it.
 *
 * Returns the dequeued thread if it one was found, NULL otherwise.
 */
static tcb_t *
signal_futex(unsigned int tag)
{
    int h = hash_tag(tag);
    tcb_t *head = futex_hash[h].head;
    tcb_t *previous = NULL;

    /* Search for a waiting thread that matches our target tag. */
    while (head != NULL) {
        /* If we have found our thread, dequeue it and finish. */
        if (head->futex_tag == tag) {
            dequeue_futex(&futex_hash[h], previous, head);
            return head;
        }
        previous = head;
        head = head->next;
    }

    /* No thread was found. */
    return NULL;
}

/*
 * Signal a futex waiting on tag 'tag'.
 */
ATTRIBUTE_NORETURN(sys_futex_signal);
NORETURN void
sys_futex_signal(unsigned int tag)
{
    /* The zero tag is reserved for system use. */
    if (EXPECT_FALSE(tag == 0))
        syscall_return_error(1, EINVAL);

    spin_lock(&futex_lock);

    /* Try waking a thread. */
    tcb_t *tcb = signal_futex(tag);
    if (EXPECT_TRUE(tcb)) {
        /* If one was found, activate it and return. */
        tcb_t *next = activate_schedule(tcb);
        spin_unlock(&futex_lock);
        set_syscall_return_val_success(current_tcb, 0);
        switch_to(next);
    }

    /*
     * Otherwise, no thread was found, and we need to add a signal to the
     * pending list.
     *
     * Ensure that we have space to store the pending signal.
     */
    if (EXPECT_FALSE(num_pending_tags >= max_tcbs)) {
        spin_unlock(&futex_lock);
        syscall_return_error(1, ENOMEM);
    }

    /* Add the pending signal. */
    futex_pending_tags[num_pending_tags++] = tag;
    spin_unlock(&futex_lock);
    syscall_return_success(0);
}

