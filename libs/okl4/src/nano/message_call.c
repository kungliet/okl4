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

#include <okl4/errno.h>
#include <okl4/types.h>
#include <okl4/message.h>

#include "message_helpers.h"

static int
send_wait(okl4_kcap_t dest, okl4_kcap_t src, void *buff, okl4_word_t buff_size,
        void * recv_buff, okl4_word_t recv_buff_size,
        okl4_word_t *recv_bytes, int blocking_send)
{
    int i;
    int result;
    union {
        okl4_word_t mr[IPC_NUM_MRS];
        unsigned char buff[IPC_NUM_MRS * sizeof(okl4_word_t)];
    } u;

    /* Ensure sane parameters. */
    assert(buff_size <= OKL4_MESSAGE_MAX_SIZE);
    assert(buff != NULL || buff_size == 0);
    assert(recv_buff_size <= OKL4_MESSAGE_MAX_SIZE);
    assert(recv_buff != NULL || recv_buff_size == 0);

    /* We insist that the buffer is word-aligned. */
    assert((okl4_word_t)buff % sizeof(okl4_word_t) == 0);

    /* Zero off our buffer. */
    for (i = 0; i < IPC_NUM_MRS; i++) {
        u.mr[i] = 0;
    }

    /* Store the number of bytes in the last byte of the message. */
    u.buff[OKL4_MESSAGE_MAX_SIZE] = buff_size;

    /* Load up words out of the buffer. */
    _okl4_copy_memory(u.buff, buff, buff_size);

    /* Perform the IPC. */
    result = okn_syscall_ipc_call(&u.mr[0], &u.mr[1], &u.mr[2], &u.mr[3],
            &u.mr[4], &u.mr[5], &u.mr[6], dest,
            (blocking_send ? OKN_IPC_BLOCKING : OKN_IPC_NON_BLOCKING));
    if (result != 0) {
        return OKL4_INVALID_ARGUMENT;
    }

    /* Determine the number of bytes in the message, transferred in the
     * last byte of the message. */
    okl4_word_t bytes = u.buff[OKL4_MESSAGE_MAX_SIZE];

    /* Copy to the message buffer. */
    _okl4_copy_memory(recv_buff, u.buff, min(recv_buff_size, bytes));

    /* Copy meta-data back to user. */
    if (recv_bytes) {
        *recv_bytes = bytes;
    }

    return OKL4_OK;
}

int
okl4_message_call(okl4_kcap_t dest, void * buff, okl4_word_t buff_size,
        void * recv_buff, okl4_word_t recv_buff_size,
        okl4_word_t * recv_bytes)
{
    return send_wait(dest, dest, buff, buff_size, recv_buff,
            recv_buff_size, recv_bytes, 1);
}

