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

#ifndef __OKL4__PD_H__
#define __OKL4__PD_H__

#include <assert.h>

#include <compat/c.h>

#include <okl4/types.h>
#include <okl4/memsec.h>
#include <okl4/bitmap.h>

#if defined(OKL4_KERNEL_MICRO)
#include <okl4/kspace.h>
#endif

/**
 *  @file
 *
 *  The pd.h Header.
 *
 *  The pd.h header provides the functionality required for using
 *  the @a protection @a domain (or @a PDs) interface.
 *
 *  Protection domains are a mechanism used for protecting access to
 *  resources in OKL4.  There are two aspects of resource protection
 *  implemented by the PD,  protection of virtual memory mappings and
 *  protection of capability lists.  That is, threads executing in
 *  the same PD share the same set of virtual memory mappings and a single
 *  capability list. Conversely no two PDs share mappings and the clist
 *  unless explicitly specified.
 *
 */

/**
 * Default size for the kclist created by a protection domain. 
 *
 */
#define OKL4_PD_DEFAULT_KCLIST_SIZE  256

/** 
 * Default number of threads supported by a single protection domain. Note that
 * this does mean that this number of threads can always be created, it is also
 * dependent on the availability of capabilities in the caller and PD's clists.  
 *
 */
#define OKL4_PD_DEFAULT_MAX_THREADS  32

/**
 *  The okl4_pd_t structure is used to represent the protection domain object.
 *  The protection domain is a mechanism for protecting access to
 *  resources in OKL4.
 *
 */
struct okl4_pd
{
    /* List of memory container nodes (can either be memsecs or zones) attached
     * to this protection domain, sorted by increasing virtual base address. */
    OKL4_MICRO(_okl4_mcnode_t *mem_container_list;)

    /* Clist of the PD creator - needed to create threads in this PD.  */
    OKL4_MICRO(okl4_kclist_t *root_kclist;)

    /* KSpace used to back this protection domain. This maintains a list of the
     * threads in the PD, as well as the kclist for the space. */
    OKL4_MICRO(okl4_kspace_t *kspace;)

    /** Memsec used to represent the UTCB area of the protection domain. */
    OKL4_MICRO(okl4_memsec_t utcb_memsec;)

    /* Next protection domain in a 'pd_dict_t' class. */
    OKL4_MICRO(okl4_pd_t *dict_next;)

    /* The PD can construct a fixed number of kthreads with minimal user effort.
     * To do this the PD reserves a chunk of memory which it must manage as
     * threads are created/deleted. This is a pointer to that memory. */
    okl4_kthread_t *thread_pool;

    /* Bitmap allocator to keep track of which slots in the above kthreads
     * memory have been used (or not). */
    okl4_bitmap_allocator_t *thread_alloc;

    /* Pager thread for all items in this PD. */
    OKL4_MICRO(okl4_kcap_t default_pager;)

    /* This protection domain's extension. */
    OKL4_MICRO(okl4_extension_t *extension;)

    /*
     * Construction Items.
     *
     * These items are used to allow a PD to construct its own kspace (and
     * other kernel objects) out of user provided pools.
     *
     * These items should only be used during construction and destruction of
     * the PD. In particular, they should not be used as a reference for the
     * resources used by the PD, as they will not be initialised if the user
     * provided these items (instead of asking the PD to construct them on
     * their behalf.)
     */
#if defined(OKL4_KERNEL_MICRO)
    struct {
        /* Pool of kclists to create a kspace from. */
        okl4_kclistid_pool_t *kclistid_pool;

        /* Pool of kspace IDs to create a kspace from. */
        okl4_kspaceid_pool_t *kspaceid_pool;

        /* Pool of virtual memory to allocate virtual memory for. This is only
         * used in the current version of libOKL4 to allocate a UTCB area for
         * the created space. */
        okl4_virtmem_pool_t *virtmem_pool;

        /* Area allocated for the UTCB area. */
        okl4_virtmem_item_t utcb_area_virt_item;

        /* Allocated kspace identifier. */
        okl4_kspaceid_t kspace_id;

        /* Allocated kclist identifier. */
        okl4_kclistid_t kclist_id;

        /* UTCB area object. This points to memory further in our struct. */
        okl4_utcb_area_t *utcb_area;

        /* Clist used by the space. */
        okl4_kclist_t *kclist;

