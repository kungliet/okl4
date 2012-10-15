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

#ifndef __OKL4__MEMSEC_H__
#define __OKL4__MEMSEC_H__

#include <assert.h>

#include <okl4/arch/config.h>
#include <okl4/errno.h>
#include <okl4/types.h>
#include <okl4/physmem_item.h>
#include <okl4/virtmem_pool.h>

#include "mem_container.h"

/**
 *  @file
 *
 *  The memsec.h Header.
 *
 *  The memsec.h header provides the functionality required for
 *  using the memory section interface.
 *  The memory section object represents a virtual memory region together 
 *  with its associated lookup, mapping, and unmapping strategies.
 *
 *  The memory section object represents a virtual memory region designed to be
 *  used in virtual memory management systems as opposed to the  
 *  virtmem object which is purely designed for representing a region of
 *  virtual memory for allocation purposes.
 *
 *  Each memory section is associated with a mapping function, which determines
 *  where and how to map to physical memory, a lookup function, which performs
 *  a virtual to physical address lookup, and a destroy function, which performs
 *  an unmapping.  Each of these associated functions are stored as callback
 *  functions in the memory section structure, to be called when the
 *  corresponding operation is invoked. The memory section structure also
 *  stores state related information such as access permissions.
 *
 *  As with other OKL4 library objects, memory sections are initialized using attribute
 *  objects.
 *
 */

/*
 * Static Declaration Macros
 */

/**
 *  The OKL4_MEMSEC_SIZE() macro is used to determine the amount of memory
 *  required to dynamically allocate a memory section if it is
 *  initialized using the @a attr attribute, in bytes.
 *
 */
#define OKL4_MEMSEC_SIZE   \
    (sizeof(struct okl4_memsec))

/**
 *  The OKL4_MEMSEC_DECLARE() macro is used to delare and statically 
 *  allocate memory for a memsec object structure with the name specified 
 *  by the @a name argument.
 *
 *  This macro only handles memory allocation. The initialization
 *  function must be called separately.
 *
 */
#define OKL4_MEMSEC_DECLARE(name) \
    okl4_word_t __##name[OKL4_MEMSEC_SIZE];                       \
    static okl4_memsec_t *(name) = (okl4_memsec_t *) __##name

/*
 * Dynamic Initializer
 */

/**
 *  The OKL4_MEMSEC_SIZE_ATTR() macro is used to determines the amount 
 *  of memory required to dynamically allocate a memory section object, if the
 *  object is initialized with @a attr attribute, in bytes.
 *
 */
#define OKL4_MEMSEC_SIZE_ATTR(attr) \
    (sizeof(struct okl4_memsec))

/*
 * Memsec mapping callbacks.
 */

/**
 *  The okl4_premap_func_t function type defines the function prototype for
 *  all memory section premap callbacks.  
 *  When a memsection is first attached to a protection domain, zone, or PD
 *  extension, this function provides the option for cell managers to
 *  perform premapping.  Premapping a memory section is desirable when demand
 *  paging is not neccessary and the cell manager wishes to avoid incurring
 *  page faults. The premap callback is called when a mememory section is being
 *  attached, and is called iteratively along the virtual memory region of
 *  the memory section.
 *
 *  The premap function is expected to translate a virtual memory address
 *  to a physical memory mapping.  The callback is provided with the premap
 *  address @a premap_addr, and returns the target physical memory item to
 *  be mapped in @a map_item.  In addition, the callback returns the base
 *  virtual address on which to perform the mapping using the @a map_vaddr argument.
 *  The results @a map_vaddr and @a map_item determine how the virtual
 *  memory starting from @a map_vaddr should be mapped. The premap
 *  address must be covered within this mapping region.
 *
 *  The returned mapping is applicable for a virtual memory region that is
 *  the same size as @a map_item.  The thread calling the premap callback
 *  will then invoke the premap callback on the next virtual memory region
 *  in the memory section.
 *
 *  The premap callback @a must not perform the actual virtual-to-physical
 *  mapping operation.  This is performed after the callback
 *  returns.
 *
 *  This function type requires the following arguments:
 *
 *  @param memsec
 *    The memory section to premap.
 *
 *  @param premap_vaddr
 *    The virtual address to premap.
 *
 *  @param map_item
 *    The physical memory to premap onto.  Its contents will contain the
 *    physical memory item on function return.
 *
 *  @param map_vaddr
 *    The base virtual memory address to premap from. Its contents will
 *    contain the virtual memory address on function return.
 *
 *  This function type returns the following codes:
 *
 *  @param $0$
 *    A mapping to physical memory is returned.  A mapping operation should
 *    be invoked after this callback returns.
 *
 *  @param Not $0$
 *    No physical memory is returned.  No mapping operation should be
 *    invoked.
 *
 */
