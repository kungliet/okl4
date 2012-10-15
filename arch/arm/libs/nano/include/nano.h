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


#ifndef __OKNANO_ARCH_H__
#define __OKNANO_ARCH_H__

/*
 * IPC syscalls.
 *
 * This syscall binding is a little different to every other syscall,
 * merely because of the large number of input arguments it takes.
 *
 * The ARM ABI requires that all arguments past 'r3' must be pushed
 * onto the stack. Unfortunately for us, the kernel does not have
 * access to our stack. Instead, we marshel in the arguments manually
 * and perform the trap ourself.
 *
 * Using the many 'register unsigned long rX asm("rX")' will usually
 * give us a win as the GCC register allocator will often select registers
 * to avoid explicity copies.
 */
__inline__ static long
okn_syscall_ipc_send(
        unsigned long arg0, unsigned long arg1,
        unsigned long arg2, unsigned long arg3,
        unsigned long arg4, unsigned long arg5,
        unsigned long arg6,
        unsigned long dest, unsigned long operation)
{
    register unsigned long r0 asm("r0");
    register unsigned long r1 asm("r1") = arg1;
    register unsigned long r2 asm("r2") = arg2;
    register unsigned long r3 asm("r3") = arg3;
    register unsigned long r4 asm("r4") = arg4;
    register unsigned long r5 asm("r5") = arg5;
    register unsigned long r6 asm("r6") = arg6;
    register unsigned long r7 asm("r7") = arg0;
    register unsigned long r8 asm("r8") = dest;
    register unsigned long r9 asm("r9") = operation;

    __asm__ __volatile__(
            "swi     #12\n\t"
            : "=r"(r0), "+r"(r1), "+r"(r2), "+r"(r3),
              "+r"(r4), "+r"(r5), "+r"(r6), "+r"(r7),
              "+r"(r8), "+r"(r9)
            : /* no other inputs */
            : "cc", "memory", "r12", "r13");

    return r0;
}

__inline__ static long
okn_syscall_ipc_recv(
        unsigned long *arg0, unsigned long *arg1,
        unsigned long *arg2, unsigned long *arg3,
        unsigned long *arg4, unsigned long *arg5,
        unsigned long *arg6, unsigned long *sender,
        unsigned long dest, unsigned long operation)
{
    register unsigned long r0 asm("r0") = dest;
    register unsigned long r1 asm("r1") = operation;
    register unsigned long r2 asm("r2");
    register unsigned long r3 asm("r3");
    register unsigned long r4 asm("r4");
    register unsigned long r5 asm("r5");
    register unsigned long r6 asm("r6");
    register unsigned long r7 asm("r7");

    __asm__ __volatile__(
            "swi     #13\n\t"
            : "+r"(r0), "+r"(r1),
              "=r"(r2), "=r"(r3),
              "=r"(r4), "=r"(r5), "=r"(r6), "=r"(r7)
            : /* no other inputs */
            : "cc", "memory", "r12", "r13");

    if (arg0) *arg0 = r7;
    if (arg1) *arg1 = r1;
    if (arg2) *arg2 = r2;
    if (arg3) *arg3 = r3;
    if (arg4) *arg4 = r4;
    if (arg5) *arg5 = r5;
    if (arg6) *arg6 = r6;
    if (sender) *sender = r0;

    return (r0 == ~0UL ? -1 : 0);
}

__inline__ static long
okn_syscall_ipc_call(
        unsigned long *arg0, unsigned long *arg1,
        unsigned long *arg2, unsigned long *arg3,
        unsigned long *arg4, unsigned long *arg5,
        unsigned long *arg6,
        unsigned long dest, unsigned long operation)
{
    register unsigned long r0 asm("r0");
    register unsigned long r1 asm("r1") = *arg1;
    register unsigned long r2 asm("r2") = *arg2;
    register unsigned long r3 asm("r3") = *arg3;
    register unsigned long r4 asm("r4") = *arg4;
    register unsigned long r5 asm("r5") = *arg5;
    register unsigned long r6 asm("r6") = *arg6;
    register unsigned long r7 asm("r7") = *arg0;
    register unsigned long r8 asm("r8") = dest;
    register unsigned long r9 asm("r9") = operation | OKN_IPC_CALL;

    __asm__ __volatile__(
            "swi     #12\n\t"
            : "=r"(r0), "+r"(r1),
              "+r"(r2), "+r"(r3),
              "+r"(r4), "+r"(r5), "+r"(r6),
              "+r"(r7), "+r"(r8), "+r"(r9)
            : /* no other inputs */
            : "cc", "memory", "r12", "r13");

    if (arg0) *arg0 = r7;
    if (arg1) *arg1 = r1;
    if (arg2) *arg2 = r2;
    if (arg3) *arg3 = r3;
    if (arg4) *arg4 = r4;
    if (arg5) *arg5 = r5;
    if (arg6) *arg6 = r6;

    return (r0 == ~0UL ? -1 : 0);
}

/*
 * Uniprocessor systems should never spin on a futex.
 */
#define okn_futex_spin_lock(x) okn_futex_lock(x)

#endif /* __OKNANO_ARCH_H__ */