        /* KSpace used to back this protection domain. */
        okl4_kspace_t kspace_mem;

    } _init;
#endif /* OKL4_KERNEL_MICRO */
};

/*
 * Static Declaration
 */

/**
 *  The OKL4_PD_SIZE() macro is used to determine the amount of memory 
 *  required to create a PD with the attributes specified by the  
 *  @a attr argument. 
 *
 */
#if defined(OKL4_KERNEL_MICRO)
#define OKL4_PD_SIZE(kclist_size, max_threads)                                 \
    (sizeof(okl4_pd_t)                                                         \
    + OKL4_KCLIST_SIZE(kclist_size)                                            \
    + OKL4_UTCB_AREA_SIZE(max_threads)                                         \
    + _OKL4_PD_THREADS_POOL_SIZE(max_threads)                                  \
    + OKL4_BITMAP_ALLOCATOR_SIZE(max_threads))
#endif

/**
 *  The OKL4_PD_DECLARE() macro is used to declare and statically allocate memory for a
 *  protection domain strcture.
 *
 *  @param name
 *      Variable name of the PD.
 *
 *  @param kclist_size
 *      The size of the kclist of the PD.
 *
 *  @param max_threads
 *      The maximum number of threads running in the PD.
 *
 *  This macro only handles memory allocation. The initialization
 *  function must be called separately.
 *
 */
#if defined(OKL4_KERNEL_MICRO)
#define OKL4_PD_DECLARE(name, kclist_size, max_threads)                        \
     okl4_word_t __##name[OKL4_PD_SIZE(kclist_size, max_threads)];             \
     static okl4_pd_t *(name) = (okl4_pd_t *) __##name
#endif

/*
 * The number of bytes needed to track num_threads threads.
 */
#define _OKL4_PD_THREADS_POOL_SIZE(num_threads)                                \
    (sizeof(okl4_kthread_t) * (num_threads))

/*
 * Dynamic Initializer
 */

/**
 *  The OKL4_PD_SIZE_ATTR() marcro is used to determine the amount of memory 
 *  required to allocate an okl4_pd_t to be with the attributes specified by the 
 *  @a attr argument, in bytes.
 *
 */
#if defined(OKL4_KERNEL_MICRO)
#define OKL4_PD_SIZE_ATTR(attr)                                                \
        OKL4_PD_SIZE((attr)->kclist_size, (attr)->max_threads)
#endif

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_create() function creates a PD object configured according
 *  to the attributes specified by the @a attr argument.
 *
 *  This function has the following arguments:
 *
 *  @param pd
 *    A pointer to sufficient memory to create a PD.
 *
 *  @param attr
 *    The attributes which determine how the new PD is configured.
 *
 *  This function returns the following error codes:
 *
 *  @param OKL4_OK
 *    The creation was successful.
 *
 *  @param OKL4_ALLOC_EXHAUSTED
 *    There were insufficient resources to create the new PD. This means
 *    that the supplied pools (for example, the kclist identifier pool, the kspace
 *    identifier pool, or the virtual memory pool) do not contain
 *    sufficient free identifiers or memory.
 *
 *  @param others
 *    The return value from L4_ThreadControl() system call.
 *
 *  Please refer to the @a OKL4 @a Reference @a Manual for a more detailed explanation of 
 *  the L4_ThreadControl() system call. 
 *
 */
int okl4_pd_create(okl4_pd_t *pd, okl4_pd_attr_t *attr);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_memsec_attach() function attaches an existing memory section specified 
 *  by the @a memsec argument to an existing PD specified by the @a pd argument. 
 *  This will add the virtual memory region covered by the
 *  memory section to the protection domain. That is,  all threads running
 *  within the the PD will have access to this memory section.
 *
 *  This function requires the following arguments:
 *
 *  @param pd
 *    The protection domain to attach to.
 *
 *  @param memsec
 *    The memory section to be attached.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    Attachment was successful.
 *
 *  @param OKL4_ALLOC_EXHAUSTED
 *    The attachment failed because the virtual address range of the
 *    memory section conflicts with that of an already attached memory section or
 *    zone.
 *
 */
