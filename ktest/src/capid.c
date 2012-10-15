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
#include <stdio.h>
#include <string.h>

/* Calculate how many bits should be in each field. */

#if defined(L4_64BIT)
# define TYPE_BITS    4
# define INDEX_BITS   60 
#else
# define TYPE_BITS    4
# define INDEX_BITS   28
#endif


/*
 * Define a random thread and version number.  We don't really care
 * about the value so long as it is non-zero.
 */
#define RANDOM_INDEX_NO 9
#define RANDOM_TYPE     8

/*
\begin{test}{capid0100}
  \TestDescription{Verify values for constant capids nilthread, anythread, waitnotify, myselfconst}
  \TestFunctionalityTested{Cap ID constants}
  \TestImplementationProcess{
    \begin{enumerate}
      \item Check that L4\_nilthread is all-bits 0
      \item Check that L4\_anythread is all-bits 1
      \item Check that L4\_waitnotify is -2L
      \item Check that L4\_myselfconst is -3L
    \end{enumerate}
  }
  \TestImplementationStatus{Implemented}
  \TestIsFullyAutomated{Yes}
  \TestRegressionStatus{In regression test suite}
\end{test}
*/
START_TEST(capid0100)
{
    L4_CapId_t tmp;
    char bit_buf[sizeof(L4_CapId_t)];

    /* Check the values of the constants. */
    fail_unless(L4_nilthread.raw == -4L, "bad L4_nilthread.");
    fail_unless(L4_nilthread.X.type == TYPE_SPECIAL, "L4_nilthread not TYPE_SPECIAL");

    memset(bit_buf, '\xff', sizeof(bit_buf));
    tmp = L4_anythread;
    fail_unless(memcmp (&tmp, bit_buf, sizeof(bit_buf)) == 0,
                 "L4_anythread isn't all-bits 1.");
    fail_unless(L4_anythread.X.type == TYPE_SPECIAL, "L4_anythread not TYPE_SPECIAL");

    fail_unless(L4_waitnotify.raw == -2L,
                 "L4_waitnotify not -2.");
    fail_unless(L4_waitnotify.X.type == TYPE_SPECIAL, "L4_waitnotify not TYPE_SPECIAL");

    fail_unless(L4_myselfconst.raw == -3L,
                 "L4_myselfconst not -3.");
    fail_unless(L4_myselfconst.X.type == TYPE_SPECIAL, "L4_myselfconst not TYPE_SPECIAL");
}
END_TEST

/*
\begin{test}{capid0200}
  \TestDescription{Test CapId with simple arguments}
  \TestFunctionalityTested{\Func{CapId}}
  \TestImplementationProcess{
    \begin{enumerate}
      \item Initialise a CapId using \Func{CapId} using arguments (0xF, 0)
      \item Check that the returned capid is a nilthread
      \item Initialise a CapId using \Func{CapId} using arguments (0, 1)
      \item Check that its index and type are correct 
      \item Initialise a CapId using \Func{CapId} using arguments (3, 4)
      \item Check that its index and type are correct 
    \end{enumerate}
  }
  \TestImplementationStatus{Implemented}
  \TestIsFullyAutomated{Yes}
  \TestRegressionStatus{In regression test suite}
\end{test}
*/
START_TEST(capid0200)
{
    L4_CapId_t id;

    id = L4_CapId(0xF, -4);
    fail_unless(id.raw == L4_nilthread.raw,
                 "(0xF, -4) not a nilthread.");

    id = L4_CapId(0, 1);
    fail_unless(id.X.index == 1, "Cap index not set.");
    fail_unless(id.X.type == 0, "Cap type not set.");

    id = L4_CapId(3, 4);
    fail_unless(id.X.index == 4, "Cap index not set.");
    fail_unless(id.X.type == 3, "Cap type not set.");

    /* Boundary cases really checked in other tests. */
}
END_TEST

/*
\begin{test}{capid0300}
  \TestDescription{Test CapId correctly truncates incoming index argument}
  \TestFunctionalityTested{\Func{CapId}}
  \TestImplementationProcess{
    \begin{enumerate}
      \item Invoke \Func{CapId} with index argument smaller than largest possible value, and again with largest possible value
      \item Check that all bits are retained
      \item Invoke \Func{CapId} with index argument larger than largest possible value
      \item Check that these extra bits are duly discarded
    \end{enumerate}
  }
  \TestImplementationStatus{Implemented}
  \TestIsFullyAutomated{Yes}
  \TestRegressionStatus{In regression test suite}
\end{test}
*/
START_TEST(capid0300)
{
    L4_CapId_t id;

    id = L4_CapId(RANDOM_TYPE, ((L4_Word_t)1 << INDEX_BITS) - 2);
    fail_unless(L4_CapIndex(id) == ((L4_Word_t)1 << INDEX_BITS) - 2,
                 "Cap Id unexpectedly changed.");

    id = L4_CapId(RANDOM_TYPE, ((L4_Word_t)1 << INDEX_BITS) - 1);
    fail_unless(L4_CapIndex(id) == ((L4_Word_t)1 << INDEX_BITS) - 1,
                 "Cap Id unexpectedly changed.");

    id = L4_CapId(RANDOM_TYPE, ((L4_Word_t)1 << INDEX_BITS));
    fail_unless(L4_CapType(id) == RANDOM_TYPE,
                 "Cap Id affected by truncation.");

    id = L4_CapId(RANDOM_TYPE, ((L4_Word_t)1 << INDEX_BITS) + 1);
    fail_unless(L4_CapType(id) == RANDOM_TYPE,
                 "Cap Id affected by truncation.");
}
END_TEST

