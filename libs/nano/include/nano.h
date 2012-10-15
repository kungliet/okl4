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


#ifndef __OKNANO_H__
#define __OKNANO_H__

typedef unsigned long okn_thread_t;
typedef unsigned long okn_mutex_t;
typedef unsigned long okn_event_t;
typedef unsigned long okn_timer_t;

okn_event_t okn_thread_wait(void);

/* Futex system calls. */
struct okn_futex {
    unsigned long lock;
};
typedef struct okn_futex okn_futex_t;

#define OKN_FUTEX_INIT {0}
void okn_futex_init(okn_futex_t *);
void okn_futex_lock(okn_futex_t *);
void okn_futex_spin_lock(okn_futex_t *);
int okn_futex_trylock(okn_futex_t *);
void okn_futex_unlock(okn_futex_t *);

/* Semaphore system calls. */
typedef struct okn_semaphore {
    unsigned long count;
} okn_semaphore_t;

void okn_semaphore_init(okn_semaphore_t *, unsigned long val);
#define OKN_SEMAPHORE_INIT(n) {(n)}
unsigned long okn_semaphore_down(okn_semaphore_t *);
unsigned long okn_semaphore_try_down(okn_semaphore_t *);
unsigned long okn_semaphore_up(okn_semaphore_t *);

/* Barrier calls. */
typedef struct okn_barrier {
    unsigned long count;
    unsigned long expected;
    unsigned long waiters;
    unsigned long version;
} okn_barrier_t;

#define OKN_BARRIER_INIT(n) {0, (n)}
void okn_barrier_init(okn_barrier_t *barrier, int threads);
void okn_barrier(okn_barrier_t *barrier);

/* Futex system calls. */
int okn_syscall_futex_wait(int tag);
int okn_syscall_futex_signal(int tag);

/* Thread system calls. */
int okn_syscall_thread_create(void *pc, void *sp, unsigned long r0, int prio);
void okn_syscall_thread_exit(void);
unsigned int okn_syscall_thread_myself(void);
int okn_syscall_thread_join(int thread);

/* Interrupt system calls. */
int okn_syscall_interrupt_register(int interrupt);
int okn_syscall_interrupt_deregister(int interrupt);
int okn_syscall_interrupt_wait(void);

/* Stupid syscalls. */
void okn_syscall_yield(void);

/* Debugging syscalls. */
void okn_syscall_kill_cache(void);
long long okn_syscall_get_cycles(void);

/* Signalling. */
unsigned long okn_syscall_signal_poll(unsigned long mask);
unsigned long okn_syscall_signal_wait(unsigned long mask);
unsigned long okn_syscall_signal_send(int thread, unsigned long signals);

/* Blocking send/recv operation. */
#define OKN_IPC_BLOCKING      (0 << 0)
#define OKN_IPC_NON_BLOCKING  (1 << 0)

/* Convert a send operation into a call. */
#define OKN_IPC_SEND_ONLY     (0 << 1)
#define OKN_IPC_CALL          (1 << 1)

/* Open Wait Tag. */
#define OKN_IPC_OPEN_WAIT     (-1)

/* Output lock for printing. */
extern okn_futex_t output_lock;

/* Architecture-specific definitions and declarations. */
#include <nano/arch/nano.h>

/* IPC convienience functions. */
#define okn_ipc_send_1(dest, arg) \
    okn_syscall_ipc_send(arg, 0, 0, 0, 0, 0, 0, 0, dest, OKN_IPC_SEND_ONLY)
#define okn_ipc_recv_1(src, arg) \
    okn_syscall_ipc_recv(arg, 0, 0, 0, 0, 0, 0, 0, src, 0)
#define okn_ipc_wait_1(arg) \
    okn_syscall_ipc_recv(arg, 0, 0, 0, 0, 0, 0, 0, OKN_IPC_OPEN_WAIT, 0)

#endif /* __OKNANO_H__ */