int okl4_pd_memsec_attach(okl4_pd_t *pd, okl4_memsec_t *memsec);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_memsec_detach() function will detach the memory 0section 
 *  specified by the @a memsec argument from the PD specified by the @a pd
 *  argument, effectively denying the PD access to the
 *  virtual memory region covered by the memory section.
 *  Note that this function must be called for each memory section attached to
 *  a particular PD before that PD can be deleted.
 *
 *  This function requires the following arguments:
 *
 *  @param pd
 *    The protection domain to detach from.
 *
 *  @param memsec
 *    The memory section to be detached.
 *
 */
void okl4_pd_memsec_detach(okl4_pd_t *pd, okl4_memsec_t *memsec);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_zone_attach() function attaches a zone specified by the 
 *  @a zone argument to the PD specified by the @a pd argument
 *  according to the attributes specified by @a attr. This will give the PD
 *  access to all memory sections which are currently, or in the future, attached
 *  to this zone. The attributes allow the zone to be attached with access
 *  permissions.
 *
 *  Unlike memory sections a zone may be attached to multiple PDs, allowing 
 *  memory sharing between PDs.
 *
 *  This function requires the following arguments:
 *
 *  @param pd
 *    The protection domain to detach from.
 *
 *  @param zone
 *    The zone to be attached.
 *
 *  @param attr
 *  Access permissions to the zone.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    Attachment was successful.
 *
 *  @param OKL4_ALLOC_EXHAUSTED
 *    The attachment failed because the virtual address range of the zone
 *    conflicts with that of an already attached memsection/zone/extension.
 *
 */
int okl4_pd_zone_attach(okl4_pd_t *pd, okl4_zone_t *zone,
        okl4_pd_zone_attach_attr_t *attr);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_zone_detach() function will detach the zone specified by the
 *  @a zone argument from the PD specified by the @a pd argument, 
 *  effectively denying the PD access to any virtual memory
 *  covered by memory sections attached to the zone.
 *  Note that this function must be called for all zones attached to a PD
 *  before the PD can be deleted.
 *
 *  This function requires the following arguments:
 *
 *  @param pd
 *    The protection domain to detach from.
 *
 *  @param zone
 *    The zone to be detached.
 *
 */
void okl4_pd_zone_detach(okl4_pd_t *pd, okl4_zone_t *zone);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_handle_pagefault() function is used to handle a pagefault
 *  that occurs in the @a pd.  A memory container object, that is a memory
 *  section, zone, or protection domain extension may be mapped at the page fault address. 
 *  This function
 *  determines the physical mapping that is required in order to satisfy
 *  the page fault, and performs that mapping.  If no memory container is
 *  mapped at the page fault address, an error is returned.
 *
 *  This function requires the following arguments:
 *
 *  @param pd
 *    The protection domain that the page fault occured on.
 *
 *  @param space
 *    The kernel address space that the page fault occured on.
 *
 *  @param vaddr
 *    The page fault virtual memory address.
 *
 *  @param access_type
 *    The type of memory access that caused the page fault.
 *
 *  This function returns the following error codes:
 *
 *  @param OKL4_OK
 *    The memory mapping is located and successfully mapped into the faulting
 *    address space.
 *
 *  @param OKL4_UNMAPPED
 *    One of the following possibilities:
 *
 *    @li [a{)}] There is no memory container object 
 *    attached to the faulting address; or
 *
 *    @li [b{)}] There is a memory container object attached to the faulting
 *    address, but there is no memory section attached to the memory container
 *    at the faulting address; or
 *
 *    @li [c{)}] The page fault occurred in a region occupied by a PD
 *    extension, but the page fault occured when the extension is not
 *    activated.
 *
 *  @param others
 *    Return value from the L4_ProcessMapItem() system call.
 *
 *  Please refer to the @a OKL4 @a Reference @a Manual for a more detailed explanation of 
 *  the L4_ProcessMapItem() system call.
 *   
 */
int okl4_pd_handle_pagefault(okl4_pd_t *pd, okl4_kspaceid_t space,
        okl4_word_t vaddr, okl4_word_t access_type);
#endif /* OKL4_KERNEL_MICRO */