/*
\begin{test}{capid0400}
  \TestDescription{Test CapId correctly truncates incoming type argument}
  \TestFunctionalityTested{\Func{CapId}}
  \TestImplementationProcess{
    \begin{enumerate}
      \item Invoke \Func{CapId} with type argument 1 smaller than largest possible value, and again with largest possible value
      \item Check that all bits are retained
      \item Invoke \Func{CapId} with type argument larger than largest possible value
      \item Check that these extra bits are duly discarded
    \end{enumerate}
  }
  \TestImplementationStatus{Implemented}
  \TestIsFullyAutomated{Yes}
  \TestRegressionStatus{In regression test suite}
\end{test}
*/
START_TEST(capid0400)
{
    L4_CapId_t id;

    id = L4_CapId (((L4_Word_t)1 << TYPE_BITS) - 2, RANDOM_INDEX_NO);
    fail_unless(L4_CapType(id) == ((L4_Word_t)1 << TYPE_BITS) - 2,
                 "Cap Id unexpectedly changed.");

    id = L4_CapId (((L4_Word_t)1 << TYPE_BITS) - 1, RANDOM_INDEX_NO);
    fail_unless(L4_CapType(id) == ((L4_Word_t)1 << TYPE_BITS) - 1,
                 "Cap Id unexpectedly changed.");

    id = L4_CapId (((L4_Word_t)1 << TYPE_BITS), RANDOM_INDEX_NO);
    fail_unless(L4_CapIndex(id) == RANDOM_INDEX_NO,
                 "Cap Id affected by truncation.");

    id = L4_CapId (((L4_Word_t)1 << TYPE_BITS) + 1, RANDOM_INDEX_NO);
    fail_unless(L4_CapIndex(id) == RANDOM_INDEX_NO,
                 "Cap Id affected by truncation.");
}
END_TEST

/*
\begin{test}{capid0500}
  \TestDescription{Verify correct functionality of L4\_CapIndex}
  \TestFunctionalityTested{\Func{CapIndex}}
  \TestImplementationProcess{
    \begin{enumerate}
      \item Invoke \Func{CapId} for multiple valid index arguments
      \item Check that the value obtained by running L4\_CapIndex match the original inputs
    \end{enumerate}
  }
  \TestImplementationStatus{Implemented}
  \TestIsFullyAutomated{Yes}
  \TestRegressionStatus{In regression test suite}
\end{test}
*/
START_TEST(capid0500)
{
    int i;

    for (i = 0; i < INDEX_BITS; i++) {
        L4_CapId_t id = L4_CapId (0, (1UL << i));

        _fail_unless(L4_CapIndex(id) == (1UL << i), __FILE__, __LINE__,
                     "Unexpected cap id (expected 0x%lx, got 0x%lx).",
                     (1UL << i), L4_CapIndex(id));
    }
}
END_TEST

/*
\begin{test}{capid0600}
  \TestDescription{Verify correct functionality of L4\_CapType}
  \TestFunctionalityTested{\Func{CapType}}
  \TestImplementationProcess{
    \begin{enumerate}
      \item Invoke \Func{CapId} for multiple valid type arguments
      \item Check that the value obtained by running L4\_CapType match the original inputs
    \end{enumerate}
  }
  \TestImplementationStatus{Implemented}
  \TestIsFullyAutomated{Yes}
  \TestRegressionStatus{In regression test suite}
\end{test}
*/
START_TEST(capid0600)
{
    int i;

    for (i = 0; i < TYPE_BITS; i++) {
        L4_CapId_t id = L4_CapId ((1UL << i), 1);

        _fail_unless(L4_CapType(id) == (1UL << i), __FILE__, __LINE__,
                     "Unexpected cap id (expected 0x%lx, got 0x%lx).",
                     (1UL << i), L4_CapType(id));
    }
}
END_TEST

