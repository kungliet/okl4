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
 * ARM Syscall Trap Handling.
 */

#include "macro.s"
#include <nano.h>
#include <tcb_offsets.h>

#include <arch/armv5_instructions.h>

/* Unallocated syscalls jump to 'bad_syscall'. */
#define UNALLOCATED_SYSCALL  bad_syscall

/* Number of syscalls. */
#define NUM_SYSCALLS      32
#define NUM_SYSCALLS_LG2  5

/* Define a syscall that is only used when not in production
 * mode */
#if defined(CONFIG_PERF)
#define DEV_SYSCALL(x) x
#else
#define DEV_SYSCALL(x) UNALLOCATED_SYSCALL
#endif

#if defined(CONFIG_USE_TANGLED_FASTPATH)

/*
 * "swi_syscall" must be placed into the vectors subpage. See 'vectors.s' for
 * details.
 */
.section .vectors.a, "ax"

/*
 * List of syscalls.
 */
list syscall_vector
    .word sys_thread_myself                                          /* 0000 */
    .word sys_thread_create                                          /* 0001 */
    .word sys_thread_exit                                            /* 0002 */
    .word sys_futex_wait                                             /* 0003 */
    .word sys_futex_signal                                           /* 0004 */
    .word sys_yield                                                  /* 0005 */
    .word sys_interrupt_register                                     /* 0006 */
    .word sys_interrupt_deregister                                   /* 0007 */
    .word sys_interrupt_wait                                         /* 0008 */
    .word sys_signal_poll                                            /* 0009 */
    .word sys_signal_wait                                            /* 0010 */
    .word sys_signal_send                                            /* 0011 */
    .word sys_ipc_send                                               /* 0012 */
    .word sys_ipc_recv                                               /* 0013 */
    .word sys_thread_join                                            /* 0014 */
    .word UNALLOCATED_SYSCALL                                        /* 0015 */
    .word UNALLOCATED_SYSCALL                                        /* 0016 */
    .word UNALLOCATED_SYSCALL                                        /* 0017 */
    .word UNALLOCATED_SYSCALL                                        /* 0018 */
    .word UNALLOCATED_SYSCALL                                        /* 0019 */
    .word UNALLOCATED_SYSCALL                                        /* 0020 */
    .word UNALLOCATED_SYSCALL                                        /* 0021 */
    .word UNALLOCATED_SYSCALL                                        /* 0022 */
    .word UNALLOCATED_SYSCALL                                        /* 0023 */
    .word UNALLOCATED_SYSCALL                                        /* 0024 */
    .word UNALLOCATED_SYSCALL                                        /* 0025 */
    .word UNALLOCATED_SYSCALL                                        /* 0026 */
    .word UNALLOCATED_SYSCALL                                        /* 0027 */
    .word UNALLOCATED_SYSCALL                                        /* 0028 */
    .word UNALLOCATED_SYSCALL                                        /* 0029 */
    .word DEV_SYSCALL(debug_kill_cache)                              /* 0030 */
    .word DEV_SYSCALL(debug_get_cycles)                              /* 0031 */
endl syscall_vector

noreturn_function swi_syscall

    /* ldr  sp, [current_tcb] */
    LDR_OFF(COND_AL, REG_SP, current_tcb)                            /* 0020 */

    ldr     r12, [lr, #-4]                                           /* 0040 */
    str     lr, [sp, #TCB_PC]                                        /* 0030 */

    add     lr, sp, #TCB_R4                                          /* 0050 */
    stmia   lr, {r4 - r14}^                                          /* 0060 */
    and     r12, r12, #(NUM_SYSCALLS - 1)                            /* 0120 */

    adr     lr, syscall_vector                                       /* 0110 */
    add     r12, lr, r12, LSL #2                                     /* 0130 */
    ldr     r12, [r12]                                               /* 0140 */

    indirect_annotation syscall_vector
    ldr     sp, =kernel_stack                                        /* 0100 */
    bx      r12                                                      /* 0150 */

end_function swi_syscall

    /*
     * Called on an invalid syscall.
     */
function bad_syscall
1:
    b       1b                                                       /* 0160 */
end_function bad_syscall

#endif /* defined(CONFIG_USE_TANGLED_FASTPATH) */

