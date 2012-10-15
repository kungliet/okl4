/*
 * Copyright (c) 2006, National ICT Australia
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

#include <ktest/ktest.h>
#include <ktest/utility.h>
#include <stddef.h>
#include <l4/ipc.h>
#include <l4/config.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ktest/bad_capids.h>

static L4_ThreadId_t main_thread;

#ifdef ARCH_IA32
/*
 * This magic number is here to skip the checking of the whole utcb,
 * as the beginning of the utcb is changed to pass arguments to the syscall.
 */
#define __L4_TCR_SYSCALL_ARGS_OFFSET ((64+(__L4_TCR_SYSCALL_ARGS+3))*4) 
#endif

/*
\begin{test}{tcr0300}
  \TestDescription{Verify that user defined handles work correctly}
  \TestFunctionalityTested{\Func{UserDefinedHandle}}
  \TestImplementationProcess{
    \begin{enumerate}
      \item Make a copy of the UTCB and user defined handle
      \item Assign values to the handle using \Func{Set\_UserDefinedHandle}
      \item Read back values using \Func{UserDefinedHandle} and check they are the same
      \item Restore the original handle and verify that the rest of the UTCB is unchanged
    \end{enumerate}
  }
  \TestImplementationStatus{Implemented}
  \TestIsFullyAutomated{Yes}
  \TestRegressionStatus{In regression test suite}
\end{test}
*/
START_TEST(tcr0300)
{
    L4_Word_t utcb_size = L4_GetUtcbSize();
    char *utcb_copy;
    L4_Word_t* user_handle = &(get_arch_utcb_base ())[ __L4_TCR_USER_DEFINED_HANDLE];
    L4_Word_t orig = L4_UserDefinedHandle ();
    L4_Word_t new;

    utcb_copy = malloc(utcb_size);

    /* Copy the UTCB for later comparison. */
    memcpy (utcb_copy, L4_GetUtcbBase (), utcb_size);

    /*
     * Assign some random values as the user handle and see if
     * they can be read back.
     */
    new = 0;
    L4_Set_UserDefinedHandle (new);
    fail_unless (*user_handle == new, "User Handle not set.");
    fail_unless (new == L4_UserDefinedHandle (),
                 "Set handle not fetched back.");
    new = -1;
    L4_Set_UserDefinedHandle (new);
    fail_unless (*user_handle == new, "User Handle not set.");
    fail_unless (new == L4_UserDefinedHandle (),
                 "Set handle not fetched back.");

    L4_Set_UserDefinedHandle (orig);

    /* UTCB should be unchanged. */
    fail_unless(memcmp (utcb_copy, L4_GetUtcbBase (), utcb_size) == 0,
                "UTCB changed during call.");
    free(utcb_copy);
}
END_TEST

static L4_ThreadId_t partner;

static void
pager_thread(void)
{
    L4_MsgTag_t tag;
    L4_Msg_t msg;
    L4_ThreadId_t client;
    L4_Word_t label;
    long fault;

    while(1)
    {
        tag = L4_Wait(&client);

        L4_MsgStore(tag, &msg);

        if(!L4_IpcSucceeded(tag)) {
            printf("Was receiving an IPC which was cancelled?\n");
            continue;
        }

        label = L4_MsgLabel(&msg);
        /* get sign-extended (label>>4) */
        fault = (((long)(label<<16)) >> 20);
        fail_unless(fault == -2, "Pager received unknown message");
        L4_Set_MsgTag(L4_Niltag);
        L4_Call(partner);

        L4_MsgClear(&msg);
        L4_MsgLoad(&msg);
        L4_Reply(client);
    }
}

static void 
faulting_thread(void)
{
    L4_Set_MsgTag(L4_Niltag);
    L4_Send(main_thread);

    // Cause a null pointer dereference
    *((int *)0) = 1;

    fail("Should not reach");
    for (;;);
}

