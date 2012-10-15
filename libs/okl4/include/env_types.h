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

#ifndef __OKL4__ENV_TYPES_H__
#define __OKL4__ENV_TYPES_H__

/**
 *  @file
 *
 *  The env_types.h Header.
 *
 *  The okl4/env_types.h header provides the data type definitions used by the
 *  environment structures and macros for obtaining common environment
 *  entries.
 *
 */

/**
 *  Note that this structure is deprecated and should not be used.
 *
 *  This structure contains the following fields:
 *
 *  @param base
 *    The base physical address available to user-level code.
 *
 *  @param end
 *    The end physical address available to user-level code.
 *
 *  @param max_spaces
 *    The maximum number of spaces that can be created in the system,
 *    across all cells.
 *
 *  @param max_mutexes
 *    The maximum number of mutexes that can be created in the system,
 *    across all cells.
 *
 *  @param max_threads
 *    The maximum number of threads that can be created in the system,
 *    across all cells.
 *
 */
struct okl4_env_kernel_info {
    okl4_word_t base;
    okl4_word_t end;
    okl4_word_t max_spaces;
    okl4_word_t max_mutexes;
    okl4_word_t max_threads;
};


/**
 *  Note that this macro is deprecated and should not be used.
 *
 *  The OKL4_ENV_GET_KERNEL_INFO() macro is used to look up a kernel information entry 
 *  corresponding to the key @a name in the environment.
 *
 *  This macro returns a pointer to the kernel information structure or NULL if
 *  no environment item exists with the specified @a name.
 *
 */
#define OKL4_ENV_GET_KERNEL_INFO(name) \
    ((okl4_env_kernel_info_t *)okl4_env_get(name))

/**
 *  The okl4_env_segment structure describes a single physical
 *  segment available to a cell. It contains the following fields:
 *
 *  @param virt_addr
 *    Base virtual address at which the segment is mapped. If this is -1
 *    then the segment is not mapped.
 *
 *  @param segment
 *    Identifier of the kernel physical segment.
 *
 *  @param offset
 *    The base offset into the kernel physical segment.
 *
 *  @param size
 *    The size of the segment in bytes.
 *
 *  @param rights
 *    The access rights with which the segment is mapped.
 *
 *  @param cache_policy
 *    The caching policy for the segment.
 *
 */
struct okl4_env_segment {
    okl4_word_t virt_addr;
    okl4_word_t segment;
    okl4_word_t offset;
    okl4_word_t size;
    okl4_word_t rights;
    okl4_word_t cache_policy;
};

/**
 *  The okl4_env_segments structure describes all physical
 *  segments to which a cell has access. It contains the following fields:
 *
 *  @param num_segments
 *    The number of physical segments in the segment array.
 *
 *  @param segments
 *    An array of environment segment structures. Environment 
 *    segment structures are further described in Section okl4_env_segment. 
 *    Looking up a segment in the
 *    environment by name returns an index into this array.
 *
 */
struct okl4_env_segments {
    okl4_word_t num_segments;
    okl4_env_segment_t segments[1];
};

/**
 *  The OKL4_ENV_GET_SEGMENTS() macro returns a pointer to the
 *  environment segments structure corresponding to the environment key
 *  specified by the @a name argument.
 *
 */
#define OKL4_ENV_GET_SEGMENTS(name) \
    ((okl4_env_segments_t *)okl4_env_get(name))

/**
 *  The OKL4_ENV_GET_MAIN_SPACEID() macro returns a pointer to the space identifier
 *  of the main space in the cell, using @a name as the environment key.
 *  Each cell currently has an entry named MAIN_SPACE_ID.
 *
 */
#define OKL4_ENV_GET_MAIN_SPACEID(name) \
    ((okl4_word_t *)okl4_env_get(name))

/**
 *  The OKL4_ENV_GET_MAIN_CLISTID() macro returns a pointer to the clist identifier
 *  of the main clist in the cell, using @a name as the environment key.
 *  Each cell currently has an entry named MAIN_CLIST_ID.
 *
 */
#define OKL4_ENV_GET_MAIN_CLISTID(name) \
    ((okl4_word_t *)okl4_env_get(name))

/**
 *  The okl4_env_device_irqs structure contains a list of IRQs for a
 *  device. It contains the following fields:
 *
 *  @param num_irqs
 *    The number of IRQs associated with the device.
 *
 *  @param irqs
 *    An array of all IRQs belonging to the device.
 *
 *  The naming convention for environment entries is @a $<$name of
 *  device$>$_DEV_IRQ_LIST. For example, the entry for the serial device
 *  is SERIAL_DEV_IRQ_LIST.
 *
 */
struct okl4_env_device_irqs {
    okl4_word_t num_irqs;
    /*
     * FIXME: irqs is actually a variable-sized array.  To ensure that memory
     * is allocated for this array and that array semantics are preserved, it
     * is declared as size 1.  Now ADS is very "nice" in that it checks
     * accesses to this array when the index can be statically determined, and
     * when the access is out of bounds, ADS complains.  In particular
     * l4test/src/l4test.c:init_boot_params() does this very thing.
     *
     * The most straightforward solution for now is increasing the declared
     * size of this array.  Other attempts include casting, suppressing the
     * said warning, zero-sized array, and abstraction using a function call.
     *
     * Mothra issue #3011, filed by nelson.
     */
    okl4_word_t irqs[2];
};

/**
 *  @struct okl4_env_args_t
 *
 *  The okl4_env_args_t structure stores command line arguments that are
 *  provided to a cell's execution environment.  This structure is
 *  typically named "MAIN_ARGS" in the environment.
 */
struct okl4_env_args {
    int argc;
    char *argv;
};

#define OKL4_ENV_GET_ARGS(name) \
    ((okl4_env_args_t *)okl4_env_get(name))

#endif /*  __OKL4__ENV_TYPES_H__ */
