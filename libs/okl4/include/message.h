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

#ifndef __OKL4__MESSAGE_H__
#define __OKL4__MESSAGE_H__

#include <okl4/types.h>

#if defined(OKL4_KERNEL_MICRO)
#include <l4/message.h>
#endif

/**
 * @file
 *
 * An interface allowing threads to synchronously send messages with small
 * payloads to each other.
 */

#if defined(OKL4_KERNEL_MICRO)
#define _OKL4_MESSAGE_MAX_SIZE_KERNEL ((__L4_NUM_MRS - 1) * sizeof(okl4_word_t))
#endif

#if defined(OKL4_KERNEL_NANO)
#define _OKL4_MESSAGE_MAX_SIZE_KERNEL ((7 * sizeof(okl4_word_t)) - 1)
#endif

/**
 * The maximum number of bytes that may be sent or received in a single
 * messaging operation.
 */
#define OKL4_MESSAGE_MAX_SIZE _OKL4_MESSAGE_MAX_SIZE_KERNEL

/**
 * Send a message to another thread, blocking until the destination becomes
 * ready to receive.
 *
 * @param dest
 *     The destination of the message.
 *
 * @param buff
 *     The data to send.
 *
 * @param buff_size
 *     The number of bytes to send from the buffer.
 *
 * @retval OKL4_OK
 *     The message was successfully sent.
 *
 * @retval OKL4_INVALID_ARGUMENT
 *     An invalid destination was specified.
 */
int okl4_message_send(
        okl4_kcap_t dest,
        void * buff,
        okl4_word_t buff_size);

/**
 * Send a message to another thread. If the destination is not ready to
 * receive, return an error without blocking.
 *
 * This function is identical to the message reply functionality, and is
 * provided as an alias for the function.
 *
 * @param dest
 *     The destination of the message.
 *
 * @param buff
 *     The data to send.
 *
 * @param buff_size
 *     The number of bytes to send from the buffer.
 *
 * @retval OKL4_OK
 *     The message was successfully sent.
 *
 * @retval OKL4_WOULD_BLOCK
 *     The destination was not ready to receive.
 *
 * @retval OKL4_INVALID_ARGUMENT
 *     An invalid destination was specified.
 */
int okl4_message_trysend(
        okl4_kcap_t dest,
        void * buff,
        okl4_word_t buff_size);

/**
 * Wait for another thread to send a message.
 *
 * @param recv_buff
 *     The location to write the received message data to.
 *
 * @param recv_buff_size
 *     The maximum number of bytes to receive.
 *
 * @param recv_bytes
 *     The number of bytes in the received message. This may exceed the
 *     number of bytes in the receive buffer, indicating that the received
 *     message was truncated.
 *
 * @param reply_cap
 *     An identifier suitable for use in "reply" and "replywait" operations to
 *     reply to threads performing "call" operations.
 *
 *     Reply cap identifiers should not be used for any operation other than
 *     "reply" or "replywait". They are not guaranteed to be unique nor to
 *     match caps produced in some other manner.
 *
 * @retval OKL4_OK
 *     A message was successfully received.
 */
int okl4_message_wait(
        void * recv_buff,
        okl4_word_t recv_buff_size,
        okl4_word_t * recv_bytes,
        okl4_kcap_t * reply_cap);

/**
 * Send a message to another thread, and wait for a reply from them. If the
 * destination is not ready to receive, block until they become ready.
 *
 * @param dest
 *     The destination of the message.
 *
 * @param buff
 *     The data to send.
 *
 * @param buff_size
 *     The number of bytes to send from the buffer.
 *
 * @param recv_buff
 *     The location to write the received message data to.
 *
 * @param recv_buff_size
 *     The maximum number of bytes to receive.
 *
 * @param recv_bytes
 *     The number of bytes in the received message. This may exceed the
 *     number of bytes in the receive buffer, indicating that the received
 *     message was truncated.
 *
 * @retval OKL4_OK
 *     The message was successfully sent and a reply was received.
 *
 * @retval OKL4_INVALID_ARGUMENT
 *     An invalid destination was specified.
 */
int okl4_message_call(
        okl4_kcap_t dest,
        void * buff,
        okl4_word_t buff_size,
        void * recv_buff,
        okl4_word_t recv_buff_size,
        okl4_word_t * recv_bytes);

/**
 * Send a message to another thread. If the destination is not ready to
 * receive, return an error without blocking.
 *
 * @param dest
 *     The destination of the message.
 *
 * @param buff
 *     The data to send.
 *
 * @param buff_size
 *     The number of bytes to send from the buffer.
 *
 * @retval OKL4_OK
 *     The message was successfully sent.
 *
 * @retval OKL4_WOULD_BLOCK
 *     The destination was not ready to receive.
 *
 * @retval OKL4_INVALID_ARGUMENT
 *     An invalid destination was specified.
 */
int okl4_message_reply(
        okl4_kcap_t dest,
        void * buff,
        okl4_word_t buff_size);

/**
 * Send a message to another thread. If the destination is not ready to
 * receive, return an error without blocking. If the send succeeds, wait
 * for another thread to send a message, blocking if necessary.
 *
 * @param dest
 *     The destination of the message.
 *
 * @param buff
 *     The data to send.
 *
 * @param buff_size
 *     The number of bytes to send from the buffer.
 *
 * @param recv_buff
 *     The location to write the received message data to.
 *
 * @param recv_buff_size
 *     The maximum number of bytes to receive.
 *
 * @param recv_bytes
 *     The number of bytes in the received message. This may exceed the
 *     number of bytes in the receive buffer, indicating that the received
 *     message was truncated.
 *
 * @param reply_cap
 *     An identifier suitable for use in "reply" and "replywait" operations to
 *     reply to threads performing "call" operations.
 *
 *     Reply cap thread identifiers should not be used for any operation other
 *     than "reply" or "replywait". They are not guaranteed to be unique or to
 *     match caps produced in some other manner.
 *
 * @retval OKL4_OK
 *     The message was successfully sent, and a message was received.
 *
 * @retval OKL4_WOULD_BLOCK
 *     The destination was not ready to receive.
 *
 * @retval OKL4_INVALID_ARGUMENT
 *     An invalid destination was specified.
 */
int okl4_message_replywait(
        okl4_kcap_t dest,
        void * buff,
        okl4_word_t buff_size,
        void * recv_buff,
        okl4_word_t recv_buff_size,
        okl4_word_t * recv_bytes,
        okl4_kcap_t * reply_cap);

#endif /* !__OKL4__MESSAGE_H__ */

