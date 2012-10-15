/*********************************************************************
 *                
 * Copyright (C) 2001-2004,  Karlsruhe University
 *                
 * File path:     l4/ia32/syscalls.h
 * Description:   x86 syscall implementations
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
 *                
 * $Id: syscalls.h,v 1.20.2.8 2004/11/23 14:24:37 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __L4__X86__SYSCALLS_H__
#define __L4__X86__SYSCALLS_H__

#include __L4_INC_ARCH(vregs.h)
#include __L4_INC_ARCH(specials.h)
#include <l4/message.h>


#if defined(__pic__)
# define __L4_SAVE_REGS		"	pushl %%ebx; pushl %%ebp\n"
# define __L4_RESTORE_REGS	"	popl  %%ebp; popl  %%ebx\n"
# define __L4_CLOBBER_REGS	"cc"
#else
# define __L4_SAVE_REGS		"	pushl %%ebp		\n"
# define __L4_RESTORE_REGS	"	popl  %%ebp		\n"
# define __L4_CLOBBER_REGS	"ebx", "cc"
#endif


L4_INLINE void * L4_KernelInterface (L4_Word_t *ApiVersion,
				     L4_Word_t *ApiFlags,
				     L4_Word_t *KernelId)
{
    void * base_address;

    __asm__ __volatile__ (
	"/* L4_KernelInterface() */ 			\n"
	"	lock; nop				\n"

	: /* outputs */
	"=a" (base_address),
	"=c" (*ApiVersion),
	"=d" (*ApiFlags),
	"=S" (*KernelId)

	/* no inputs */
	/* no clobbers */
	);

    return base_address;
}


L4_INLINE L4_ThreadId_t L4_ExchangeRegisters (L4_ThreadId_t dest,
					      L4_Word_t control,
					      L4_Word_t sp,
					      L4_Word_t ip,
					      L4_Word_t flags,
					      L4_Word_t UserDefHandle,
					      L4_ThreadId_t pager,
					      L4_Word_t *old_control,
					      L4_Word_t *old_sp,
					      L4_Word_t *old_ip,
					      L4_Word_t *old_flags,
					      L4_Word_t *old_UserDefhandle,
					      L4_ThreadId_t *old_pager)
{
    L4_ThreadId_t	result;

    struct {
	L4_Word_t	ebp;
	L4_Word_t       edi;
	L4_Word_t	ebx;
	L4_Word_t	esi;
    } in;

    in.ebp = pager.raw;
    in.ebx = UserDefHandle;
    in.esi = ip;
    in.edi = flags;

    __asm__ __volatile__ (
	"/* L4_ExchangeRegisters() */			\n"
	__L4_SAVE_REGS
	"       pushl   %%esi                           \n"
	"       movl    (%%esi), %%ebp                  \n"
	"       movl    4(%%esi), %%edi                 \n"
	"       movl    8(%%esi), %%ebx                 \n"
	"       movl    12(%%esi), %%esi                \n"
	"	call	__L4_ExchangeRegisters		\n"
        "       xchgl   %%esi, (%%esp)                  \n"
        "       movl    %%edi, 4(%%esi)                 \n"
        "       movl    %%ebx, 8(%%esi)                 \n"
        "       movl    %%ebp, (%%esi)                  \n"
        "       popl    %%esi                           \n"
	__L4_RESTORE_REGS
        :
	"=a" (result), "=c" (*old_control), "=d" (*old_sp), "=S" (*old_ip)
        :
	"a" (dest.raw), "c" (control), "d" (sp), "S" (&in)
        :
	"edi", "memory", __L4_CLOBBER_REGS);

    old_pager->raw = in.ebp;
    *old_flags = in.edi;    
    *old_UserDefhandle = in.ebx;
    return result;
}


L4_INLINE L4_Word_t L4_ThreadControl (L4_ThreadId_t dest,
				      L4_ThreadId_t SpaceSpecifier,
				      L4_ThreadId_t Scheduler,
				      L4_ThreadId_t Pager,
				      void * UtcbLocation)
{
    L4_Word_t result;
    L4_Word_t dummy;

    __asm__ __volatile__ (
	"/* L4_ThreadControl() */			\n"
	__L4_SAVE_REGS
	"	call	__L4_ThreadControl		\n"
	__L4_RESTORE_REGS

	: /* outputs */
	"=a" (result),
	"=c" (dummy),
	"=d" (dummy),
	"=S" (dummy),
	"=D" (dummy)

	: /* inputs */
	"0" (dest),
	"1" (Pager),
	"2" (Scheduler),
	"3" (SpaceSpecifier),
	"4" (UtcbLocation)

	: /* clobbers */
	__L4_CLOBBER_REGS);

    return result;
}


