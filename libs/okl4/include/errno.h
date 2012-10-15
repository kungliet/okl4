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

#ifndef __OKL4__ERRNO_H__
#define __OKL4__ERRNO_H__

#include <okl4/arch/config.h>
#include <okl4/types.h>

/**
 *  @file
 *
 *  The errno.h Header.
 *
 *  The errno.h header provides the definitions for error numbers used
 *  by the OKL4 Library.  Error numbers can be divided into the following
 *  categories:
 *
 *  @li Success code(s).
 *
 *  @li Generic error codes.
 *
 *  @li Kernel error codes that map to an equivalent error condition
 *  returned by the kernel.
 *
 *  It is important to note that these error conditions are not mutually
 *  exclusive. However, only one error condition is returned even when
 *  multiple error conditions are applicable.
 *
 */

/**
 *  The operation was successful.
 */
#define OKL4_OK               (0)

/**
 *  One or more arguments to the function were invalid. This may also occur
 *  when a combination of arguments were incompatible with each other.
 */
#define OKL4_INVALID_ARGUMENT (-2)

/**
 *  A blocking kernel operation was aborted by another thread.
 */
#define OKL4_CANCELLED        (-22)

/**
 *  An argument to the function was out of range for the given object. For
 *  instance, this error code is returned if a caller attempts to allocate a
 *  range of memory from a pool outside the range covered by that pool.
 */
#define OKL4_OUT_OF_RANGE     (-3)

/**
 *  The requested resource is already in use. For instance, this error code is
 *  returned if a caller attempts to allocate a specific range of memory from a
 *  resource pool that has already been allocated.
 */
#define OKL4_IN_USE           (-4)

/**
 *  The given allocator has insufficient resources to perform the requested
 *  allocation. This may be due to the allocator being completely exhausted of
 *  resources or as a result of internal fragmentation of range-based
 *  allocators.
 */
#define OKL4_ALLOC_EXHAUSTED  (-5)

/**
 *  The attempted operation could not be carried out, as it would require the
 *  current thread to block. This error may occur, for instance, if a caller
 *  attempts to perform a non-blocking mutex aquire or non-blocking message
 *  send.
 */
#define OKL4_WOULD_BLOCK        (-17)

/**
 *  Returned when the caller attempts to perform a protection domain operation
 *  on a virtual address that the protection domain has no mapping for. For
 *  instance, this error is returned when okl4_pd_handle_pagefault() is called
 *  on a virtual address not covered by the protection domain.
 */
#define OKL4_UNMAPPED           (-21)

/**
 *  Translate a kernel error code into a libokl4 equivalent error code.
 */
int _okl4_convert_kernel_errno(okl4_word_t errno);

#endif /* !__OKL4__ERRNO_H__ */