/*
\begin{test}{capid0700}
  \TestDescription{Verify correct functionality of IsCapEqual}
  \TestFunctionalityTested{\Func{IsCapEqual}}
  \TestImplementationProcess{
    \begin{enumerate}
      \item Check constant capids (nilthread, anythread, etc) equal themselves
      \item Check constant capids do not equal each other
      \item Check normal capid matches itself, and not other normal capids with a different index
      \item Check normal capid does not equal constant capid anythread
    \end{enumerate}
  }
  \TestImplementationStatus{Implemented}
  \TestIsFullyAutomated{Yes}
  \TestRegressionStatus{In regression test suite}
\end{test}
*/
START_TEST(capid0700)
{
    L4_CapId_t id1 = L4_CapId (0, 20);

    fail_unless(L4_IsCapEqual (L4_nilthread, L4_nilthread),
                 "nilthread doesn't equal itself");
    fail_unless(L4_IsCapEqual (L4_anythread, L4_anythread),
                 "anythread doesn't equal itself");
    fail_unless(L4_IsCapEqual (L4_waitnotify, L4_waitnotify),
                 "waitnotify doesn't equal itself");

    fail_unless(!L4_IsCapEqual (L4_nilthread, L4_anythread),
                 "nilthread equals anythread");
    fail_unless(!L4_IsCapEqual (L4_waitnotify, L4_nilthread),
                 "waitnotify equals nilthread");

    fail_unless(L4_IsCapEqual (id1, L4_CapId (0, 20)),
                 "Normal Id doesn't equal itself.");

    fail_unless(!L4_IsCapEqual (id1, L4_anythread),
                 "Anythread matches normal cap");
}
END_TEST

/*
\begin{test}{capid0800}
  \TestDescription{Verify correct functionality of IsCapNotEqual}
  \TestFunctionalityTested{\Func{IsCapNotEqual}}
  \TestImplementationProcess{
    \begin{enumerate}
      \item Check constant capids (nilthread, anythread, etc) against themselves
      \item Check constant capids against each other
      \item Check normal capid against itself, and against other normal capids with differing index
      \item Check normal capid does not equal constant capid anythread
    \end{enumerate}
  }
  \TestImplementationStatus{Implemented}
  \TestIsFullyAutomated{Yes}
  \TestRegressionStatus{In regression test suite}
\end{test}
*/
START_TEST(capid0800)
{
    L4_CapId_t id1 = L4_CapId(0, 20);

    fail_unless(!L4_IsCapNotEqual (L4_nilthread, L4_nilthread),
                 "nilthread doesn't equal itself");
    fail_unless(!L4_IsCapNotEqual (L4_anythread, L4_anythread),
                 "anythread doesn't equal itself");
    fail_unless(!L4_IsCapNotEqual (L4_waitnotify, L4_waitnotify),
                 "waitnotify doesn't equal itself");

    fail_unless(L4_IsCapNotEqual (L4_nilthread, L4_anythread),
                 "nilthread equals anythread");
    fail_unless(L4_IsCapNotEqual (L4_waitnotify, L4_nilthread),
                 "waitnotify equals nilthread");

    fail_unless(!L4_IsCapNotEqual (id1, L4_CapId(0, 20)),
                 "Normal Id doesn't equal itself.");
    fail_unless(L4_IsCapNotEqual (id1, L4_CapId(0, 50)),
                 "Different threads equal");

    fail_unless(L4_IsCapNotEqual (id1, L4_anythread),
                 "Anythread matches normal thread");
}
END_TEST

/*
\begin{test}{capid0900}
  \TestDescription{Verify correct functionality of IsNilThread}
  \TestFunctionalityTested{\Func{IsNilThread}}
  \TestImplementationProcess{
    \begin{enumerate}
      \item Check constant capid nilthread
      \item Check none of the other constant capids are a nilthread
      \item Check none of Myself(), MyGlobalId() or a normal capid are a nilthread
      \item Check CapId(0, 0) is a nilthread
    \end{enumerate}
  }
  \TestImplementationStatus{Implemented}
  \TestIsFullyAutomated{Yes}
  \TestRegressionStatus{In regression test suite}
\end{test}
*/
START_TEST(capid0900)
{
    fail_unless(L4_IsNilThread (L4_nilthread),
                 "nilthread isn't.");
    fail_unless(!L4_IsNilThread (L4_anythread),
                 "anythread is a nilthread.");
    fail_unless(!L4_IsNilThread (L4_waitnotify),
                 "waitnotify is a nilthread");
    fail_unless(!L4_IsNilThread (L4_Myself()),
                 "Myself() is a nilthread");
    fail_unless(!L4_IsNilThread (L4_MyGlobalId()),
                 "MyGlobalId() is a nilthread");
    fail_unless(!L4_IsNilThread (L4_GlobalId (10, 1)),
                 "GlobalId(10, 1) is a nilthread");
    fail_unless(L4_IsNilThread (L4_SpecialCapId(~3)),
                 "SpecialCapId(0~3 isn't a nilthread");

}
END_TEST

static void test_setup(void)
{
    initThreads(0);
}


static void test_teardown(void)
{
}

TCase *
make_cap_id_tcase(void)
{
    TCase *tc;

    initThreads(0);

    tc = tcase_create("Cap Id Tests");
    tcase_add_checked_fixture(tc, test_setup, test_teardown);

    tcase_add_test(tc, capid0100);
    tcase_add_test(tc, capid0200);
    tcase_add_test(tc, capid0300);
    tcase_add_test(tc, capid0400);
    tcase_add_test(tc, capid0500);
    tcase_add_test(tc, capid0600);
    tcase_add_test(tc, capid0700);
    tcase_add_test(tc, capid0800);
    tcase_add_test(tc, capid0900);

    return tc;
}