/**
 *  The okl4_pd_thread_create() function is used to create a thread with  
 *  specified attributes and runs in the specified PD. Each PD may only contain a
 *  limited number of threads.
 *
 *  A successful call to okl4_pd_thread_create() will allocate a cap from
 *  the PD parent kclist and if it is
 *  different from the parent kclist, a cap from the PD kclist.
 *  Once a thread has been created within a PD it will have access to any
 *  memory sections, zones or extensions attached to the PD.
 *
 *  The function has the following arguments:
 *
 *  @param pd
 *    The PD within which the thread will run.
 *
 *  @param thread_attr
 *    Attribute describing the new thread.
 *
 *  @param thread
 *    Returns a pointer to a newly created.
 *
 *  This function returns the following error codes:
 *
 *  @param OKL4_OK
 *    Thread creation was successful.
 *
 *  @param OKL4_ALLOC_EXHAUSTED
 *    There are insufficient resources to create a new thread. This could
 *    mean that either the PD already contains the maximum number of
 *    threads or that the PD parent kclist has insufficient caps to create a
 *    new thread.
 *
 *  @param other
 *    Return value from either L4_ThreadControl() or L4_CreateIpcCap().
 *  
 *  Please refer to the @a OKL4 @a Reference @a Manual for a more detailed explanation of 
 *  the L4_ThreadControl() and L4_CreateIpcCap() system calls. 
 *
 */
int okl4_pd_thread_create(okl4_pd_t *pd,
        okl4_pd_thread_create_attr_t *thread_attr, okl4_kthread_t **thread);

/**
 *  The okl4_pd_thread_start() function starts the thread specified by the 
 *  @a thread argument executing at the address specified by the okl4_pd_setspip () function. 
 *  This thread must already exist within the specified PD.
 *
 *  @param pd
 *    The PD where the thread was created.
 *
 *  @param thread
 *    The thread to start executing.
 *
 */
void okl4_pd_thread_start(okl4_pd_t *pd, okl4_kthread_t *thread);

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_thread_delete() function deletes the specified thread from the
 *  specified PD. Calling the free () function on the thread pointer
 *  after this call will result in an error.
 *
 *  @param pd
 *    The PD from which the thread is to be deleted.
 *
 *  @param thread
 *    The thread to be deleted.
 *
 */
void okl4_pd_thread_delete(okl4_pd_t *pd, okl4_kthread_t *thread);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_NANO)
/**
 *  Block execution of the caller until the given thread exits.
 *
 *  This function requires the following arguments:
 *
 *  @param kthread
 *    The kthread to wait for.
 *
 *  This function returns the following error conditions:
 *
 *  @retval OKL4_OK
 *    The given thread has successfully exited.
 *
 *  @retval OKL4_INVALID_THREAD
 *    The given thread does not exist.
 *
 */
int okl4_pd_thread_join(okl4_pd_t *pd, okl4_kthread_t *thread);
#endif /* OKL4_KERNEL_NANO */

#if defined(OKL4_KERNEL_NANO)
/**
 *  This function causes the current thread to halt execution.
 *
 *  This function does not return.
 */
NORETURN void okl4_pd_thread_exit(void);
#endif /* OKL4_KERNEL_NANO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_createcap() function gives the threads residing in @a pd
 *  a capability to send an IPC to the thread specified by the 
 *  @a thread argument.  The thread may exist in PD specified by the 
 *  @a pd argument or in another PD.
 *
 *  It is not necessary to use this function when the kclist used by @a
 *  pd is the root kclist, and will waste  root kclist
 *  slots.  In this situation threads within @a pd will already have the
 *  necessary capabilities to IPC @a thread.
 *
 *  Note that every capability created using this function must be
 *  deleted using okl4_pd_deletecap ()  function before the PD or its kclist is
 *  deleted.
 *
 *  This function has the following arguments:
 *
 *  @param pd
 *    The PD whose threads will have permission to IPC @a thread.
 *
 *  @param thread
 *    The thread which can receive an IPC after this call.
 *
 *  @param cap
 *    A kcap returned to the caller which can then be used in an L4 IPC
 *    system call to @a thread.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    Creation was successful.
 *
 *  @param others
 *    Return value from L4_CreateIpcCap().
 *
 *  Please refer to the @a OKL4 @a Reference @a Manual for a more detailed explanation of 
 *  the system L4_CreateIpcCap() call.  
 *
 */
int okl4_pd_createcap(okl4_pd_t *pd, okl4_kthread_t *thread, okl4_kcap_t *cap);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_deletecap() function removes the kcap, @a cap, from the
 *  kclist associated with the PD specified by the @a pd argument. 
 *  Threads within the PD will no longer be
 *  able to IPC the thread associated with this @a cap.
 *
 *  @param pd
 *    The PD from whose kclist @a cap will be removed.
 *
 *  @param cap
 *    The capability to be removed from the @a pd kclist.
 *
 */
