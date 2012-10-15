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

#include <ktest/bad_capids.h>
#include <ktest/ktest.h>
#include <ktest/utility.h>
#include <ktest/ktest.h>
#include <l4/ipc.h>
#include <l4/schedule.h>
#include <l4/kdebug.h>
#include <l4/config.h>
#include <stdio.h>

extern L4_ThreadId_t test_tid;
#if !defined(NO_UTCB_RELOCATE)
static L4_Word_t utcb_target;
#endif

static void 
waiting_thread(void)
{
    L4_MsgTag_t tag;
    L4_ThreadId_t partner = L4_anythread;

    tag = L4_Wait(&partner);
    fail_unless(L4_IpcFailed(tag), "Thread unexpectedly received message successfully");

    L4_Send(L4_Myself());
}

static void 
sending_thread(void)
{
    L4_MsgTag_t tag;

    tag = L4_Send(test_tid);
    fail_unless(L4_IpcFailed(tag), "Thread unexpectedly sent message successfully");

    L4_Send(L4_Myself());
}

static void 
waiting_notify_thread(void)
{
    L4_MsgTag_t tag;
    L4_Word_t mask;

    L4_Set_NotifyMask(1);
    L4_Accept(L4_AsynchItemsAcceptor);
    tag = L4_WaitNotify(&mask);
    fail_unless(L4_IpcFailed(tag), "Thread unexpectedly received asynchronous message successfully");

    L4_Send(L4_Myself());
}

static void 
waiting_cancel_thread(void)
{
    L4_MsgTag_t tag;
    L4_ThreadId_t partner = L4_anythread;

    tag = L4_Wait(&partner);
    fail_unless(L4_IpcFailed(tag), "Thread unexpectedly received message successfully");
    fail_if(L4_ErrorCode() == L4_ErrCanceled, "IPC operation has been cancelled");

    L4_Send(L4_Myself());
}

#if !defined(NO_UTCB_RELOCATE)
static void 
get_utcb_target_thread(void)
{
    utcb_target = (L4_Word_t)L4_GetUtcbBase(); //Can't use get_arch_utcb_base()
    L4_Call(test_tid);
    //do nothing.
    while(1);
}
#endif

static L4_ThreadId_t
create_waiting_thread(void (*ip)(void))
{
    L4_Word_t result;
    L4_ThreadId_t tid;

    tid = createThread(ip);
    result = L4_Set_Priority(tid, 200);
    fail_unless(result != L4_SCHEDRESULT_ERROR,
                "L4_Set_Priority() failed to set the new priority");
    L4_ThreadSwitch(tid); // Give it time to start waiting for IPC operation

    return tid;
}


