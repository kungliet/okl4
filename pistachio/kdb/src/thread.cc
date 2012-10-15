/*
 * Copyright (c) 2004, Karlsruhe University
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
 * Description:   Kdebug stuff for V4 threads
 */
#include <l4.h>
#include <kdb/tid_format.h>
#include <kdb/print.h>
#include <capid.h>
#include <tcb.h>


#if defined(CONFIG_KDB_CONS)

/* From generic/print.cc */

int print_dec (const word_t val,
               const int width = 0,
               const char pad = ' ');


int SECTION (SEC_KDEBUG)
print_tcb(word_t val, word_t width, word_t precision, bool adjleft)
{
    capid_t tid = capid(val);

    /* If the TID has a human-readable name, use that. */
    if (kdb_tid_format.X.human) {
        if (tid.is_nilthread()) {
            return print_string("NIL_THRD", width, precision);
        }

        if (tid.is_anythread()) {
            return print_string("ANY_THRD", width, precision);
        }

        if (tid.is_waitnotify()) {
            return print_string("WAIT_NOTIFY", width, precision);
        }

        if (tid.is_myself()) {
            return print_string("MYSELF_ID", width, precision);
        }

#if defined(CONFIG_THREAD_NAMES)
        /* Are we the idle thread? */
        if (val == (word_t)get_idle_tcb()) {
            return print_string("idle_thread", width, precision);
        }

        word_t n = 0;
        tcb_t *tcb = NULL;

        if (tid.is_threadhandle()) {
            tcb = lookup_tcb_by_handle_locked(tid.get_raw());
            if (tcb != NULL) {
                tcb->unlock_read();
                if (tcb->debug_name[0] != '\0') {
                    n = print_string("R-", 2, 0);
                }
            }
        } else if (tid.get_type() == TYPE_CAP && get_current_clist() != NULL) {
            tcb = get_current_clist()->lookup_ipc_cap_locked(tid);
            if (tcb != NULL) {
                tcb->unlock_read();
            }
        } else {
            /* TCB may be a pointer to the middle of a tcb_t structure. Search through
             * all the TCBs in the system to see if it is. This is a slow operation, so
             * we avoid doing it until last. */
            tcb = get_tcb((addr_t)val);
        }

        /* If the thread has a debugging name, print it out. */
        if (tcb != NULL) {
            if (tcb->debug_name[0] != '\0') {
                n += print_string(tcb->debug_name, width, precision);
                return n;
            }
        }
#endif
    }

    /* We're dealing with something which does not have a name set. */
    word_t n = 0;
    tcb_t *tcb = NULL;

    if (tid.is_threadhandle()) {
        tcb = lookup_tcb_by_handle_locked(tid.get_raw());
        if (tcb != NULL) {
            n += print_string("R-", 2, 0);
            tcb->unlock_read();
        }
    } else if (tid.get_type() == TYPE_CAP && get_current_clist() != NULL) {
        tcb = get_current_clist()->lookup_ipc_cap_locked(tid);
        if (tcb != NULL) {
            tcb->unlock_read();
        }
    } else {
        /* TCB may be a pointer to the middle of a tcb_t structure. Search through
         * all the TCBs in the system to see if it is. This is a slow operation, so
         * we avoid doing it until last. */
        tcb = get_tcb((addr_t)val);
    }

    bool f_both = (word_t)TID_FORMAT_VALUE_BOTH == kdb_tid_format.X.value;
    bool f_cap = (tid.get_type() == TYPE_CAP) ? (word_t)TID_FORMAT_VALUE_CAP == kdb_tid_format.X.value : 0;
    bool f_tcb = tcb ? true : (word_t)TID_FORMAT_VALUE_TCB == kdb_tid_format.X.value;

    if (f_cap || f_both)
    {
        if (kdb_tid_format.X.sep != 0) {
            // Insert a separator into cap
            n += print_hex_sep (tid.get_type(),
                               kdb_tid_format.X.sep, ".");
        } else {
            n += print_hex (tid.get_type());
        }

        n += print_string ("-");
        width -= width > n ? n : 0;
        n += print_hex (tid.get_index(),
                        f_both ? 0 : width, 0, true);

        if (f_both) {
            n += print_string ("/");
        } else {
            return n;
        }
    }

    if (f_tcb) {
        /* Print a plain hexadecimal address. */
        n += print_hex((word_t)tcb, 0, sizeof (word_t) * 2);
    } else {
        /* Print a plain hexadecimal address. */
        n += print_hex(tid.get_raw(), 0, sizeof (word_t) * 2);
    }
    return n;
}

#endif