void okl4_pd_deletecap(okl4_pd_t *pd, okl4_kcap_t cap);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_delete() function deletes the PD specified by the @a pd argument. If the PD was
 *  origianlly created using pools then all resources used by the PD will be returned to
 *  these pools. Also all threads running within the PD will be deleted.
 *  Please note that this function will not free the memory used by the PD
 *  object. The user must call the appropriate @a detach function on all
 *  attached objects before deleting the PD.
 *
 */
void okl4_pd_delete(okl4_pd_t *pd);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_extension_attach() function attaches the specified PD extension,
 *  @a ext, to the PD, @a pd. Threads will then PD will then be able to
 *  access the extension by executing the OKL4_EXTENSION_ACTIVATE ()
 *  macro. 
 *
 *  This function requires the following arguments:
 *
 *  @param pd
 *    The protection domain to attach to.
 *
 *  @param ext
 *    The extension to attach.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    Attachment was successful.
 *
 *  @param OKL4_INVALID_ARGUMENT
 *    The @a pd is not the same PD that the extension @a ext was configured
 *    to attach to.
 *
 *  @param OKL4_IN_USE
 *    The @a pd already has an extension attached.
 *
 *  @param OKL4_ALLOC_EXHAUSTED
 *    The attachment failed because the virtual address range of the
 *    extension conflicts with that of an already attached
 *    memory section or zone.
 *
 */
int okl4_pd_extension_attach(okl4_pd_t *pd, okl4_extension_t *ext);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_extension_detach() function will detach the extension, @a ext,
 *  from the PD, @a pd. Threads within the PD @a pd will no longer be able
 *  to run within the extension.
 *
 *  Note that this function must be called for any extensions attached to a
 *  PD before the PD can be deleted.
 *
 *  This function requires the following arguments:
 *
 *  @param pd
 *    The protection domain to detach from.
 *
 *  @param ext
 *    The extension to detach.
 *
 */
void okl4_pd_extension_detach(okl4_pd_t *pd, okl4_extension_t *ext);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_attr_t type represents attributes for creating PDs. A
 *  single attribute object can be used to create muliple PDs. Before use
 *  the attribute object should be initailized with default values using the
 *  okl4_pd_pattrinit () function. It can then be customized using  
 *  individual setter functions.
 *
 */
struct okl4_pd_attr
{
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif

    /** Clist of the creator of this pd. Needed to create threads in the PD. */
    okl4_kclist_t *root_kclist;

    /** KSpace to associate with this protection domain. If this is NULL,
     * the PD will create the kspace using provided pools. */
    okl4_kspace_t *kspace;

    /** KClist to use for this PD. Allows multiple PDs to share the one
     * KClist. */
    okl4_kclist_t *kclist;

    /** Pool of kclists to create a kspace from. If NULL, a kclist must be
     * provided.*/
    okl4_kclistid_pool_t *kclistid_pool;

    /** Pool of kspace IDs to create a kspace from. */
    okl4_kspaceid_pool_t *kspaceid_pool;

    /** Pool of virtual memory to allocate virtual memory for. This is only
     * used in the current version of libOKL4 to allocate a UTCB area for
     * the created space. */
    okl4_virtmem_pool_t *virtmem_pool;

    /** Size of the kclist to use. */
    okl4_word_t kclist_size;

    /** Maximum number of supported threads in the protection domain. */
    okl4_word_t max_threads;

    /** Default pager for all threads in this PD. */
    okl4_kcap_t default_pager;

    /** Should this PD be privileged? */
    okl4_word_t privileged;
};
#endif /* OKL4_KERNEL_MICRO */

/**
 *  The protection domain thread creation attribute,
 *  okl4_pd_thread_create_attr, represents all attributes needed to create
 *  a thread in running within a PD.
 *
 *  The following parameters are encoded within the attribute structure:
 *
 *  @li The desired @a priority of the new thread;
 *
 *  @li The stack (@a sp) and instruction (@a ip) pointers for the thread.
 *
 *  @li The @a pager thread that will handle any page faults triggered by
 *  the newly created thread.
 *
 */
struct okl4_pd_thread_create_attr
{
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif

    /* Thread priority. */
    okl4_word_t priority;