typedef int (*okl4_premap_func_t)(
        okl4_memsec_t *memsec, okl4_word_t vaddr, okl4_physmem_item_t
        *map_item, okl4_word_t *dest_vaddr);

/**
 *  The okl4_access_func_t function type defines the function prototype for
 *  all memory section access callbacks.  
 *  When a page fault occurs during a thread's memory access, the faulting
 *  thread's pager is notified.  The memory section that the
 *  fault occurred in is looked up, with the access callback then called on
 *  the faulting memory section.
 *
 *  The access callback is expected to determine the physical memory region
 *  that should be mapped to the faulting virtual memory.  The callback is
 *  provided with the faulting address @a fault_vaddr and returns the
 *  target physical memory item to be mapped in the @a map_item argument.  In addition,
 *  the callback returns the base virtual address to perform the mapping on
 *  in the @a map_vaddr argument.  It should be noted that in the majority
 *  of cases, @a map_vaddr will be different to @a fault_vaddr.  This is due to 
 *  mappings being always page-aligned and possibly spanning multiple pages,
 *  whereas a page fault may occur anywhere inside a page.  Also, it should
 *  be noted that the returned physical memory does not necessarily need to
 *  cover the entire memory section.
 *
 *  It is possible that @a map_item is returned.  This occurs when
 *  the faulting thread tries to access a virtual address to which it should
 *  not have access, that is, it is an unhandled page fault.  It
 *  is up to the pager to determine the action to be taken in this situation. 
 *
 *  The access callback @a must not perform the actual virtual-to-physical
 *  mapping operation.  The mapping operation is performed after the callback
 *  returns.
 *
 *  This function may be called multiple times for the same faulting
 *  virtual address.
 *
 *  This function type requires the following arguments:
 *
 *  @param memsec
 *    The memory section to perform mapping on.
 *
 *  @param fault_vaddr
 *    The page fault virtual address.
 *
 *  @param map_item
 *    The physical memory to map onto, which will resolve the current page
 *    fault.  Its contents will contain the physical memory item on
 *    function return.
 *
 *  @param map_vaddr
 *    The base virtual memory address to map from, which will resolve the
 *    current page fault.  Its contents will contain the virtual memory
 *    address on function return.
 *
 *  This function type returns the following codes:
 *
 *  @param $0$
 *    A mapping to physical memory is returned.  A mapping operation should
 *    be invoked after this callback returns.
 *
 *  @param Not $0$
 *    No physical memory is returned.  No mapping operation should be
 *    invoked.
 *
 */
typedef int (*okl4_access_func_t)(
        okl4_memsec_t *memsec, okl4_word_t vaddr,
        okl4_physmem_item_t *map_item, okl4_word_t *dest_vaddr);

/**
 *  The okl4_destroy_func_t  type defines the function prototype
 *  for all memory section destroy callbacks.  
 *
 *  The destroy callback is invoked when a memory section specified by the 
 *  @a memsec argument is torn down.  This
 *  callback allows the system builder to free any resources used in the 
 *  access and premap callbacks.
 *
 */
typedef void (*okl4_destroy_func_t)(okl4_memsec_t *memsec);

/**
 *  The okl4_memsec structure is used to represent the memory section
 *  object.  The memory section object contains a virtual memory region and
 *  provides methods of determining how this region maps to physical
 *  memory.
 *
 *  The following operations may be performed on a memory section:
 *
 *  @li Dynamic memory allocation.
 *
 *  @li Initialization.
 *
 *  @li Query.
 *
 *  @li Callback invocation.
 *
 */
struct okl4_memsec
{
    /* This element is a generic memory container. It represents the 'super
     * class' of a struct okl4_memsec. So we can say okl4_memsec_t 'is a'
     * _okl4_mem_container_t. */
    _okl4_mem_container_t super;