L4_INLINE L4_Clock_t L4_SystemClock (void)
{
    L4_Clock_t result;

    __asm__ __volatile__ (
	"/* L4_SystemClock() */				\n"
	"	call	__L4_SystemClock		\n"

	: /* outputs */
	"=A" (result.raw)

	: /* no inputs */
	: /* clobbers */
	"ecx", "esi", "edi");

    return result;
}


L4_INLINE void L4_ThreadSwitch (L4_ThreadId_t dest)
{
    __asm__ __volatile__ (
	"/* L4_ThreadSwitch() */			\n"
	"	call	__L4_ThreadSwitch		\n"

	: /* no outputs */

	: /* inputs */
	"a" (dest)

	/* no clobbers */
	);
}


L4_INLINE L4_Word_t  L4_Schedule (L4_ThreadId_t dest,
				  L4_Word_t TimeControl,
				  L4_Word_t ProcessorControl,
				  L4_Word_t prio,
				  L4_Word_t PreemptionControl,
				  L4_Word_t * old_TimeControl)
{
    L4_Word_t result;
    L4_Word_t dummy;

    __asm__ __volatile__ (
	"/* L4_Schedule() */				\n"
	__L4_SAVE_REGS
	"	call	__L4_Schedule			\n"
	__L4_RESTORE_REGS

	: /* outputs */
	"=a" (result),
	"=c" (dummy),
	"=d" (*old_TimeControl),
	"=S" (dummy),
	"=D" (dummy)

	: /* inputs */
	"0" (dest),
	"1" (prio),
	"2" (TimeControl),
	"3" (ProcessorControl),
	"4" (PreemptionControl)

	: /* clobbers */
	__L4_CLOBBER_REGS);
    return result;
}

L4_INLINE L4_MsgTag_t L4_Ipc (L4_ThreadId_t to,
			      L4_ThreadId_t FromSpecifier,
			      L4_Word_t Timeouts,
			      L4_ThreadId_t * from)
{
    L4_MsgTag_t mr0;
    L4_Word_t mr1, mr2;
    L4_ThreadId_t result;
    L4_Word_t * utcb = __L4_X86_Utcb ();

#if defined(__pic__)
    __asm__ __volatile__ (
	"/* L4_Ipc() */					\n"
	__L4_SAVE_REGS
	"	call	__L4_Ipc			\n"
        "	movl	%%ebp, %%ecx			\n"
	"	movl	%%ebx, %%edx			\n"
	__L4_RESTORE_REGS

	: /* outputs */
	"=S" (mr0),
	"=a" (result),
	"=d" (mr1),
	"=c" (mr2)
        
	: /* inputs */
	"S" (utcb[0]),
	"a" (to.raw),
        "D" (utcb),
	"c" (Timeouts),
	"d" (FromSpecifier)
	);
#else
    L4_Word_t dummy;

    __asm__ __volatile__ (
	"/* L4_Ipc() */					\n"
	__L4_SAVE_REGS
	"	call	__L4_Ipc			\n"
        "	movl	%%ebp, %%ecx			\n"
	__L4_RESTORE_REGS

	: /* outputs */
	"=S" (mr0),
	"=a" (result),
	"=b" (mr1),
	"=c" (mr2),
        "=d" (dummy)
        
	: /* inputs */
	"S" (utcb[0]),
	"a" (to.raw),
        "D" (utcb),
	"c" (Timeouts),
	"d" (FromSpecifier)
	);
#endif

    if (! L4_IsNilThread (FromSpecifier)) {
	*from = result; 
	utcb[1] = mr1;
	utcb[2] = mr2;
    }

    return mr0;
}