    /* Thread stack pointer. */
    okl4_word_t sp;

    /* Thread start instruction pointer. */
    okl4_word_t ip;

    /* Pager for this thread. */
    okl4_kcap_t pager;
};

#if defined(OKL4_KERNEL_MICRO)
/**
 *  Attributes determining how a zone should be attached to a protection domain.
 */
struct okl4_pd_zone_attach_attr
{
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif

    /* Permissions mask for all memsections in zone */
    okl4_word_t perms;
};
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_attr_init() function is used to initailize the specified attribute
 *  object, @a attr, to contain default values for PD creation. This
 *  function must be called on the attribute object before the attribute
 *  setter functions are called and before it is used to create
 *  a PD.
 *
 */
void okl4_pd_attr_init(okl4_pd_attr_t *attr);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_attr_setkclistsize() function is used in conjunction with
 *  okl4_pd_attr_setkclistidpool () function to specify the size of the kclist that
 *  will be created for a specified PD. By default this is set to
 *  OKL4_PD_DEFAULT_KCLIST_SIZE.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    A pointer to the attribute object to modify.
 *
 *  @param kclistsize
 *    The size of the kclist of the new PD.
 *
 */
INLINE void okl4_pd_attr_setkclistsize(okl4_pd_attr_t *attr, okl4_word_t kclistsize);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_attr_setmaxthreads() function is used to set the  
 *  maximum number of threads to be contained in a PD created using the attibute 
 *  specified by the @a attr argument.  
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute object to modify.
 *
 *  @param maxthreads
 *    The maximum number of threads that can be created within a PD.
 *
 */
INLINE void okl4_pd_attr_setmaxthreads(okl4_pd_attr_t *attr, okl4_word_t maxthreads);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_attr_setkclistidpool() function provides the PD creation
 *  function with a pool of kclist identifiers from which it can allocate a
 *  particular kclist id. It then uses this identifier to create a kclist for the
 *  PD. Alternatively a PD can be allocated a specific kclist using the
 *  okl4_pd_attr_setkclist () function. These two function calls are mutually
 *  exclusive.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    A pointer to the attribute object to modify.
 *
 *  @param kclistid_pool
 *    A pointer to a bitmap allocator which represents a pool of valid
 *    kclist identifiers.
 *
 */
INLINE void okl4_pd_attr_setkclistidpool(okl4_pd_attr_t *attr,
        okl4_kclistid_pool_t *kclistid_pool);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_attr_setutcbmempool() function provides the PD with a pool
 *  of virtual memory from which it can allocate a utcb area. The pool of
 *  virtual memory must be of sufficient size to accomodate a thread
 *  control block (TCB) for every thread created within the PD. For
 *  example, if the PD is configured to contain a maximum of 32 threads
 *  then the provided virtual memory pool must be a minimum of 32 *
 *  sizeof(okl4_utcb_t) bytes in size.
 *
 *  It is an error to specify a valid virtual memory pool using
 *  okl4_pd_attr_setutcbmempool () and a valid kspace using
 *  okl4_pd_attr_setkspace () because the provided kspace should already
 *  container a utcb area.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    A pointer to the attribute object to modify.
 *
 *  @param virtmem_pool
 *    A pointer to a pool of pool of virtual memory.
 *
 */
INLINE void okl4_pd_attr_setutcbmempool(okl4_pd_attr_t *attr,
        okl4_virtmem_pool_t *virtmempool);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_attr_setkspacedidpool() function provides the PD creation
 *  function with a pool of kspace identifiers to be used to 
 *  allocate a unique identifier which can be used to create a new kspace for
 *  the PD. Alternatively a PD can use a specific kspace by calling
 *  okl4_pd_attr_setkspace (). Note that these two function calls are mutually
 *  exclusive.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    A pointer to the attribute object to modify.
 *
 *  @param kspaceid_pool
 *    A pointer to a bitmap allocator representing a pool of kspace
 *    identifiers.
 *
 */
INLINE void okl4_pd_attr_setkspaceidpool(okl4_pd_attr_t *attr,
        okl4_kspaceid_pool_t *kspaceid_pool);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_attr_setkspace() function provides a PD with a specific
 *  kspace, as opposed to allowing a kspace to be automatically created for
 *  the PD. Note that this function and
 *  okl4_pd_attr_setspaceidpool () are mutually exclusive.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    A pointer to the attribute object to modify.
 *
 *  @param kspaceid_pool
 *    A pointer to a kspace to be used by the PD.
 *
 */
