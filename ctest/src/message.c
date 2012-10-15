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

#include <stdio.h>
#include <ctest/ctest.h>

#include <okl4/message.h>
#include <okl4/kthread.h>

#include "helpers.h"

/* Child thread prototype. */
typedef void(*msg_test_child_fn)(okl4_word_t id, okl4_kcap_t *peers,
        okl4_word_t test_size);

static void
message_test_child_entry(okl4_word_t id, okl4_kcap_t *peers, void *arg)
{
    okl4_word_t i;
    msg_test_child_fn *entry_points = (msg_test_child_fn *)arg;

    /* Run the test for every test size. */
    for (i = 0; i <= OKL4_MESSAGE_MAX_SIZE; i++) {
        entry_points[id](id, peers, i);
    }
}

static void
do_message_test(msg_test_child_fn *children)
{
    okl4_word_t num_children;
    okl4_word_t i;

    /* Count number of children threads. */
    num_children = 0;
    for (i = 0; children[i] != NULL; i++) {
        num_children++;
    }

    /* Run our children. */
    multithreaded_test(num_children, message_test_child_entry, children);
}

/*
 * Buffer setup/check functionality.
 */

static char *
new_recv_buff(okl4_word_t test_size)
{
    okl4_word_t i;
    char *buff;

    /* Setup the buffer. */
    buff = malloc(test_size + 1);
    assert(buff);
    for (i = 0; i < test_size + 1; i++) {
        buff[i] = (char)-1;
    }

    return buff;
}

static void
check_and_free_recv_buff(char *buff, okl4_word_t test_size)
{
    okl4_word_t i;

    /* Check message. */
    for (i = 0; i < test_size; i++) {
        assert(buff[i] == (char)(1 + i));
    }

    /* Ensure no extra bytes were written. */
    assert(buff[test_size] == (char)-1);
    free(buff);
}

static char *
new_send_buff(okl4_word_t test_size)
{
    okl4_word_t i;
    char *buff;

    /* Setup the buffer. */
    buff = malloc(test_size + 1);
    assert(buff);
    for (i = 0; i < test_size; i++) {
        buff[i] = (char)(1 + i);
    }

    return buff;
}

static void
check_and_free_send_buff(char *buff, okl4_word_t test_size)
{
    free(buff);
}

/*
 * Message Operations
 */

enum msg_op {
    OP_SEND,
    OP_WAIT,
    OP_REPLY,
    OP_REPLYWAIT,
    OP_CALL
};

/* Constants for 'do_ipc_op'. */
#define EXPECT_SUCCESS  0
#define EXPECT_ERROR    1

/*
 * Perform an IPC operation, checking the results.
 */
static void
do_ipc_op(enum msg_op op, okl4_kcap_t dest,
        okl4_kcap_t *from, okl4_word_t test_size,
        int expect_error)
{
    char *send_buff, *recv_buff;
    int error;
    okl4_word_t bytes;
    int do_send_check = 0;
    int do_recv_check = 0;

    recv_buff = new_recv_buff(test_size);
    send_buff = new_send_buff(test_size);

    switch (op) {
    case OP_SEND:
        do_send_check = 1;
        error = okl4_message_send(dest, send_buff, test_size);
        break;

    case OP_WAIT:
        do_recv_check = 1;
        error = okl4_message_wait(recv_buff, test_size, &bytes, from);
        break;

    case OP_REPLY:
        do_send_check = 1;
        error = okl4_message_reply(dest, send_buff, test_size);
        break;

    case OP_REPLYWAIT:
        do_recv_check = 1;
        do_send_check = 1;
        error = okl4_message_replywait(dest, send_buff, test_size,
                recv_buff, test_size, &bytes, from);
        break;

    case OP_CALL:
        do_recv_check = 1;
        do_send_check = 1;
        error = okl4_message_call(dest, send_buff, test_size,
                recv_buff, test_size, &bytes);
        break;

    default:
        while (1) {
            assert(!"Unknown IPC operation.");
        }
    }

    /* Ensure we got an error if we were expecting one. */
    if (expect_error) {
        assert(error);
        return;
    }

    /* Otherwise, make sure everything was okay. */
    assert(!error);
    if (do_send_check) {
        check_and_free_send_buff(send_buff, test_size);
    } else {
        free(send_buff);
    }
    if (do_recv_check) {
        check_and_free_recv_buff(recv_buff, test_size);
    } else {
        free(recv_buff);
    }
}


