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

#ifndef __OKL4__NOTIFY_H__
#define __OKL4__NOTIFY_H__

#include <okl4/types.h>


/**
 * @file
 *
 * An interface allowing threads to asynchronously send single bits of
 * information to each other.
 */

/**
 * Send the given bits to a destination thread.
 *
 * If the destination thread is waiting for one or more of bits being sent,
 * this call will wake the destination.
 *
 * If one or more of the bit being sent have already been set since the
 * destination's last "wait" or "poll" call, those bits will be ignored.
 *
 * This operation will always be performed without causing the current
 * thread to block.
 *
 * @param dest
 *     The destination thread to send to.
 *
 * @param bits
 *     The bits to send, encoded as a bitmask.
 *
 * @retval OKL4_OK
 *     The notification was successfully sent.
 *
 * @retval OKL4_INVALID_ARGUMENT
 *     An invalid destination was specified.
 */
int okl4_notify_send(
        okl4_kcap_t dest,
        okl4_word_t bits);

/**
 * Determine if any bits set in "mask" have been sent to us since the last
 * time the current thread either polled or waited on those bits.
 *
 * Any bits received in the provided bitmask will be returned and cleared
 * until next time they are received.
 *
 * @param mask
 *     A bitmask indicating which bits should be polled.
 *
 * @param bits
 *     The result of the poll.
 */
void okl4_notify_poll(
        okl4_word_t mask,
        okl4_word_t * bits);

/**
 * Block until at least one bit in "mask" is sent to the current thread.
 *
 * Any bits received in the provided bitmask will be returned and cleared
 * until next time they are received. If a notification has already been
 * sent, this call will return without blocking.
 *
 * @param mask
 *     A bitmask indicating which bits should be waited on.
 *
 * @param bits
 *     The result of the wait.
 */
void okl4_notify_wait(
        okl4_word_t mask,
        okl4_word_t * bits);

#endif /* !__OKL4__NOTIFY_H__ */