INLINE void okl4_pd_attr_setkspace(okl4_pd_attr_t *attr,  okl4_kspace_t *kspace);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_attr_setkclist() provides a PD with a specific kclist, as
 *  opposed to allowing a kclist to be automatically created. This function
 *  is useful when sharing a kclist between multiple PDs. Please note that
 *  a PD cannot be created with an attribute object that specifies both a
 *  valid kclist and a valid kclistid_pool (see okl4_pd_setclistpool()).
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    A pointer to the attribute object to modify.
 *
 *  @param kclist
 *    A pointer to a kclist to be used by the PD.
 *
 */
INLINE void okl4_pd_attr_setkclist(okl4_pd_attr_t *attr, okl4_kclist_t *kclist);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_attr_setrootkclist() function provides the PD with a
 *  reference to the kclist of the kthread/kspace which created the PD, in
 *  effect the 'manager' of the PD. This is required as
 *  okl4_pd_thread_create () uses a capability from the callers kclist.
 *
 *  This function requires the following arguments: 
 *
 *  @param attr
 *    A pointer to the attribute object to modify.
 *
 *  @param kclist
 *    A pointer to the kclist of the PD creator/manager.
 *
 */
INLINE void okl4_pd_attr_setrootkclist(okl4_pd_attr_t *attr, okl4_kclist_t *kclist);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_attr_setdefaultpager() function sets the default pager for
 *  all threads within a PD. If on thread creation, no pager is specified
 *  then the thread will be configured with @a defaultpager as its pager.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    A pointer to the attribute object to modify.
 *
 *  @param defaultpager
 *    A kcap for the PD default pager.
 *
 */
INLINE void okl4_pd_attr_setdefaultpager(okl4_pd_attr_t *attr,
        okl4_kcap_t defaultpager);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_attr_setprivileged() function configures whether or not the
 *  created PD is privileged. Threads within a privileged PD can make
 *  system calls which use kernel resources. A non-privileged PD may not
 *  create/destroy any kernel resources such as threads, spaces, clists
 *  etc.
 *
 *  This function requires the following arguments: 
 *
 *  @param attr
 *    A pointer to the attribute object to modify.
 *
 *  @param privileged
 *    A boolean value where privileged == 0 is false and privileged > 0 is
 *    true.
 *
 */
INLINE void okl4_pd_attr_setprivileged(okl4_pd_attr_t *attr, okl4_word_t privileged);
#endif /* OKL4_KERNEL_MICRO */

/**
 *  The okl4_pd_thread_create_attr_init() function initializes the PD
 *  thread creation attribute with default values. 
 *
 */
INLINE void okl4_pd_thread_create_attr_init(okl4_pd_thread_create_attr_t *attr);

/**
 *  The okl4_pd_thread_create_attr_setspip() sets the stack and instruction
 *  pointers for all threads created with the attributes specified by the @a attr argument.
 *
 *  The function has the following arguments:
 *
 *  @param attr
 *    The attribute.
 *
 *  @param sp
 *    The initial stack pointer for the new thread.
 *
 *  @param ip
 *    The initial instruction pointer for the new thread.
 *
 */
INLINE void okl4_pd_thread_create_attr_setspip(okl4_pd_thread_create_attr_t *attr,
        okl4_word_t sp, okl4_word_t ip);

/**
 *  The okl4_pd_thread_create_attr_setpriority() function sets the priority
 *  of all threads created with the attributes specified by the @a attr argument.
 *
 *  The function has the following arguments:
 *
 *  @param attr
 *    The attribute.
 *
 *  @param priority
 *    The priority for the new thread.
 *
 */
INLINE void okl4_pd_thread_create_attr_setpriority(okl4_pd_thread_create_attr_t *attr,
        okl4_word_t priority);

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_pd_thread_create_attr_setpager() function sets the page-fault
 *  handling thread for all threads created with the attributes specified by the @a attr argument.
 *
 *  The function has the following arguments:
 *
 *  @param attr
 *    The attribute.
 *
 *  @param pager
 *    A kcap for the pager.
 *
 */
INLINE void okl4_pd_thread_create_attr_setpager(okl4_pd_thread_create_attr_t *attr,
        okl4_kcap_t pager);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  
 */