L4_INLINE L4_MsgTag_t L4_Lipc (L4_ThreadId_t to,
			       L4_ThreadId_t FromSpecifier,
			       L4_Word_t Timeouts,
			       L4_ThreadId_t * from)
{
    L4_MsgTag_t mr0;
    L4_Word_t mr1, mr2;
    L4_ThreadId_t result;
    L4_Word_t * utcb = __L4_X86_Utcb ();

#if defined(__pic__)
    __asm__ __volatile__ (
	"/* L4_Lipc() */				\n"
	__L4_SAVE_REGS
	"	call	__L4_Lipc			\n"
        "	movl	%%ebp, %%ecx			\n"
	"	movl	%%ebx, %%edx			\n"
	__L4_RESTORE_REGS

	: /* outputs */
	"=S" (mr0),
	"=a" (result),
	"=d" (mr1),
	"=c" (mr2)
        
	: /* inputs */
	"S" (utcb[0]),
	"a" (to.raw),
        "D" (utcb),
	"c" (Timeouts),
	"d" (FromSpecifier)
	);
#else
    L4_Word_t dummy;

    __asm__ __volatile__ (
	"/* L4_Lipc() */				\n"
	__L4_SAVE_REGS
	"	call	__L4_Lipc			\n"
        "	movl	%%ebp, %%ecx			\n"
	__L4_RESTORE_REGS
	
	: /* outputs */
	"=S" (mr0),
	"=a" (result),
	"=b" (mr1),
	"=c" (mr2),
        "=d" (dummy)
        
	: /* inputs */
	"S" (utcb[0]),
	"a" (to.raw),
        "D" (utcb),
	"c" (Timeouts),
	"d" (FromSpecifier)
	);
#endif

    if (! L4_IsNilThread (FromSpecifier)) {
	*from = result; 
	utcb[1] = mr1;
	utcb[2] = mr2;
    }

    return mr0;
}

L4_INLINE void L4_Unmap (L4_Word_t control)
{
    L4_Word_t dummy;
    L4_Word_t * utcb = __L4_X86_Utcb ();

    __asm__ __volatile__ (
	"/* L4_Unmap() */				\n"
	__L4_SAVE_REGS
	"	call	__L4_Unmap			\n"
	__L4_RESTORE_REGS

	: /* outputs */
	"=S" (utcb[0]),
	"=D" (dummy),
	"=a" (dummy)

	: /* inputs */
	"0" (utcb[0]),
	"1" (utcb),
	"2" (control)

	: /* clobbered */
	"ecx", "edx", __L4_CLOBBER_REGS);
}


L4_INLINE L4_Word_t L4_SpaceControl (L4_ThreadId_t SpaceSpecifier,
				     L4_Word_t control,
				     L4_Fpage_t KernelInterfacePageArea,
				     L4_Fpage_t UtcbArea,
				     L4_ThreadId_t redirector,
				     L4_Word_t *old_control)
{
    L4_Word_t result, dummy;

    __asm__ __volatile__ (
	"/* L4_SpaceControl() */			\n"
	__L4_SAVE_REGS
	"	call	__L4_SpaceControl		\n"
	__L4_RESTORE_REGS

	: /* outputs */
	"=a" (result),
	"=c" (*old_control),
	"=d" (dummy),
	"=S" (dummy),
	"=D" (dummy)

	: /* inputs */
	"0" (SpaceSpecifier),
	"1" (control),
	"2" (KernelInterfacePageArea),
	"3" (UtcbArea),
	"4" (redirector)

	: /* clobbers */
	__L4_CLOBBER_REGS);

    return result;
}


L4_INLINE L4_Word_t L4_ProcessorControl (L4_Word_t ProcessorNo,
					 L4_Word_t InternalFrequency,
					 L4_Word_t ExternalFrequency,
					 L4_Word_t voltage)
{
    L4_Word_t result, dummy;

    __asm__ __volatile__ (
	"/* L4_ProcessorControl() */			\n"
	__L4_SAVE_REGS
	"	call	__L4_ProcessorControl		\n"
	__L4_RESTORE_REGS

	: /* outputs */
	"=a" (result),
	"=c" (dummy),
	"=d" (dummy),
	"=S" (dummy),
	"=D" (dummy)

	: /* inputs */
	"0" (ProcessorNo),
	"1" (InternalFrequency),
	"2" (ExternalFrequency),
	"3" (voltage)

	: /* clobbers */
	__L4_CLOBBER_REGS);

    return result;
}


L4_INLINE L4_Word_t L4_MemoryControl (L4_Word_t control,
				 const L4_Word_t * attributes)
{
    L4_Word_t result, dummy;
    L4_Word_t * utcb = __L4_X86_Utcb ();

    __asm__ __volatile__ (
	"/* L4_MemoryControl() */			\n"
	__L4_SAVE_REGS
	"	movl	12(%6), %%ebp			\n"
	"	movl	8(%6), %%ebx			\n"
	"	movl	4(%6), %%edx			\n"
	"	movl	(%6), %%ecx			\n"
	"	call	__L4_MemoryControl		\n"
	__L4_RESTORE_REGS

	: /* outputs */
	"=a" (result),
 	"=c" (dummy),
	"=d" (dummy),
	"=S" (dummy),
	"=D" (dummy)

	: /* inputs */
	"0" (control),
	"1" (attributes),
	"3" (utcb[0]),
	"4" (utcb)

	: /* clobbers */
	__L4_CLOBBER_REGS);

    return result;
}


#endif /* !__L4__X86__SYSCALLS_H__ */