/*
\begin{test}{tcr0400}
  \TestDescription{Verify that a thread's pager and exception_handler
                   can only be set to sane values}
  \TestFunctionalityTested{\Func{Set\_PagerOf}}
  \TestImplementationProcess{
    \begin{enumerate}
      \item Create a new thread
      \item Create a new pager and set it to be the pager of the new thread
      \item Also set pager as exception handler (to confirm call works)
      \item Let new thread pagefault and ensure pager receives the pagefault IPC
      \item Try setting the pager to nilthread and check pager still receives pagefault IPC
      \item Try setting exception handler to nilthread.
      \item Try setting the pager to an invalid thread, confirm error value and check pager still receives pagefault IPC
      \item Try setting exception handler to an invalid thread and confirm error value.
    \end{enumerate}
  }
  \TestImplementationStatus{Implemented}
  \TestIsFullyAutomated{Yes}
  \TestRegressionStatus{In regression test suite}
\end{test}
*/
START_TEST(tcr0400)
{
    L4_ThreadId_t pager, faulting_tid, new;
    L4_CapId_t new_cap;
    word_t r;
    
    partner = main_thread;
    pager = createThreadInSpace(KTEST_SPACE, pager_thread);
    faulting_tid = createThread(faulting_thread);
    L4_Set_MsgTag(L4_Niltag);
    L4_Receive(faulting_tid);
    r = L4_Set_PagerOf(faulting_tid, pager);
    fail_unless(r == 1, "Can't set pager");

    /*
     * Note: Exception handler not used in test, so setting to pager shouldn't
     * cause problems.
     */
    r = L4_Set_ExceptionHandlerOf(faulting_tid, pager);  
    fail_unless(r == 1, "Can't set exception handler");
    
    L4_Set_MsgTag(L4_Niltag);
    L4_Receive(pager);
    /*
     * Assign some random values as the pager and see if they
     * change the pager.
     */
    new = L4_nilthread;
    r = L4_Set_PagerOf(faulting_tid, new);
    fail_unless(r == 1, "Setting nilthread shouldn't give error\n");
    r = L4_Set_ExceptionHandlerOf(faulting_tid, new);
    fail_unless(r == 1, "Setting nilthread shouldn't give error\n");
    
    L4_Set_MsgTag(L4_Niltag);
    L4_Call(pager);

    new_cap = L4_CapId(0x3, 5);
    new = new_cap;
    r = L4_Set_PagerOf(faulting_tid, new);
    fail_unless(r == 0, "Setting garbage pager didn't return error");
    fail_unless(L4_ErrorCode() == L4_ErrInvalidParam,
                "Setting garbage pager gave wrong error val");
    
    r = L4_Set_ExceptionHandlerOf(faulting_tid, new);
    fail_unless(r == 0, "Setting garbage exception handler didn't return error");
    fail_unless(L4_ErrorCode() == L4_ErrInvalidParam,
                "Setting garbage exception handler gave wrong error val");
    
    L4_Set_MsgTag(L4_Niltag);
    L4_Call(pager);

    deleteThread(faulting_tid);
    deleteThread(pager);
}
END_TEST

static L4_ThreadId_t pager, faulting_tid;

static void unpriv_thread(void)
{
    L4_ThreadId_t new;
    L4_CapId_t new_cap;

    L4_Set_MsgTag(L4_Niltag);
    L4_Receive(pager);
    /*
     * Assign some random values as the pager and see if they
     * change the pager.
     */
    new = L4_nilthread;
    L4_Set_PagerOf(faulting_tid, new);
    L4_Set_MsgTag(L4_Niltag);
    L4_Call(pager);

    new_cap = L4_CapId(0x3, 5);
    new = new_cap;
    L4_Set_PagerOf(faulting_tid, new);
    L4_Set_MsgTag(L4_Niltag);
    L4_Call(pager);

    new = L4_GlobalId(5, 1);
    L4_Set_PagerOf(faulting_tid, new);
    L4_Set_MsgTag(L4_Niltag);
    L4_Call(pager);

    L4_Set_MsgTag(L4_Niltag);
    L4_Call(main_thread);
    for (;;);
}

/*
\begin{test}{tcr0500}
  \TestDescription{Verify that an unprivileged thread cannot set another thread's pager}
  \TestFunctionalityTested{\Func{Set\_PagerOf}}
  \TestImplementationProcess{
    \begin{enumerate}
      \item Create a new thread (faulting\_thread)
      \item Create a new pager and set it to be the pager of the faulting\_thread
      \item Create a new thread (unpriv\_thread) in a new unprivileged address space
      \item Let faulting\_thread pagefault and ensure pager receives the pagefault IPC
      \item When pager receives pagefault IPC it sends an IPC message to unpriv\_thread
      \item unpriv\_thread tries to set the faulting\_thread's pager to nilthread and check pager still receives pagefault IPC
      \item unpriv\_thread tries to set the faulting\_thread's pager to an invalid value and check pager still receives pagefault IPC
      \item unpriv\_thread tries to set the faulting\_thread's pager to a valid value and check pager still receives pagefault IPC
      \item unpriv\_thread sends an IPC message to the main thread
      \item Receive the IPC and delete all the threads
    \end{enumerate}
  }
  \TestImplementationStatus{Implemented}
  \TestIsFullyAutomated{Yes}
  \TestRegressionStatus{In regression test suite}
\end{test}
*/
START_TEST(tcr0500)
{
    L4_ThreadId_t unpriv;
    L4_MsgTag_t tag;

    pager = createThreadInSpace(KTEST_SPACE, pager_thread);
    faulting_tid = createThread(faulting_thread);
    unpriv = createThreadInSpace(L4_nilspace, unpriv_thread);
    partner = unpriv;

    L4_Set_MsgTag(L4_Niltag);
    L4_Receive(faulting_tid);
    L4_Set_PagerOf(faulting_tid, pager);
    L4_Set_MsgTag(L4_Niltag);
    tag = L4_Receive(unpriv);
    fail_unless(L4_IpcSucceeded (tag), "ipc failed");

    deleteThread(unpriv);
    deleteThread(faulting_tid);
    deleteThread(pager);
}
END_TEST