INLINE void okl4_pd_zone_attach_attr_init(okl4_pd_zone_attach_attr_t *attr);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  
 */
INLINE void okl4_pd_zone_attach_attr_setperms(okl4_pd_zone_attach_attr_t *attr,
        okl4_word_t perms);
#endif /* OKL4_KERNEL_MICRO */

/*
 * Inline functions.
 */

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_pd_attr_setkclistsize(okl4_pd_attr_t *attr, okl4_word_t kclistsize)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_PD_ATTR);

    attr->kclist_size = kclistsize;
}
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_pd_attr_setmaxthreads(okl4_pd_attr_t *attr, okl4_word_t maxthreads)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_PD_ATTR);

    attr->max_threads = maxthreads;
}
#endif

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_pd_attr_setkclistidpool(okl4_pd_attr_t *attr,
        okl4_kclistid_pool_t *kclistid_pool)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_PD_ATTR);

    attr->kclistid_pool = kclistid_pool;
}
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_pd_attr_setutcbmempool(okl4_pd_attr_t *attr,
        okl4_virtmem_pool_t *virtmempool)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_PD_ATTR);

    attr->virtmem_pool = virtmempool;
}
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_pd_attr_setkspaceidpool(okl4_pd_attr_t *attr,
        okl4_kspaceid_pool_t *kspaceid_pool)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_PD_ATTR);

    attr->kspaceid_pool = kspaceid_pool;
}
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_pd_attr_setkspace(okl4_pd_attr_t *attr,  okl4_kspace_t *kspace)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_PD_ATTR);

    attr->kspace = kspace;
}
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_pd_attr_setkclist(okl4_pd_attr_t *attr, okl4_kclist_t *kclist)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_PD_ATTR);

    attr->kclist = kclist;
}
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_pd_attr_setrootkclist(okl4_pd_attr_t *attr, okl4_kclist_t *kclist)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_PD_ATTR);

    attr->root_kclist = kclist;
}
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_pd_attr_setdefaultpager(okl4_pd_attr_t *attr,
        okl4_kcap_t defaultpager)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_PD_ATTR);

    attr->default_pager = defaultpager;
}
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_pd_attr_setprivileged(okl4_pd_attr_t *attr, okl4_word_t privileged)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_PD_ATTR);

    attr->privileged = privileged;
}
#endif /* OKL4_KERNEL_MICRO */
#if defined(OKL4_KERNEL_MICRO)
#endif /* OKL4_KERNEL_MICRO */

INLINE void
okl4_pd_thread_create_attr_init(okl4_pd_thread_create_attr_t *attr)
{
    assert(attr != NULL);

    OKL4_SETUP_MAGIC(attr, OKL4_MAGIC_PD_THREAD_CREATE_ATTR);
    attr->sp = ~0UL;
    attr->ip = ~0UL;
    attr->priority = 0;
    OKL4_MICRO(attr->pager.raw = L4_nilthread.raw;)
}

INLINE void
okl4_pd_thread_create_attr_setspip(okl4_pd_thread_create_attr_t *attr,
        okl4_word_t sp, okl4_word_t ip)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_PD_THREAD_CREATE_ATTR);

    attr->sp = sp;
    attr->ip = ip;
}

INLINE void
okl4_pd_thread_create_attr_setpriority(okl4_pd_thread_create_attr_t *attr,
        okl4_word_t priority)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_PD_THREAD_CREATE_ATTR);

    attr->priority = priority;
}

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_pd_thread_create_attr_setpager(okl4_pd_thread_create_attr_t *attr,
        okl4_kcap_t pager)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_PD_THREAD_CREATE_ATTR);

    attr->pager = pager;
}
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_pd_zone_attach_attr_init(okl4_pd_zone_attach_attr_t *attr)
{
    assert(attr != NULL);

    OKL4_SETUP_MAGIC(attr, OKL4_MAGIC_PD_ZONE_ATTACH_ATTR);
    attr->perms = OKL4_PERMS_FULL;
}
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_pd_zone_attach_attr_setperms(okl4_pd_zone_attach_attr_t *attr,
        okl4_word_t perms)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_PD_ZONE_ATTACH_ATTR);

    attr->perms = perms;
}
#endif /* OKL4_KERNEL_MICRO */

#endif /* !__OKL4__PD_H__ */