/*
 * MSG0100 : Simple send/wait combination.
 */

static void
msg0100_send(okl4_word_t id, okl4_kcap_t *peers, okl4_word_t test_size)
{
    do_ipc_op(OP_SEND, peers[1], NULL, test_size, EXPECT_SUCCESS);
}

static void
msg0100_wait(okl4_word_t id, okl4_kcap_t *peers, okl4_word_t test_size)
{
    do_ipc_op(OP_WAIT, OKL4_NULL_KCAP, NULL, test_size, EXPECT_SUCCESS);
}

START_TEST(MSG0100)
{
    msg_test_child_fn children[] = {
            msg0100_send,
            msg0100_wait,
            NULL
            };
    do_message_test(children);
}
END_TEST

/*
 * MSG0200 : Simple call/reply combination.
 */

static void
msg0200_call(okl4_word_t id, okl4_kcap_t *peers, okl4_word_t test_size)
{
    do_ipc_op(OP_CALL, peers[1], NULL, test_size, EXPECT_SUCCESS);
}

static void
msg0200_reply(okl4_word_t id, okl4_kcap_t *peers, okl4_word_t test_size)
{
    okl4_kcap_t from;

    do_ipc_op(OP_WAIT, OKL4_NULL_KCAP, &from, test_size, EXPECT_SUCCESS);
    do_ipc_op(OP_REPLY, from, NULL, test_size, EXPECT_SUCCESS);
}

START_TEST(MSG0200)
{
    msg_test_child_fn children[] = {
            msg0200_call,
            msg0200_reply,
            NULL
            };
    do_message_test(children);
}
END_TEST

/*
 * MSG0300 : Simple call/replywait combination.
 */

static void
msg0300_call(okl4_word_t id, okl4_kcap_t *peers, okl4_word_t test_size)
{
    do_ipc_op(OP_CALL, peers[1], NULL, test_size, EXPECT_SUCCESS);
    do_ipc_op(OP_CALL, peers[1], NULL, test_size, EXPECT_SUCCESS);
}

static void
msg0300_replywait(okl4_word_t id, okl4_kcap_t *peers, okl4_word_t test_size)
{
    okl4_kcap_t from;

    do_ipc_op(OP_WAIT, OKL4_NULL_KCAP, &from, test_size, EXPECT_SUCCESS);
    do_ipc_op(OP_REPLYWAIT, from, &from, test_size, EXPECT_SUCCESS);
    do_ipc_op(OP_REPLY, from, NULL, test_size, EXPECT_SUCCESS);
}

START_TEST(MSG0300)
{
    msg_test_child_fn children[] = {
            msg0300_call,
            msg0300_replywait,
            NULL
            };
    do_message_test(children);
}
END_TEST

/*
 * MSG0400 : TrySend to a thread not waiting.
 */

static void
msg0400_receiver(okl4_word_t id, okl4_kcap_t *peers, okl4_word_t test_size)
{
    /* We won't be ready to receive until the other side first performs a
     * receive. */
    do_ipc_op(OP_CALL, peers[1], NULL, test_size, EXPECT_SUCCESS);
}

static void
msg0400_sender(okl4_word_t id, okl4_kcap_t *peers, okl4_word_t test_size)
{
    /* Should fail, as peer is trying to send to us. */
    do_ipc_op(OP_REPLY, peers[0], NULL, test_size, EXPECT_ERROR);

    /* Let our peer's send succeed. */
    do_ipc_op(OP_WAIT, OKL4_NULL_KCAP, NULL, test_size, EXPECT_SUCCESS);

    /* We can now send. */
    do_ipc_op(OP_REPLY, peers[0], NULL, test_size, EXPECT_SUCCESS);
}

START_TEST(MSG0400)
{
    msg_test_child_fn children[] = {
            msg0400_receiver,
            msg0400_sender,
            NULL
            };
    do_message_test(children);
}
END_TEST

TCase *
make_message_tcase(void)
{
    TCase * tc;

    tc = tcase_create("Message Interface");
    tcase_add_test(tc, MSG0100);
    tcase_add_test(tc, MSG0200);
    tcase_add_test(tc, MSG0300);
    tcase_add_test(tc, MSG0400);

    return tc;
}