/*
\begin{test}{tcr0600}
  \TestDescription{Verify that attempts to create invalid tids fail}
  \TestFunctionalityTested{\Func{ThreadControl}}
  \TestImplementationProcess{
    \begin{enumerate}
      \item Attempt to create tids greater than max supported by invoking \Func{ThreadControl}
      \item Verify that they fail and returns the correct error code
    \end{enumerate}
  }
  \TestImplementationStatus{Implemented}
  \TestIsFullyAutomated{Yes}
  \TestRegressionStatus{In regression test suite}
\end{test}
*/
START_TEST(tcr0600)
{
    L4_CapId_t idx_start, idx_end;
    L4_Word_t type;

    for (type = TYPE_CAP; type <= TYPE_SPECIAL; type++) {
        idx_start = L4_CapId(type, kernel_max_threads + 1); // Exclude root, test handler and main test threads from the list of thread ids
        idx_end = L4_CapId(type, kernel_max_threads + 100);
        run_bad_capids_tests(SYSCALL_THREADCONTROL | THREAD_CREATE, idx_start, idx_end, 0);
    }
}
END_TEST

/*
\begin{test}{tcr0601}
  \TestDescription{Verify that attempts to create invalid tids fail}
  \TestFunctionalityTested{\Func{ThreadControl}}
  \TestImplementationProcess{
    \begin{enumerate}
      \item Attempt to create negative tids by invoking \Func{ThreadControl}
      \item Verify that they fail and returns the correct error code
    \end{enumerate}
  }
  \TestImplementationStatus{Implemented}
  \TestIsFullyAutomated{Yes}
  \TestRegressionStatus{In regression test suite}
\end{test}
*/
START_TEST(tcr0601)
{
    L4_CapId_t idx_start, idx_end;
    L4_Word_t type;
    L4_Word_t max_threads = 200;

    idx_start = L4_CapId(TYPE_CAP, -kernel_max_threads); // Exclude root, test handler and main test threads from the list of thread ids
    idx_end = L4_CapId(TYPE_CAP, -1);
    run_bad_capids_tests(SYSCALL_THREADCONTROL | THREAD_CREATE, idx_start, idx_end, 0);

    if (kernel_max_threads < 200) {
        max_threads = kernel_max_threads;
    }
    for (type = 0x1; type <= TYPE_SPECIAL; type++) {
        idx_start = L4_CapId(type, -max_threads);
        idx_end = L4_CapId(type, -1);
        run_bad_capids_tests(SYSCALL_THREADCONTROL | THREAD_CREATE, idx_start, idx_end, 0);
    }
}
END_TEST

/*
\begin{test}{tcr0602}
  \TestDescription{Verify that attempts to create invalid tids fail}
  \TestFunctionalityTested{\Func{ThreadControl}}
  \TestImplementationProcess{
    \begin{enumerate}
      \item Attempt to create tids with valid indexes but bad types by invoking \Func{ThreadControl}
      \item Verify that they fail and returns the correct error code
    \end{enumerate}
  }
  \TestImplementationStatus{Implemented}
  \TestIsFullyAutomated{Yes}
  \TestRegressionStatus{In regression test suite}
\end{test}
*/
START_TEST(tcr0602)
{
    L4_CapId_t idx_start, idx_end;
    L4_Word_t type;
    L4_Word_t max_threads = 200;

    if (kernel_max_threads < 200) {
        max_threads = kernel_max_threads;
    }
    for (type = 0x1; type <= TYPE_SPECIAL; type++) {
        idx_start = L4_CapId(type, 0);
        idx_end = L4_CapId(type, max_threads);
        run_bad_capids_tests(SYSCALL_THREADCONTROL | THREAD_CREATE, idx_start, idx_end, 0);
    }
}
END_TEST

