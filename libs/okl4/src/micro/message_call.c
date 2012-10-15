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

#include <assert.h>

#include <okl4/errno.h>
#include <okl4/types.h>
#include <okl4/message.h>

#include <l4/ipc.h>

#include "message_helpers.h"

static int
send_wait(okl4_kcap_t dest, okl4_kcap_t src, void *buff, okl4_word_t buff_size,
        void * recv_buff, okl4_word_t recv_buff_size,
        okl4_word_t *recv_bytes, okl4_kcap_t *reply_cap, int blocking_send)
{
    L4_MsgTag_t tag;
    L4_ThreadId_t from;
    okl4_word_t bytes_received;

    /* Ensure sane parameters. */
    assert(buff_size <= OKL4_MESSAGE_MAX_SIZE);
    assert(buff != NULL || buff_size == 0);
    assert(recv_buff != NULL || recv_buff_size == 0);

    /* Setup our virtual registers. */
    tag = L4_Make_MsgTag(buff_size, 0);
    if (blocking_send) {
        L4_Set_SendBlock(&tag);
    } else {
        L4_Clear_SendBlock(&tag);
    }
    L4_Set_ReceiveBlock(&tag);
    tag.X.u = (buff_size + sizeof(okl4_word_t) - 1) / sizeof(okl4_word_t);
    _okl4_message_copy_buff_to_mrs(buff, buff_size);

    /* Perform the IPC. */
    L4_Set_NotifyMask(0);
    tag = L4_Ipc(dest, src, tag, &from);
    if (!L4_IpcSucceeded(tag)) {
        return _okl4_message_errno(tag, L4_ErrorCode());
    }

    /* Determine the number of words received, which is in the label. */
    bytes_received = L4_Label(tag);

    /* Ensure that the number of bytes the message claims to contain is roughly
     * the same as the number of words we received. */
    assert(ROUND_UP(bytes_received, sizeof(okl4_word_t))
            == L4_UntypedWords(tag) * sizeof(okl4_word_t));

    /* Copy to the buffer. */
    _okl4_message_copy_mrs_to_buff(recv_buff,
            min(recv_buff_size, bytes_received));

    /* Give parameters back to the caller. */
    if (recv_bytes != NULL) {
        *recv_bytes = bytes_received;
    }
    if (reply_cap != NULL) {
        *reply_cap = from;
    }

    return OKL4_OK;
}

int
okl4_message_call(okl4_kcap_t dest, void * buff, okl4_word_t buff_size,
        void * recv_buff, okl4_word_t recv_buff_size,
        okl4_word_t * recv_bytes)
{
    return send_wait(dest, dest, buff, buff_size, recv_buff,
            recv_buff_size, recv_bytes, NULL, 1);
}

int
okl4_message_replywait(okl4_kcap_t dest, void *buff, okl4_word_t buff_size,
        void *recv_buff, okl4_word_t recv_buff_size, okl4_word_t *recv_bytes,
        okl4_kcap_t *reply_cap)
{
    return send_wait(dest, L4_anythread, buff, buff_size, recv_buff,
            recv_buff_size, recv_bytes, reply_cap, 0);
}