    /* A memory container node. This contains a 'next' pointer needed to build
     * lists of memsections. In the case that a memsection was to be a part of
     * several lists we would need several of these. */
    _okl4_mcnode_t list_node;

    /* Page size of pages contained within this memsec. */
    okl4_word_t page_size;

    /* Permissions mask. This limits the permissions on the underlying physical
     * memory this memsec maps to. */
    okl4_word_t perms;

    /* Memory attributes mask. This enables more strict requirements to be
     * placed on memory cacheability. The precise values used in this field are
     * architecture dependent. */
    okl4_word_t attributes;

    /* Determine what physseg item a virtual address should be premapped to. */
    okl4_premap_func_t premap_callback;

    /* Lookup a virtual address, allocating physical memory if necessary. */
    okl4_access_func_t access_callback;

    /* Destroy this mapper object. */
    okl4_destroy_func_t destroy_callback;

    /** Object that this memsec is attached to. Can be either a pd, zone or
     * pd extension. */
    void *owner;
};

/**
 *  The okl4_memsec_init() function is used to initialize the memory section
 *  specified by the @a ms argument with the attributes specified by the 
 *  @a attr argument.
 *
 *  This function requires the following arguments:
 *
 *  @param ms
 *    Memory section to be initialized.
 *
 *  @param attr
 *    Attribute to be used to initialize the memsection.
 *
 */
void okl4_memsec_init(okl4_memsec_t *ms, okl4_memsec_attr_t *attr);

/**
 *  The okl4_memsec_premap() function is used to perform a premap callback
 *  invocation on the memory section specified by the @a ms argument.  
 *  This operation determines where  the virtual memory address specified by
 *  the @a access_vaddr argument should be premapped in
 *  physical memory.  Information regarding how the virtual memory should be
 *  premapped is returned using the other arguments to the function.
 *
 *  This function is a simple wrapper to the memory section premap callback.  
 *  It does not perform any mapping operations.
 *  Mapping operations should be called using the output arguments obtained
 *  from this function on the function return.
 *
 *   This function requires the following arguments:
 *
 *  @param ms
 *    The memory section to perform the premap operation on.
 *
 *  @param premap_vaddr
 *    The premap virtual address.
 *
 *  @param phys
 *    The physical memory to map onto.  This is an output argument.
 *
 *  @param map_vaddr
 *    The base virtual memory address to map from. This is an output
 *    argument.
 *
 *  @param page_size
 *    The page size to map with.  This is an output argument.
 *
 *  @param perms
 *    The access permissions to map with.  This is an output argument.
 *
 *  @param attributes
 *    The cache attributes to map with.  This is an output argument.
 *
 *  @param OKL4_OK
 *    Creation was successful.
 *
 *  @param other
 *    Return value from memory section premap callback.
 *
 */
int okl4_memsec_premap(okl4_memsec_t *ms, okl4_word_t vaddr,
        okl4_physmem_item_t *phys, okl4_word_t *dest_vaddr,
        okl4_word_t *page_size, okl4_word_t *perms, okl4_word_t *attributes);

/**
 *  The okl4_memsec_access() function is used to perform an access callback
 *  invocation on the memory section specified by the @a ms argument.  
 *  This operation determines where the virtual memory address specified by 
 *  the @a access_vaddr argument should be mapped in
 *  physical memory.  Information regarding how the virtual memory should be
 *  mapped is returned in various arguments in the function.
 *
 *  This function is a simple wrapper to the memsection access callback.
 *  It does not perform any mapping operations.
 *  Mapping operations should be called with the output arguments obtained
 *  from this function on function return.
 *
 *  @param ms
 *    The memory section to perform the access operation on.
 *
 *  @param access_vaddr
 *    The page fault virtual address.
 *
 *  @param phys
 *    The physical memory to map onto.  This is an output argument.
 *
 *  @param map_vaddr
 *    The base virtual memory address to map from. This is an output
 *    argument.
 *
 *  @param page_size
 *    The page size to map with.  This is an output argument.
 *
 *  @param perms
 *    The access permissions to map with.  This is an output argument.
 *
 *  @param attributes
 *    The cache attributes to map with.  This is an output argument.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    Memory section access was successful.
 *
 *  @param other
 *    Return value from memory section access callback.
 *
 */