/*
\begin{test}{tcr0603}
  \TestDescription{Verify that attempts to create valid tids works}
  \TestFunctionalityTested{\Func{ThreadControl}}
  \TestImplementationProcess{
    \begin{enumerate}
      \item Attempt to create valid tids by invoking \Func{ThreadControl}
      \item Verify that they succeed
    \end{enumerate}
  }
  \TestImplementationStatus{Implemented}
  \TestIsFullyAutomated{Yes}
  \TestRegressionStatus{In regression test suite}
\end{test}
*/
START_TEST(tcr0603)
{
    L4_CapId_t idx_start, idx_end;
    L4_Word_t max_threads = 200;

    if (kernel_max_threads < 200) {
        max_threads = kernel_max_threads;
    }
    // Exclude root, test handler and main test threads from the list of thread ids
    idx_start = L4_CapId(TYPE_CAP, L4_ThreadNo(KTEST_SERVER) + 4);
    idx_end = L4_CapId(TYPE_CAP, max_threads);
    run_bad_capids_tests(SYSCALL_THREADCONTROL | THREAD_CREATE, idx_start, idx_end, 1);
}
END_TEST

/*
\begin{test}{tcr0604}
  \TestDescription{Verify that attempts to delete invalid tids fail}
  \TestFunctionalityTested{\Func{ThreadControl}}
  \TestImplementationProcess{
    \begin{enumerate}
      \item Create a new waiting thread
      \item Attempt to delete known invalid tids by invoking \Func{ThreadControl}
      \item Verify that they fail and returns the correct error code, and that no attempts are received by the waiting thread
    \end{enumerate}
  }
  \TestImplementationStatus{Implemented}
  \TestIsFullyAutomated{Yes}
  \TestRegressionStatus{In regression test suite}
\end{test}
*/
START_TEST(tcr0604)
{
    L4_CapId_t idx_start, idx_end;
    L4_Word_t type;
    L4_Word_t max_threads = 200;

    // Exclude root, test handler and main test threads from the list of thread ids
    idx_start = L4_CapId(TYPE_CAP, L4_ThreadNo(KTEST_SERVER) + 4);
    idx_end = L4_CapId(TYPE_CAP, kernel_max_threads);
    run_bad_capids_tests(SYSCALL_THREADCONTROL | THREAD_DELETE, idx_start, idx_end, 0);

    if (kernel_max_threads < 200) {
        max_threads = kernel_max_threads;
    }
    for (type = 0x1; type <= TYPE_SPECIAL; type++) {
        idx_start = L4_CapId(type, 0);
        idx_end = L4_CapId(type, max_threads);
        run_bad_capids_tests(SYSCALL_THREADCONTROL | THREAD_DELETE, idx_start, idx_end, 0);
    }
}
END_TEST

/*
\begin{test}{tcr0605}
  \TestDescription{Verify that attempts to set pager of invalid tids fails}
  \TestFunctionalityTested{\Func{ThreadControl}}
  \TestImplementationProcess{
    \begin{enumerate}
      \item Create a new waiting thread
      \item Attempt to delete known invalid tids by invoking \Func{ThreadControl}
      \item Verify that they fail and returns the correct error code, and that no attempts are received by the waiting thread
    \end{enumerate}
  }
  \TestImplementationStatus{Implemented}
  \TestIsFullyAutomated{Yes}
  \TestRegressionStatus{In regression test suite}
\end{test}
*/
START_TEST(tcr0605)
{
    L4_CapId_t idx_start, idx_end;
    L4_Word_t type;
    L4_Word_t max_threads = 200;

    // Exclude root, test handler and main test threads from the list of thread ids
    idx_start = L4_CapId(TYPE_CAP, L4_ThreadNo(KTEST_SERVER) + 4);
    idx_end = L4_CapId(TYPE_CAP, kernel_max_threads);
    run_bad_capids_tests(SYSCALL_THREADCONTROL | THREAD_SET_PAGER, idx_start, idx_end, 0);

    if (kernel_max_threads < 200) {
        max_threads = kernel_max_threads;
    }
    for (type = 0x1; type <= TYPE_SPECIAL; type++) {
        idx_start = L4_CapId(type, 0);
        idx_end = L4_CapId(type, max_threads);
        run_bad_capids_tests(SYSCALL_THREADCONTROL | THREAD_SET_PAGER, idx_start, idx_end, 0);
    }
}
END_TEST

extern L4_ThreadId_t test_tid;

static void test_setup(void)
{
    initThreads(0);
    main_thread = test_tid;
}

static void test_teardown(void)
{
}

TCase *
make_threadcontrol_tcase(void)
{
    TCase *tc;

    initThreads(0);

    tc = tcase_create("Thread Control Register Tests");
    tcase_add_checked_fixture(tc, test_setup, test_teardown);

    /* Not implemmented pending SMP hardware. */
    tcase_add_test(tc, tcr0300);
    tcase_add_test(tc, tcr0400);
    tcase_add_test(tc, tcr0500);
    tcase_add_test(tc, tcr0600);
    tcase_add_test(tc, tcr0601);
    tcase_add_test(tc, tcr0602);
    tcase_add_test(tc, tcr0603);
    tcase_add_test(tc, tcr0604);
    tcase_add_test(tc, tcr0605);

    return tc;
}
