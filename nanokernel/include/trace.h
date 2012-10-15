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
 * Kernel Event Tracing Functionality
 */

#ifndef __TRACE_H__
#define __TRACE_H__

#include <nano.h>

#if defined(CONFIG_TRACEBUFFER)

/*
 * Events to trace
 */
#define TRACE_EVENT_INTERRUPT       0
#define TRACE_EVENT_SWITCH_TO       1
#define TRACE_EVENT_SMT_RESCHEDULE  2

/*
 * Trace an event into the trace buffer.
 *
 * Users may wish to use the simplier 'ASM_TRACE_EVENT' macro instead
 * of these ones unless advanced functionality is required.
 *
 * To log an event into the trace buffer, users perform the following
 * sequence of events:
 *
 *     ASM_TRACE_EVENT_START(TYPE) ; Clobbers r31, p0
 *     r0 = <event number>         ; r0-r3 restored after ASM_TRACE_EVENT_END()
 *     r1 = <parameter #1>
 *     r2 = <parameter #2>
 *     r3 = <parameter #3>
 *     ASM_TRACE_EVENT_END()       ; Re-clobbers r31, p0
 *
 * This will place an entry of type "TYPE" into the tracebuffer with parameters
 * (r1, r2, r3). Registers r1--r3 will be restored to their values prior to the
 * ASM_TRACE_EVENT_START() after the ASM_TRACE_EVENT_END() is executed. r31 and
 * p0 are permently clobbered.
 *
 * Code within the block will not be executed when tracing for the event
 * is not enabled.
 *
 * Before the macro is called, a stack must be set up, the kernel's GP register
 * must be valid, and the R23_SHADOW_SSR register must contain an SSR with a
 * valid unit number in it.
 */
#define ASM_TRACE_EVENT_START(event_num)                               \
        r31 = memw(#trace_events);                                     \
        {                                                              \
        p0 = tstbit(r31, # ## event_num);                              \
        if (!p0.new) jump:t 10001f;                                    \
        }                                                              \
        call start_trace_event;                                        \

#define ASM_TRACE_EVENT_END()                                          \
        call end_trace_event;                                          \
        10001:

/*
 * Trace an event into the trace buffer.
 *
 * 'event_num' specifies the event that is being traced, and is a number
 * between 0 and 31 inclusive. 'arg1' through 'arg3' are arguments to
 * trace, and may either be small immeadiate values or registers.
 *
 * Before the macro is called, a stack must be set up, the kernel's GP register
 * must be valid, and the R23_SHADOW_SSR register must contain an SSR with a
 * valid unit number in it.
 *
 * The macro clobbers 'p0' and 'r31'. All other registers will be retained.
 */
#define ASM_TRACE_EVENT(event_num, arg1, arg2, arg3)                   \
        ASM_TRACE_EVENT_START(event_num);                              \
        {                                                              \
        r0 = # ## event_num;                                           \
        r1 = arg1;                                                     \
        r2 = arg2;                                                     \
        r3 = arg3;                                                     \
        }                                                              \
        ASM_TRACE_EVENT_END()

#else /* !defined(CONFIG_TRACEBUFFER) */

/* Ignore any trace events. */
#define ASM_TRACE_EVENT(event_num, arg1, arg2, arg3)

#endif /* !defined(CONFIG_TRACEBUFFER) */

#endif /* __TRACE_H__ */
