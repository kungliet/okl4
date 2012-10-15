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

#ifndef __OKL4__ENV_H__
#define __OKL4__ENV_H__

#include <okl4/types.h>

extern okl4_env_t *__okl4_environ;

/**
 *  @file
 *
 *  The env.h Header.
 *
 *  The env.h header provides the functionality required for obtaining
 *  predefined, pre-constructed and pre-initialized objects from Elfweaver as well
 *  as general metadata about the execution environment. The collection of
 *  objects and metadata obtained in this manner are referred to as the @a
 *  OKL4 object environment.
 *
 *  The OKL4 object environment behaves in a similar manner to a @a dictionary.  It is
 *  queried by providing strings as lookup @a keys.  The returned value is
 *  of arbitary type and must be cast appropriately prior to interpretation.
 *
 */

/** Magic header value for the OKL4 environment structure. */
#define OKL4_ENV_HEADER_MAGIC  0xb2a4UL

/**
 *  The okl4_envitem structure is used to represent a single object in
 *  the OKL4 object environment. It should be noted that this structure 
 *  is internal to the implementation of the OKL4 object environment.
 *
 */
struct okl4_envitem {
    char *name;
    void *item;
};

/**
 *  The okl4_env structure is used to represent the entire OKL4
 *  object environment. It should be noted that this structure 
 *  is internal to the implementation of the OKL4 object environment.
 *
 */
struct okl4_env {
    okl4_u16_t len;
    okl4_u16_t id;

     /* Variable sized Array of environment items. */
    okl4_envitem_t env_item[1];
};

/**
 *  The okl4_env_get() function is used to lookup an environment entry with
 *  the lookup key specified by the @a name argument. Note that @a name 
 *  should be specified in uppercase.
 *
 *  This function returns a pointer to the environment item or NULL if no
 *  environment item exists with the given @a name.
 *
 */
void *okl4_env_get(const char *name);

int _strcasecmp(const char *s1, const char *s2);

#include "env_types.h"

/**
 *  
 */
okl4_env_segment_t *okl4_env_get_segment(const char *name);

/**
 *  The okl4_env_get_args() function is used to retrieve the okl4_env_args_t
 *  structure in the environment, which is used to stroe command line arguments
 *  provided to the cell.
 *
 *  @param name
 *      The name of the environment variable.  This is typically
 *      "MAIN_ARGS".
 *
 *  @retval NULL
 *      This environment variable does not exist.
 *
 *  @retval others
 *      The pointer to a okl4_env_args_t structure.
 *
 */
okl4_env_args_t *okl4_env_get_args(const char *name);

/**
 *  Given a virtual address of the root server, return details about
 *  the physical memory associated with it.
 *
 *  This function requires the following arguments:
 *
 *  @param vaddr
 *      The virtual address to query.
 *
 *  @param phys
 *      The physical memory item associated with the virtual address.
 *
 *  @param offset
 *      The offset at which the virtal address is located in the 
 *      physical memory item.
 *
 *  @param attributes
 *      The memory attributes associated with the physical
 *      memory.
 *
 *  @param perms
 *      The memory permissions associated with the physical
 *      memory.
 *
 */
int okl4_env_get_physseg_page(okl4_word_t vaddr, okl4_physmem_item_t *phys,
        okl4_word_t *offset, okl4_word_t *attributes, okl4_word_t *perms);


#endif /* !__OKL4__ENV_H__ */
