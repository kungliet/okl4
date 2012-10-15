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

#ifndef __OKL4__EXTENSION_H__
#define __OKL4__EXTENSION_H__

#include <assert.h>

#include <okl4/arch/config.h>

#include <l4/types.h>
#include <l4/config.h>
#include <l4/misc.h>

#include <okl4/errno.h>
#include <okl4/types.h>
#include <okl4/physmem_item.h>
#include <okl4/virtmem_pool.h>
#include <okl4/pd.h>
#include <okl4/kspace.h>

/**
 * Minimum alignment for extension virtual addresses and extension
 * sizes.
 */
#ifdef ARM_SHARED_DOMAINS
#define OKL4_EXTENSION_ALIGNMENT 0x100000
#else
#define OKL4_EXTENSION_ALIGNMENT OKL4_DEFAULT_PAGESIZE
#endif

#include <okl4/extension_attr.h>

/**
 *  @file
 *
 *  The extension.h Header.
 *
 *  The extension.h header provides the functionality required for 
 *  using protection domain extensions, otherwise known as PD extensions.
 *
 *  PD extensions are a mechanism for temporarily extending the execution
 *  scope of protection domains.  When a PD operates in extended mode, it
 *  is granted access to an additional memory section not accessible in normal
 *  mode in addition to memory sections already accessible to the PD in normal
 *  mode.  Each PD extension only supports one extended memsection.  Access
 *  to capability lists are unchanged during PD extension.
 *
 *  PD extensions are utilized by creating a PD extension object
 *  and attaching a memory section to that extension. This
 *  extended memory section wll be the granted access to the PD in extended
 *  mode.
 *
 *  PD extensions are activated by threads in the PD.  Threads are provided
 *  with a PD extension token to refer to the
 *  extension.  Threads with access to the token may then switch between PD
 *  normal and extended mode by invoking the token.  Access to PD
 *  extensions are restricted to the threads that are aware of
 *  the PD extension token.
 *
 */

/**
 *  The okl4_extension_token struct is used to represent the @a PD
 *  extension token.  PD extension tokens are used by threads to refer to
 *  a particular PD extension in order to activate and deactivate the extension.
 *  This token can be passed directly to threads within a protection
 *  domain.
 *
 */
struct okl4_extension_token
{
    okl4_kspaceid_t src_space;
    okl4_kspaceid_t dst_space;
};

/* UTCB Parameter to pass to L4_SpaceSwitch(). */
#ifdef NO_UTCB_RELOCATE
#define _OKL4_EXTENSION_UTCB_BASE ((void *)-1)
#else
#define _OKL4_EXTENSION_UTCB_BASE ((void *)L4_GetUtcbBase())
#endif

/**
 *  The OKL4_EXTENSION_ACTIVATE() macro is used to activate an extension,
 *  granting access an additional memory section. This macro requires a single 
 *  argument, @a token, which is used to specify the particular PD extension to activate.
 *
 */
#define OKL4_EXTENSION_ACTIVATE(token)                                        \
    do {                                                                      \
        L4_Word_t _okl4_res;                                                  \
        _okl4_res = L4_SpaceSwitch(L4_myselfconst, (token)->dst_space,        \
                _OKL4_EXTENSION_UTCB_BASE);                                   \
        assert(_okl4_res == 1);                                               \
    } while (0)

/**
 *  The OKL4_EXTENSION_DEACTIVATE() macro is used to deactivate an extension,
 *  denying  further access to the additional memory section.  This macro requires a single 
 *  argument, @a token, which is used to specify the particular PD extension to deactivate.
 *
 */
#define OKL4_EXTENSION_DEACTIVATE(token)                                      \
    do {                                                                      \
        L4_Word_t _okl4_res;                                                  \
        _okl4_res = L4_SpaceSwitch(L4_myselfconst, (token)->src_space,        \
                _OKL4_EXTENSION_UTCB_BASE);                                   \
        assert(_okl4_res == 1);                                               \
    } while (0)

/**
 *  The okl4_extension structure is used to represent the PD extension
 *  object. 
 *
 */
struct okl4_extension
{
    /* This element is a generic memory container. It represents the 'super
     * class' of a struct okl4_extension. So we can say struct okl4_extension
     * 'is a' struct _okl4_mem_container. */
    _okl4_mem_container_t super;

    /* Memory reserved allowing us to be placed in our container's linked list
     * of containers. */
    _okl4_mcnode_t node;

    /* List of memsecs attached to this extension, sorted by increasing virtual
     * address. */
    _okl4_mcnode_t *mem_container_list;

    /* KSpace associated with this extension. */
    okl4_kspace_t *kspace;

    /* Base protection domain. */
    okl4_pd_t *base_pd;

    /* Are we currently attached to 'base_pd'? */
    okl4_word_t attached;

    /*
     * Construction Items.
     *
     * These items are used to allow a extension to construct its own kspace
     * (and other kernel objects) out of user provided pools.
     *
     * These items should only be used during construction and destruction of
     * the extension.
     */
    struct {
        /* Pool of kspace IDs to create a kspace from. */
        okl4_kspaceid_pool_t *kspaceid_pool;

        /* Allocated kspace identifier. */
        okl4_kspaceid_t kspace_id;