int okl4_memsec_access(okl4_memsec_t *ms, okl4_word_t vaddr,
        okl4_physmem_item_t *phys, okl4_word_t *dest_vaddr,
        okl4_word_t *page_size, okl4_word_t *perms, okl4_word_t *attributes);

/**
 *  The okl4_memsec_destroy() function is used to perform a destroy
 *  callback invocation on memory section specified by the @a ms argument.
 *  This function provides a simple wrapper to the memory section destroy callback
 *  function.  It does not perform any unmapping
 *  operations.
 *
 */
void okl4_memsec_destroy(okl4_memsec_t *ms);

/* Internal memsec initialisation function. */
void _okl4_memsec_init(okl4_memsec_t *memsec);

/**
 *  The okl4_memsec_attr structure is used to represent the memory section
 *  attributes and is used to initialize memory sections.
 *
 *  The following parameters are required when creating a memory section:
 *
 *  @li The virtual memory region represented by the memory section.
 *
 *  @li The secondary attributes associated with mapping the memory section,
 *  including access permissions and cache attributes.
 *
 *  @li The callback functions to perform access, premap, and destruction
 *  operations.  The choice of these functions directly affect where and 
 *  how this memory section is mapped to physical memory.
 *
 *  Each of these parameters should be set appropriately in the memory section
 *  attribute before creating the memory section.  Note that the memory section 
 *  attribute must be initialized before it is used to initialize memory sections.
 *
 */
struct okl4_memsec_attr {
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif

    /* Memory permissions. */
    okl4_word_t perms;

    /* Memory attributes. */
    okl4_word_t attributes;

    /* Virtual address range this memsection is responsable for. */
    okl4_virtmem_item_t range;

    /* Page size to be used in this memsec. */
    okl4_word_t page_size;

    /* Callback functions used to define how the virtual address range covered
     * by this memsec is mapped to physical memory. */
    okl4_premap_func_t premap_callback;
    okl4_access_func_t access_callback;
    okl4_destroy_func_t destroy_callback;
};

/**
 *  The okl4_memsec_attr_init() function is used to initialize the
 *  memory section attribute specified by the @a attr argument to 
 *  default values.  It must be invoked on an attribute before it 
 *  may be used for memory section initialization.
 *
 */
INLINE void okl4_memsec_attr_init(okl4_memsec_attr_t *attr);

/**
 *  The okl4_memsec_attr_setrange() function is used to encode the
 *  memory section attribute @a attr with a virtual memory region specified 
 *  by the @a range argument. The
 *  memory section created with this attribute will be used to manage the
 *  virtual memory mapping of the @a range region.   
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to be encoded.  Its contents will reflect the new encoding
 *    on function return.
 *
 *  @param range
 *    The virtmem item containing the virtual memory region.
 *
 */
INLINE void okl4_memsec_attr_setrange(okl4_memsec_attr_t *attr,
        okl4_virtmem_item_t range);

/**
 *  The okl4_memsec_attr_setperms() function is used to encode the
 *  memory section attribute specified by the @a attr argument with the  
 *  access permissions specified by the @a perms argument. These access
 *  permissions are used to control access to the virtual-to-physical
 *  mappings contained within the memory section.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to be encoded.  Its contents will reflect the new encoding
 *    on function return.
 *
 *  @param perms
 *    The access permissions permitted on the memory section.
 *
 */
INLINE void okl4_memsec_attr_setperms(okl4_memsec_attr_t *attr, okl4_word_t perms);

/**
 *  The okl4_memsec_attr_setattributes() function is used to encode the
 *  memory section attribute specified by the @a attr argument with 
 *  the cache attributes specified by the @a attributes argument.  This parameter
 *  determines how the physical memory mapped to the memory section will be
 *  cached.
 *
 *  This function requires the following arguments: 
 *
 *  @param attr
 *    The attribute to be encoded.  Its contents will reflect the new encoding
 *    on function return.
 *
 *  @param attributes
 *    The cache behaviour of physical memory mapped to this memory section.
 *
 */
INLINE void okl4_memsec_attr_setattributes(okl4_memsec_attr_t *attr,
        okl4_word_t attributes);

/**
 *  
 */
INLINE void okl4_memsec_attr_setpagesize(okl4_memsec_attr_t *attr,
        okl4_word_t pagesize);