int
run_bad_capids_tests(int syscall, L4_CapId_t idx_start, L4_CapId_t idx_end, L4_Word_t expect_result)
{
    L4_CapId_t idx_cur;
    L4_ThreadId_t waiting_tid = L4_nilthread;
    L4_SpaceId_t space = L4_nilspace;
    L4_MsgTag_t tag;
    L4_ThreadId_t from;
    L4_Word_t dummy, res;

    for (idx_cur = idx_start; L4_IsCapNotEqual(idx_cur, idx_end); idx_cur.X.index++) {
        L4_ThreadId_t tid = idx_cur;

        if (isSystemThread(tid)) {
            continue;
        }

        switch (syscall & SYSCALL_MASK) {
            case SYSCALL_IPC:
                if ((syscall & OP_MASK) == IPC_SEND) {
                    if (L4_IsNilThread(waiting_tid))
                        waiting_tid = create_waiting_thread(waiting_thread);
                    if (L4_IsThreadNotEqual(waiting_tid, tid)) {
                        tag = L4_Send_Nonblocking(tid);
                        if (expect_result == FAILURE)
                            fail_unless(L4_IpcFailed(tag), "Message succesfully sent to invalid thread id");
                        if (expect_result == SUCCESS)
                            fail_unless(L4_IpcSucceeded(tag), "Message unsuccesfully sent to valid thread id");
                    }
                    break;
                }
                if ((syscall & OP_MASK) == IPC_REPLY_WAIT) {
                    if (L4_IsNilThread(waiting_tid))
                        waiting_tid = create_waiting_thread(waiting_thread);
                    if (L4_IsThreadNotEqual(waiting_tid, tid)) {
                        tag = L4_ReplyWait(tid, &from);
                        if (expect_result == FAILURE)
                            fail_unless(L4_IpcFailed(tag), "Message succesfully sent to invalid thread id");
                        if (expect_result == SUCCESS)
                            fail_unless(L4_IpcSucceeded(tag), "Message unsuccesfully sent to valid thread id");
                    }
                    break;
                }
                if ((syscall & OP_MASK) == IPC_RECEIVE) {
                    if (L4_IsNilThread(waiting_tid))
                        waiting_tid = create_waiting_thread(sending_thread);
                    if (L4_IsThreadNotEqual(waiting_tid, tid)) {
                        tag = L4_Receive_Nonblocking(tid);
                        if (expect_result == FAILURE)
                            fail_unless(L4_IpcFailed(tag), "Message succesfully received from invalid thread id");
                        if (expect_result == SUCCESS)
                            fail_unless(L4_IpcSucceeded(tag), "Message unsuccesfully received from valid thread id");
                    }
                    break;
                }
                if ((syscall & OP_MASK) == AIPC_NOTIFY) {
                    if (L4_IsNilThread(waiting_tid))
                        waiting_tid = create_waiting_thread(waiting_notify_thread);
                    if (L4_IsThreadNotEqual(waiting_tid, tid)) 
                        L4_Notify(tid, 0x1);
                    break;
                }
                break;

            case SYSCALL_EXREGS:
                if ((syscall & OP_MASK) == EXREG_SP_OF) {
                    if (L4_IsNilThread(waiting_tid))
                        waiting_tid = create_waiting_thread(waiting_cancel_thread);
                    if (L4_IsThreadNotEqual(waiting_tid, tid)) {
                        res = L4_ExchangeRegisters(tid, L4_ExReg_Deliver,
                                                    0, 0, 0, 0, L4_nilthread,
                                                    &dummy, &dummy, &dummy, &dummy, &dummy);
                        if (expect_result == FAILURE)
                            fail_unless(res == 0, "Succesfully read stack pointer of invalid thread id");
                        if (expect_result == SUCCESS)
                            fail_unless(res, "Unsuccesfully read stack pointer of valid thread id");
                    }
                    break;
                }
                if ((syscall & OP_MASK) == EXREG_ABORT_IPC) {
                    if (L4_IsNilThread(waiting_tid))
                        waiting_tid = create_waiting_thread(waiting_cancel_thread);
                    if (L4_IsThreadNotEqual(waiting_tid, tid)) {
                        res = L4_ExchangeRegisters(tid, L4_ExReg_AbortIPC,
                                                    0, 0, 0, 0, L4_nilthread,
                                                    &dummy, &dummy, &dummy, &dummy, &dummy);
                        if (expect_result == FAILURE)
                            fail_unless(res == 0, "Succesfully cancelled IPC of invalid thread id");
                        if (expect_result == SUCCESS)
                            fail_unless(res, "Unsuccesfully cancelled IPC of valid thread id");
                    }
                    break;
                }
                break;

            case SYSCALL_SCHEDULE:
                res = L4_Set_Priority(tid, 10);
                if (expect_result == FAILURE)
                    fail_unless(res == 0, "Succesfully set priority of invalid thread id");
                if (expect_result == SUCCESS)
                    fail_unless(res, "Unsuccesfully set priority of valid thread id");
                break;

            case SYSCALL_THREADCONTROL:
                if ((syscall & OP_MASK) == THREAD_CREATE) {
#ifndef NO_UTCB_RELOCATE
                    L4_Word_t utcb = (L4_Word_t) (L4_PtrSize_t)L4_GetUtcbBase() + 16*UTCB_SIZE;
#else
                    L4_Word_t utcb = -1UL;
#endif
                    res = L4_ThreadControl(tid, KTEST_SPACE, KTEST_SERVER, KTEST_SERVER, KTEST_SERVER,
                         0, (void *)(L4_PtrSize_t)utcb);
                    if (expect_result == FAILURE) {
                        fail_unless(res == 0, "L4_ThreadControl() created an invalid thread id");
                        _fail_unless(L4_ErrorCode() == L4_ErrInvalidThread, __FILE__, __LINE__, "Wrong Error code: %lx", L4_ErrorCode());
                    }
                    if (expect_result == SUCCESS) {
                        fail_unless(res, "L4_ThreadControl() failed to create valid thread id");
                        L4_ThreadControl(tid, L4_nilspace, L4_nilthread, L4_nilthread, L4_nilthread, 0, (void *)0);
                    }
                    break;
                }
                if ((syscall & OP_MASK) == THREAD_DELETE) {
                    if (L4_IsNilThread(waiting_tid))
                        waiting_tid = create_waiting_thread(waiting_thread);
                    if (L4_IsThreadNotEqual(waiting_tid, tid)) {
                        res = L4_ThreadControl(tid, L4_nilspace, L4_nilthread, L4_nilthread, L4_nilthread, 0, (void *)0);
                        if (expect_result == FAILURE) {
                            fail_unless(res == 0, "L4_ThreadControl() deleted an invalid thread id");
                            fail_unless(L4_ErrorCode() == L4_ErrInvalidThread, "Wrong Error code");
                        }
                    }
                    break;
                }
                if ((syscall & OP_MASK) == THREAD_SET_PAGER) {
                    if (L4_IsNilThread(waiting_tid))
                        waiting_tid = create_waiting_thread(waiting_thread);
                    if (L4_IsThreadNotEqual(waiting_tid, tid)) {
                        res = L4_Set_PagerOf(tid, KTEST_SERVER);
                        if (expect_result == FAILURE) {
                            fail_unless(res == 0, "L4_ThreadControl() set pager of an invalid thread id");
                            fail_unless(L4_ErrorCode() == L4_ErrInvalidThread, "Wrong Error code");
                        }
                    }
                    break;
                }
                break;

            case SYSCALL_SPACESWITCH:
                if ((syscall & OP_MASK) == SPSW_INVALID_TID) {
                    if (L4_IsNilSpace(space))
                        space = createSpace();
#if defined(NO_UTCB_RELOCATE)
                    //switch tid to new space.
                    res = L4_SpaceSwitch(tid, space, (void *)(L4_PtrSize_t)-1UL);
#else
                    L4_ThreadId_t dummy_id;
                    //try to get utcb area of target address space
                    dummy_id = createThreadInSpace(space, get_utcb_target_thread);
                    L4_Receive(dummy_id);
                    deleteThread(dummy_id);
                    res = L4_SpaceSwitch(tid, space, (void *)(L4_PtrSize_t)utcb_target);
#endif
                    if (expect_result == FAILURE) {
                        fail_unless(res == 0, "Successfully switched invalid thread id to valid space");
                        fail_unless(L4_ErrorCode() == L4_ErrInvalidThread, "Wrong Error code");
                    }
                    break;
                }
                break;

            default:
                break;
        }
    }
    if (!L4_IsNilThread(waiting_tid))
        deleteThread(waiting_tid);
    if (!L4_IsNilSpace(space))
        deleteSpace(space);

    return 0;
}