        /* Memory used to back the kspace. */
        okl4_kspace_t kspace_mem;
    } _init;
};

/*
 * Static Declaration Macros
 */

/**
 *  The OKL4_EXTENSION_SIZE() macro is used to determine the amount of
 *  memory required to dynamically allocate an extension structure in bytes. 
 *
 */
#define OKL4_EXTENSION_SIZE                                                    \
    (sizeof(struct okl4_extension))

/**
 *  The OKL4_EXTENSION_DECLARE() macro is used to declare statically
 *  allocated memory for an extension.
 *
 *  This function requires the following arguments:
 *
 *  @param name
 *    Variable name of the extension object.
 *
 */
#define OKL4_EXTENSION_DECLARE(name)                                           \
    okl4_word_t __##name[OKL4_EXTENSION_SIZE];                                 \
    static okl4_extension_t *(name) = (okl4_extension_t *) __##name

/* Dynamic initializers */

/**
 * The OKL4_EXTENSION_SIZE_ATTR macro is used to determine the 
 * amount of memory required to dynamically allocate
 * an extension structure if it is initialized with the @a attr
 * attribute in bytes.
 *
 */
#define OKL4_EXTENSION_SIZE_ATTR(attr) \
        (sizeof(struct okl4_extension))

/**
 *  The okl4_extension_create() function is used to create a PD extension
 *  object with the attributes specified by the @a attr argument. 
 *  Creating a PD extension involces creating a new kspace as specified in 
 *  Section okl4_kspace_create(). 
 *
 *  This function requires the following arguments:
 *
 *  @param ext
 *    The PD extension to create.  Its contents will reference the newly
 *    created extension after the function returns.
 *
 *  @param attr
 *    The attribute to create the extension with.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    The operation was successful.
 *
 *  @param OKL4_INVALID_ARGUMENT
 *    The base PD is not privileged and it may not contain extensions.
 *
 *  @param OKL4_ALLOC_EXHAUSTED
 *    The kspace id pool is exhausted and a new kspace cannot be allocated.
 *
 *  @param others
 *    The return value from L4_SpaceControl().
 *
 *  Please refer to the @a OKL4 @a Reference @a Manual for a more detailed explanation of 
 *  the L4_SpaceControl() system call.
 *
 */
int okl4_extension_create(okl4_extension_t *ext, okl4_extension_attr_t *attr);

/**
 *  The okl4_extension_delete() function is used to delete a PD extension
 *  object specified by the @a ext argument.  The extension must not contain an  
 *  attached memory section.
 *
 */
void okl4_extension_delete(okl4_extension_t *ext);

/**
 *  The okl4_extension_memsec_attach() function is used to attach a
 *  memory section specified by the @a memsec argument to an 
 *  extension specified by the @a ext argument.  The thread executing in
 *  PD extended mode will be granted access to this memory section.  The
 *  virtual memory region that @a memsec is located at must be entirely
 *  contained in the virtual memory region occupied by @a ext. Note that
 *  only a single memory section may be attached to a PD extension at a given time.
 *
 *  This function requires the following arguments:
 *
 *  @param ext
 *    The extension to which the memsection is to be attached.
 *
 *  @param memsec
 *    The memory section to be attached.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    The attachment was successful.
 *
 *  @param OKL4_INVALID_ARGUMENT
 *    The @a memsec falls outside the virtual memory region of the @a ext.
 *
 *  @param OKL4_IN_USE
 *    The extension already contains an attached memory section. 
 *
 */
int okl4_extension_memsec_attach(okl4_extension_t *ext, okl4_memsec_t *memsec);

/**
 *  The okl4_extension_memsec_detach() function is used to detach a
 *  memory section @a memsec from an extension @a ext. 
 *
 *  This function requires the following arguments:
 *
 *  @param ext
 *    The extension from which the memory section is to be detached.
 *
 *  @param memsec
 *    The memory section to be detached.
 *
 */
void okl4_extension_memsec_detach(okl4_extension_t *ext, okl4_memsec_t *memsec);

/**
 *  The okl4_extension_gettoken() function is used to retrieve a token that
 *  represents a particular PD extension.  This token is used by threads to
 *  transition the PD in and out of extended mode.
 *
 */
INLINE okl4_extension_token_t okl4_extension_gettoken(okl4_extension_t *extension);

/* No comment */
int _okl4_extension_access(okl4_extension_t *ext, okl4_word_t vaddr,
        okl4_physmem_item_t *phys, okl4_word_t *dest_vaddr, okl4_word_t
        *page_size, okl4_word_t *perms, okl4_word_t *attributes);
void _okl4_extension_premap_mem_container(okl4_extension_t *ext,
        _okl4_mem_container_t *container, okl4_word_t attach_perms);

/*
 * Inline functions.
 */

INLINE okl4_extension_token_t
okl4_extension_gettoken(okl4_extension_t *extension)
{
    okl4_extension_token_t token;

    assert(extension != NULL);
    assert(extension->attached);

    token.src_space = extension->base_pd->kspace->id;
    token.dst_space = extension->kspace->id;
    return token;
}

#endif /* !__OKL4__EXTENSION_H__ */