/**
 *  The okl4_memsec_attr_setpremapcallback() function is used to set the
 *  memory section attribute specified by the @a attr argument with the 
 *  lookup callback function specified by the @a premapcallback argument.
 *
 *  This function requires the following arguments: 
 *
 *  @param attr
 *    The attribute to be encoded.
 *
 *  @param premapcallback
 *    The premap callback to be set.
 *
 */
INLINE void okl4_memsec_attr_setpremapcallback(okl4_memsec_attr_t *attr,
        okl4_premap_func_t premapcallback);

/**
 *  The okl4_memsec_attr_setaccesscallback() function is used to set the
 *  memory section attribute specified by the @a attr argument with the access callback function
 *  specified by the @a accesscallback argument. 
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to be encoded.
 *
 *  @param accesscallback
 *    The access callback to be set.
 *
 */
INLINE void okl4_memsec_attr_setaccesscallback(okl4_memsec_attr_t *attr,
        okl4_access_func_t accesscallback);

/**
 *  The okl4_memsec_attr_setdestroycallback() function is used to set the
 *  memory section attribute specified by the  @a attr argument with the 
 *  destroy callback function specified by the @a destroycallback argument. 
 *
 *  This function requires the following arguments: 
 *
 *  @param attr
 *    The attribute to be encoded.
 *
 *  @param destroycallback
 *    The destroy callback to be set.
 *
 */
INLINE void okl4_memsec_attr_setdestroycallback(okl4_memsec_attr_t *attr,
        okl4_destroy_func_t destroycallback);

/**
 *  The okl4_memsec_getrange() function is used to retrieve the virtual
 *  memory range contained in the memory section specified by the @a ms
 *  argument.
 *
 */
INLINE okl4_virtmem_item_t okl4_memsec_getrange(okl4_memsec_t *ms);

/*
 * Inline functions.
 */

INLINE void
okl4_memsec_attr_init(okl4_memsec_attr_t *attr)
{
    assert(attr != NULL);

    OKL4_SETUP_MAGIC(attr, OKL4_MAGIC_MEMSEC_ATTR);
    okl4_range_item_setsize(&attr->range, 0);
    attr->perms = OKL4_DEFAULT_PERMS;
    attr->attributes = OKL4_DEFAULT_MEM_ATTRIBUTES;
    attr->page_size = OKL4_DEFAULT_PAGESIZE;
    attr->premap_callback = NULL;
    attr->access_callback = NULL;
    attr->destroy_callback = NULL;
}

INLINE void
okl4_memsec_attr_setrange(okl4_memsec_attr_t *attr,
        okl4_virtmem_item_t range)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_MEMSEC_ATTR);

    attr->range = range;
}

INLINE void
okl4_memsec_attr_setperms(okl4_memsec_attr_t *attr, okl4_word_t perms)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_MEMSEC_ATTR);

    attr->perms = perms;
}

INLINE void
okl4_memsec_attr_setattributes(okl4_memsec_attr_t *attr,
        okl4_word_t attributes)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_MEMSEC_ATTR);

    attr->attributes = attributes;
}

INLINE void
okl4_memsec_attr_setpagesize(okl4_memsec_attr_t *attr, okl4_word_t pagesize)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_MEMSEC_ATTR);

    attr->page_size = pagesize;
}

INLINE void
okl4_memsec_attr_setpremapcallback(okl4_memsec_attr_t *attr,
        okl4_premap_func_t premapcallback)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_MEMSEC_ATTR);

    attr->premap_callback = premapcallback;
}

INLINE void
okl4_memsec_attr_setaccesscallback(okl4_memsec_attr_t *attr,
        okl4_access_func_t accesscallback)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_MEMSEC_ATTR);

    attr->access_callback = accesscallback;
}

INLINE void
okl4_memsec_attr_setdestroycallback(okl4_memsec_attr_t *attr,
        okl4_destroy_func_t destroycallback)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_MEMSEC_ATTR);

    attr->destroy_callback = destroycallback;
}

INLINE okl4_virtmem_item_t
okl4_memsec_getrange(okl4_memsec_t *ms)
{
    okl4_range_item_t item;

    assert(ms != NULL);

    okl4_range_item_setrange(&item, ms->super.range.base,
            ms->super.range.size);

    return item;
}


#endif /* !__OKL4__MEMSEC_H__ */
